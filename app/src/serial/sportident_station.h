// sportident_station.h

#ifndef SPORTIDENT_STATION_H
#define SPORTIDENT_STATION_H

#include "sportident_types.h"
#include "isportident_interface.h"
#include "sportident_cardmanager.h"

#include <QSerialPort>
#include <QTimer>
#include <QQueue>

namespace SportIdent {

class SportIdentStation : public ISportIdentInterface {
    Q_OBJECT

public:
    explicit SportIdentStation(QObject* parent = nullptr);
    ~SportIdentStation() override;

    // Подключение/отключение
    void reset() override;
    bool connectToStation(const QString& portName, int baudRate = 38400) override;
    void disconnectToStation() override;
    bool isConnectedToStation() const override { return m_serialPort && m_serialPort->isOpen(); }

    // Основные операции
    void acknowledgeCard() override;

    // Управление станцией
    void beep(int count = 1) override;
    void powerOff() override;
    QDateTime getStationTime() override;
    void setStationTime(const QDateTime& time) override;
    StationInfo getStationInfo() override;
    StationConfig getStationConfig() override;

    // Конфигурация
    void setOperatingMode(uint8_t mode) override;
    void setStationCode(uint16_t code) override;
    void setExtendedProtocol(bool extended) override;
    void setAutoSendMode(bool autoSend) override;

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

    // Слоты для менеджера чтения карт
    void onRequestBlock(uint8_t blockNumber);
    void onSendAckSignal();
    void onCancelCurrentOperation();

    // Слоты для событий менеджера
    void onCardDataReady(const CardData& cardData);
    void onReadComplete(const CardData& cardData);
    void onReadError(const QString& error);
    void onAcknowledgeRequired();
    void onStateChanged(CardReadState newState, CardReadState oldState);
    void onProgressChanged(int blocksRead, int totalBlocks);

private:
    // Структура команды
    struct CommandFrame {
        uint8_t command;
        QByteArray parameters;
        QByteArray response;
        bool waitingForResponse;
        QDateTime timestamp;

        // Конструктор для удобства
        CommandFrame(uint8_t cmd = 0, const QByteArray& params = QByteArray())
            : command(cmd), parameters(params), waitingForResponse(false) {}
    };

    // Вспомогательные методы
    void initializeStation();
    void updateStationConfig();
    void updateStationInfo();

    // Работа с последовательным портом
    bool sendRawData(const QByteArray& data);
    void sendAck();
    QByteArray readRawData(int timeoutMs = 1000);

    // Работа с протоколом
    CommandFrame sendCommand(uint8_t command, const QByteArray& parameters = QByteArray());
    void sendCommandAsync(uint8_t command, const QByteArray& parameters = QByteArray());

    // Обработка данных
    void processIncomingData();
    //void processIncomingData(const QByteArray& data);
    void handleFrame(uint8_t command, const QByteArray& data);
    void handleCardDetection(const QByteArray& data);
    void handleCardRemoval();

    // Проверка и парсинг фреймов
    QByteArray parseFrame(const QByteArray& rawData);

    // Формирование фреймов
    QByteArray createCommandFrame(uint8_t command, const QByteArray& parameters);

    // Декодирование ответов
    uint16_t decodeStationCode(const QByteArray& data);

    // Менеджер чтения карт
    CardReadManager* m_cardManager;

    // Состояние
    QScopedPointer<QSerialPort> m_serialPort;

    // Буферы данных
    QByteArray m_readBuffer;
    QQueue<CommandFrame> m_commandQueue;

    // Текущее состояние
    bool m_connected;
    bool m_legacyProtocol;
    StationConfig m_stationConfig;
    StationInfo m_stationInfo;

    // Настройки
    bool m_autoAcknowledge;
    bool m_readAllBlocks;
    int m_maxRetries;

    int m_baudRate;
    QString m_portName;

    // Отладка
    bool m_debugEnabled;

    // Константы
    static constexpr int DEFAULT_TIMEOUT = 2000;
    static constexpr int MAX_RETRIES = 3;
    static constexpr int BUFFER_SIZE = 4096;
};

} // namespace SportIdent

#endif // SPORTIDENT_STATION_H
