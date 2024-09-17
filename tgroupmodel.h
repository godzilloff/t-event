#ifndef TGROUPMODEL_H
#define TGROUPMODEL_H

#include <QAbstractItemModel>
#include <QObject>

#include "group.h"
#include "qsportevent.h"

class TgroupModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TgroupModel(QObject *parent = nullptr);
    //explicit TgroupModel(QSportEvent &event);
    ~TgroupModel();

    int reInit(QSportEvent &event);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;


private:
    QVector<group*> *groups_ = nullptr;
    //QVector<course*> *courses_ = nullptr;
    race* thisRace = nullptr;

signals:
};

#endif // TGROUPMODEL_H
