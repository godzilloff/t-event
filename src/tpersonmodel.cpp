#include "tpersonmodel.h"

#include <QTime>

TpersonModel::TpersonModel(QObject *parent)
    : QAbstractTableModel{parent}
{}

TpersonModel::~TpersonModel()
{}

int TpersonModel::reInit(std::shared_ptr<QSportEvent> &event)
{
    event_ptr = event;
    persons_ = event->getRaces().at(0)->getPtrPersons();
    return 0;
}

int TpersonModel::rowCount(const QModelIndex &parent) const
{
    return persons_->count();
}

int TpersonModel::columnCount(const QModelIndex &parent) const
{
    return 10;
}

QVariant TpersonModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole){
        QString res ="";
        if (persons_->count()>index.row()){
            switch (index.column()) {
            case 0: res = persons_->at(index.row())->getSurname(); break;
            case 1: res = persons_->at(index.row())->getName(); break;
            case 2: res = persons_->at(index.row())->getQual(); break;
            case 3: res = event_ptr->getNameGroup(persons_->at(index.row())->getGroupId()); break;
            case 4: res = event_ptr->getNameOrganization(persons_->at(index.row())->getOrganizationId()); break;
            case 5: res = QString::number(persons_->at(index.row())->getYear()); break;
            case 6: res = QString::number(persons_->at(index.row())->getBib()); break;
            case 7: res = QString::number(persons_->at(index.row())->getCardNumber()); break;
            case 8: res = QTime::fromMSecsSinceStartOfDay(persons_->at(index.row())->getStartTime()).toString("hh:mm:ss"); break;
            //case 9:
            case 10: res = persons_->at(index.row())->getComment(); break;
            default:
                break;
            }
        }
        return res;
    }
    return QVariant();
}

QVariant TpersonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:return QVariant(QString::fromUtf8("Фамилия"));
        case 1:return QVariant(QString::fromUtf8("Имя"));
        case 2:return QVariant(QString::fromUtf8("Квал."));
        case 3:return QVariant(QString::fromUtf8("Группа"));
        case 4:return QVariant(QString::fromUtf8("Команда"));
        case 5:return QVariant(QString::fromUtf8("ГР"));
        case 6:return QVariant(QString::fromUtf8("Номер"));
        case 7:return QVariant(QString::fromUtf8("№ чипа"));
        case 8:return QVariant(QString::fromUtf8("Старт"));
        case 9:return QVariant(QString::fromUtf8("Финиш"));
        case 10:return QVariant(QString::fromUtf8("Комментарий"));
        default: return QVariant();
        }
    }
    return section + 1;
}
