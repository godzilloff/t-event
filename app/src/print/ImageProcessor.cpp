// ImageProcessor.cpp
#include "ImageProcessor.h"

#include <QDir>
#include <QDateTime>

QImage ImageProcessor::cropEmptySpace(const QImage& image, int bottomMargin) {
    int lastNonEmptyRow = 0;
    int width = image.width();
    int widthBytes = (width + 7) / 8;
    
    for (int y = image.height() - 1; y >= 0; y--) {
        const uchar* scanLine = image.scanLine(y);
        bool emptyRow = true;
        
        for (int x = 0; x < widthBytes; x++) {
            if (scanLine[x] != 0xFF) {  // 0xFF = полностью белая строка
                emptyRow = false;
                break;
            }
        }
        
        if (!emptyRow) {
            lastNonEmptyRow = y;
            break;
        }
    }
    
    int newHeight = qMin(lastNonEmptyRow + bottomMargin, image.height() - 1);
    return image.copy(0, 0, width, newHeight);
}

QImage ImageProcessor::invertColors(const QImage& image) {
    QImage inverted = image.convertToFormat(QImage::Format_Mono);
    
    for (int y = 0; y < inverted.height(); y++) {
        uchar* scanLine = inverted.scanLine(y);
        int widthBytes = (inverted.width() + 7) / 8;
        
        for (int x = 0; x < widthBytes; x++) {
            scanLine[x] = ~scanLine[x];  // Инверсия битов
        }
    }
    
    return inverted;
}

QImage ImageProcessor::prepareForPrint(const QImage& image, ColorMode mode) {
    QImage result = image;
    
    // Сначала обрезаем пустое пространство
    result = cropEmptySpace(result);
    
    // Затем применяем цветовой режим
    if (mode == ColorMode::WhiteTextOnBlackBackground) {
        result = invertColors(result);
    }
    
    return result;
}

void ImageProcessor::saveImageToFile(const QImage& image, const QString& prefix) {
    QString filename = QString("%1_%2.png")
                           .arg(prefix)
                           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz"));

    QString path = QDir::currentPath() + "/debug_output/" + filename;

    // Создаём папку, если её нет
    QDir().mkpath("debug_output");

    // Сохраняем в PNG (без потерь)
    image.save(path, "PNG");
    qDebug() << "Сохранён превью:" << path;
}
