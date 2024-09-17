#include "tgroupmodel.h"

TgroupModel::TgroupModel(QObject *parent)
    : QAbstractTableModel{parent}
{}

TgroupModel::~TgroupModel()
{}

int TgroupModel::reInit(QSportEvent &event)
{
    groups_ = event.getRaces().at(0)->getPtrGroups();
    thisRace = event.getRaces().at(0);
    return 0;
}

int TgroupModel::rowCount(const QModelIndex &parent) const
{
    return groups_->count();
}

int TgroupModel::columnCount(const QModelIndex &parent) const
{
    return 13;
}

QVariant TgroupModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole){
        QString res ="";
        if (groups_->count()>index.row()){
            switch (index.column()) {
            case 0: res = groups_->at(index.row())->getName() ; break;
            case 1: res = groups_->at(index.row())->getLongName(); break;
            case 2: res = thisRace->getNameCourse(groups_->at(index.row())->getCourseId()) ; break;
            case 3: res = QString::number(groups_->at(index.row())->getPprice()); break;
            case 4: res = QString::number(thisRace->getLengthCourse(groups_->at(index.row())->getCourseId())); break;
            case 5: res = QString::number(thisRace->getCntKPCourse(groups_->at(index.row())->getCourseId())); break;
            case 6: res = QString::number(thisRace->getClimb(groups_->at(index.row())->getCourseId())); break;
            case 7: res = QString::number(groups_->at(index.row())->getMin_year()); break;
            case 8: res = QString::number(groups_->at(index.row())->getMax_year()); break;
            case 9: res = QTime::fromMSecsSinceStartOfDay(groups_->at(index.row())->getStart_interval()).toString("hh:mm:ss"); break;
            case 10: res = QString::number(groups_->at(index.row())->getStart_corridor()); break;
            case 11: res = QString::number(groups_->at(index.row())->getOrder_in_corridor()); break;

            default: break;
            }
        }
        return res;
    }
    return QVariant();
}

QVariant TgroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:return QVariant(QString::fromUtf8("Название"));
        case 1:return QVariant(QString::fromUtf8("Полное наименование"));
        case 2:return QVariant(QString::fromUtf8("Дистанция"));
        case 3:return QVariant(QString::fromUtf8("Взнос"));
        case 4:return QVariant(QString::fromUtf8("Длина, м"));
        case 5:return QVariant(QString::fromUtf8("КП"));
        case 6:return QVariant(QString::fromUtf8("Набор, м"));
        case 7:return QVariant(QString::fromUtf8("Мин.  ГР"));
        case 8:return QVariant(QString::fromUtf8("Макс. ГР"));
        case 9:return QVariant(QString::fromUtf8("Интервал"));
        case 10:return QVariant(QString::fromUtf8("Коридор"));
        case 11:return QVariant(QString::fromUtf8("Порядок в кор."));
        default: return QVariant();
        }
    }
    return section + 1;
}
