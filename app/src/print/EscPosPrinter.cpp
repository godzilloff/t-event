// EscPosPrinter.cpp
#include "EscPosPrinter.h"
#include <QDebug>

// === PrintImageCommand ===
PrintImageCommand::PrintImageCommand(const QImage& image)
    : m_image(image)
{}

QByteArray PrintImageCommand::toByteArray() const {
    QByteArray command;
    
    // GS v 0 m xL xH yL yH d1...dk
    command.append(static_cast<char>(0x1D));  // GS
    command.append(static_cast<char>(0x76));  // v
    command.append(static_cast<char>(0x30));  // 0
    command.append(static_cast<char>(0x00));  // m
    
    int widthBytes = (m_image.width() + 7) / 8;
    command.append(static_cast<char>(widthBytes % 256));
    command.append(static_cast<char>(widthBytes / 256));
    
    int height = m_image.height();
    command.append(static_cast<char>(height % 256));
    command.append(static_cast<char>(height / 256));
    
    command.append(imageToRasterData(m_image));
    
    return command;
}

QByteArray PrintImageCommand::imageToRasterData(const QImage& image) const {
    QByteArray rasterData;
    QImage bwImage = image.convertToFormat(QImage::Format_Mono);
    
    int widthBytes = (bwImage.width() + 7) / 8;
    
    for (int y = 0; y < bwImage.height(); y++) {
        const uchar* scanLine = bwImage.scanLine(y);
        for (int x = 0; x < widthBytes; x++) {
            // Инверсия для правильной интерпретации принтером
            rasterData.append(static_cast<char>(~scanLine[x]));
        }
    }
    
    return rasterData;
}

// === CutPaperCommand ===
QByteArray CutPaperCommand::toByteArray() const {
    QByteArray command;
    command.append(static_cast<char>(0x1B));  // ESC
    command.append(static_cast<char>(0x64));  // d
    command.append(static_cast<char>(0x00));  // 0
    return command;
}

// === InitializePrinterCommand ===
QByteArray InitializePrinterCommand::toByteArray() const {
    QByteArray command;
    command.append(static_cast<char>(0x1B));  // ESC
    command.append(static_cast<char>(0x40));  // @
    return command;
}

// === CompositeCommand ===
void CompositeCommand::addCommand(std::unique_ptr<IPrinterCommand> command) {
    m_commands.push_back(std::move(command));
}

QByteArray CompositeCommand::toByteArray() const {
    QByteArray result;
    for (const auto& cmd : m_commands) {
        result.append(cmd->toByteArray());
    }
    return result;
}
