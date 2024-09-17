#ifndef TORGANIZATIONMODEL_H
#define TORGANIZATIONMODEL_H

#include <QAbstractItemModel>
#include <QObject>

#include "organization.h"
#include "qsportevent.h"

class TorganizationModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TorganizationModel(QObject *parent = nullptr);
    //explicit TorganizationModel(QSportEvent &event);
    ~TorganizationModel();

    int reInit(QSportEvent &event);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;


private:
    QVector<organization*> *organizations_ = nullptr;

signals:
};

#endif // TORGANIZATIONMODEL_H
