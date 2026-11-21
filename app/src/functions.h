#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QString>
#include <QDateTime>

void splitString(const QString &inputString, QString &lastName, QString &firstNameMiddleName);

static QString formatDurationTenths(qint64 msecs);

qint64 tenthsOfDay(const QDateTime& dt);
QString formatTenths(qint64 tenths);

#endif // FUNCTIONS_H
