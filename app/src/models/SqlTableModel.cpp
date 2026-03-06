#include "SqlTableModel.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDebug>

#include <QSqlRelationalTableModel>
#include <QSortFilterProxyModel>

// =============== SqlTableModel ===============

SqlTableModel::SqlTableModel(QObject* parent, QSqlDatabase db)
    : QSqlRelationalTableModel(parent, db)
    , m_competitionId(-1)
    , m_idFieldName("id")
{
    setEditStrategy(QSqlTableModel::OnManualSubmit);
    qDebug() << "SqlTableModel создан, this:" << this;
}

QVariant SqlTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    // Проверяем скрытые столбцы - возвращаем пустые данные
    if (m_hiddenColumns.contains(index.column())) {
        if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::UserRole) {
            return QVariant();
        }
    }

    // Для всех остальных случаев используем стандартный механизм Qt
    return QSqlRelationalTableModel::data(index, role);
}

bool SqlTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role != Qt::EditRole || !index.isValid()) {
        return false;
    }

    // Получаем старые данные
    qint64 id = recordId(index);
    QHash<QString, QVariant> oldData = recordData(id);

    emit recordAboutToBeChanged(id, oldData);

    bool result = QSqlRelationalTableModel::setData(index, value, role);

    if (result) {
        // Получаем новые данные
        //QHash<QString, QVariant> newData = recordData(id);
        emit recordChanged(id);
    }

    return result;
}

void SqlTableModel::setTable(const QString& tableName)
{
    qDebug() << "SqlTableModel::setTable()";
    qDebug() << "  Таблица:" << tableName;
    qDebug() << "  this:" << this;
    qDebug() << "  database().isOpen():" << database().isOpen();

    QSqlRelationalTableModel::setTable(tableName);

    // Определяем поле ID
    QSqlRecord rec = record();
    for (int i = 0; i < rec.count(); ++i) {
        if (rec.fieldName(i).toLower() == "id" ||
            rec.fieldName(i).toLower().endsWith("_id")) {
            m_idFieldName = rec.fieldName(i);
            break;
        }
    }

    qDebug() << "  Поле ID:" << m_idFieldName;
}

void SqlTableModel::setCompetitionFilter(qint64 competitionId)
{
    qDebug() << "SqlTableModel::setCompetitionFilter() для таблицы" << tableName();

    m_competitionId = competitionId;

    // Проверяем, есть ли поле competition_id в текущей таблице/VIEW
    bool hasCompetitionId = false;
    QSqlRecord rec = record();
    for (int i = 0; i < rec.count(); ++i) {
        if (rec.fieldName(i) == "competition_id") {
            hasCompetitionId = true;
            break;
        }
    }

    if (!hasCompetitionId) {
        qDebug() << "Таблица/VIEW" << tableName() << "не имеет competition_id, снимаю фильтр";
        setFilter("");
        select();
        return;
    }

    if (m_competitionId > 0) {
        QString filter = QString("competition_id = %1").arg(m_competitionId);
        qDebug() << "Устанавливаю фильтр:" << filter;
        setFilter(filter);
    } else {
        qDebug() << "Снимаю фильтр";
        setFilter("");
    }

    // Выполняем select
    if (!select()) {
        qWarning() << "Ошибка select():" << lastError().text();
    } else {
        qDebug() << "select() выполнен, строк:" << rowCount() << ", колонок:" << columnCount();
    }
}

void SqlTableModel::setupRelationsAfterQuery()
{
    if (tableName() == "participants") {
        // Устанавливаем отношения, но не вызываем select()
        // т.к. данные уже загружены через setQuery
        setRelation(fieldIndex("delegation_id"),
                    QSqlRelation("delegations", "id", "name"));
        setRelation(fieldIndex("distance_id"),
                    QSqlRelation("distances", "id", "name"));
        setRelation(fieldIndex("age_group_id"),
                    QSqlRelation("age_groups", "id", "name"));
    }
}

bool SqlTableModel::addRecord(const QHash<QString, QVariant>& data)
{
    QSqlRecord rec = record();
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        rec.setValue(it.key(), it.value());
    }

    if (insertRecord(-1, rec)) {
        return submitAll();
    }

    return false;
}

bool SqlTableModel::updateRecord(qint64 id, const QHash<QString, QVariant>& data)
{
    for (int i = 0; i < rowCount(); ++i) {
        if (recordId(index(i, 0)) == id) {
            QSqlRecord rec = record(i);
            for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
                rec.setValue(it.key(), it.value());
            }
            setRecord(i, rec);
            return submitAll();
        }
    }

    return false;
}

bool SqlTableModel::deleteRecord(qint64 id)
{
    for (int i = 0; i < rowCount(); ++i) {
        if (recordId(index(i, 0)) == id) {
            if (removeRow(i)) {
                return submitAll();
            }
            break;
        }
    }

    return false;
}

qint64 SqlTableModel::recordId(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return -1;
    }

    return data(this->index(index.row(), fieldIndex(m_idFieldName)), Qt::DisplayRole).toLongLong();
}

QHash<QString, QVariant> SqlTableModel::recordData(qint64 id) const
{
    QHash<QString, QVariant> result;

    for (int i = 0; i < rowCount(); ++i) {
        if (recordId(index(i, 0)) == id) {
            QSqlRecord rec = record(i);
            for (int j = 0; j < rec.count(); ++j) {
                result[rec.fieldName(j)] = rec.value(j);
            }
            break;
        }
    }

    return result;
}

void SqlTableModel::refresh()
{
    // Сохраняем текущий фильтр
    QString currentFilter = filter();

    // Выполняем select
    select();

    // Восстанавливаем фильтр
    if (!currentFilter.isEmpty()) {
        setFilter(currentFilter);
        select();
    }
}

QVariant SqlTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        // Для скрытых столбцов возвращаем пустые заголовки
        if (m_hiddenColumns.contains(section)) {
            if (role == Qt::DisplayRole || role == Qt::EditRole) {
                return QVariant();
            }
        }

        if (role == Qt::DisplayRole) {
            // Возвращаем пользовательский заголовок если есть
            if (m_customHeaders.contains(section)) {
                return m_customHeaders[section];
            }

            // Или стандартный заголовок из БД
            return QSqlRelationalTableModel::headerData(section, orientation, role);
        }
    }

    return QSqlRelationalTableModel::headerData(section, orientation, role);
}

void SqlTableModel::setColumnHeader(int column, const QString& header)
{
    m_customHeaders[column] = header;
    emit headerDataChanged(Qt::Horizontal, column, column);
}

void SqlTableModel::setColumnHeaders(const QMap<int, QString>& headers)
{
    m_customHeaders = headers;
    if (!m_customHeaders.isEmpty()) {
        emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
    }
}

void SqlTableModel::hideColumn(int column)
{
    m_hiddenColumns.insert(column);
}

void SqlTableModel::hideColumns(const QList<int>& columns)
{
    for (int col : columns) {
        m_hiddenColumns.insert(col);
    }
}

bool SqlTableModel::select()
{
    int oldRowCount = rowCount(); // Проверяем, действительно ли изменились данные

    bool result = QSqlTableModel::select();

    if (result) {
        int newRowCount = rowCount();
        if (oldRowCount != newRowCount) {
            emit dataLoaded();
        } else {
            emit dataLoaded(); // или не испускать, если ничего не изменилось
        }
    }

    return result;
}

// =============== FilterProxyModel ===============

FilterProxyModel::FilterProxyModel(QObject* parent)
    : AbstractProxyModel(parent) // Изменено с QSortFilterProxyModel на AbstractProxyModel
{
    connect(this, &QAbstractItemModel::modelReset,
            this, &FilterProxyModel::onSourceModelChanged);
    connect(this, &QAbstractItemModel::rowsInserted,
            this, &FilterProxyModel::onSourceModelChanged);
}

void FilterProxyModel::setCompetitionFilter(qint64 competitionId)
{
    m_competitionId_prx = competitionId;
    invalidateFilter();
}

void FilterProxyModel::setTextFilter(const QString& text)
{
    m_filterText = text;
    invalidateFilter();
}

void FilterProxyModel::setFilterColumns(const QList<int>& columns)
{
    m_filterColumns = columns;
    invalidateFilter();
}

void FilterProxyModel::setColumnVisible(int column, bool visible)
{
    if (visible) {
        m_visibleColumns.insert(column);
    } else {
        m_visibleColumns.remove(column);
    }
    invalidateFilter();
}

bool FilterProxyModel::isColumnVisible(int column) const
{
    if (m_visibleColumns.isEmpty()) {
        return true;
    }
    return m_visibleColumns.contains(column);
}

void FilterProxyModel::setVisibleColumns(const QList<int>& columns)
{
    // qDebug() << "FilterProxyModel::setVisibleColumns для" << objectName()
    //          << "устанавливаю:" << columns;

    m_visibleColumns.clear();
    for (int col : columns) {
        m_visibleColumns.insert(col);
    }

    // qDebug() << "  m_visibleColumns теперь:" << m_visibleColumns;

    invalidateFilter();           // Инвалидируем фильтр
    invalidateColumnsFilter();    // Также инвалидируем фильтр колонок
}

QModelIndex FilterProxyModel::mapToSourcePublic(const QModelIndex& proxyIndex) const
{
    return mapToSource(proxyIndex);
}

QModelIndex FilterProxyModel::mapFromSourcePublic(const QModelIndex& sourceIndex) const
{
    return mapFromSource(sourceIndex);
}

qint64 FilterProxyModel::recordId(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid()) {
        return -1;
    }

    QModelIndex sourceIndex = mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) {
        return -1;
    }

    // Предполагаем, что ID находится в первой колонке (0)
    QModelIndex idIndex = sourceModel()->index(sourceIndex.row(), 0);
    return sourceModel()->data(idIndex).toLongLong();
}


bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    // Если нет фильтра, принимаем строку
    if (m_filterText.isEmpty() && m_competitionId_prx == -1) {
        return true;
    }

    QModelIndex index;

    // Проверяем по competition_id если нужно
    if (m_competitionId_prx != -1) {
        // Предполагаем, что competition_id находится в колонке 1
        index = sourceModel()->index(sourceRow, 1, sourceParent);
        if (index.data().toLongLong() != m_competitionId_prx) {
            return false;
        }
    }

    // Проверяем по текстовому фильтру
    if (!m_filterText.isEmpty()) {
        // Если указаны конкретные колонки для поиска
        if (!m_filterColumns.isEmpty()) {
            for (int column : m_filterColumns) {
                index = sourceModel()->index(sourceRow, column, sourceParent);
                if (index.data().toString().contains(m_filterText, Qt::CaseInsensitive)) {
                    return true;
                }
            }
            return false;
        } else {
            // Ищем по всем колонкам
            for (int i = 0; i < sourceModel()->columnCount(); ++i) {
                index = sourceModel()->index(sourceRow, i, sourceParent);
                if (index.data().toString().contains(m_filterText, Qt::CaseInsensitive)) {
                    return true;
                }
            }
            return false;
        }
    }

    return true;
}

bool FilterProxyModel::filterAcceptsColumn(int sourceColumn, const QModelIndex& sourceParent) const
{
    Q_UNUSED(sourceParent);

    // Важно: проверяем, для какой модели вызывается фильтр
    QString caller = objectName();

    // Для отладки покажем первые вызовы для каждой модели
    static QSet<QString> loggedModels;
    if (!loggedModels.contains(caller) && !caller.isEmpty()) {
        loggedModels.insert(caller);
        qDebug() << "FilterProxyModel::filterAcceptsColumn для" << caller
                 << "sourceColumn:" << sourceColumn
                 << "m_visibleColumns:" << m_visibleColumns;
    }

    // Если список видимых колонок пуст, показываем все
    if (m_visibleColumns.isEmpty()) {
        return true;
    }

    // Проверяем, есть ли колонка в списке видимых
    bool visible = m_visibleColumns.contains(sourceColumn);

    // Для отладки покажем только rejection для виртуальных колонок results
    if (!visible && sourceColumn >= 13 && caller.contains("Results")) {
        qDebug() << "  ВНИМАНИЕ: виртуальная колонка" << sourceColumn
                 << "скрыта для" << caller;
    }

    return visible;
}

void FilterProxyModel::onSourceModelChanged()
{
    // Сбрасываем видимые колонки при изменении модели
    m_visibleColumns.clear();
    invalidateFilter();
}
