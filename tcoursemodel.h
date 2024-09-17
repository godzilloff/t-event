#ifndef TCOURSEMODEL_H
#define TCOURSEMODEL_H

#include <QAbstractItemModel>
#include <QObject>

#include "course.h"
#include "qsportevent.h"

class TcourseModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TcourseModel(QObject *parent = nullptr);
    //explicit TcourseModel(QSportEvent &event);
    ~TcourseModel();

    int reInit(QSportEvent &event);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;


private:
    QVector<course*> *courses_ = nullptr;

signals:
};

#endif // TCOURSEMODEL_H
