// PrinterFacade.h
#pragma once
#include "ImageGenerator.h"
#include "ImageProcessor.h"
#include "SerialPortManager.h"
#include "EscPosPrinter.h"
#include <memory>

class PrinterFacade {
public:
    PrinterFacade();

    bool connect(const QString& portName, qint32 baudRate = QSerialPort::Baud460800);
    void disconnect();
    bool isConnected() const { return m_serialManager->isOpen(); }

    // Печать текста (превращает текст в изображение)
    bool printText(const QString& text,
                   ColorMode colorMode = ColorMode::BlackTextOnWhiteBackground,
                   const PrintSettings& settings = PrintSettings());

    // Печать готового изображения
    bool printImage(const QImage& image,
                    ColorMode colorMode = ColorMode::BlackTextOnWhiteBackground);

    // Печать изображения с предварительной обработкой (обрезать пустые края, применить цветовой режим)
    bool printProcessedImage(const QImage& image,
                             ColorMode colorMode = ColorMode::BlackTextOnWhiteBackground,
                             bool cropEmptySpace = true,
                             bool cutPaper = true);

    // Удобные методы для быстрой печати
    bool printNormal(const QString& text);      // чёрный на белом
    bool printInverted(const QString& text);    // белый на чёрном
    bool printNormalImage(const QImage& image); // чёрное на белом (изображение)
    bool printInvertedImage(const QImage& image); // белое на чёрном (изображение)

    void setDefaultSettings(const PrintSettings& settings);
    PrintSettings defaultSettings() const;

    // Дополнительные настройки
    void setDefaultColorMode(ColorMode mode);
    ColorMode defaultColorMode() const;

private:
    // Внутренний метод для отправки изображения на принтер
    bool sendImageToPrinter(const QImage& image, bool cutPaper = true);

    std::unique_ptr<SerialPortManager> m_serialManager;
    ImageGenerator m_imageGenerator;
    PrintSettings m_defaultSettings;
    ColorMode m_defaultColorMode = ColorMode::BlackTextOnWhiteBackground;
};
