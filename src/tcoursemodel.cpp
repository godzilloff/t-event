#include "tcoursemodel.h"

TcourseModel::TcourseModel(QObject *parent)
    : QAbstractTableModel{parent}
{}

TcourseModel::~TcourseModel()
{}

//TcourseModel::TcourseModel(QSportEvent &event)
int TcourseModel::reInit(QSportEvent &event)
{
    courses_ = event.getRaces().at(0)->getPtrCources();
    return 0;
}

int TcourseModel::rowCount(const QModelIndex &parent) const
{
    return courses_->count();
}

int TcourseModel::columnCount(const QModelIndex &parent) const
{
    return 5;
}

QVariant TcourseModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole){
        QString res ="";
        if (courses_->count()>index.row()){
            switch (index.column()) {
            case 0: res = courses_->at(index.row())->getName(); break;
            case 1: res = QString::number(courses_->at(index.row())->getLength()); break;
            case 2: res = QString::number(courses_->at(index.row())->getClimb()); break;
            case 3: res = QString::number(courses_->at(index.row())->getCntControll()); break;
            case 4: res = courses_->at(index.row())->getPathCC(); break;
            default: break;
            }
        }
        return res;
    }
    return QVariant();
}

QVariant TcourseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:return QVariant(QString::fromUtf8("Название"));
        case 1:return QVariant(QString::fromUtf8("Длина, м"));
        case 2:return QVariant(QString::fromUtf8("Набор, м"));
        case 3:return QVariant(QString::fromUtf8("КП"));
        case 4:return QVariant(QString::fromUtf8("Контрольные пункты"));
        default: return QVariant();
        }
    }
    return section + 1;
}
