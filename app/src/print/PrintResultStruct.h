// PrintResultStruct.h
#ifndef PRINTRESULTSTRUCT_H
#define PRINTRESULTSTRUCT_H

#include <QString>
#include <QDateTime>
#include <QImage>

struct PrintResultData {
    // Информация о соревновании
    QString competitionName;
    QDate competitionDate;
    
    // Информация об участнике
    QString fullName;
    QString bibNumber;
    QString chipNumber;
    QString ageGroup;
    QString delegationName;
    
    // Времена
    QDateTime startTime;
    QDateTime finishTime;
    int resultSeconds;
    
    // QR-код
    QString onlineResultsUrl;
    
    // Дополнительная информация
    QString rank;           // Место
    QString timeLoss;       // Отставание
    
    // Форматирование результата
    QString formattedResult() const {
        QTime resultTime(0, 0, 0);
        resultTime = resultTime.addSecs(resultSeconds);
        return resultTime.toString("hh:mm:ss");
    }
    
    QString formattedStartTime() const {
        return startTime.toString("hh:mm:ss");
    }
    
    QString formattedFinishTime() const {
        return finishTime.toString("hh:mm:ss");
    }
    
    QString formattedDate() const {
        return competitionDate.toString("dd.MM.yyyy");
    }
};

#endif // PRINTRESULTSTRUCT_H
