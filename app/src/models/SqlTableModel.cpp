#include "SqlTableModel.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDebug>

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
    qDebug() << "SqlTableModel::select() для таблицы:" << tableName();
    //bool result = QSqlTableModel::select();
    bool result = QSqlRelationalTableModel::select();

    if (result) {
        qDebug() << "  Успех! Строк:" << rowCount();
        emit dataLoaded(); // Сигнализируем о загрузке данных
    }

    return result;
}

// =============== FilterProxyModel ===============

FilterProxyModel::FilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    // Обновляемся при изменении исходной модели
    connect(this, &QSortFilterProxyModel::sourceModelChanged,
            this, &FilterProxyModel::onSourceModelChanged);
}

void FilterProxyModel::onSourceModelChanged()
{
    if (sourceModel()) {
        // Принудительное обновление при смене модели
        invalidate();
        sort(0); // Сортировка по первому столбцу
    }
}

// Фильтрация
void FilterProxyModel::setCompetitionFilter(qint64 competitionId)
{
    m_competitionId_prx = competitionId;
    invalidateFilter();
}

void FilterProxyModel::setTextFilter(const QString& text)
{
    if (m_filterText != text) {
        m_filterText = text;
        invalidateFilter();
    }
}

void FilterProxyModel::setFilterColumns(const QList<int>& columns)
{
    m_filterColumns = columns;
    invalidateFilter();
}

// Управление видимостью столбцов
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
        return true; // Если не задано, показываем все
    }
    return m_visibleColumns.contains(column);
}

void FilterProxyModel::setVisibleColumns(const QList<int>& columns)
{
    m_visibleColumns = QSet<int>(columns.begin(), columns.end());
    invalidateFilter();
}

// Доступ к данным
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
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) {
        return -1;
    }

    SqlTableModel* sourceModel = qobject_cast<SqlTableModel*>(this->sourceModel());
    if (!sourceModel) {
        return -1;
    }

    return sourceModel->recordId(sourceIndex);
}

// Фильтрация строк
bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (!sourceModel()) {
        return false;
    }

    // 1. Фильтр по competition_id (если установлен)
    if (m_competitionId_prx > 0) {
        // Ищем столбец с competition_id в исходной модели
        int competitionColumn = -1;
        for (int col = 0; col < sourceModel()->columnCount(); ++col) {
            QString fieldName = sourceModel()->headerData(col, Qt::Horizontal).toString();
            if (fieldName.compare("competition_id", Qt::CaseInsensitive) == 0) {
                competitionColumn = col;
                break;
            }
        }

        // Если нашли столбец competition_id, проверяем значение
        if (competitionColumn >= 0) {
            QModelIndex competitionIndex = sourceModel()->index(sourceRow, competitionColumn, sourceParent);
            QVariant competitionValue = sourceModel()->data(competitionIndex, Qt::DisplayRole);
            bool ok = false;
            qint64 rowCompetitionId = competitionValue.toLongLong(&ok);

            if (!ok || rowCompetitionId != m_competitionId_prx) {
                return false; // Пропускаем строку, если competition_id не совпадает
            }
        }
        // Если столбца competition_id нет, пропускаем фильтрацию
    }

    // 2. Текстовый фильтр (если установлен)
    if (!m_filterText.isEmpty()) {
        bool textFound = false;

        // Если указаны конкретные столбцы для фильтрации
        if (!m_filterColumns.isEmpty()) {
            for (int col : m_filterColumns) {
                if (col >= 0 && col < sourceModel()->columnCount()) {
                    QModelIndex index = sourceModel()->index(sourceRow, col, sourceParent);
                    QString data = sourceModel()->data(index, Qt::DisplayRole).toString();
                    if (data.contains(m_filterText, Qt::CaseInsensitive)) {
                        textFound = true;
                        break;
                    }
                }
            }
        } else {
            // Ищем во всех столбцах
            for (int col = 0; col < sourceModel()->columnCount(); ++col) {
                // Если заданы видимые столбцы, ищем только в них
                if (!m_visibleColumns.isEmpty() && !m_visibleColumns.contains(col)) {
                    continue;
                }

                QModelIndex index = sourceModel()->index(sourceRow, col, sourceParent);
                QString data = sourceModel()->data(index, Qt::DisplayRole).toString();
                if (data.contains(m_filterText, Qt::CaseInsensitive)) {
                    textFound = true;
                    break;
                }
            }
        }

        if (!textFound) {
            return false;
        }
    }

    return true;
}

// Фильтрация столбцов
bool FilterProxyModel::filterAcceptsColumn(int sourceColumn, const QModelIndex& sourceParent) const
{
    Q_UNUSED(sourceParent);

    // Если список видимых столбцов пуст - показываем все столбцы
    if (m_visibleColumns.isEmpty()) {
        return true;
    }

    // Иначе проверяем, видим ли этот столбец
    return m_visibleColumns.contains(sourceColumn);
}
