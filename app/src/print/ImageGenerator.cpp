// ImageGenerator.cpp
#include "ImageGenerator.h"
#include <QPainter>

ImageGenerator::ImageGenerator(const PrintSettings& settings)
    : m_settings(settings)
{}

QImage ImageGenerator::generateFromText(const QString& text) const {
    QImage image(m_settings.width, m_settings.maxHeight, QImage::Format_Mono);
    image.fill(1);  // 1 = белый фон
    
    QPainter painter(&image);
    painter.setPen(Qt::black);
    painter.setFont(m_settings.font);
    
    int textWidth = m_settings.width - m_settings.leftMargin - m_settings.rightMargin;
    QRect textRect(m_settings.leftMargin, m_settings.topMargin,
                   textWidth, m_settings.maxHeight - m_settings.topMargin);
    
    painter.drawText(textRect, Qt::TextWordWrap | Qt::AlignTop | Qt::AlignLeft, text);
    painter.end();
    
    return image;
}

void ImageGenerator::setSettings(const PrintSettings& settings) {
    m_settings = settings;
}

const PrintSettings& ImageGenerator::settings() const {
    return m_settings;
}
