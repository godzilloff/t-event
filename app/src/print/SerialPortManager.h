// SerialPortManager.h
#pragma once
#include <QSerialPort>
#include <QString>
#include <QByteArray>

class SerialPortManager {
public:
    SerialPortManager();
    ~SerialPortManager();
    
    bool open(const QString& portName, qint32 baudRate = QSerialPort::Baud460800);
    void close();
    bool write(const QByteArray& data);
    bool isOpen() const;
    QString lastError() const;

private:
    QSerialPort m_serial;
    QString m_lastError;
};
