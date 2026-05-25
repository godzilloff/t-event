//isportident_interface.h

#ifndef ISPORTIDENT_INTERFACE_H
#define ISPORTIDENT_INTERFACE_H

#include "sportident_types.h"
#include <QObject>

namespace SportIdent {

class ISportIdentInterface : public QObject {
    Q_OBJECT

public:
    explicit ISportIdentInterface(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ISportIdentInterface() = default;

    // Подключение/отключение
    virtual void reset() = 0;
    virtual bool connectToStation(const QString& portName, int baudRate = 38400) = 0; // Изменили имя
    virtual void disconnectToStation() = 0;
    virtual bool isConnectedToStation() const = 0;

    // Основные операции
    virtual void acknowledgeCard() = 0;

    // Управление станцией
    virtual void setStationTimeTxt() = 0;
    virtual void beep(int count = 1) = 0;
    virtual void powerOff() = 0;
    virtual QDateTime getStationTime() = 0;
    virtual void setStationTime(const QDateTime& time) = 0;
    virtual StationInfo getStationInfo() = 0;
    virtual StationConfig getStationConfig() = 0;
    
    // Конфигурация
    virtual void setOperatingMode(uint8_t mode) = 0;
    virtual void setStationCode(uint16_t code) = 0;
    virtual void setExtendedProtocol(bool extended) = 0;
    virtual void setAutoSendMode(bool autoSend) = 0;

signals:
    void cardDetected(uint32_t cardNumber, CardType type);
    void cardRemoved();
    void cardReadComplete(const CardData& cardData);
    void stationConnected(const StationInfo& info);
    void stationDisconnected();
    void errorOccurred(const QString& errorMessage);
    void debugMessage(const QString& message);
    void rawDataReceived(const QByteArray& data);
    void rawDataSent(const QByteArray& data);
};

} // namespace SportIdent

#endif // ISPORTIDENT_INTERFACE_H
