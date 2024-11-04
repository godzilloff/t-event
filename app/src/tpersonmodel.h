#ifndef TPERSONMODEL_H
#define TPERSONMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QObject>

#include "person.h"
#include "qsportevent.h"

class TpersonModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum ColNumTablePerson {
        CSurname = 0,
        CName,
        CQual,
        CGroup,
        COrg,
        CYear,
        CBib,
        CCardNum,
        CStart,
        CFinish,
        CComent,
        CID
    };

    explicit TpersonModel(QObject *parent = nullptr);
    //explicit TpersonModel(QSportEvent &event);
    ~TpersonModel();

    int reInit(std::shared_ptr<QSportEvent> &event);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;


private:
    std::shared_ptr<QSportEvent> event_ptr = nullptr;
    QVector<person*> *persons_ = nullptr;

signals:
};

// ==============

class TPersonProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    TPersonProxyModel(QObject *parent = nullptr){};

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

protected:
           //bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
           //bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:

};

#endif // TPERSONMODEL_H
