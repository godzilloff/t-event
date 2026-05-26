// ImageProcessor.h
#pragma once
#include <QImage>

enum class ColorMode {
    BlackTextOnWhiteBackground,  // Обычный режим
    WhiteTextOnBlackBackground   // Инвертированный
};

class ImageProcessor {
public:
    static QImage cropEmptySpace(const QImage& image, int bottomMargin = 60);
    static QImage invertColors(const QImage& image);
    static QImage prepareForPrint(const QImage& image, ColorMode mode);
    static void saveImageToFile(const QImage& image, const QString& prefix = "print_preview");
};
