#include "functions.h"

// Функция для разделения строки на две части
void splitString(const QString &inputString, QString &lastName, QString &firstNameMiddleName) {
    QString input = inputString.trimmed();
    // Поиск пробела в строке
    int spaceIndex = input.indexOf(' ');

    // Если пробел найден
    if (spaceIndex != -1) {
        // Получение фамилии
        lastName = input.left(spaceIndex).trimmed();
        // Удаление фамилии из исходной строки
        input.remove(0, spaceIndex + 1);

        firstNameMiddleName = input.trimmed();

        // Если в строке есть ещё один пробел
        // if (input.contains(' ')) {
        //     // Разделение имени и отчества
        //     int nextSpaceIndex = input.indexOf(' ');
        //     firstNameMiddleName = input.mid(0, nextSpaceIndex);
        //     input.remove(0, nextSpaceIndex + 1); // Удаление имени из строки
        // } else {
        //     // Если в строке только имя после фамилии, то добавляем его к отчеству
        //     firstNameMiddleName = input;
        // }
    } else { // Если пробел не найден, то предполагается, что в строке только фамилия и имя
        lastName = input.trimmed();
        firstNameMiddleName = "";
    }
}

static QString formatDurationTenths(qint64 msecs)
{
    if (msecs < 0)
        msecs = 0;

    // округление до десятых
    qint64 tenths = (msecs + 50) / 100;     // всего десятых секунд
    const qint64 h = tenths / 36000;  tenths %= 36000;
    const qint64 m = tenths / 600;    tenths %= 600;
    const qint64 s = tenths / 10;
    const qint64 t = tenths % 10;

    return QStringLiteral("%1:%2:%3.%4")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'))
        .arg(t);
}

// Время суток в десятых долях секунды, округлённое к ближайшему.
// Возвращает -1, если отметки нет.
qint64 tenthsOfDay(const QDateTime& dt)
{
    if (!dt.isValid())
        return -1;

    const QTime t = dt.time();
    const qint64 ms = t.hour()   * 3600000LL
                      + t.minute() *   60000LL
                      + t.second() *    1000LL
                      + t.msec();

    return (ms + 50) / 100;   // округление, а не отсечение
}

// Форматирование "HH:mm:ss.t"
QString formatTenths(qint64 tenths)
{
    if (tenths < 0)
        tenths = 0;

    const qint64 h = tenths / 36000; tenths %= 36000;
    const qint64 m = tenths /   600; tenths %=   600;
    const qint64 s = tenths /    10;
    const qint64 t = tenths %    10;

    return QStringLiteral("%1:%2:%3.%4")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'))
        .arg(t);
}
