#include "tresultmodel.h"

TresultModel::TresultModel(QObject *parent)
    : QAbstractTableModel{parent}
{}

TresultModel::~TresultModel()
{}

int TresultModel::reInit(std::shared_ptr<QSportEvent>& event)
{
    event_ptr = event;
    persons_ = event->getRaces().at(0)->getPtrPersons();
    results_ = event->getRaces().at(0)->getPtrResults();
    thisRace = event->getRaces().at(0);
    return 0;
}

int TresultModel::rowCount(const QModelIndex &parent) const
{
    return results_->count();
}

int TresultModel::columnCount(const QModelIndex &parent) const
{
    return 16;
}

QVariant TresultModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole){
        QString res ="";
        if (results_->count()>index.row()){
            int bib = results_->at(index.row())->getBibResult();
            switch (index.column()) {
            case CName:     res = thisRace->getNameFromBib(bib); break;
            case CSurname:  res = thisRace->getSurnameFromBib(bib); break;
            case CGroup:    res = thisRace->getNameGroupFromBib(bib); break;
            case COrg:      res = thisRace->getNameOrganizationFromBib(bib); break;
            case CBib:      res = QString::number(results_->at(index.row())->getBibResult()); break;
            case CCardNum:  res = QString::number(results_->at(index.row())->getCardNumber() ); break;
            case CStart:    res = QTime::fromMSecsSinceStartOfDay(results_->at(index.row())->getStartMsec()).toString("hh:mm:ss"); break;
            case CFinish:   res = QTime::fromMSecsSinceStartOfDay(results_->at(index.row())->getFinishMsec()).toString("hh:mm:ss"); break;
            case CResult:   res = QTime::fromMSecsSinceStartOfDay(results_->at(index.row())->getResultMsec()).toString("hh:mm:ss.z"); break;
            case CStatus:   res = results_->at(index.row())->getResultStatusName();
            default:
                break;
            }
        }
        return res;
    }
    return QVariant();
}

QVariant TresultModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case CName:     return QVariant(QString::fromUtf8("Фамилия"));
        case CSurname:  return QVariant(QString::fromUtf8("Имя"));
        case CGroup:    return QVariant(QString::fromUtf8("Группа"));
        case COrg:      return QVariant(QString::fromUtf8("Команда"));
        case CBib:      return QVariant(QString::fromUtf8("Номер"));
        case CCardNum:  return QVariant(QString::fromUtf8("№ чипа"));
        case CStart:    return QVariant(QString::fromUtf8("Старт"));
        case CFinish:   return QVariant(QString::fromUtf8("Финиш"));
        case CResult:   return QVariant(QString::fromUtf8("Результат"));
        case CStatus:   return QVariant(QString::fromUtf8("Статус"));
        case COffset:   return QVariant(QString::fromUtf8("Отсечка"));
        case CPenalty:  return QVariant(QString::fromUtf8("Штраф"));
        case CPenaltyLaps:return QVariant(QString::fromUtf8("Штр.круги"));
        case CRank:     return QVariant(QString::fromUtf8("Место"));
        case CType:     return QVariant(QString::fromUtf8("Тип"));
        case CRent:     return QVariant(QString::fromUtf8("Аренда чипа"));
        default:        return QVariant();
        }
    }
    return section + 1;
}

// ==============

QVariant TResultProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return sourceModel()->headerData(section, orientation, role);
}
