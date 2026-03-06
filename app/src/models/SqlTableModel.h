#ifndef SQLTABLEMODEL_H
#define SQLTABLEMODEL_H

#include <QSqlRelationalTableModel>
#include <QSortFilterProxyModel>

#include "AbstractProxyModel.h"

class SqlTableModel : public QSqlRelationalTableModel
{
    Q_OBJECT

public:
    explicit SqlTableModel(QObject* parent = nullptr, QSqlDatabase db = QSqlDatabase());

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    void setTable(const QString& tableName) override;
    void setCompetitionFilter(qint64 competitionId);
    void setupRelationsAfterQuery();

    bool addRecord(const QHash<QString, QVariant>& data);
    bool updateRecord(qint64 id, const QHash<QString, QVariant>& data);
    bool deleteRecord(qint64 id);

    qint64 recordId(const QModelIndex& index) const;
    QHash<QString, QVariant> recordData(qint64 id) const;

    void refresh();
    // Переопределяем для установки заголовков
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Метод для установки пользовательских заголовков
    void setColumnHeader(int column, const QString& header);
    void setColumnHeaders(const QMap<int, QString>& headers);

    // Метод для скрытия столбцов
    void hideColumn(int column);
    void hideColumns(const QList<int>& columns);

public slots:
    bool select() override;

signals:
    void recordAboutToBeChanged(qint64 id, const QHash<QString, QVariant>& oldData);
    void recordChanged(qint64 id);
    void dataLoaded();

private:
    qint64 m_competitionId = -1;
    QString m_idFieldName = "id";

    QMap<int, QString> m_customHeaders;
public:
    QSet<int> m_hiddenColumns;
};

// ===============================

class FilterProxyModel : public AbstractProxyModel
{
    Q_OBJECT

public:
    explicit FilterProxyModel(QObject* parent = nullptr);

    void setCompetitionFilter(qint64 competitionId);
    void setTextFilter(const QString& text) override;
    void setFilterColumns(const QList<int>& columns);

    // Публичные методы для доступа
    QModelIndex mapToSourcePublic(const QModelIndex& proxyIndex) const;
    QModelIndex mapFromSourcePublic(const QModelIndex& sourceIndex) const;
    qint64 recordId(const QModelIndex& proxyIndex) const override;

    // Управление видимостью столбцов
    void setColumnVisible(int column, bool visible) override;
    bool isColumnVisible(int column) const override;
    void setVisibleColumns(const QList<int>& columns) override;

private slots:
    void onSourceModelChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex& sourceParent) const override;

private:
    qint64 m_competitionId_prx = -1;
    QString m_filterText;
    QList<int> m_filterColumns;
    QSet<int> m_visibleColumns;
};

#endif // SQLTABLEMODEL_H
