// AbstractProxyModel.h

#ifndef ABSTRACTPROXYMODEL_H
#define ABSTRACTPROXYMODEL_H

#include <QSortFilterProxyModel>

class AbstractProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit AbstractProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}
    virtual ~AbstractProxyModel() = default;

    // Виртуальные методы для общего интерфейса
    virtual void setTextFilter(const QString& text) { Q_UNUSED(text); }
    virtual void setVisibleColumns(const QList<int>& columns) {
        Q_UNUSED(columns);
        qDebug() << "WARNING: setVisibleColumns not implemented in" << metaObject()->className();
    }

    virtual qint64 recordId(const QModelIndex& index) const {
        return index.data().toLongLong();
    }

    virtual void recalculateRanks() {}
    virtual void refreshData() {}

    virtual void setColumnVisible(int column, bool visible) {
        Q_UNUSED(column);
        Q_UNUSED(visible);
    }

    virtual bool isColumnVisible(int column) const {
        Q_UNUSED(column);
        return true;
    }

    virtual QList<int> visibleColumns() const { return QList<int>(); }
};

#endif // ABSTRACTPROXYMODEL_H
