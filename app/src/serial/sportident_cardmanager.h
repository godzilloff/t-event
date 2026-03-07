#ifndef SPORTIDENT_CARDMANAGER_H
#define SPORTIDENT_CARDMANAGER_H

#include "sportident_types.h"
#include <QObject>
#include <QTimer>

namespace SportIdent {

class SportIdentStation;

class CardReadManager : public QObject {
    Q_OBJECT

public:
    explicit CardReadManager(SportIdentStation* station, QObject* parent = nullptr);
    ~CardReadManager();

    // Основные методы
    void startReading(uint32_t cardNumber, CardType cardType);
    void cancelReading();
    void sendAcknowledge();
    
    // Обработка событий от станции
    void handleBlockData(uint8_t blockNumber, const QByteArray& data);
    void handleCardRemoved();
    void handleError(const QString& error);
    
    // Состояние
    void reset();
    bool isReading() const { return m_session.inProgress(); }
    bool isComplete() const { return m_session.isComplete(); }
    CardData getCardData() const { return m_session.decodedData; }
    CardReadState getState() const { return m_session.state; }
    
    // Настройки
    void setAutoAcknowledge(bool enable) { m_autoAcknowledge = enable; }
    void setReadAllBlocks(bool enable) { m_readAllBlocks = enable; }
    
signals:
    void stateChanged(CardReadState newState, CardReadState oldState);
    void progressChanged(int blocksRead, int totalBlocks);
    void cardDataReady(const CardData& cardData);
    void readComplete(const CardData& cardData);
    void readError(const QString& error);
    void acknowledgeRequired();
    void debugMessage(const QString& message);
    
    // Сигналы для станции
    void requestBlock(uint8_t blockNumber);
    void sendAckSignal();
    void cancelCurrentOperation();

private slots:
    void onTimeout();
    void onRetryTimeout();

private:
    // Внутренние методы
    void changeState(CardReadState newState);
    void sendNextBlockRequest();
    void processBlock0(const QByteArray& data);
    void determineBlocksToRead();
    void completeReading();
    void handleErrorState(const QString& error);
    
    // Таймеры
    QTimer* m_timeoutTimer;
    QTimer* m_retryTimer;
    int m_retryCount;
    
    // Сессия чтения
    ReadSession m_session;
    
    // Настройки
    bool m_autoAcknowledge;
    bool m_readAllBlocks;
    int m_maxRetries;
    
    // Ссылка на станцию
    SportIdentStation* m_station;
    
    // Константы
    static constexpr int READ_TIMEOUT = 2000;  // 2 секунды на блок
    static constexpr int RETRY_DELAY = 500;    // 0.5 секунды между повторами
    static constexpr int MAX_RETRIES = 3;
};

} // namespace SportIdent

#endif // SPORTIDENT_CARDMANAGER_H
