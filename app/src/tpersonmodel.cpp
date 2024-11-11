#include "tpersonmodel.h"

#include <QTime>

TpersonModel::TpersonModel(QObject* parent) : QAbstractTableModel{parent} {}

TpersonModel::~TpersonModel() {}

int TpersonModel::reInit(std::shared_ptr<QSportEvent>& event) {
  event_ptr = event;
  persons_ = event->getRaces().at(0)->getPtrPersons();
  return 0;
}

int TpersonModel::rowCount(const QModelIndex& parent) const {
  return persons_->count();
}

int TpersonModel::columnCount(const QModelIndex& parent) const { return 12; }

QVariant TpersonModel::data(const QModelIndex& index, int role) const {
  if (role == Qt::DisplayRole) {
    QString res = "";
    if (persons_->count() > index.row()) {
      switch (index.column()) {
        case CSurname:
          res = persons_->at(index.row())->getSurname();
          break;
        case CName:
          res = persons_->at(index.row())->getName();
          break;
        case CQual:
          res = persons_->at(index.row())->getQual();
          break;
        case CGroup:
          res =
              event_ptr->getNameGroup(persons_->at(index.row())->getGroupId());
          break;
        case COrg:
          res = event_ptr->getNameOrganization(
              persons_->at(index.row())->getOrganizationId());
          break;
        case CYear:
          res = QString::number(persons_->at(index.row())->getYear());
          break;
        case CBib:
          res = QString::number(persons_->at(index.row())->getBib());
          break;
        case CCardNum:
          res = QString::number(persons_->at(index.row())->getCardNumber());
          break;
        case CStart:
          res = QTime::fromMSecsSinceStartOfDay(
                    persons_->at(index.row())->getStartTime())
                    .toString("hh:mm:ss");
          break;
        // case CFinish:
        case CComent:
          res = persons_->at(index.row())->getComment();
          break;
        case CID:
          res = persons_->at(index.row())->getId();
          break;
        default:
          break;
      }
    }
    return res;
  }
  return QVariant();
}

QVariant TpersonModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const {
  if (role != Qt::DisplayRole) return QVariant();
  if (orientation == Qt::Horizontal) {
    switch (section) {
      case CSurname:
        return QVariant(QString::fromUtf8("Фамилия"));
      case CName:
        return QVariant(QString::fromUtf8("Имя"));
      case CQual:
        return QVariant(QString::fromUtf8("Квал."));
      case CGroup:
        return QVariant(QString::fromUtf8("Группа"));
      case COrg:
        return QVariant(QString::fromUtf8("Команда"));
      case CYear:
        return QVariant(QString::fromUtf8("ГР"));
      case CBib:
        return QVariant(QString::fromUtf8("Номер"));
      case CCardNum:
        return QVariant(QString::fromUtf8("№ чипа"));
      case CStart:
        return QVariant(QString::fromUtf8("Старт"));
      case CFinish:
        return QVariant(QString::fromUtf8("Финиш"));
      case CComent:
        return QVariant(QString::fromUtf8("Комментарий"));
      case CID:
        return QVariant(QString::fromUtf8("ID"));
      default:
        return QVariant();
    }
  }
  return section + 1;
}

// ==============

QVariant TPersonProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return sourceModel()->headerData(section, orientation, role);
}

void TPersonProxyModel::setOrganization(const QString &orgName)
{
    organzationName = orgName;
}

bool TPersonProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex indOrg = sourceModel()->index(
        sourceRow, TpersonModel::ColNumTablePerson::COrg,sourceParent);
    if ((organzationName != "") ||
        (sourceModel()->data(indOrg).toString() == organzationName)) return false;
    else return true;
}
