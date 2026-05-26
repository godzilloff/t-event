// SerialPortManager.cpp
#include "SerialPortManager.h"
#include <QDebug>

SerialPortManager::SerialPortManager() = default;

SerialPortManager::~SerialPortManager() {
    close();
}

bool SerialPortManager::open(const QString& portName, qint32 baudRate) {
    m_serial.setPortName(portName);
    m_serial.setBaudRate(baudRate);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);
    
    if (!m_serial.open(QIODevice::ReadWrite)) {
        m_lastError = m_serial.errorString();
        qDebug() << "Ошибка открытия порта:" << m_lastError;
        return false;
    }
    
    return true;
}

void SerialPortManager::close() {
    if (m_serial.isOpen()) {
        m_serial.close();
    }
}

bool SerialPortManager::write(const QByteArray& data) {
    if (!m_serial.isOpen()) {
        m_lastError = "Порт не открыт";
        return false;
    }
    
    qint64 bytesWritten = m_serial.write(data);
    m_serial.waitForBytesWritten(5000);
    
    if (bytesWritten != data.size()) {
        m_lastError = QString("Отправлено только %1 из %2 байт")
                      .arg(bytesWritten).arg(data.size());
        return false;
    }
    
    return true;
}

bool SerialPortManager::isOpen() const {
    return m_serial.isOpen();
}

QString SerialPortManager::lastError() const {
    return m_lastError;
}
