#include "torganizationmodel.h"

TorganizationModel::TorganizationModel(QObject *parent)
    : QAbstractTableModel{parent}
{}

TorganizationModel::~TorganizationModel()
{}

int TorganizationModel::reInit(std::shared_ptr<QSportEvent> &event)
{
    organizations_ = event->getRaces().at(0)->getPtrOrganizations();
    return 0;
}

int TorganizationModel::rowCount(const QModelIndex &parent) const
{
    return organizations_->count();
}

int TorganizationModel::columnCount(const QModelIndex &parent) const
{
    return 5;
}

QVariant TorganizationModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole){
        QString res ="";
        if (organizations_->count()>index.row()){
            switch (index.column()) {
            case 0: res = organizations_->at(index.row())->getName() ; break;
            case 1: res = organizations_->at(index.row())->getContact(); break;
            case 2: res = organizations_->at(index.row())->getCode() ; break;
            case 3: res = organizations_->at(index.row())->getCountry(); break;
            case 4: res = organizations_->at(index.row())->getRegion(); break;
            default: break;
            }
        }
        return res;
    }
    return QVariant();
}

QVariant TorganizationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:return QVariant(QString::fromUtf8("Название"));
        case 1:return QVariant(QString::fromUtf8("Контактные данные"));
        case 2:return QVariant(QString::fromUtf8("Код"));
        case 3:return QVariant(QString::fromUtf8("Страна"));
        case 4:return QVariant(QString::fromUtf8("Субъект РФ"));
        default: return QVariant();
        }
    }
    return section + 1;
}
