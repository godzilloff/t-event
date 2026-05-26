// ImageGenerator.h
#pragma once
#include <QImage>
#include <QString>
#include <QFont>

struct PrintSettings {
    int width = 464;           // ширина в точках (58мм)
    int maxHeight = 1500;      // максимальная высота
    int leftMargin = 1;       // левый отступ
    int rightMargin = 1;      // правый отступ
    int topMargin = 10;        // верхний отступ
    int bottomMargin = 30;     // нижний отступ
    QFont font{QFont("Arial", 22)};  // шрифт
};

class ImageGenerator {
public:
    explicit ImageGenerator(const PrintSettings& settings = PrintSettings());
    
    QImage generateFromText(const QString& text) const;
    void setSettings(const PrintSettings& settings);
    const PrintSettings& settings() const;

private:
    PrintSettings m_settings;
};
