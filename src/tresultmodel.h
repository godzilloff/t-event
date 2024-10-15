#ifndef TRESULTMODEL_H
#define TRESULTMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QObject>

#include "result.h"
#include "race.h"
#include "qsportevent.h"

class TresultModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum ColNumTableResult {
        CName = 0,
        CSurname,
        CGroup,
        COrg,
        CBib,
        CCardNum,
        CStart,
        CFinish,
        CResult,
        CStatus,
        COffset,
        CPenalty,
        CPenaltyLaps,
        CRank,
        CType,
        CRent
    };

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

// ==============

class TResultProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    TResultProxyModel(QObject *parent = nullptr){};

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

protected:
    //bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    //bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:

};

#endif // TRESULTMODEL_H
