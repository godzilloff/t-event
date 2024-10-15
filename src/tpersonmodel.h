#ifndef TPERSONMODEL_H
#define TPERSONMODEL_H

#include <QAbstractItemModel>
#include <QObject>

#include "person.h"
#include "qsportevent.h"

class TpersonModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TpersonModel(QObject *parent = nullptr);
    //explicit TpersonModel(QSportEvent &event);
    ~TpersonModel();

    int reInit(QSportEvent &event);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;


private:
    QSportEvent *event_ptr = nullptr;
    //QVector<organization*> *organizations_ = nullptr;
    QVector<person*> *persons_ = nullptr;

signals:
};

#endif // TPERSONMODEL_H
