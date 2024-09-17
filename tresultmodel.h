#ifndef TRESULTMODEL_H
#define TRESULTMODEL_H

#include <QAbstractItemModel>
#include <QObject>

#include "result.h"
#include "race.h"
#include "qsportevent.h"

class TresultModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TresultModel(QObject *parent = nullptr);
    //explicit TresultModel(QSportEvent &event);
    ~TresultModel();

    int reInit(QSportEvent &event);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;


private:
    QSportEvent *event_ptr = nullptr;
    QVector<person*> *persons_ = nullptr;
    QVector<result*> *results_ = nullptr;
    race* thisRace = nullptr;

signals:
};

#endif // TRESULTMODEL_H
