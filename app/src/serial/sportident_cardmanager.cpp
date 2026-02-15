#include "sportident_cardmanager.h"
#include "sportident_station.h"
#include "sportident_constants.h"
#include "sportident_decoder.h"
#include <QCoreApplication>
#include <QDebug>

namespace SportIdent {

CardReadManager::CardReadManager(SportIdentStation* station, QObject* parent)
    : QObject(parent)
    , m_retryCount(0)
    , m_autoAcknowledge(true)
    , m_readAllBlocks(false)
    , m_maxRetries(MAX_RETRIES)
    , m_station(station) {
    
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &CardReadManager::onTimeout);
    
    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, &CardReadManager::onRetryTimeout);
}

CardReadManager::~CardReadManager() {
    cancelReading();
}

void CardReadManager::startReading(uint32_t cardNumber, CardType cardType) {
    // Отменяем текущее чтение, если есть
    if (m_session.inProgress()) {
        emit debugMessage(tr("Cancelling previous read session"));
        cancelReading();
    }
    
    // Инициализируем новую сессию
    m_session.reset();
    m_session.cardNumber = cardNumber;
    m_session.cardType = cardType;
    m_session.detectionTime = QDateTime::currentDateTime();
    m_session.state = CardReadState::CardDetected;
    
    m_retryCount = 0;
    
    emit debugMessage(tr("Starting read session for card %1 (type: %2)")
                     .arg(cardNumber)
                     .arg(static_cast<int>(cardType)));
    
    // Начинаем с чтения блока 0
    changeState(CardReadState::ReadingBlock0);
    
    // Запрашиваем блок 0
    emit requestBlock(0);
    m_session.currentBlock = 0;

    // Ожидаем блок 0
    changeState(CardReadState::AwaitingBlock0);
    
    // Запускаем таймер таймаута
    m_timeoutTimer->start(READ_TIMEOUT);
}

void CardReadManager::cancelReading() {
    if (m_session.inProgress()) {
        emit debugMessage(tr("Cancelling read session"));
        
        m_timeoutTimer->stop();
        m_retryTimer->stop();
        
        emit cancelCurrentOperation();
        
        changeState(CardReadState::Idle);
        m_session.reset();
    }
}

void CardReadManager::sendAcknowledge() {
    if (m_session.state == CardReadState::ReadingComplete) {
        emit debugMessage(tr("Sending ACK for card %1").arg(m_session.cardNumber));
        
        changeState(CardReadState::SendingAck);
        emit sendAckSignal();
        
        // После отправки ACK переходим в завершенное состояние
        changeState(CardReadState::Acknowledged);
        
        emit readComplete(m_session.decodedData);
    }
}

void CardReadManager::handleBlockData(uint8_t blockNumber, const QByteArray& data) {
    // Проверяем, что это ответ на текущий запрос
    if (!m_session.inProgress() || m_session.currentBlock != static_cast<int>(blockNumber)) {
        emit debugMessage(tr("Unexpected block %1 received, expected %2")
                              .arg(QString::number(blockNumber),QString::number(m_session.currentBlock) ));
        return;
    }
    
    // Сбрасываем таймеры
    m_timeoutTimer->stop();
    m_retryTimer->stop();
    m_retryCount = 0;
    
    emit debugMessage(tr("Received block %1, size = %2 bytes")
                          .arg(QString::number(blockNumber),QString::number(data.size())));
    
    // Добавляем данные в сырой буфер
    m_session.rawData.append(data);
    m_session.blocksRead++;
    
    // Обновляем прогресс
    emit progressChanged(m_session.blocksRead, m_session.blocksToRead);
    
    // Обработка в зависимости от состояния
    switch (m_session.state) {
    case CardReadState::AwaitingBlock0:
        // Обрабатываем блок 0
        processBlock0(data);
        break;
        
    case CardReadState::AwaitingBlock:
        // Обрабатываем обычный блок
        if (m_session.blocksRead >= m_session.blocksToRead) {
            // Все блоки прочитаны
            completeReading();
        } else {
            // Запрашиваем следующий блок
            sendNextBlockRequest();
        }
        break;
        
    default:
        emit debugMessage(tr("Received block in unexpected state: %1")
                         .arg(static_cast<int>(m_session.state)));
        break;
    }
}

void CardReadManager::handleCardRemoved() {
    if (m_session.inProgress()) {
        emit debugMessage(tr("Card removed during reading"));
        handleErrorState(tr("Card removed during reading"));
    }
}

void CardReadManager::handleError(const QString& error) {
    if (m_session.inProgress()) {
        handleErrorState(error);
    }
}

void CardReadManager::onTimeout() {
    if (!m_session.inProgress()) {
        return;
    }
    
    m_retryCount++;
    
    if (m_retryCount >= m_maxRetries) {
        // Превышено количество попыток
        handleErrorState(tr("Timeout after %1 retries for block %2")
                             .arg(QString::number(m_retryCount),QString::number(m_session.currentBlock)));
    } else {
        // Пытаемся снова
        emit debugMessage(tr("Timeout for block %1, retry %2/%3")
                .arg(QString::number(m_session.currentBlock)
                    ,QString::number(m_retryCount)
                    ,QString::number(m_maxRetries)));
        
        m_retryTimer->start(RETRY_DELAY);
    }
}

void CardReadManager::onRetryTimeout() {
    if (!m_session.inProgress()) {
        return;
    }
    
    // Повторно запрашиваем текущий блок
    emit debugMessage(tr("Retrying block %1").arg(m_session.currentBlock));
    emit requestBlock(static_cast<uint8_t>(m_session.currentBlock));
    
    // Перезапускаем таймер таймаута
    m_timeoutTimer->start(READ_TIMEOUT);
}

void CardReadManager::changeState(CardReadState newState) {
    if (m_session.state == newState) {
        return;
    }
    
    CardReadState oldState = m_session.state;
    m_session.state = newState;
    
    emit debugMessage(tr("State change: %1 -> %2")
                     .arg(static_cast<int>(oldState))
                     .arg(static_cast<int>(newState)));
    
    emit stateChanged(newState, oldState);
}

void CardReadManager::processBlock0(const QByteArray& data) {
    try {
        emit debugMessage(tr("Processing block 0"));
        
        // Для блока 0 определяем тип карты и количество блоков для чтения
        if (data.size() < 128) {
            throw SIException("Block 0 too small");
        }
        
        // Определяем тип карты по номеру (уже известен, но можно проверить)
        // Читаем счетчик отметок
        int punchCounterOffset = 0;
        
        switch (m_session.cardType) {
        case CardType::SI9:
            punchCounterOffset = Constants::CardOffsets::SI9.rc;
            m_session.blocksToRead = Constants::CardOffsets::SI9.blocks;
            break;
        case CardType::SI10:
        case CardType::SI11:
        case CardType::SIAC:
            punchCounterOffset = Constants::CardOffsets::SI10.rc;
            m_session.blocksToRead = Constants::CardOffsets::SI10.blocks;
            break;
        default:
            throw SIException("Unsupported card type");
        }
        
        if (punchCounterOffset < data.size()) {
            m_session.decodedData.punchCount = static_cast<uint8_t>(data.at(punchCounterOffset));
            emit debugMessage(tr("Card has %1 punches").arg(m_session.decodedData.punchCount));
        }
        
        // Определяем, сколько блоков нужно читать
        determineBlocksToRead();
        
        // Переходим к чтению остальных блоков
        changeState(CardReadState::DeterminingBlocks);
        sendNextBlockRequest();
        
    } catch (const std::exception& e) {
        handleErrorState(tr("Error processing block 0: %1").arg(e.what()));
    }
}

void CardReadManager::determineBlocksToRead() {
    // Определяем сколько блоков нужно читать в зависимости от типа карты
    if (m_readAllBlocks) {
        // Читаем все блоки
        switch (m_session.cardType) {
        case CardType::SI9:
            m_session.blocksToRead = Constants::CardOffsets::SI9.blocks;
            break;
        case CardType::SI10:
        case CardType::SI11:
        case CardType::SIAC:
            m_session.blocksToRead = Constants::CardOffsets::SI10.blocks;
            break;
        default:
            m_session.blocksToRead = 2; // По умолчанию
        }
    } else {
        // Читаем только необходимые блоки
        switch (m_session.cardType) {
        case CardType::SI9:
            // SI9: блок 0 и блок 4 (отметки)
            m_session.blocksToRead = 2;
            break;
            
        case CardType::SI10:
        case CardType::SI11:
        case CardType::SIAC:
            {
                // SI10: блок 0 и необходимое количество блоков с отметками
                // Каждый блок содержит 32 отметки (128 байт / 4 байта на отметку)
                int maxPunchesPerBlock = 32;
                int blocksNeededForPunches = 1 + (maxPunchesPerBlock - m_session.decodedData.punchCount ) / maxPunchesPerBlock;
                //int blocksNeededForPunches = 1 + (m_session.decodedData.punchCount + maxPunchesPerBlock - 1) / maxPunchesPerBlock;

                // SI10 имеет блоки 0-7, но блоки 1-3 содержат личную информацию
                // Если нужно читать личную информацию, включаем блоки 1-3
                int personalInfoBlocks = 0;// 3; // Блоки 1, 2, 3
                m_session.blocksToRead = 1 + personalInfoBlocks + blocksNeededForPunches;

                // Не превышаем максимальное количество блоков
                if (m_session.blocksToRead > Constants::CardOffsets::SI10.blocks) {
                    m_session.blocksToRead = Constants::CardOffsets::SI10.blocks;
                }
            }
            break;
            
        default:
            m_session.blocksToRead = 2;
        }
    }
    
    emit debugMessage(tr("Need to read %1 blocks total").arg(m_session.blocksToRead));
}

void CardReadManager::sendNextBlockRequest() {
    if (m_session.blocksRead >= m_session.blocksToRead) {
        // Все блоки прочитаны
        completeReading();
        return;
    }
    
    // Определяем следующий блок для чтения
    int nextBlock = -1;
    
    if (m_session.blocksRead == 1) {
        nextBlock = 4; // Пропускаем блоки 1-2-3 с личной информацией
    } else {
        // Читаем следующие блоки по порядку
        if (m_session.cardType == CardType::SI10 && m_session.currentBlock == 3) {
            // После блока 3 переходим к блоку 4 (отметки)
            nextBlock = 4;
        } else {
            nextBlock = m_session.currentBlock + 1;
        }
    }
    
    if (nextBlock < 0 || nextBlock >= 8) {
        handleErrorState(tr("Invalid next block: %1").arg(nextBlock));
        return;
    }
    
    m_session.currentBlock = nextBlock;
    
    emit debugMessage(tr("Requesting block %1").arg(nextBlock));
    changeState(CardReadState::AwaitingBlock);
    
    // Запрашиваем блок
    emit requestBlock(static_cast<uint8_t>(nextBlock));
    
    // Запускаем таймер таймаута
    m_timeoutTimer->start(READ_TIMEOUT);
}

void CardReadManager::completeReading() {
    emit debugMessage(tr("All blocks received, decoding data..."));
    
    try {
        // Декодируем собранные данные
        m_session.decodedData = DataDecoder::decodeCardData(
            m_session.rawData, 
            m_session.cardType,
            m_session.detectionTime
        );
        
        // Устанавливаем номер карты
        m_session.decodedData.cardNumber = m_session.cardNumber;
        m_session.decodedData.cardType = m_session.cardType;
        
        emit debugMessage(tr("Card data decoded successfully"));
        emit debugMessage(tr("Card %1: %2 punches, start: %3, finish: %4")
                    .arg(QString::number(m_session.decodedData.cardNumber)
                        ,QString::number(m_session.decodedData.punches.size())
                        ,m_session.decodedData.startTime.toString("hh:mm:ss")
                        ,m_session.decodedData.finishTime.toString("hh:mm:ss")));
        
        changeState(CardReadState::ReadingComplete);
        
        // Отправляем данные
        emit cardDataReady(m_session.decodedData);
        
        // Автоматическое подтверждение, если включено
        if (m_autoAcknowledge) {
            sendAcknowledge();
        } else {
            emit acknowledgeRequired();
        }
        
    } catch (const std::exception& e) {
        handleErrorState(tr("Error decoding card data: %1").arg(e.what()));
    }
}

void CardReadManager::handleErrorState(const QString& error) {
    emit debugMessage(tr("Read error: %1").arg(error));
    
    m_timeoutTimer->stop();
    m_retryTimer->stop();
    
    changeState(CardReadState::Error);
    emit readError(error);
    
    // Сбрасываем сессию через некоторое время
    QTimer::singleShot(1000, this, [this]() {
        if (m_session.state == CardReadState::Error) {
            m_session.reset();
            changeState(CardReadState::Idle);
        }
    });
}

} // namespace SportIdent
