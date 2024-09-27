#ifndef QSPORTEVENT_H
#define QSPORTEVENT_H

#include <QObject>
#include <QVector>

#include "race.h"

class QSportEvent : public QObject
{
    Q_OBJECT
public:
    explicit QSportEvent(QObject *parent = nullptr);
    ~QSportEvent();

    void importSportorgJSON(const QString& path);
    void exportSportorgJSON(const QString& path);

    const QString getNameOrganization(const QString& id);
    const QString getNameGroup(const QString& id);

    QVector<race*> getRaces(){return races_;};
    const st_race& getDataRace(){return races_.at(0)->getDataRace();};
    const QJsonObject getResultToOnline(const QString& number);
    bool empty(){ return races_.size() == 0;};

signals:


private:
    QVector<race*> races_;
    QJsonObject toJson() const;

};

#endif // QSPORTEVENT_H
