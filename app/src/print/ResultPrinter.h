// ResultPrinter.h
#ifndef RESULTPRINTER_H
#define RESULTPRINTER_H

#include <QObject>
#include <QImage>
#include "PrintResultStruct.h"
#include "print/ImageGenerator.h"
#include "print/ImageProcessor.h"

class ResultPrinter : public QObject
{
    Q_OBJECT

public:
    explicit ResultPrinter(QObject* parent = nullptr);
    
    // Генерация изображения для печати
    QImage generateResultImage(const PrintResultData& data, 
                               const PrintSettings& settings = PrintSettings());
    
    // Генерация QR-кода
    QImage generateQRCode(const QString& url, int size = 200);
    
    // Установка/получение URL для online результатов
    void setOnlineResultsBaseUrl(const QString& url);
    QString onlineResultsBaseUrl() const { return m_onlineResultsBaseUrl; }

private:
    void drawHeader(QPainter& painter, const PrintResultData& data, int& y);
    void drawParticipantInfo(QPainter& painter, const PrintResultData& data, int& y);
    void drawTimes(QPainter& painter, const PrintResultData& data, int& y);
    void drawQRCode(QPainter& painter, const QImage& qrCode, int& y);
    void drawFooter(QPainter& painter, int& y);
    
    QImage createQRCodeImage(const QString& url, int size);
    
    QString m_onlineResultsBaseUrl;
    PrintSettings m_defaultSettings;
};

#endif // RESULTPRINTER_H
