// sportident_station.cpp

#include "sportident_station.h"
#include <QCoreApplication>
#include <QThread>
#include <QtEndian>
#include <QDebug>
#include <QElapsedTimer>
#include <QMutexLocker>
#include <QMessageBox>

#include "sportident_crc.h"
#include "sportident_decoder.h"
#include "sportident_constants.h"
#include "isportident_interface.h"

namespace SportIdent {

// Статические константы
constexpr int SportIdentStation::DEFAULT_TIMEOUT;
constexpr int SportIdentStation::MAX_RETRIES;
constexpr int SportIdentStation::BUFFER_SIZE;

SportIdentStation::SportIdentStation(QObject* parent)
    : ISportIdentInterface(parent)
    , m_cardManager(new CardReadManager(this, this))
    , m_serialPort(nullptr)
    , m_connected(false)
    , m_legacyProtocol(false)
    , m_baudRate(38400)
    , m_debugEnabled(true) {

    // Подключаем сигналы менеджера чтения карт
    connect(m_cardManager, &CardReadManager::requestBlock,
            this, &SportIdentStation::onRequestBlock);
    connect(m_cardManager, &CardReadManager::sendAckSignal,
            this, &SportIdentStation::onSendAckSignal);
    connect(m_cardManager, &CardReadManager::cancelCurrentOperation,
            this, &SportIdentStation::onCancelCurrentOperation);

    connect(m_cardManager, &CardReadManager::cardDataReady,
            this, &SportIdentStation::onCardDataReady);
    connect(m_cardManager, &CardReadManager::readComplete,
            this, &SportIdentStation::onReadComplete);
    connect(m_cardManager, &CardReadManager::readError,
            this, &SportIdentStation::onReadError);
    connect(m_cardManager, &CardReadManager::acknowledgeRequired,
            this, &SportIdentStation::onAcknowledgeRequired);
    connect(m_cardManager, &CardReadManager::stateChanged,
            this, &SportIdentStation::onStateChanged);
    connect(m_cardManager, &CardReadManager::progressChanged,
            this, &SportIdentStation::onProgressChanged);
    connect(m_cardManager, &CardReadManager::debugMessage,
            this, &SportIdentStation::debugMessage);
}

SportIdentStation::~SportIdentStation() {
    disconnectToStation();
}

void SportIdentStation::reset() {
    emit debugMessage(tr("Resetting SportIdentStation..."));

    // Отключаемся если подключены
    if (isConnectedToStation()) {
        disconnectToStation();
    }

    // Полный сброс менеджера карт
    if (m_cardManager) {
        m_cardManager->reset();
    }

    // Сброс всех состояний
    m_connected = false;
    m_legacyProtocol = false;
    m_autoAcknowledge = true;
    m_readAllBlocks = false;
    m_maxRetries = MAX_RETRIES;
    m_readBuffer.clear();
    m_commandQueue.clear();

    // Уничтожаем и создаем заново serial port
    //m_serialPort.reset();  // Удаляем старый порт

    emit debugMessage(tr("SportIdentStation reset complete"));
}

bool SportIdentStation::connectToStation(const QString& portName, int baudRate) {
    if (m_connected) {
        emit debugMessage(tr("Already connected, disconnecting first..."));
        disconnectToStation();
    }

    if (m_serialPort) {
        m_serialPort.reset();  // Удаляем старый порт если есть
    }

    try {
        m_portName = portName;
        m_baudRate = baudRate;

        // Создание и настройка последовательного порта
        m_serialPort.reset(new QSerialPort());
        m_serialPort->setPortName(portName);
        m_serialPort->setBaudRate(baudRate);
        m_serialPort->setDataBits(QSerialPort::Data8);
        m_serialPort->setParity(QSerialPort::NoParity);
        m_serialPort->setStopBits(QSerialPort::OneStop);
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

        emit debugMessage(tr("Opening port %1 at %2 baud...")
                              .arg(portName, QString::number(baudRate)) );

        // Подключение сигналов
        connect(m_serialPort.data(), &QSerialPort::readyRead,
                this, &SportIdentStation::onReadyRead);
        connect(m_serialPort.data(), &QSerialPort::errorOccurred,
                this, &SportIdentStation::onErrorOccurred);

        // Открытие порта
        if (!m_serialPort->open(QIODevice::ReadWrite)) {
            QString error = tr("Cannot open port %1: %2")
                                .arg( portName, m_serialPort->errorString() );
            emit errorOccurred(error);
            emit debugMessage(error);
            return false;
        }

        emit debugMessage(tr("Port opened successfully"));

        // Очистка буферов
        m_serialPort->clear();
        m_readBuffer.clear();
        m_commandQueue.clear();

        // Пробуждение станции
        QByteArray wakeup;
        wakeup.append(Constants::WAKEUP);
        wakeup.append(Constants::STX);

        emit debugMessage(tr("Sending wakeup: FF 02"));

        if (!sendRawData(wakeup)) {
            QString error = tr("Failed to wake up station");
            emit errorOccurred(error);
            emit debugMessage(error);
            m_serialPort->close();
            return false;
        }

        // Установка мастер-режима
        CommandFrame response = sendCommand(Constants::C_SET_MS,
                                            QByteArray(1, Constants::P_MS_DIRECT));

        if (response.command != Constants::C_SET_MS) {
            // Попробуем на низкой скорости
            m_baudRate = 4800;
            m_serialPort->setBaudRate(m_baudRate);

            if (!sendRawData(wakeup)) {
                emit errorOccurred(tr("Failed to wake up station at 4800 baud"));
                m_serialPort->close();
                return false;
            }

            response = sendCommand(Constants::C_SET_MS,
                                   QByteArray(1, Constants::P_MS_DIRECT));

            if (response.command != Constants::C_SET_MS) {
                emit errorOccurred(tr("Station doesn't respond properly"));
                m_serialPort->close();
                return false;
            }
        }

        m_connected = true;

        // Обновление конфигурации
        emit debugMessage(tr("Updating station configuration..."));
        updateStationConfig();
        updateStationInfo();

        emit stationConnected(m_stationInfo);
        emit debugMessage(tr("Connected to station at %1").arg(portName));

        return true;

    } catch (const std::exception& e) {
        QString error = tr("Connection error: %1").arg(e.what());
        emit errorOccurred(error);
        emit debugMessage(error);
        if (m_serialPort && m_serialPort->isOpen()) {
            m_serialPort->close();
        }
        return false;
    }
}

void SportIdentStation::disconnectToStation() {
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    m_connected = false;
    m_readBuffer.clear();
    m_commandQueue.clear();

    // Отменяем чтение карты
    m_cardManager->cancelReading();

    emit stationDisconnected();
    emit debugMessage(tr("Disconnected from station"));
}

void SportIdentStation::onReadyRead() {
    if (!m_serialPort || !m_serialPort->isOpen()) {
        emit debugMessage(tr("onReadyRead: port not open"));
        return;
    }

    qint64 bytesAvailable = m_serialPort->bytesAvailable();

    if (bytesAvailable == 0) { return; }

    // Читаем все доступные данные
    QByteArray newData = m_serialPort->readAll();

    if (newData.isEmpty()) {
        emit debugMessage(tr("onReadyRead: no data read"));
        return;
    }

    m_readBuffer.append(newData);

    // Обработка накопленных данных
    processIncomingData();
}

void SportIdentStation::processIncomingData() {
    // emit debugMessage(tr("processIncomingData: buffer size = %1").arg(m_readBuffer.size()));

    while (m_readBuffer.size() > 0) {
        // Проверяем, не начинается ли с WAKEUP (0xFF)
        if (static_cast<uint8_t>(m_readBuffer[0]) == Constants::WAKEUP) {
            // emit debugMessage(tr("processIncomingData: discarding WAKEUP byte"));
            m_readBuffer.remove(0, 1);
            continue;
        }

        // Ищем STX
        int stxPos = m_readBuffer.indexOf(Constants::STX);
        if (stxPos < 0) {
            // emit debugMessage(tr("processIncomingData: no STX found, clearing buffer"));
            m_readBuffer.clear();
            break;
        }

        // Удаляем мусор до STX
        if (stxPos > 0) {
            // QString discarded = m_readBuffer.left(stxPos).toHex(' ').toUpper();
            // emit debugMessage(tr("processIncomingData: discarding %1 bytes before STX: %2").arg(stxPos).arg(discarded));
            m_readBuffer.remove(0, stxPos);
        }

        // Нужен хотя бы STX + command
        if (m_readBuffer.size() < 2) {
            // emit debugMessage(tr("processIncomingData: not enough data for command"));
            break;
        }

        uint8_t command = static_cast<uint8_t>(m_readBuffer[1]);
        // emit debugMessage(tr("processIncomingData: command = 0x%1").arg(command, 2, 16, QChar('0')));

        // Определяем длину фрейма
        int frameLength = 0;

        if (m_legacyProtocol && command < 0x80) {
            // Старый протокол: ищем ETX
            int etxPos = m_readBuffer.indexOf(Constants::ETX, 1);
            if (etxPos < 0) {
                // emit debugMessage(tr("processIncomingData: no ETX found, waiting"));
                break;
            }
            frameLength = etxPos + 1;
        } else {
            // Новый протокол: проверяем длину
            if (m_readBuffer.size() < 3) {
                // emit debugMessage(tr("processIncomingData: not enough data for length byte"));
                break;
            }

            uint8_t length = static_cast<uint8_t>(m_readBuffer[2]);
            frameLength = 1 + 1 + 1 + length + 2 + 1; // STX + cmd + len + data + CRC + ETX

            if (m_readBuffer.size() < frameLength) {
                // emit debugMessage(tr("processIncomingData: waiting for more data, need %1, have %2").arg(frameLength).arg(m_readBuffer.size()));
                break;
            }
        }

        // Извлекаем полный фрейм
        QByteArray frame = m_readBuffer.left(frameLength);
        // QString frameHex = frame.toHex(' ').toUpper();
        // emit debugMessage(tr("processIncomingData: extracted frame (%1 bytes): %2").arg(frameLength).arg(frameHex));

        try {
            // Обработка фрейма
            QByteArray parsedData = parseFrame(frame);
            if (!parsedData.isEmpty()) {
                handleFrame(command, parsedData);
            }

            // Удаляем обработанный фрейм из буфера
            m_readBuffer.remove(0, frameLength);
            // emit debugMessage(tr("processIncomingData: frame processed, buffer size = %1").arg(m_readBuffer.size()));

        } catch (const std::exception& e) {
            QString error = tr("Error processing frame: %1").arg(e.what());
            emit errorOccurred(error);
            emit debugMessage(error);

            // Пропускаем этот байт и пробуем снова
            m_readBuffer.remove(0, 1);
        }
    }
}

void SportIdentStation::handleFrame(uint8_t command, const QByteArray& data) {
    // emit debugMessage(tr("handleFrame: command 0x%1, data size = %2").arg(command, 2, 16, QChar('0')).arg(data.size()));

    switch (command) {
    case Constants::C_SET_MS:
        emit debugMessage(tr("handleFrame: C_SET_MS response"));
        m_commandQueue.enqueue(CommandFrame(command, data));
        break;

    case Constants::C_SI9_DET:
        handleCardDetection(data);
        break;

    case Constants::C_SI_REM:
        handleCardRemoval();
        break;

    case Constants::C_GET_SI9:
        // emit debugMessage(tr("handleFrame: C_GET_SI9 response"));
        // m_commandQueue.enqueue(CommandFrame(command, data));

        // Обработка данных блока через менеджер
        if (data.size() >= 1) {
            uint8_t blockNumber = static_cast<uint8_t>(data[0]);
            QByteArray blockData = data.mid(1); // Без номера блока
            m_cardManager->handleBlockData(blockNumber, blockData);
        }
        break;

    case Constants::C_GET_SYS_VAL:
        emit debugMessage(tr("handleFrame: C_GET_SYS_VAL response"));
        m_commandQueue.enqueue(CommandFrame(command, data));
        break;

    case Constants::C_GET_TIME:
        emit debugMessage(tr("handleFrame: C_GET_TIME response"));
        m_commandQueue.enqueue(CommandFrame(command, data));
        break;

    case Constants::C_BEEP:
        emit debugMessage(tr("handleFrame: C_BEEP response"));
        m_commandQueue.enqueue(CommandFrame(command, data));
        break;

    default:
        emit debugMessage(tr("handleFrame: unknown command 0x%1").arg(command, 2, 16, QChar('0')));
        m_commandQueue.enqueue(CommandFrame(command, data));
        break;
    }
}

void SportIdentStation::handleCardDetection(const QByteArray& data) {
    if (data.size() < 4) { // Минимум для C_SI9_DET
        emit errorOccurred(tr("Invalid card detection data"));
        return;
    }

    try {
        // Декодирование номера карты
        QByteArray cardNumberBytes;

        // Для C_SI9_DET данные начинаются со 2-го байта
        if (data.size() >= 4) {
            cardNumberBytes.append(1, '\0');
            cardNumberBytes.append(data.mid(1, 3)); // Байты 1-3 содержат номер
        }

        uint32_t cardNumber = DataDecoder::decodeCardNumber(cardNumberBytes);
        CardType cardType = DataDecoder::detectCardType(cardNumber);

        emit cardDetected(cardNumber, cardType);
        emit debugMessage(tr("Card detected: %1 (type: %2)")
                              .arg(QString::number(cardNumber)
                                   ,QString::number(static_cast<int>(cardType))));

        // Запускаем чтение карты через менеджер
        m_cardManager->startReading(cardNumber, cardType);

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Card detection error: %1").arg(e.what()));
    }
}

void SportIdentStation::handleCardRemoval() {
    emit cardRemoved();
    emit debugMessage(tr("Card removed"));

    // Уведомляем менеджер об извлечении карты
    m_cardManager->handleCardRemoved();
}


void SportIdentStation::onErrorOccurred(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError) {
        return;
    }

    QString errorMsg;
    switch (error) {
    case QSerialPort::DeviceNotFoundError:
        errorMsg = tr("Device not found");
        break;
    case QSerialPort::PermissionError:
        errorMsg = tr("Permission denied");
        break;
    case QSerialPort::OpenError:
        errorMsg = tr("Cannot open device");
        break;
    case QSerialPort::NotOpenError:
        errorMsg = tr("Device not open");
        break;
    case QSerialPort::WriteError:
        errorMsg = tr("Write error");
        break;
    case QSerialPort::ReadError:
        errorMsg = tr("Read error");
        break;
    case QSerialPort::ResourceError:
        errorMsg = tr("Resource error");
        break;
    case QSerialPort::UnsupportedOperationError:
        errorMsg = tr("Unsupported operation");
        break;
    case QSerialPort::TimeoutError:
        errorMsg = tr("Timeout");
        break;
    case QSerialPort::UnknownError:
    default:
        errorMsg = tr("Unknown error");
        break;
    }

    QString fullError = tr("Serial port error: %1").arg(errorMsg);
    emit errorOccurred(fullError);
    emit debugMessage(fullError);

    if (m_connected) {
        disconnectToStation();
    }
}

void SportIdentStation::acknowledgeCard() {
    // Отправляем подтверждение через менеджер
    m_cardManager->sendAcknowledge();
}

// Слоты для менеджера чтения карт
void SportIdentStation::onRequestBlock(uint8_t blockNumber) {
    try {
        QByteArray blockRequest(1, static_cast<char>(blockNumber));
        sendCommandAsync(Constants::C_GET_SI9, blockRequest);

        emit debugMessage(tr("Requested block %1").arg(blockNumber));
    } catch (const std::exception& e) {
        m_cardManager->handleError(tr("Failed to request block %1: %2")
                                       .arg(QString::number(blockNumber),e.what()));
    }
}

void SportIdentStation::onSendAckSignal() {
    sendAck();
}

void SportIdentStation::onCancelCurrentOperation() {
    // Можем отправить команду отмены или просто игнорировать
    emit debugMessage(tr("Current operation cancelled"));
}

void SportIdentStation::onCardDataReady(const CardData& cardData) {
    emit cardReadComplete(cardData);
    emit debugMessage(tr("Card data ready for card %1").arg(cardData.cardNumber));
}

void SportIdentStation::onReadComplete(const CardData& cardData) {
    emit debugMessage(tr("Card read complete for card %1").arg(cardData.cardNumber));
}

void SportIdentStation::onReadError(const QString& error) {
    emit errorOccurred(tr("Card read error: %1").arg(error));
}

void SportIdentStation::onAcknowledgeRequired() {
    emit debugMessage(tr("Card requires acknowledgement"));
    // Можно автоматически подтвердить или запросить у пользователя
    if (m_autoAcknowledge) {
        acknowledgeCard();
    }
}

void SportIdentStation::onStateChanged(CardReadState newState, CardReadState oldState) {
    emit debugMessage(tr("Card read state changed: %1 -> %2")
                          .arg(QString::number(static_cast<int>(oldState))
                               ,QString::number(static_cast<int>(newState))));
}

void SportIdentStation::onProgressChanged(int blocksRead, int totalBlocks) {
    emit debugMessage(tr("Reading progress: %1/%2 blocks")
                          .arg(QString::number(blocksRead),QString::number(totalBlocks)));
}

void SportIdentStation::beep(int count) {
    // QMutexLocker locker(&m_serialMutex);

    if (!m_connected) {
        return;
    }

    try {
        // Ограничиваем count в диапазоне 1-255
        if (count < 1) count = 1;
        if (count > 255) count = 255;

        CommandFrame response = sendCommand(Constants::C_BEEP,
                                            QByteArray(1, static_cast<char>(count)));

        if (response.command != Constants::C_BEEP) {
            throw SIException("Beep command failed");
        }

        emit debugMessage(tr("Beep %1 times").arg(count));

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Beep error: %1").arg(e.what()));
    }
}

void SportIdentStation::powerOff() {
    // QMutexLocker locker(&m_serialMutex);

    if (!m_connected) {
        return;
    }

    try {
        CommandFrame response = sendCommand(Constants::C_OFF);
        emit debugMessage(tr("Station powered off"));

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Power off error: %1").arg(e.what()));
    }

    // Всегда отключаемся после выключения станции
    disconnectToStation();
}

QDateTime SportIdentStation::getStationTime() {
    // QMutexLocker locker(&m_serialMutex);

    if (!m_connected) {
        throw SIException("Not connected");
    }

    try {
        CommandFrame response = sendCommand(Constants::C_GET_TIME);

        if (response.command != Constants::C_GET_TIME || response.response.size() < 7) {
            throw SIException("Invalid time response");
        }

        const QByteArray& data = response.response;

        int year = static_cast<uint8_t>(data[0]) + 2000;
        int month = static_cast<uint8_t>(data[1]);
        int day = static_cast<uint8_t>(data[2]);
        uint8_t ampm = static_cast<uint8_t>(data[3]) & 0x01;

        uint16_t seconds = (static_cast<uint8_t>(data[4]) << 8) |
                           static_cast<uint8_t>(data[5]);

        int hour = ampm * 12 + seconds / 3600;
        seconds %= 3600;
        int minute = seconds / 60;
        int second = seconds % 60;

        // Миллисекунды
        uint8_t msByte = static_cast<uint8_t>(data[6]);
        int milliseconds = static_cast<int>(msByte * 1000.0 / 256.0);

        QDateTime stationTime(QDate(year, month, day),
                              QTime(hour, minute, second, milliseconds));

        emit debugMessage(tr("Station time: %1").arg(stationTime.toString()));

        return stationTime;

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Get time error: %1").arg(e.what()));
        throw;
    }
}

void SportIdentStation::setStationTime(const QDateTime& time) {
    // QMutexLocker locker(&m_serialMutex);

    if (!m_connected) {
        throw SIException("Not connected");
    }

    try {
        QByteArray timeData;

        // Год (последние две цифры)
        timeData.append(static_cast<char>(time.date().year() % 100));

        // Месяц
        timeData.append(static_cast<char>(time.date().month()));

        // День
        timeData.append(static_cast<char>(time.date().day()));

        // День недели и AM/PM
        int dayOfWeek = time.date().dayOfWeek();
        if (dayOfWeek == 7) dayOfWeek = 0; // Воскресенье = 0

        uint8_t ampm = (time.time().hour() >= 12) ? 1 : 0;
        uint8_t dayByte = static_cast<uint8_t>((dayOfWeek << 1) | ampm);
        timeData.append(static_cast<char>(dayByte));

        // Секунды с полуночи/полудня
        int hour12 = time.time().hour() % 12;
        if (hour12 == 0) hour12 = 12;

        uint16_t seconds = hour12 * 3600 +
                           time.time().minute() * 60 +
                           time.time().second();

        timeData.append(static_cast<char>((seconds >> 8) & 0xFF));
        timeData.append(static_cast<char>(seconds & 0xFF));

        // Доли секунды
        int ms = time.time().msec();
        uint8_t msByte = static_cast<uint8_t>(ms * 256.0 / 1000.0);
        timeData.append(static_cast<char>(msByte));

        CommandFrame response = sendCommand(Constants::C_SET_TIME, timeData);

        if (response.command != Constants::C_SET_TIME) {
            throw SIException("Set time command failed");
        }

        emit debugMessage(tr("Station time set to: %1").arg(time.toString()));

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Set time error: %1").arg(e.what()));
        throw;
    }
}

StationInfo SportIdentStation::getStationInfo() {
    // QMutexLocker locker(&m_serialMutex);

    if (!m_connected) {
        throw SIException("Not connected");
    }

    return m_stationInfo;
}

StationConfig SportIdentStation::getStationConfig() {
    // QMutexLocker locker(&m_serialMutex);

    if (!m_connected) {
        throw SIException("Not connected");
    }

    return m_stationConfig;
}

void SportIdentStation::setOperatingMode(uint8_t mode) {
    // QMutexLocker locker(&m_serialMutex);

    if (!m_connected) {
        throw SIException("Not connected");
    }

    // Проверка поддерживаемых режимов
    QVector<uint8_t> supportedModes = {
        Constants::M_CONTROL,
        Constants::M_START,
        Constants::M_FINISH,
        Constants::M_READOUT,
        Constants::M_CLEAR,
        Constants::M_CHECK
    };

    if (!supportedModes.contains(mode)) {
        throw SIException("Unsupported operating mode");
    }

    try {
        QByteArray modeData;
        modeData.append(Constants::O_MODE);
        modeData.append(static_cast<char>(mode));

        CommandFrame response = sendCommand(Constants::C_SET_SYS_VAL, modeData);

        if (response.command != Constants::C_SET_SYS_VAL) {
            throw SIException("Set operating mode failed");
        }

        // Обновление конфигурации
        updateStationConfig();

        emit debugMessage(tr("Operating mode set to: %1").arg(mode));

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Set operating mode error: %1").arg(e.what()));
        throw;
    }
}

void SportIdentStation::setStationCode(uint16_t code) {
    // QMutexLocker locker(&m_serialMutex);

    if (!m_connected) {
        throw SIException("Not connected");
    }

    if (code < 1 || code > 1023) {
        throw SIException("Station code must be between 1 and 1023");
    }

    try {
        QByteArray codeData;
        codeData.append(Constants::O_STATION_CODE);

        // Младший байт
        codeData.append(static_cast<char>(code & 0xFF));

        // Старший байт (первые 2 бита + остальные 1)
        uint8_t highByte = (code >> 2) | 0b00111111;
        codeData.append(static_cast<char>(highByte));

        CommandFrame response = sendCommand(Constants::C_SET_SYS_VAL, codeData);

        if (response.command != Constants::C_SET_SYS_VAL) {
            throw SIException("Set station code failed");
        }

        // Обновление конфигурации
        updateStationConfig();

        emit debugMessage(tr("Station code set to: %1").arg(code));

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Set station code error: %1").arg(e.what()));
        throw;
    }
}

void SportIdentStation::setExtendedProtocol(bool extended) {
    // QMutexLocker locker(&m_serialMutex);

    if (!m_connected) {
        throw SIException("Not connected");
    }

    try {
        m_stationConfig.extendedProtocol = extended;

        // Формирование байта конфигурации
        uint8_t configByte = 0;
        if (extended) configByte |= (1 << 0);
        if (m_stationConfig.autoSendMode) configByte |= (1 << 1);
        if (m_stationConfig.handshakeMode) configByte |= (1 << 2);
        if (m_stationConfig.passwordAccess) configByte |= (1 << 4);
        if (m_stationConfig.punchReadout) configByte |= (1 << 7);

        QByteArray configData;
        configData.append(Constants::O_PROTO);
        configData.append(static_cast<char>(configByte));

        CommandFrame response = sendCommand(Constants::C_SET_SYS_VAL, configData);

        if (response.command != Constants::C_SET_SYS_VAL) {
            throw SIException("Set extended protocol failed");
        }

        // Обновление конфигурации
        updateStationConfig();

        emit debugMessage(tr("Extended protocol: %1").arg(extended ? "enabled" : "disabled"));

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Set extended protocol error: %1").arg(e.what()));
        throw;
    }
}

void SportIdentStation::setAutoSendMode(bool autoSend) {
    // QMutexLocker locker(&m_serialMutex);

    if (!m_connected) {
        throw SIException("Not connected");
    }

    try {
        m_stationConfig.autoSendMode = autoSend;
        m_stationConfig.handshakeMode = !autoSend;

        // Формирование байта конфигурации
        uint8_t configByte = 0;
        if (m_stationConfig.extendedProtocol) configByte |= (1 << 0);
        if (autoSend) configByte |= (1 << 1);
        if (!autoSend) configByte |= (1 << 2);
        if (m_stationConfig.passwordAccess) configByte |= (1 << 4);
        if (m_stationConfig.punchReadout) configByte |= (1 << 7);

        QByteArray configData;
        configData.append(Constants::O_PROTO);
        configData.append(static_cast<char>(configByte));

        CommandFrame response = sendCommand(Constants::C_SET_SYS_VAL, configData);

        if (response.command != Constants::C_SET_SYS_VAL) {
            throw SIException("Set auto send mode failed");
        }

        // Обновление конфигурации
        updateStationConfig();

        emit debugMessage(tr("Auto send mode: %1").arg(autoSend ? "enabled" : "disabled"));

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Set auto send mode error: %1").arg(e.what()));
        throw;
    }
}

// =========== PRIVATE METHODS ===========

void SportIdentStation::updateStationConfig() {
    if (!m_connected) {
        return;
    }

    try {
        // Чтение конфигурации протокола
        QByteArray protoRequest;
        protoRequest.append(Constants::O_PROTO);
        protoRequest.append('\x01'); // Длина

        CommandFrame protoResponse = sendCommand(Constants::C_GET_SYS_VAL, protoRequest);

        if (protoResponse.command == Constants::C_GET_SYS_VAL &&
            protoResponse.response.size() >= 2) {

            uint8_t configByte = static_cast<uint8_t>(protoResponse.response[1]);

            m_stationConfig.extendedProtocol = (configByte & (1 << 0)) != 0;
            m_stationConfig.autoSendMode = (configByte & (1 << 1)) != 0;
            m_stationConfig.handshakeMode = (configByte & (1 << 2)) != 0;
            m_stationConfig.passwordAccess = (configByte & (1 << 4)) != 0;
            m_stationConfig.punchReadout = (configByte & (1 << 7)) != 0;
        }

        // Чтение режима работы
        QByteArray modeRequest;
        modeRequest.append(Constants::O_MODE);
        modeRequest.append('\x01');

        CommandFrame modeResponse = sendCommand(Constants::C_GET_SYS_VAL, modeRequest);

        if (modeResponse.command == Constants::C_GET_SYS_VAL &&
            modeResponse.response.size() >= 2) {

            m_stationConfig.operatingMode = static_cast<uint8_t>(modeResponse.response[1]);
        }

        // Чтение кода станции
        QByteArray codeRequest;
        codeRequest.append(Constants::O_STATION_CODE);
        codeRequest.append('\x02'); // 2 байта

        CommandFrame codeResponse = sendCommand(Constants::C_GET_SYS_VAL, codeRequest);

        if (codeResponse.command == Constants::C_GET_SYS_VAL &&
            codeResponse.response.size() >= 3) {

            uint8_t lowByte = static_cast<uint8_t>(codeResponse.response[1]);
            uint8_t highByte = static_cast<uint8_t>(codeResponse.response[2]);

            // Извлекаем только первые 2 бита из старшего байта
            m_stationConfig.stationCode = lowByte | ((highByte & 0b11000000) << 2);
        }

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Update config error: %1").arg(e.what()));
    }
}

void SportIdentStation::updateStationInfo() {
    if (!m_connected) {
        return;
    }

    try {
        // Чтение серийного номера (4 байта)
        QByteArray serialRequest;
        //serialRequest.append(Constants::O_SERIAL_NO);
        serialRequest.append('\x00');
        serialRequest.append('\x04');

        CommandFrame serialResponse = sendCommand(Constants::C_GET_SYS_VAL, serialRequest);

        if (serialResponse.command == Constants::C_GET_SYS_VAL &&
            serialResponse.response.size() >= 5) {

            const QByteArray& data = serialResponse.response;
            m_stationInfo.serialNumber =
                (static_cast<uint32_t>(static_cast<uint8_t>(data[1])) << 24) |
                (static_cast<uint32_t>(static_cast<uint8_t>(data[2])) << 16) |
                (static_cast<uint32_t>(static_cast<uint8_t>(data[3])) << 8) |
                static_cast<uint32_t>(static_cast<uint8_t>(data[4]));
        }

        // Чтение версии прошивки (3 байта)
        QByteArray firmwareRequest;
        firmwareRequest.append(Constants::O_FIRMWARE);
        firmwareRequest.append('\x03');

        CommandFrame firmwareResponse = sendCommand(Constants::C_GET_SYS_VAL, firmwareRequest);

        if (firmwareResponse.command == Constants::C_GET_SYS_VAL &&
            firmwareResponse.response.size() >= 4) {

            const QByteArray& data = firmwareResponse.response;
            m_stationInfo.firmwareVersion = QString("%1.%2.%3")
                                                .arg(QString::number(static_cast<uint8_t>(data[1]))
                                                ,QString::number(static_cast<uint8_t>(data[2]))
                                                ,QString::number(static_cast<uint8_t>(data[3])));
        }

        // Чтение ID модели (2 байта)
        QByteArray modelRequest;
        modelRequest.append(Constants::O_MODEL_ID);
        modelRequest.append('\x02');

        CommandFrame modelResponse = sendCommand(Constants::C_GET_SYS_VAL, modelRequest);

        if (modelResponse.command == Constants::C_GET_SYS_VAL &&
            modelResponse.response.size() >= 3) {

            const QByteArray& data = modelResponse.response;
            m_stationInfo.modelId =
                (static_cast<uint16_t>(static_cast<uint8_t>(data[1])) << 8) |
                static_cast<uint16_t>(static_cast<uint8_t>(data[2]));
        }

        // Чтение размера памяти (1 байт)
        QByteArray memoryRequest;
        memoryRequest.append(Constants::O_MEM_SIZE);
        memoryRequest.append('\x01');

        CommandFrame memoryResponse = sendCommand(Constants::C_GET_SYS_VAL, memoryRequest);

        if (memoryResponse.command == Constants::C_GET_SYS_VAL &&
            memoryResponse.response.size() >= 2) {

            m_stationInfo.memorySize = static_cast<uint8_t>(memoryResponse.response[1]);
        }

        // Чтение кода станции из конфигурации
        m_stationInfo.stationCode = m_stationConfig.stationCode;

        emit debugMessage(tr("Station info updated"));

    } catch (const std::exception& e) {
        emit errorOccurred(tr("Update station info error: %1").arg(e.what()));
    }
}

SportIdentStation::CommandFrame SportIdentStation::sendCommand(uint8_t command,
                                                               const QByteArray& parameters) {
    if (!m_serialPort || !m_serialPort->isOpen()) {
        throw SIException("Not connected");
    }

    CommandFrame frame(command, parameters);
    frame.waitingForResponse = true;
    frame.timestamp = QDateTime::currentDateTime();

    // Формирование фрейма
    QByteArray frameData = createCommandFrame(command, parameters);

    // Отправка
    if (m_debugEnabled) {
        QString hexString = frameData.toHex(' ').toUpper();
        emit debugMessage(tr("Sending command 0x%1: %2").arg(command, 2, 16, QChar('0')).arg(hexString));
    }

    if (!sendRawData(frameData)) {
        throw SIException("Failed to send command");
    }

    // Ожидание ответа
    QElapsedTimer timer;
    timer.start();

    while (frame.waitingForResponse && timer.elapsed() < DEFAULT_TIMEOUT) {
        // Даем время на обработку событий
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        // Проверяем очередь команд
        bool found = false;
        for (int i = 0; i < m_commandQueue.size(); i++) {
            if (m_commandQueue[i].command == command) {
                frame = m_commandQueue.takeAt(i);
                if (m_debugEnabled) {
                    emit debugMessage(tr("Command response received for 0x%1").arg(command, 2, 16, QChar('0')));
                }
                found = true;
                break;
            }
        }

        if (found) {
            break;
        }

        // Небольшая пауза
        QThread::msleep(10);
    }

    if (frame.waitingForResponse) {
        if (m_debugEnabled) {
            emit debugMessage(tr("Command timeout for command 0x%1").arg(command, 2, 16, QChar('0')));
        }
        throw SITimeoutException("Command timeout");
    }

    return frame;
}

void SportIdentStation::sendCommandAsync(uint8_t command, const QByteArray& parameters) {
    if (!m_serialPort || !m_serialPort->isOpen()) {
        throw SIException("Not connected");
    }

    // Формирование фрейма
    QByteArray frameData = createCommandFrame(command, parameters);

    // Отправка без ожидания ответа
    if (m_debugEnabled) {
        QString hexString = frameData.toHex(' ').toUpper();
        emit debugMessage(tr("Sending async command 0x%1: %2").arg(command, 2, 16, QChar('0')).arg(hexString));
    }

    if (!sendRawData(frameData)) {
        throw SIException("Failed to send async command");
    }
}

void SportIdentStation::sendAck() {
    if (!m_serialPort || !m_serialPort->isOpen()) {
        return;
    }

    QByteArray ack;
    ack.append(Constants::ACK);

    if (sendRawData(ack)) {
        emit debugMessage(tr("ACK sent successfully"));
    }
}

bool SportIdentStation::sendRawData(const QByteArray& data) {
    if (!m_serialPort || !m_serialPort->isOpen()) {
        return false;
    }

    qint64 bytesWritten = m_serialPort->write(data);

    if (bytesWritten != data.size()) {
        QString error = tr("Failed to send data: written %1 of %2 bytes - %3")
                            .arg(bytesWritten).arg(data.size()).arg(m_serialPort->errorString());
        emit errorOccurred(error);
        emit debugMessage(error);
        return false;
    }

    if (!m_serialPort->waitForBytesWritten(DEFAULT_TIMEOUT)) {
        QString error = tr("Timeout while sending data - %1").arg(m_serialPort->errorString());
        emit errorOccurred(error);
        emit debugMessage(error);
        return false;
    }

    if (m_debugEnabled) {
        QString hexString = data.toHex(' ').toUpper();
        emit debugMessage(tr("Sent: %1").arg(hexString));
        emit rawDataSent(data);
    }

    return true;
}

QByteArray SportIdentStation::createCommandFrame(uint8_t command,
                                                 const QByteArray& parameters) {
    QByteArray frame;

    // STX
    frame.append(Constants::STX);

    // Команда
    frame.append(static_cast<char>(command));

    // Длина (команда + параметры + станция = 1 + params + 2)
    uint8_t length = static_cast<uint8_t>(parameters.size());
    frame.append(static_cast<char>(length));

    // Параметры
    frame.append(parameters);

    // CRC
    QByteArray dataForCrc = frame.mid(1); // Без STX
    quint16 crc = CRC16::calculate(dataForCrc);

    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    frame.append(static_cast<char>(crc & 0xFF));

    // ETX
    frame.append(Constants::ETX);

    return frame;
}

QByteArray SportIdentStation::parseFrame(const QByteArray& rawData) {
    if (rawData.isEmpty() || rawData[0] != Constants::STX) {
        emit errorOccurred(tr("Invalid frame: no STX"));
        return QByteArray();
    }

    uint8_t command = static_cast<uint8_t>(rawData[1]);

    if (m_legacyProtocol && command < 0x80) {
        // Старый протокол
        if (rawData[rawData.size() - 1] != Constants::ETX) {
            emit errorOccurred(tr("Invalid frame: no ETX"));
            return QByteArray();
        }

        // Возвращаем данные без STX и ETX
        return rawData.mid(1, rawData.size() - 2);

    } else {
        // Новый протокол
        if (rawData.size() < 5) { // Минимум: STX + cmd + len + crc + ETX
            emit errorOccurred(tr("Frame too short"));
            return QByteArray();
        }

        uint8_t length = static_cast<uint8_t>(rawData[2]);

        // Проверяем длину
        int expectedLength = 1 + 1 + 1 + length + 2 + 1; // STX + cmd + len + data + CRC + ETX
        if (rawData.size() != expectedLength) {
            emit errorOccurred(tr("Frame length mismatch"));
            return QByteArray();
        }

        // Проверяем ETX
        if (rawData[rawData.size() - 1] != Constants::ETX) {
            emit errorOccurred(tr("Invalid frame: no ETX"));
            return QByteArray();
        }

        // Проверяем CRC
        QByteArray dataForCrc = rawData.mid(1, 1 + 1 + length); // Команда + длина + данные
        QByteArray receivedCrc = rawData.mid(1 + 1 + length + 1, 2); // После данных

        if (!CRC16::verify(dataForCrc, receivedCrc)) {
            emit errorOccurred(tr("CRC check failed"));
            return QByteArray();
        }

        // Извлекаем данные (без кода станции)
        // Данные начинаются с позиции 5: STX(0) + cmd(1) + len(2) + station(3,4)
        int dataStart = 5;
        int dataLength = length - 2; // Вычитаем 2 байта кода станции

        if (dataStart + dataLength > static_cast<int>(rawData.size()) - 3) { // -3 для CRC+ETX
            emit errorOccurred(tr("Invalid data length"));
            return QByteArray();
        }

        // Код станции
        uint16_t stationCode =
            (static_cast<uint8_t>(rawData[3]) << 8) |
            static_cast<uint8_t>(rawData[4]);

        m_stationConfig.stationCode = stationCode;

        return rawData.mid(dataStart, dataLength);
    }
}

} // namespace SportIdent
