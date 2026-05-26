// PrinterFacade.cpp
#include "PrinterFacade.h"

#include <QDebug>
#include <QBuffer>

PrinterFacade::PrinterFacade()
    : m_serialManager(std::make_unique<SerialPortManager>())
{}

bool PrinterFacade::connect(const QString& portName, qint32 baudRate) {
    return m_serialManager->open(portName, baudRate);
}

void PrinterFacade::disconnect() {
    m_serialManager->close();
}

// === Печать текста ===
bool PrinterFacade::printText(const QString& text,
                              ColorMode colorMode,
                              const PrintSettings& settings) {
    if (!m_serialManager->isOpen()) {
        qDebug() << "Принтер не подключён";
        return false;
    }

    // 1. Генерируем изображение
    ImageGenerator generator(settings);
    QImage image = generator.generateFromText(text);

    // 2. Обрабатываем изображение (обрезаем + применяем цветовой режим)
    QImage preparedImage = ImageProcessor::prepareForPrint(image, colorMode);

    // 3. Формируем команды ESC/POS
    auto compositeCmd = std::make_unique<CompositeCommand>();
    compositeCmd->addCommand(std::make_unique<InitializePrinterCommand>());
    compositeCmd->addCommand(std::make_unique<PrintImageCommand>(preparedImage));
    compositeCmd->addCommand(std::make_unique<CutPaperCommand>());

    // 4. Отправляем
    return m_serialManager->write(compositeCmd->toByteArray());
}

// === Печать изображения ===
bool PrinterFacade::printImage(const QImage& image,
                               ColorMode colorMode) {
    if (!m_serialManager->isOpen()) {
        qDebug() << "Принтер не подключён";
        return false;
    }

    // 2. Обрабатываем изображение (обрезаем + применяем цветовой режим)
    QImage preparedImage = ImageProcessor::prepareForPrint(image, colorMode);

    // 3. Формируем команды ESC/POS
    auto compositeCmd = std::make_unique<CompositeCommand>();
    compositeCmd->addCommand(std::make_unique<InitializePrinterCommand>());
    compositeCmd->addCommand(std::make_unique<PrintImageCommand>(preparedImage));
    compositeCmd->addCommand(std::make_unique<CutPaperCommand>());

    // 4. Отправляем
    return m_serialManager->write(compositeCmd->toByteArray());
}

// === Удобные методы ===
bool PrinterFacade::printNormal(const QString& text) {
    return printText(text, ColorMode::BlackTextOnWhiteBackground, m_defaultSettings);
}

bool PrinterFacade::printInverted(const QString& text) {
    return printText(text, ColorMode::WhiteTextOnBlackBackground, m_defaultSettings);
}

bool PrinterFacade::printNormalImage(const QImage& image) {
    return printImage(image, ColorMode::BlackTextOnWhiteBackground);
}

bool PrinterFacade::printInvertedImage(const QImage& image) {
    return printImage(image, ColorMode::WhiteTextOnBlackBackground);
}

void PrinterFacade::setDefaultSettings(const PrintSettings& settings) {
    m_defaultSettings = settings;
}

PrintSettings PrinterFacade::defaultSettings() const {
    return m_defaultSettings;
}

void PrinterFacade::setDefaultColorMode(ColorMode mode) {
    m_defaultColorMode = mode;
}

ColorMode PrinterFacade::defaultColorMode() const {
    return m_defaultColorMode;
}
