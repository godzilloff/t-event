#pragma once
#include <QByteArray>

class IPrinterCommand {
public:
    virtual ~IPrinterCommand() = default;
    virtual QByteArray toByteArray() const = 0;
};
