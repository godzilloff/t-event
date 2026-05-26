// EscPosPrinter.h
#pragma once
#include "IPrinterCommand.h"
#include <QImage>
#include <QByteArray>

class PrintImageCommand : public IPrinterCommand {
public:
    explicit PrintImageCommand(const QImage& image);
    QByteArray toByteArray() const override;

private:
    QByteArray imageToRasterData(const QImage& image) const;
    QImage m_image;
};

class CutPaperCommand : public IPrinterCommand {
public:
    QByteArray toByteArray() const override;
};

class InitializePrinterCommand : public IPrinterCommand {
public:
    QByteArray toByteArray() const override;
};

class CompositeCommand : public IPrinterCommand {
public:
    void addCommand(std::unique_ptr<IPrinterCommand> command);
    QByteArray toByteArray() const override;

private:
    std::vector<std::unique_ptr<IPrinterCommand>> m_commands;
};
