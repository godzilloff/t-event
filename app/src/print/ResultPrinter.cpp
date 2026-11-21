// ResultPrinter.cpp
#include "ResultPrinter.h"
#include <QPainter>
#include <QFont>
#include <QFontDatabase>
#include <QDebug>
#include <QDateTime>
#include <QUrl>
#include <QByteArray>
#include <QBuffer>
#include <QCryptographicHash>

// Простая реализация QR-кода (можно заменить на реальную библиотеку)
// Для production используйте библиотеку qrencode
static QImage generateSimpleQRCode(const QString& text, int size) {
    QImage qrCode(size, size, QImage::Format_Mono);
    qrCode.fill(Qt::white);
    
    QPainter painter(&qrCode);
    painter.setPen(Qt::black);
    
    // Создаем простую сетку для демонстрации
    // В реальном проекте используйте библиотеку qrencode
    int cellSize = size / 21; // 21x21 - минимальный размер QR-кода
    if (cellSize < 1) cellSize = 1;
    
    // Заполняем простым узором (для демонстрации)
    for (int i = 0; i < 21 && i * cellSize < size; ++i) {
        for (int j = 0; j < 21 && j * cellSize < size; ++j) {
            if ((i + j) % 2 == 0) {
                painter.fillRect(i * cellSize, j * cellSize, cellSize, cellSize, Qt::black);
            }
        }
    }
    
    // Добавляем угловые маркеры
    int markerSize = 7 * cellSize;
    painter.fillRect(0, 0, markerSize, markerSize, Qt::black);
    painter.fillRect(cellSize, cellSize, markerSize - 2*cellSize, markerSize - 2*cellSize, Qt::white);
    painter.fillRect(2*cellSize, 2*cellSize, markerSize - 4*cellSize, markerSize - 4*cellSize, Qt::black);
    
    painter.fillRect(size - markerSize, 0, markerSize, markerSize, Qt::black);
    painter.fillRect(size - markerSize + cellSize, cellSize, markerSize - 2*cellSize, markerSize - 2*cellSize, Qt::white);
    painter.fillRect(size - markerSize + 2*cellSize, 2*cellSize, markerSize - 4*cellSize, markerSize - 4*cellSize, Qt::black);
    
    painter.fillRect(0, size - markerSize, markerSize, markerSize, Qt::black);
    painter.fillRect(cellSize, size - markerSize + cellSize, markerSize - 2*cellSize, markerSize - 2*cellSize, Qt::white);
    painter.fillRect(2*cellSize, size - markerSize + 2*cellSize, markerSize - 4*cellSize, markerSize - 4*cellSize, Qt::black);
    
    painter.end();
    
    // Конвертируем в RGB для лучшего отображения
    return qrCode.convertToFormat(QImage::Format_RGB32);
}

ResultPrinter::ResultPrinter(QObject* parent)
    : QObject(parent)
{
    // Настройки по умолчанию для термопринтера
    m_defaultSettings.width = 384;  // 58mm принтер
    m_defaultSettings.font = QFont("Arial", 18);
    m_defaultSettings.bottomMargin = 10;
    m_defaultSettings.topMargin = 10;
    m_defaultSettings.leftMargin = 10;
    m_defaultSettings.rightMargin = 10;
}

void ResultPrinter::setOnlineResultsBaseUrl(const QString& url)
{
    m_onlineResultsBaseUrl = url;
}

QImage ResultPrinter::generateQRCode(const QString& url, int size)
{
    return createQRCodeImage(url, size);
}

QImage ResultPrinter::createQRCodeImage(const QString& url, int size)
{
    // Для реального использования подключите libqrencode
    // Пример интеграции с qrencode:
    /*
    QRcode* qrcode = QRcode_encodeString(url.toUtf8().constData(), 
                                         0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (qrcode) {
        QImage image(qrcode->width, qrcode->width, QImage::Format_Mono);
        // ... заполнение изображения
        QRcode_free(qrcode);
        return image;
    }
    */
    
    // Временная реализация для демонстрации
    return generateSimpleQRCode(url, size);
}

QImage ResultPrinter::generateResultImage(const PrintResultData& data, 
                                           const PrintSettings& settings)
{
    PrintSettings printSettings = settings;
    if (printSettings.width <= 0) {
        printSettings.width = m_defaultSettings.width;
    }
    if (printSettings.font.pointSize() <= 0) {
        printSettings.font.setPointSize(12);
    }
    
    // Предварительный расчет высоты
    int height = 500;
    QImage image(printSettings.width, height, QImage::Format_RGB32);
    image.fill(Qt::white);
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    
    int y = printSettings.bottomMargin;
    
    // Рисуем заголовок
    drawHeader(painter, data, y);
    
    // Рисуем информацию об участнике
    drawParticipantInfo(painter, data, y);
    
    // Рисуем времена
    drawTimes(painter, data, y);
    
    // Генерируем и рисуем QR-код
    if (!data.onlineResultsUrl.isEmpty()) {
        QImage qrCode = createQRCodeImage(data.onlineResultsUrl, 150);
        drawQRCode(painter, qrCode, y);
    }
    
    // Рисуем подвал
    // drawFooter(painter, y);
    
    // Обрезаем изображение по реальной высоте
    if (y + 20 < height) {
        return image.copy(0, 0, printSettings.width, y + 20);
    }
    
    return image;
}

void ResultPrinter::drawHeader(QPainter& painter, const PrintResultData& data, int& y)
{
    int margin = 10;
    int width = painter.device()->width();
    
    // Жирный шрифт для заголовка
    QFont titleFont = painter.font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    
    // Название соревнования
    QRect titleRect(margin, y, width - 2*margin, 40);
    painter.drawText(titleRect, Qt::AlignCenter, data.competitionName);
    y += 45;
}

void ResultPrinter::drawParticipantInfo(QPainter& painter, const PrintResultData& data, int& y)
{
    int margin = 10;
    int width = painter.device()->width();
    int labelWidth = 100;
    int valueStart = labelWidth + margin;
    
    QFont normalFont = painter.font();
    normalFont.setPointSize(18);
    painter.setFont(normalFont);
    
    // ФИО (большим шрифтом)
    QFont nameFont = normalFont;
    nameFont.setPointSize(18);
    nameFont.setBold(true);
    painter.setFont(nameFont);
    
    QRect nameRect(margin, y, width - 2*margin, 35);
    painter.drawText(nameRect, Qt::AlignCenter, data.fullName);
    y += 40;
    
    painter.setFont(normalFont);
    
    // Номер участника
    painter.drawText(margin, y + 15, "Номер:");
    painter.drawText(valueStart, y + 15, data.bibNumber);
    y += 25;

    // Чип
    painter.drawText(margin, y + 15, "Чип:");
    painter.drawText(valueStart, y + 15, data.chipNumber);
    y += 25;
    
    // Группа
    if (!data.ageGroup.isEmpty()) {
        painter.drawText(margin, y + 15, "Группа:");
        painter.drawText(valueStart, y + 15, data.ageGroup);
        y += 25;
    }
    
    // Делегация
    if (!data.delegationName.isEmpty()) {
        painter.drawText(margin, y + 15, "Команда:");
        painter.drawText(valueStart+15, y + 15, data.delegationName);
        y += 25;
    }

    y += 15;
}

void ResultPrinter::drawTimes(QPainter& painter, const PrintResultData& data, int& y)
{
    int margin = 10;
    // int width = painter.device()->width();
    int labelWidth = 80;
    int valueStart = labelWidth + margin + 60;
    
    QFont normalFont = painter.font();
    normalFont.setPointSize(18);
    painter.setFont(normalFont);
    
    // Время старта
    painter.drawText(margin, y + 15, "Старт:");
    painter.drawText(valueStart, y + 15, data.formattedStartTime());
    y += 30;
    
    // Время финиша
    painter.drawText(margin, y + 15, "Финиш:");
    painter.drawText(valueStart, y + 15, data.formattedFinishTime());
    y += 30;
    
    // Результат (крупным шрифтом)
    QFont resultFont = normalFont;
    resultFont.setPointSize(18);
    resultFont.setBold(true);
    painter.setFont(resultFont);
    
    painter.drawText(margin, y + 20, "РЕЗУЛЬТАТ:");
    painter.drawText(valueStart + 10, y + 20, data.formattedResult());
    y += 60;
}

void ResultPrinter::drawQRCode(QPainter& painter, const QImage& qrCode, int& y)
{
    int margin = 10;
    int width = painter.device()->width();
    
    if (qrCode.isNull()) {
        return;
    }
    
    // Масштабируем QR-код под ширину
    int qrSize = qMin(150, width - 2*margin);
    QImage scaledQR = qrCode.scaled(qrSize, qrSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    int qrX = (width - scaledQR.width()) / 2;
    
    painter.drawImage(qrX, y, scaledQR);
    y += scaledQR.height() + 10;
    
    // Подпись под QR-кодом
    QFont smallFont = painter.font();
    smallFont.setPointSize(8);
    painter.setFont(smallFont);
    
    QRect textRect(margin, y, width - 2*margin, 20);
    painter.drawText(textRect, Qt::AlignCenter, "QR-код для просмотра онлайн-результатов");
    y += 25;
}

void ResultPrinter::drawFooter(QPainter& painter, int& y)
{
    int margin = 10;
    int width = painter.device()->width();
    
    QFont smallFont = painter.font();
    smallFont.setPointSize(8);
    painter.setFont(smallFont);
    painter.setPen(QPen(Qt::gray, 1));
    
    QDateTime now = QDateTime::currentDateTime();
    QString footer = QString("Распечатано: %1").arg(now.toString("dd.MM.yyyy HH:mm:ss"));
    
    QRect footerRect(margin, y, width - 2*margin, 15);
    painter.drawText(footerRect, Qt::AlignCenter, footer);
    y += 20;
    
    // Линия отреза
    painter.setPen(QPen(Qt::black, 1, Qt::DotLine));
    for (int x = margin; x < width - margin; x += 10) {
        painter.drawLine(x, y, x + 5, y);
    }
    y += 10;
}
