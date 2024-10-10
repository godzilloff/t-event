#include "tresultmodel.h"

TresultModel::TresultModel(QObject *parent)
    : QAbstractTableModel{parent}
{}

TresultModel::~TresultModel()
{}

int TresultModel::reInit(QSportEvent &event)
{
    event_ptr = &event;
    persons_ = event.getRaces().at(0)->getPtrPersons();
    results_ = event.getRaces().at(0)->getPtrResults();
    thisRace = event.getRaces().at(0);
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
            case 0: res = thisRace->getNameFromBib(bib); break;
            case 1: res = thisRace->getSurnameFromBib(bib); break;
            case 2: res = thisRace->getNameGroupFromBib(bib); break;
            case 3: res = thisRace->getNameOrganizationFromBib(bib); break;
            case 4: res = QString::number(results_->at(index.row())->getBibResult()); break;
            case 5: res = QString::number(results_->at(index.row())->getCardNumber() ); break;
            case 6: res = QTime::fromMSecsSinceStartOfDay(results_->at(index.row())->getStartMsec()).toString("hh:mm:ss"); break;
            case 7: res = QTime::fromMSecsSinceStartOfDay(results_->at(index.row())->getFinishMsec()).toString("hh:mm:ss"); break;
            case 8: res = QTime::fromMSecsSinceStartOfDay(results_->at(index.row())->getResultMsec()).toString("hh:mm:ss.z"); break;
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
        case 0:return QVariant(QString::fromUtf8("Фамилия"));
        case 1:return QVariant(QString::fromUtf8("Имя"));
        case 2:return QVariant(QString::fromUtf8("Группа"));
        case 3:return QVariant(QString::fromUtf8("Команда"));
        case 4:return QVariant(QString::fromUtf8("Номер"));
        case 5:return QVariant(QString::fromUtf8("№ чипа"));
        case 6:return QVariant(QString::fromUtf8("Старт"));
        case 7:return QVariant(QString::fromUtf8("Финиш"));
        case 8:return QVariant(QString::fromUtf8("Результат"));
        case 9:return QVariant(QString::fromUtf8("Статус"));
        case 10:return QVariant(QString::fromUtf8("Отсечка"));
        case 11:return QVariant(QString::fromUtf8("Штраф"));
        case 12:return QVariant(QString::fromUtf8("Штр.круги"));
        case 13:return QVariant(QString::fromUtf8("Место"));
        case 14:return QVariant(QString::fromUtf8("Тип"));
        case 15:return QVariant(QString::fromUtf8("Аренда чипа"));
        default: return QVariant();
        }
    }
    return section + 1;
}

// ==============

QVariant TResultProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return sourceModel()->headerData(section, orientation, role);
}
