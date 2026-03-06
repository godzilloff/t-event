// ResultsProxyModel.cpp - исправленная версия

#include "ResultsProxyModel.h"
#include "ResultColumns.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <algorithm>
#include <QTime>
#include <QDateTime>
#include <QColor>

ResultsProxyModel::ResultsProxyModel(QObject* parent)
    : AbstractProxyModel(parent)
    , m_cacheValid(false)
    , m_sourceColumnCount(0)
    , m_currentSortColumn(-1)
    , m_currentSortOrder(Qt::AscendingOrder)
{
    setDynamicSortFilter(true);
    setSortRole(Qt::UserRole);

    qDebug() << "ResultsProxyModel created";
}

ResultsProxyModel::~ResultsProxyModel()
{
    qDebug() << "ResultsProxyModel destroyed";
}

void ResultsProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    qDebug() << "ResultsProxyModel::setSourceModel";

    if (this->sourceModel()) {
        disconnect(this->sourceModel(), nullptr, this, nullptr);
    }

    QSortFilterProxyModel::setSourceModel(sourceModel);

    if (sourceModel) {
        connect(sourceModel, &QAbstractItemModel::modelReset,
                this, &ResultsProxyModel::updateSourceColumnCount);
        connect(sourceModel, &QAbstractItemModel::columnsInserted,
                this, &ResultsProxyModel::updateSourceColumnCount);
        connect(sourceModel, &QAbstractItemModel::columnsRemoved,
                this, &ResultsProxyModel::updateSourceColumnCount);
        connect(sourceModel, &QAbstractItemModel::dataChanged,
                this, &ResultsProxyModel::onSourceDataChanged);
        connect(sourceModel, &QAbstractItemModel::rowsInserted,
                this, [this]() { m_cacheValid = false; rebuildCache(); });
        connect(sourceModel, &QAbstractItemModel::rowsRemoved,
                this, [this]() { m_cacheValid = false; rebuildCache(); });

        updateSourceColumnCount();
    }
}

void ResultsProxyModel::updateSourceColumnCount()
{
    if (!sourceModel()) return;

    int newCount = sourceModel()->columnCount();
    if (newCount != m_sourceColumnCount && newCount > 0) {
        qDebug() << "ResultsProxyModel::updateSourceColumnCount - source columns:"
                 << newCount << "(was:" << m_sourceColumnCount << ")";

        beginResetModel();
        m_sourceColumnCount = newCount;
        m_cacheValid = false;
        endResetModel();

        // Перестраиваем кеш после изменения структуры
        rebuildCache();
    }
}

void ResultsProxyModel::onSourceDataChanged(const QModelIndex& topLeft,
                                            const QModelIndex& bottomRight,
                                            const QVector<int>& roles)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);
    m_cacheValid = false;

    // Если данные изменились, пересчитываем кеш
    rebuildCache();
}

int ResultsProxyModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (!sourceModel()) return 0;

    return m_sourceColumnCount + ResultColumn::VIRTUAL_COLUMN_COUNT;
}

int ResultsProxyModel::rowCount(const QModelIndex& parent) const
{
    if (!sourceModel()) return 0;
    return QSortFilterProxyModel::rowCount(parent);
}

QModelIndex ResultsProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!sourceModel() || row < 0 || row >= rowCount(parent) ||
        column < 0 || column >= columnCount(parent)) {
        return QModelIndex();
    }

    // Для обычных колонок используем базовую реализацию
    if (column < m_sourceColumnCount) {
        return QSortFilterProxyModel::index(row, column, parent);
    }

    // Для виртуальных колонок нам нужно получить ID результата
    // Сначала получаем прокси-индекс для первой колонки
    QModelIndex firstColIndex = QSortFilterProxyModel::index(row, 0, parent);
    if (!firstColIndex.isValid()) {
        return QModelIndex();
    }

    // Получаем исходный индекс для первой колонки
    QModelIndex sourceIndex = mapToSource(firstColIndex);
    if (!sourceIndex.isValid()) {
        return QModelIndex();
    }

    // Получаем ID из исходной модели
    qint64 resultId = sourceModel()->data(sourceIndex).toLongLong();

    // Создаем индекс с internalId = resultId
    return createIndex(row, column, quintptr(resultId));
}

QModelIndex ResultsProxyModel::parent(const QModelIndex& child) const
{
    Q_UNUSED(child);
    return QModelIndex(); // плоская модель
}

QVariant ResultsProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section >= m_sourceColumnCount) {
            int virtualIndex = section - m_sourceColumnCount;
            switch (virtualIndex) {
            case 0: return tr("Место");
            case 1: return tr("Отставание");
            case 2: return tr("% от лидера");
            default: return QVariant();
            }
        }
    }

    return QSortFilterProxyModel::headerData(section, orientation, role);
}

QVariant ResultsProxyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !sourceModel()) {
        return QVariant();
    }

    // Для обычных колонок используем базовую реализацию
    if (index.column() < m_sourceColumnCount) {
        return QSortFilterProxyModel::data(index, role);
    }

    // Для виртуальных колонок
    if (!m_cacheValid) {
        const_cast<ResultsProxyModel*>(this)->rebuildCache();
    }

    // Получаем ID результата из internalId
    quintptr internalId = index.internalId();
    if (internalId == 0) {
        return QVariant();
    }

    qint64 resultId = static_cast<qint64>(internalId);

    if (!m_resultsCache.contains(resultId)) {
        return QVariant();
    }

    const ResultData& data = m_resultsCache[resultId];
    int virtualCol = index.column() - m_sourceColumnCount;

    if (role == Qt::UserRole) {
        switch (virtualCol) {
        case 0: return data.rank;
        case 1: return data.timeLoss;
        case 2:
            if (data.resultSeconds > 0) {
                int leaderTime = data.resultSeconds - data.timeLoss;
                if (leaderTime > 0) {
                    return (double)data.resultSeconds / leaderTime;
                }
            }
            return 1.0;
        }
    }
    else if (role == Qt::DisplayRole) {
        switch (virtualCol) {
        case 0:
            return data.rank > 0 ? QString::number(data.rank) : "—";
        case 1:
            if (data.timeLoss == 0) return "Лидер";
            if (data.timeLoss > 0) return formatTimeLoss(data.timeLoss);
            return "—";
        case 2:
            if (data.timeLoss == 0) return "100%";
            if (data.resultSeconds > 0) {
                int leaderTime = data.resultSeconds - data.timeLoss;
                if (leaderTime > 0) {
                    double percent = (double)data.resultSeconds / leaderTime * 100.0;
                    return QString("%1%").arg(percent, 0, 'f', 1);
                }
            }
            return "—";
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        switch (virtualCol) {
        case 0: return Qt::AlignCenter;
        case 1:
        case 2: return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
    }

    return QVariant();
}

qint64 ResultsProxyModel::recordId(const QModelIndex& index) const
{
    if (!index.isValid()) return -1;

    if (index.column() < m_sourceColumnCount) {
        QModelIndex sourceIndex = mapToSource(index);
        if (!sourceIndex.isValid()) return -1;

        QModelIndex idIndex = sourceModel()->index(sourceIndex.row(), 0);
        return sourceModel()->data(idIndex).toLongLong();
    }

    // Для виртуальных колонок используем internalId
    quintptr internalId = index.internalId();
    return static_cast<qint64>(internalId);
}

QModelIndex ResultsProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid() || !sourceModel()) {
        return QModelIndex();
    }

    // Для виртуальных колонок
    if (proxyIndex.column() >= m_sourceColumnCount) {
        quintptr internalId = proxyIndex.internalId();
        if (internalId == 0) return QModelIndex();

        // Ищем строку с таким ID в исходной модели
        for (int row = 0; row < sourceModel()->rowCount(); ++row) {
            QModelIndex idx = sourceModel()->index(row, 0);
            if (sourceModel()->data(idx).toLongLong() == static_cast<qint64>(internalId)) {
                return idx;
            }
        }
        return QModelIndex();
    }

    // Для обычных колонок используем базовую реализацию
    return QSortFilterProxyModel::mapToSource(proxyIndex);
}

QModelIndex ResultsProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid() || !sourceModel()) {
        return QModelIndex();
    }

    // Получаем прокси-индекс через базовый класс
    QModelIndex proxyIndex = QSortFilterProxyModel::mapFromSource(sourceIndex);
    if (!proxyIndex.isValid()) {
        return QModelIndex();
    }

    return proxyIndex;
}

// void ResultsProxyModel::sort(int column, Qt::SortOrder order)
// {
//     qDebug() << "ResultsProxyModel::sort() - column:" << column << "order:" << order;

//     m_currentSortColumn = column;
//     m_currentSortOrder = order;

//     // Для сортировки по виртуальным колонкам мы должны переопределить логику
//     // и используем первую колонку как базовую
//     QSortFilterProxyModel::sort(0, order);
// }

// void ResultsProxyModel::sort(int column, Qt::SortOrder order)
// {
//     qDebug() << "ResultsProxyModel::sort() - column:" << column << "order:" << order;

//     m_currentSortColumn = column;
//     m_currentSortOrder = order;

//     // Для виртуальных колонок мы должны использовать первую колонку как базовую
//     // но наша переопределенная lessThan будет использовать правильные данные из кеша
//     if (column >= m_sourceColumnCount) {
//         // Сортируем по первой колонке, но lessThan будет использовать виртуальные данные
//         QSortFilterProxyModel::sort(0, order);
//     } else {
//         // Для обычных колонок сортируем напрямую по выбранной колонке
//         QSortFilterProxyModel::sort(column, order);
//     }
// }

// void ResultsProxyModel::sort(int column, Qt::SortOrder order)
// {
//     qDebug() << "ResultsProxyModel::sort() - column:" << column << "order:" << order;

//     m_currentSortColumn = column;
//     m_currentSortOrder = order;

//     if (column >= m_sourceColumnCount) {
//         // Для виртуальных колонок сортируем по первой колонке,
//         // но lessThan будет использовать правильные данные из кеша
//         QSortFilterProxyModel::sort(0, order);
//     } else {
//         // Для обычных колонок сортируем напрямую
//         QSortFilterProxyModel::sort(column, order);
//     }
// }

void ResultsProxyModel::sort(int column, Qt::SortOrder order)
{
    qDebug() << "\n=== SORT CALLED ===";
    qDebug() << "  column:" << column;
    qDebug() << "  order:" << (order == Qt::AscendingOrder ? "ASC" : "DESC");
    qDebug() << "  sourceColumnCount:" << m_sourceColumnCount;
    qDebug() << "  isVirtual:" << (column >= m_sourceColumnCount);

    m_currentSortColumn = column;
    m_currentSortOrder = order;

    if (column >= m_sourceColumnCount) {
        qDebug() << "  -> delegating to base sort(0) for virtual column";
        // QSortFilterProxyModel::sort(0, order);
        AbstractProxyModel::sort(0, order);
    } else {
        qDebug() << "  -> delegating to base sort(" << column << ") for regular column";
        // QSortFilterProxyModel::sort(column, order);
        AbstractProxyModel::sort(column, order);
    }
    qDebug() << "=== SORT FINISHED ===\n";
    //this->invalidate();

}

// bool ResultsProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
// {
//     if (!source_left.isValid() || !source_right.isValid()) {
//         return QSortFilterProxyModel::lessThan(source_left, source_right);
//     }

//     // Получаем ID из исходных индексов
//     qint64 leftId = sourceModel()->data(source_left).toLongLong();
//     qint64 rightId = sourceModel()->data(source_right).toLongLong();

//     // Если сортировка по обычной колонке (не виртуальной)
//     if (m_currentSortColumn < m_sourceColumnCount) {
//         // Получаем данные из исходной модели для колонки сортировки
//         QModelIndex leftIdx = source_left.sibling(source_left.row(), m_currentSortColumn);
//         QModelIndex rightIdx = source_right.sibling(source_right.row(), m_currentSortColumn);

//         if (!leftIdx.isValid() || !rightIdx.isValid()) {
//             return false;
//         }

//         QVariant leftData = sourceModel()->data(leftIdx, sortRole());
//         QVariant rightData = sourceModel()->data(rightIdx, sortRole());

//         return leftData.compare(leftData,rightData) < 0;
//     }

//     // Сортировка по виртуальной колонке
//     if (!m_cacheValid) {
//         const_cast<ResultsProxyModel*>(this)->rebuildCache();
//     }

//     if (!m_resultsCache.contains(leftId) || !m_resultsCache.contains(rightId)) {
//         return false;
//     }

//     const ResultData& leftData = m_resultsCache[leftId];
//     const ResultData& rightData = m_resultsCache[rightId];

//     int virtualCol = m_currentSortColumn - m_sourceColumnCount;

//     bool result = false;
//     switch (virtualCol) {
//     case 0: // Rank
//         result = leftData.rank < rightData.rank;
//         break;
//     case 1: // TimeLoss
//         result = leftData.timeLoss < rightData.timeLoss;
//         break;
//     case 2: // Percent
//         if (leftData.resultSeconds > 0 && rightData.resultSeconds > 0) {
//             int leftLeader = leftData.resultSeconds - leftData.timeLoss;
//             int rightLeader = rightData.resultSeconds - rightData.timeLoss;

//             double leftPct = leftLeader > 0 ? (double)leftData.resultSeconds / leftLeader : 1.0;
//             double rightPct = rightLeader > 0 ? (double)rightData.resultSeconds / rightLeader : 1.0;

//             result = leftPct < rightPct;
//         } else {
//             result = false;
//         }
//         break;
//     default:
//         result = false;
//     }

//     return m_currentSortOrder == Qt::AscendingOrder ? result : !result;
// }

// bool ResultsProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
// {
//     if (!source_left.isValid() || !source_right.isValid()) {
//         return QSortFilterProxyModel::lessThan(source_left, source_right);
//     }

//     // Получаем ID из исходных индексов (это ID из первой колонки)
//     qint64 leftId = sourceModel()->data(source_left).toLongLong();
//     qint64 rightId = sourceModel()->data(source_right).toLongLong();

//     // Если сортировка по обычной колонке
//     if (m_currentSortColumn < m_sourceColumnCount) {
//         // Получаем данные из исходной модели для колонки сортировки
//         QModelIndex leftIdx = source_left.sibling(source_left.row(), m_currentSortColumn);
//         QModelIndex rightIdx = source_right.sibling(source_right.row(), m_currentSortColumn);

//         if (!leftIdx.isValid() || !rightIdx.isValid()) {
//             return false;
//         }

//         QVariant leftData = sourceModel()->data(leftIdx, sortRole());
//         QVariant rightData = sourceModel()->data(rightIdx, sortRole());

//         //bool lessThanResult = leftData < rightData;
//         //return m_currentSortOrder == Qt::AscendingOrder ? lessThanResult : !lessThanResult;
//         QPartialOrdering comparison = QVariant::compare(leftData, rightData);

//         if (comparison == QPartialOrdering::Unordered) {
//             return false;
//         }

//         if (m_currentSortOrder == Qt::AscendingOrder) {
//             return comparison == QPartialOrdering::Less;
//         } else {
//             return comparison == QPartialOrdering::Greater;
//         }
//     }

//     // Сортировка по виртуальной колонке
//     if (!m_cacheValid) {
//         const_cast<ResultsProxyModel*>(this)->rebuildCache();
//     }

//     if (!m_resultsCache.contains(leftId) || !m_resultsCache.contains(rightId)) {
//         qWarning() << "ResultsProxyModel::lessThan - ID not found in cache:" << leftId << rightId;
//         return false;
//     }

//     const ResultData& leftData = m_resultsCache[leftId];
//     const ResultData& rightData = m_resultsCache[rightId];

//     int virtualCol = m_currentSortColumn - m_sourceColumnCount;

//     bool result = false;
//     switch (virtualCol) {
//     case 0: // Rank
//         result = leftData.rank < rightData.rank;
//         break;
//     case 1: // TimeLoss
//         result = leftData.timeLoss < rightData.timeLoss;
//         break;
//     case 2: // Percent
//         if (leftData.resultSeconds > 0 && rightData.resultSeconds > 0) {
//             int leftLeader = leftData.resultSeconds - leftData.timeLoss;
//             int rightLeader = rightData.resultSeconds - rightData.timeLoss;

//             double leftPct = leftLeader > 0 ? (double)leftData.resultSeconds / leftLeader : 1.0;
//             double rightPct = rightLeader > 0 ? (double)rightData.resultSeconds / rightLeader : 1.0;

//             result = leftPct < rightPct;
//         } else {
//             result = false;
//         }
//         break;
//     default:
//         result = false;
//     }

//     // ВАЖНО: для виртуальных колонок мы уже учли порядок сортировки в switch
//     // Поэтому просто возвращаем result
//     return result;
// }

// bool ResultsProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
// {
//     if (!source_left.isValid() || !source_right.isValid()) {
//         return QSortFilterProxyModel::lessThan(source_left, source_right);
//     }

//     // Получаем ID из исходных индексов (это ID из первой колонки)
//     qint64 leftId = sourceModel()->data(source_left).toLongLong();
//     qint64 rightId = sourceModel()->data(source_right).toLongLong();

//     // Если сортировка по обычной колонке
//     if (m_currentSortColumn < m_sourceColumnCount) {
//         // ВАЖНО: создаем индексы напрямую через sourceModel(), не через sibling()
//         QModelIndex leftIdx = sourceModel()->index(source_left.row(), m_currentSortColumn);
//         QModelIndex rightIdx = sourceModel()->index(source_right.row(), m_currentSortColumn);

//         if (!leftIdx.isValid() || !rightIdx.isValid()) {
//             qWarning() << "Invalid indices for comparison:" << leftIdx << rightIdx;
//             return false;
//         }

//         QVariant leftData = sourceModel()->data(leftIdx, sortRole());
//         QVariant rightData = sourceModel()->data(rightIdx, sortRole());

//         // Для отладки раскомментируйте при необходимости
//         qDebug() << "ResultsProxyModel::lessThan - обычная колонка" << m_currentSortColumn
//                  << "left row:" << source_left.row() << "data:" << leftData.toString()
//                  << "right row:" << source_right.row() << "data:" << rightData.toString();

//         // Используем QVariant::compare для Qt 6
//         auto comparison = QVariant::compare(leftData, rightData);

//         if (comparison == QPartialOrdering::Less) {
//             return m_currentSortOrder == Qt::AscendingOrder;
//         } else if (comparison == QPartialOrdering::Greater) {
//             return m_currentSortOrder == Qt::DescendingOrder;
//         }

//         return false; // равны или несравнимы
//     }

//     // Сортировка по виртуальной колонке
//     if (!m_cacheValid) {
//         const_cast<ResultsProxyModel*>(this)->rebuildCache();
//     }

//     if (!m_resultsCache.contains(leftId) || !m_resultsCache.contains(rightId)) {
//         qWarning() << "ResultsProxyModel::lessThan - ID not found in cache:" << leftId << rightId;
//         return false;
//     }

//     const ResultData& leftData = m_resultsCache[leftId];
//     const ResultData& rightData = m_resultsCache[rightId];

//     int virtualCol = m_currentSortColumn - m_sourceColumnCount;

//     bool result = false;
//     switch (virtualCol) {
//     case 0: // Rank
//         result = leftData.rank < rightData.rank;
//         break;
//     case 1: // TimeLoss
//         result = leftData.timeLoss < rightData.timeLoss;
//         break;
//     case 2: // Percent
//         if (leftData.resultSeconds > 0 && rightData.resultSeconds > 0) {
//             int leftLeader = leftData.resultSeconds - leftData.timeLoss;
//             int rightLeader = rightData.resultSeconds - rightData.timeLoss;

//             double leftPct = leftLeader > 0 ? (double)leftData.resultSeconds / leftLeader : 1.0;
//             double rightPct = rightLeader > 0 ? (double)rightData.resultSeconds / rightLeader : 1.0;

//             result = leftPct < rightPct;
//         } else {
//             result = false;
//         }
//         break;
//     default:
//         result = false;
//     }

//     // Для виртуальных колонок порядок уже учтен в switch
//     return result;
// }

// bool ResultsProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
// {
//     if (!source_left.isValid() || !source_right.isValid()) {
//         return QSortFilterProxyModel::lessThan(source_left, source_right);
//     }

//     // Получаем ID из исходных индексов (это ID из первой колонки)
//     qint64 leftId = sourceModel()->data(source_left).toLongLong();
//     qint64 rightId = sourceModel()->data(source_right).toLongLong();

//     // Если сортировка по обычной колонке
//     if (m_currentSortColumn < m_sourceColumnCount) {
//         // ВАЖНО: создаем индексы напрямую через sourceModel()
//         QModelIndex leftIdx = sourceModel()->index(source_left.row(), m_currentSortColumn);
//         QModelIndex rightIdx = sourceModel()->index(source_right.row(), m_currentSortColumn);

//         if (!leftIdx.isValid() || !rightIdx.isValid()) {
//             qWarning() << "Invalid indices for comparison:" << leftIdx << rightIdx;
//             return false;
//         }

//         // Для обычных колонок используем DisplayRole, так как UserRole пустой
//         QVariant leftData = sourceModel()->data(leftIdx, Qt::DisplayRole);
//         QVariant rightData = sourceModel()->data(rightIdx, Qt::DisplayRole);

//         // Расширенная отладка
//         QString leftHeader = sourceModel()->headerData(m_currentSortColumn, Qt::Horizontal).toString();
//         QString rightHeader = sourceModel()->headerData(m_currentSortColumn, Qt::Horizontal).toString();

//         qDebug() << "\n--- lessThan debug ---";
//         qDebug() << "  Сортировка по колонке:" << m_currentSortColumn << "(" << leftHeader << ")";
//         qDebug() << "  Левая строка:" << source_left.row()
//                  << "ID:" << leftId
//                  << "данные:" << leftData.toString()
//                  << "тип:" << leftData.typeName();
//         qDebug() << "  Правая строка:" << source_right.row()
//                  << "ID:" << rightId
//                  << "данные:" << rightData.toString()
//                  << "тип:" << rightData.typeName();
//         qDebug() << "  Порядок сортировки:" << (m_currentSortOrder == Qt::AscendingOrder ? "ASC" : "DESC");

//         // Используем QVariant::compare для Qt 6
//         auto comparison = QVariant::compare(leftData, rightData);

//         qDebug() << "  Результат сравнения:"
//                  << (comparison == QPartialOrdering::Less ? "Less" :
//                          (comparison == QPartialOrdering::Greater ? "Greater" :
//                               (comparison == QPartialOrdering::Equivalent ? "Equal" : "Unordered")));

//         // Для возрастающего порядка: возвращаем true если left < right
//         // Для убывающего порядка: возвращаем true если left > right
//         bool result;
//         if (m_currentSortOrder == Qt::AscendingOrder) {
//             result = (comparison == QPartialOrdering::Less);
//         } else {
//             result = (comparison == QPartialOrdering::Greater);
//         }

//         qDebug() << "  lessThan результат:" << result;
//         qDebug() << "----------------------\n";

//         return result;
//     }

//     // Сортировка по виртуальной колонке
//     if (!m_cacheValid) {
//         const_cast<ResultsProxyModel*>(this)->rebuildCache();
//     }

//     if (!m_resultsCache.contains(leftId) || !m_resultsCache.contains(rightId)) {
//         qWarning() << "ResultsProxyModel::lessThan - ID not found in cache:" << leftId << rightId;
//         return false;
//     }

//     const ResultData& leftData = m_resultsCache[leftId];
//     const ResultData& rightData = m_resultsCache[rightId];

//     int virtualCol = m_currentSortColumn - m_sourceColumnCount;

//     bool result = false;
//     switch (virtualCol) {
//     case 0: // Rank
//         result = leftData.rank < rightData.rank;
//         break;
//     case 1: // TimeLoss
//         result = leftData.timeLoss < rightData.timeLoss;
//         break;
//     case 2: // Percent
//         if (leftData.resultSeconds > 0 && rightData.resultSeconds > 0) {
//             int leftLeader = leftData.resultSeconds - leftData.timeLoss;
//             int rightLeader = rightData.resultSeconds - rightData.timeLoss;

//             double leftPct = leftLeader > 0 ? (double)leftData.resultSeconds / leftLeader : 1.0;
//             double rightPct = rightLeader > 0 ? (double)rightData.resultSeconds / rightLeader : 1.0;

//             result = leftPct < rightPct;
//         } else {
//             result = false;
//         }
//         break;
//     default:
//         result = false;
//     }

//     return result;
// }

// bool ResultsProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
// {
//     if (!source_left.isValid() || !source_right.isValid()) {
//         return QSortFilterProxyModel::lessThan(source_left, source_right);
//     }

//     // Если сортировка по обычной колонке
//     if (m_currentSortColumn < m_sourceColumnCount) {
//         // ВАЖНО: создаем индексы напрямую через sourceModel()
//         QModelIndex leftIdx = sourceModel()->index(source_left.row(), m_currentSortColumn);
//         QModelIndex rightIdx = sourceModel()->index(source_right.row(), m_currentSortColumn);

//         if (!leftIdx.isValid() || !rightIdx.isValid()) {
//             qWarning() << "Invalid indices for comparison:" << leftIdx << rightIdx;
//             return false;
//         }

//         // Для обычных колонок используем DisplayRole
//         QVariant leftData = sourceModel()->data(leftIdx, Qt::DisplayRole);
//         QVariant rightData = sourceModel()->data(rightIdx, Qt::DisplayRole);

//         // Расширенная отладка
//         QString leftHeader = sourceModel()->headerData(m_currentSortColumn, Qt::Horizontal).toString();

//         qDebug() << "\n--- lessThan debug ---";
//         qDebug() << "  Сортировка по колонке:" << m_currentSortColumn << "(" << leftHeader << ")";
//         qDebug() << "  Левая строка:" << source_left.row()
//                  << "данные:" << leftData.toString()
//                  << "тип:" << leftData.typeName();
//         qDebug() << "  Правая строка:" << source_right.row()
//                  << "данные:" << rightData.toString()
//                  << "тип:" << rightData.typeName();
//         qDebug() << "  Порядок сортировки:" << (m_currentSortOrder == Qt::AscendingOrder ? "ASC" : "DESC");

//         // Специальная обработка для числовых данных в строковом формате
//         bool leftOk, rightOk;

//         // Пробуем преобразовать в числа, если это возможно
//         if (m_currentSortColumn == 0 || m_currentSortColumn == 6 || m_currentSortColumn == 11) {
//             // Колонки с числовыми ID
//             qlonglong leftNum = leftData.toLongLong(&leftOk);
//             qlonglong rightNum = rightData.toLongLong(&rightOk);

//             if (leftOk && rightOk) {
//                 qDebug() << "  Числовое сравнение:" << leftNum << "vs" << rightNum;
//                 bool result = m_currentSortOrder == Qt::AscendingOrder ?
//                                   (leftNum < rightNum) : (leftNum > rightNum);
//                 qDebug() << "  lessThan результат:" << result;
//                 qDebug() << "----------------------\n";
//                 return result;
//             }
//         }
//         else if (m_currentSortColumn == 4) {
//             // Колонка с временем результата
//             int leftSec = convertTimeToSeconds(leftData.toString());
//             int rightSec = convertTimeToSeconds(rightData.toString());

//             qDebug() << "  Временное сравнение:" << leftSec << "vs" << rightSec;
//             bool result = m_currentSortOrder == Qt::AscendingOrder ?
//                               (leftSec < rightSec) : (leftSec > rightSec);
//             qDebug() << "  lessThan результат:" << result;
//             qDebug() << "----------------------\n";
//             return result;
//         }
//         else if (m_currentSortColumn == 5) {
//             // Колонка с временем результата
//             int leftSec = convertTimeToSeconds(leftData.toString());
//             int rightSec = convertTimeToSeconds(rightData.toString());

//             qDebug() << "  Временное сравнение:" << leftSec << "vs" << rightSec;
//             bool result = m_currentSortOrder == Qt::AscendingOrder ?
//                               (leftSec < rightSec) : (leftSec > rightSec);
//             qDebug() << "  lessThan результат:" << result;
//             qDebug() << "----------------------\n";
//             return result;
//         }

//         // Для строк используем обычное строковое сравнение с учетом локали
//         QString leftStr = leftData.toString();
//         QString rightStr = rightData.toString();

//         // Используем QString::compare для более предсказуемого результата
//         int stringCompare = QString::compare(leftStr, rightStr, Qt::CaseInsensitive);

//         bool result;
//         if (m_currentSortOrder == Qt::AscendingOrder) {
//             result = (stringCompare < 0);
//         } else {
//             result = (stringCompare > 0);
//         }

//         qDebug() << "  Строковое сравнение:" << stringCompare
//                  << "(" << (stringCompare < 0 ? "меньше" : (stringCompare > 0 ? "больше" : "равно") ) << ")";
//         qDebug() << "  lessThan результат:" << result;
//         qDebug() << "----------------------\n";

//         return result;
//     }

//     // Сортировка по виртуальной колонке
//     if (!m_cacheValid) {
//         const_cast<ResultsProxyModel*>(this)->rebuildCache();
//     }

//     // Получаем ID из исходных индексов для виртуальных колонок
//     qint64 leftId = sourceModel()->data(source_left).toLongLong();
//     qint64 rightId = sourceModel()->data(source_right).toLongLong();

//     if (!m_resultsCache.contains(leftId) || !m_resultsCache.contains(rightId)) {
//         qWarning() << "ResultsProxyModel::lessThan - ID not found in cache:" << leftId << rightId;
//         return false;
//     }

//     const ResultData& leftData = m_resultsCache[leftId];
//     const ResultData& rightData = m_resultsCache[rightId];

//     int virtualCol = m_currentSortColumn - m_sourceColumnCount;

//     bool result = false;
//     switch (virtualCol) {
//     case 0: // Rank
//         result = leftData.rank < rightData.rank;
//         break;
//     case 1: // TimeLoss
//         result = leftData.timeLoss < rightData.timeLoss;
//         break;
//     case 2: // Percent
//         if (leftData.resultSeconds > 0 && rightData.resultSeconds > 0) {
//             int leftLeader = leftData.resultSeconds - leftData.timeLoss;
//             int rightLeader = rightData.resultSeconds - rightData.timeLoss;

//             double leftPct = leftLeader > 0 ? (double)leftData.resultSeconds / leftLeader : 1.0;
//             double rightPct = rightLeader > 0 ? (double)rightData.resultSeconds / rightLeader : 1.0;

//             result = leftPct < rightPct;
//         } else {
//             result = false;
//         }
//         break;
//     default:
//         result = false;
//     }

//     return result;
// }

// bool ResultsProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
// {
//     if (!source_left.isValid() || !source_right.isValid()) {
//         return QSortFilterProxyModel::lessThan(source_left, source_right);
//     }

//     // Если сортировка по обычной колонке
//     if (m_currentSortColumn < m_sourceColumnCount) {
//         QModelIndex leftIdx = sourceModel()->index(source_left.row(), m_currentSortColumn);
//         QModelIndex rightIdx = sourceModel()->index(source_right.row(), m_currentSortColumn);

//         if (!leftIdx.isValid() || !rightIdx.isValid()) {
//             qWarning() << "Invalid indices for comparison:" << leftIdx << rightIdx;
//             return false;
//         }

//         QVariant leftData = sourceModel()->data(leftIdx, Qt::DisplayRole);
//         QVariant rightData = sourceModel()->data(rightIdx, Qt::DisplayRole);

//         QString leftHeader = sourceModel()->headerData(m_currentSortColumn, Qt::Horizontal).toString();

//         qDebug() << "\n--- lessThan debug ---";
//         qDebug() << "  Сортировка по колонке:" << m_currentSortColumn << "(" << leftHeader << ")";
//         qDebug() << "  Левая строка:" << source_left.row()
//                  << "данные:" << leftData.toString()
//                  << "тип:" << leftData.typeName();
//         qDebug() << "  Правая строка:" << source_right.row()
//                  << "данные:" << rightData.toString()
//                  << "тип:" << rightData.typeName();
//         qDebug() << "  Порядок сортировки:" << (m_currentSortOrder == Qt::AscendingOrder ? "ASC" : "DESC");

//         // Сначала получаем "естественный" порядок (как если бы сортировали по возрастанию)
//         bool naturalOrder = false;

//         // Специальная обработка для разных типов колонок
//         if (m_currentSortColumn == 0 || m_currentSortColumn == 6 || m_currentSortColumn == 11) {
//             // Числовые колонки
//             qlonglong leftNum = leftData.toLongLong();
//             qlonglong rightNum = rightData.toLongLong();
//             naturalOrder = (leftNum < rightNum);
//             qDebug() << "  Числовое сравнение:" << leftNum << "<" << rightNum << "=" << naturalOrder;
//         }
//         else if (m_currentSortColumn == 4) {
//             // Колонка с временем результата
//             int leftSec = convertTimeToSeconds(leftData.toString());
//             int rightSec = convertTimeToSeconds(rightData.toString());
//             naturalOrder = (leftSec < rightSec);
//             qDebug() << "  Временное сравнение:" << leftSec << "<" << rightSec << "=" << naturalOrder;
//         }
//         else {
//             // Строковые колонки - используем нормализованное сравнение
//             QString leftStr = leftData.toString().toLower().trimmed();
//             QString rightStr = rightData.toString().toLower().trimmed();
//             naturalOrder = (leftStr < rightStr);
//             qDebug() << "  Строковое сравнение:" << leftStr << "<" << rightStr << "=" << naturalOrder;
//         }

//         // Применяем порядок сортировки
//         bool result;
//         if (m_currentSortOrder == Qt::AscendingOrder) {
//             result = naturalOrder;
//         } else {
//             // Для убывающего порядка инвертируем естественный порядок
//             result = !naturalOrder;
//         }

//         qDebug() << "  lessThan результат (с учетом порядка):" << result;
//         qDebug() << "----------------------\n";

//         return result;
//     }

//     // Сортировка по виртуальной колонке
//     if (!m_cacheValid) {
//         const_cast<ResultsProxyModel*>(this)->rebuildCache();
//     }

//     qint64 leftId = sourceModel()->data(source_left).toLongLong();
//     qint64 rightId = sourceModel()->data(source_right).toLongLong();

//     if (!m_resultsCache.contains(leftId) || !m_resultsCache.contains(rightId)) {
//         qWarning() << "ResultsProxyModel::lessThan - ID not found in cache:" << leftId << rightId;
//         return false;
//     }

//     const ResultData& leftData = m_resultsCache[leftId];
//     const ResultData& rightData = m_resultsCache[rightId];

//     int virtualCol = m_currentSortColumn - m_sourceColumnCount;

//     bool naturalOrder = false;
//     switch (virtualCol) {
//     case 0: // Rank
//         naturalOrder = (leftData.rank < rightData.rank);
//         break;
//     case 1: // TimeLoss
//         naturalOrder = (leftData.timeLoss < rightData.timeLoss);
//         break;
//     case 2: // Percent
//         if (leftData.resultSeconds > 0 && rightData.resultSeconds > 0) {
//             int leftLeader = leftData.resultSeconds - leftData.timeLoss;
//             int rightLeader = rightData.resultSeconds - rightData.timeLoss;

//             double leftPct = leftLeader > 0 ? (double)leftData.resultSeconds / leftLeader : 1.0;
//             double rightPct = rightLeader > 0 ? (double)rightData.resultSeconds / rightLeader : 1.0;

//             naturalOrder = (leftPct < rightPct);
//         } else {
//             naturalOrder = false;
//         }
//         break;
//     default:
//         naturalOrder = false;
//     }

//     // Для виртуальных колонок порядок сортировки применяется так же
//     return m_currentSortOrder == Qt::AscendingOrder ? naturalOrder : !naturalOrder;
// }

// bool ResultsProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
// {
//     if (!source_left.isValid() || !source_right.isValid()) {
//         return QSortFilterProxyModel::lessThan(source_left, source_right);
//     }

//     // Если сортировка по обычной колонке
//     if (m_currentSortColumn < m_sourceColumnCount) {
//         QModelIndex leftIdx = sourceModel()->index(source_left.row(), m_currentSortColumn);
//         QModelIndex rightIdx = sourceModel()->index(source_right.row(), m_currentSortColumn);

//         if (!leftIdx.isValid() || !rightIdx.isValid()) {
//             qWarning() << "Invalid indices for comparison:" << leftIdx << rightIdx;
//             return false;
//         }

//         QVariant leftData = sourceModel()->data(leftIdx, Qt::DisplayRole);
//         QVariant rightData = sourceModel()->data(rightIdx, Qt::DisplayRole);

//         QString leftHeader = sourceModel()->headerData(m_currentSortColumn, Qt::Horizontal).toString();

//         qDebug() << "\n--- lessThan debug ---";
//         qDebug() << "  Сортировка по колонке:" << m_currentSortColumn << "(" << leftHeader << ")";
//         qDebug() << "  Левая строка:" << source_left.row()
//                  << "данные:" << leftData.toString();
//         qDebug() << "  Правая строка:" << source_right.row()
//                  << "данные:" << rightData.toString();
//         qDebug() << "  Порядок сортировки:" << (m_currentSortOrder == Qt::AscendingOrder ? "ASC" : "DESC");

//         // Используем QVariant::compare для получения результата сравнения
//         auto comparison = QVariant::compare(leftData, rightData);

//         bool result;
//         if (m_currentSortOrder == Qt::AscendingOrder) {
//             // Для ASC: возвращаем true если left < right
//             result = (comparison == QPartialOrdering::Less);
//         } else {
//             // Для DESC: возвращаем true если left > right
//             result = (comparison == QPartialOrdering::Greater);
//         }

//         qDebug() << "  Результат QVariant::compare:"
//                  << (comparison == QPartialOrdering::Less ? "Less" :
//                          (comparison == QPartialOrdering::Greater ? "Greater" :
//                               (comparison == QPartialOrdering::Equivalent ? "Equal" : "Unordered")));
//         qDebug() << "  lessThan результат:" << result;
//         qDebug() << "----------------------\n";

//         return result;
//     }

//     // Сортировка по виртуальной колонке
//     if (!m_cacheValid) {
//         const_cast<ResultsProxyModel*>(this)->rebuildCache();
//     }

//     qint64 leftId = sourceModel()->data(source_left).toLongLong();
//     qint64 rightId = sourceModel()->data(source_right).toLongLong();

//     if (!m_resultsCache.contains(leftId) || !m_resultsCache.contains(rightId)) {
//         qWarning() << "ResultsProxyModel::lessThan - ID not found in cache:" << leftId << rightId;
//         return false;
//     }

//     const ResultData& leftData = m_resultsCache[leftId];
//     const ResultData& rightData = m_resultsCache[rightId];

//     int virtualCol = m_currentSortColumn - m_sourceColumnCount;

//     bool naturalOrder = false;
//     switch (virtualCol) {
//     case 0: // Rank
//         naturalOrder = (leftData.rank < rightData.rank);
//         break;
//     case 1: // TimeLoss
//         naturalOrder = (leftData.timeLoss < rightData.timeLoss);
//         break;
//     case 2: // Percent
//         if (leftData.resultSeconds > 0 && rightData.resultSeconds > 0) {
//             int leftLeader = leftData.resultSeconds - leftData.timeLoss;
//             int rightLeader = rightData.resultSeconds - rightData.timeLoss;

//             double leftPct = leftLeader > 0 ? (double)leftData.resultSeconds / leftLeader : 1.0;
//             double rightPct = rightLeader > 0 ? (double)rightData.resultSeconds / rightLeader : 1.0;

//             naturalOrder = (leftPct < rightPct);
//         } else {
//             naturalOrder = false;
//         }
//         break;
//     default:
//         naturalOrder = false;
//     }

//     return m_currentSortOrder == Qt::AscendingOrder ? naturalOrder : !naturalOrder;
// }

// bool ResultsProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
// {
//     if (!source_left.isValid() || !source_right.isValid()) {
//         return QSortFilterProxyModel::lessThan(source_left, source_right);
//     }

//     // Если сортировка по обычной колонке
//     if (m_currentSortColumn < m_sourceColumnCount) {
//         QModelIndex leftIdx = sourceModel()->index(source_left.row(), m_currentSortColumn);
//         QModelIndex rightIdx = sourceModel()->index(source_right.row(), m_currentSortColumn);

//         if (!leftIdx.isValid() || !rightIdx.isValid()) {
//             qWarning() << "Invalid indices for comparison:" << leftIdx << rightIdx;
//             return false;
//         }

//         QVariant leftData = sourceModel()->data(leftIdx, Qt::DisplayRole);
//         QVariant rightData = sourceModel()->data(rightIdx, Qt::DisplayRole);

//         QString leftHeader = sourceModel()->headerData(m_currentSortColumn, Qt::Horizontal).toString();

//         qDebug() << "\n--- lessThan debug ---";
//         qDebug() << "  Сортировка по колонке:" << m_currentSortColumn << "(" << leftHeader << ")";
//         qDebug() << "  Левая строка:" << source_left.row()
//                  << "данные:" << leftData.toString();
//         qDebug() << "  Правая строка:" << source_right.row()
//                  << "данные:" << rightData.toString();
//         qDebug() << "  Порядок сортировки:" << (m_currentSortOrder == Qt::AscendingOrder ? "ASC" : "DESC");

//         // Проверяем, являются ли данные числами в строковом формате
//         bool leftOk, rightOk;
//         qlonglong leftNum = leftData.toString().toLongLong(&leftOk);
//         qlonglong rightNum = rightData.toString().toLongLong(&rightOk);

//         bool result;
//         if (leftOk && rightOk) {
//             // Для чисел используем прямое сравнение
//             qDebug() << "  Числовое сравнение:" << leftNum << "vs" << rightNum;

//             // ВАЖНО: lessThan ВСЕГДА должен отвечать на вопрос "должен ли левый элемент быть перед правым?"
//             // независимо от порядка сортировки!
//             if (m_currentSortOrder == Qt::AscendingOrder) {
//                 // При ASC: меньшие числа должны быть перед большими
//                 result = (leftNum < rightNum);
//             } else {
//                 // При DESC: большие числа должны быть перед меньшими
//                 result = (leftNum < rightNum);
//             }
//         } else {
//             // Для строк используем QVariant::compare
//             auto comparison = QVariant::compare(leftData, rightData);

//             if (m_currentSortOrder == Qt::AscendingOrder) {
//                 result = (comparison == QPartialOrdering::Less);
//             } else {
//                 result = (comparison == QPartialOrdering::Greater);
//             }

//             qDebug() << "  Результат QVariant::compare:"
//                      << (comparison == QPartialOrdering::Less ? "Less" :
//                              (comparison == QPartialOrdering::Greater ? "Greater" :
//                                   (comparison == QPartialOrdering::Equivalent ? "Equal" : "Unordered")));
//         }

//         qDebug() << "  lessThan результат:" << result;
//         qDebug() << "----------------------\n";

//         return result;
//     }

//     // Сортировка по виртуальной колонке
//     if (!m_cacheValid) {
//         const_cast<ResultsProxyModel*>(this)->rebuildCache();
//     }

//     qint64 leftId = sourceModel()->data(source_left).toLongLong();
//     qint64 rightId = sourceModel()->data(source_right).toLongLong();

//     if (!m_resultsCache.contains(leftId) || !m_resultsCache.contains(rightId)) {
//         qWarning() << "ResultsProxyModel::lessThan - ID not found in cache:" << leftId << rightId;
//         return false;
//     }

//     const ResultData& leftData = m_resultsCache[leftId];
//     const ResultData& rightData = m_resultsCache[rightId];

//     int virtualCol = m_currentSortColumn - m_sourceColumnCount;

//     bool naturalOrder = false;
//     switch (virtualCol) {
//     case 0: // Rank
//         naturalOrder = (leftData.rank < rightData.rank);
//         break;
//     case 1: // TimeLoss
//         naturalOrder = (leftData.timeLoss < rightData.timeLoss);
//         break;
//     case 2: // Percent
//         if (leftData.resultSeconds > 0 && rightData.resultSeconds > 0) {
//             int leftLeader = leftData.resultSeconds - leftData.timeLoss;
//             int rightLeader = rightData.resultSeconds - rightData.timeLoss;

//             double leftPct = leftLeader > 0 ? (double)leftData.resultSeconds / leftLeader : 1.0;
//             double rightPct = rightLeader > 0 ? (double)rightData.resultSeconds / rightLeader : 1.0;

//             naturalOrder = (leftPct < rightPct);
//         } else {
//             naturalOrder = false;
//         }
//         break;
//     default:
//         naturalOrder = false;
//     }

//     // Для виртуальных колонок используем ту же логику
//     if (m_currentSortOrder == Qt::AscendingOrder) {
//         return naturalOrder;
//     } else {
//         return !naturalOrder;
//     }
// }

bool ResultsProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
    if (!source_left.isValid() || !source_right.isValid()) {
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

    // Если сортировка по обычной колонке
    if (m_currentSortColumn < m_sourceColumnCount) {
        QModelIndex leftIdx = sourceModel()->index(source_left.row(), m_currentSortColumn);
        QModelIndex rightIdx = sourceModel()->index(source_right.row(), m_currentSortColumn);

        if (!leftIdx.isValid() || !rightIdx.isValid()) {
            qWarning() << "Invalid indices for comparison:" << leftIdx << rightIdx;
            return false;
        }

        QVariant leftData = sourceModel()->data(leftIdx, Qt::DisplayRole);
        QVariant rightData = sourceModel()->data(rightIdx, Qt::DisplayRole);

        QString leftHeader = sourceModel()->headerData(m_currentSortColumn, Qt::Horizontal).toString();

        qDebug() << "\n--- lessThan debug ---";
        qDebug() << "  Сортировка по колонке:" << m_currentSortColumn << "(" << leftHeader << ")";
        qDebug() << "  Левая строка:" << source_left.row()
                 << "данные:" << leftData.toString();
        qDebug() << "  Правая строка:" << source_right.row()
                 << "данные:" << rightData.toString();
        qDebug() << "  Порядок сортировки:" << (m_currentSortOrder == Qt::AscendingOrder ? "ASC" : "DESC");

        // Проверяем, являются ли данные числами в строковом формате
        bool leftOk, rightOk;
        qlonglong leftNum = leftData.toString().toLongLong(&leftOk);
        qlonglong rightNum = rightData.toString().toLongLong(&rightOk);

        bool result;
        if (leftOk && rightOk) {
            // Для чисел используем прямое сравнение
            qDebug() << "  Числовое сравнение:" << leftNum << "vs" << rightNum;

            // ВАЖНО: lessThan ВСЕГДА отвечает на вопрос "должен ли левый элемент быть перед правым?"
            // Алгоритм сортировки сам учтет порядок ASC/DESC
            result = (leftNum < rightNum);
        } else {
            // Для строк используем QVariant::compare
            auto comparison = QVariant::compare(leftData, rightData);
            result = (comparison == QPartialOrdering::Less);

            qDebug() << "  Результат QVariant::compare:"
                     << (comparison == QPartialOrdering::Less ? "Less" :
                             (comparison == QPartialOrdering::Greater ? "Greater" :
                                  (comparison == QPartialOrdering::Equivalent ? "Equal" : "Unordered")));
        }

        qDebug() << "  lessThan результат:" << result;
        qDebug() << "----------------------\n";

        return result;
    }

    // Сортировка по виртуальной колонке
    if (!m_cacheValid) {
        const_cast<ResultsProxyModel*>(this)->rebuildCache();
    }

    qint64 leftId = sourceModel()->data(source_left).toLongLong();
    qint64 rightId = sourceModel()->data(source_right).toLongLong();

    if (!m_resultsCache.contains(leftId) || !m_resultsCache.contains(rightId)) {
        qWarning() << "ResultsProxyModel::lessThan - ID not found in cache:" << leftId << rightId;
        return false;
    }

    const ResultData& leftData = m_resultsCache[leftId];
    const ResultData& rightData = m_resultsCache[rightId];

    int virtualCol = m_currentSortColumn - m_sourceColumnCount;

    bool result = false;
    switch (virtualCol) {
    case 0: // Rank
        // ВАЖНО: используем ТУ ЖЕ ЛОГИКУ - отвечаем только на вопрос "меньше ли?"
        result = (leftData.rank < rightData.rank);
        qDebug() << "  Виртуальная колонка Rank:" << leftData.rank << "<" << rightData.rank << "=" << result;
        break;
    case 1: // TimeLoss
        result = (leftData.timeLoss < rightData.timeLoss);
        qDebug() << "  Виртуальная колонка TimeLoss:" << leftData.timeLoss << "<" << rightData.timeLoss << "=" << result;
        break;
    case 2: // Percent
        if (leftData.resultSeconds > 0 && rightData.resultSeconds > 0) {
            int leftLeader = leftData.resultSeconds - leftData.timeLoss;
            int rightLeader = rightData.resultSeconds - rightData.timeLoss;

            double leftPct = leftLeader > 0 ? (double)leftData.resultSeconds / leftLeader : 1.0;
            double rightPct = rightLeader > 0 ? (double)rightData.resultSeconds / rightLeader : 1.0;

            result = (leftPct < rightPct);
            qDebug() << "  Виртуальная колонка Percent:" << leftPct << "<" << rightPct << "=" << result;
        } else {
            result = false;
        }
        break;
    default:
        result = false;
    }

    qDebug() << "  lessThan результат для виртуальной колонки:" << result;
    qDebug() << "----------------------\n";

    // ВОЗВРАЩАЕМ ТОЛЬКО result, без инверсии!
    // Алгоритм сортировки сам учтет m_currentSortOrder
    return result;
}


bool ResultsProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void ResultsProxyModel::recalculateRanks()
{
    rebuildCache();
}

void ResultsProxyModel::rebuildCache()
{
    if (!sourceModel() || m_sourceColumnCount <= 0) {
        return;
    }

    emit calculationStarted();
    qDebug() << "=== ResultsProxyModel::rebuildCache() ===";

    QMap<GroupKey, QList<ResultData>> groups;
    QHash<qint64, ResultData> newCache;

    int rowCount = sourceModel()->rowCount();
    qDebug() << "Processing" << rowCount << "rows";

    for (int row = 0; row < rowCount; ++row) {
        ResultData data;
        data.resultId = getResultIdForRow(row);
        data.participantId = getParticipantIdForRow(row);
        data.resultSeconds = getResultSecondsForRow(row);
        data.ageGroup = getAgeGroupForRow(row);
        data.gender = getGenderForRow(row);
        data.rank = 0;
        data.timeLoss = 0;

        newCache[data.resultId] = data;

        if (data.resultSeconds > 0 && !data.ageGroup.isEmpty()) {
            GroupKey key;
            key.ageGroup = data.ageGroup;
            key.gender = data.gender;
            groups[key].append(data);
        }
    }

    qDebug() << "Cached" << newCache.size() << "results in" << groups.size() << "groups";

    // Рассчитываем ранги по группам
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        QList<ResultData>& groupList = it.value();

        std::sort(groupList.begin(), groupList.end(),
                  [](const ResultData& a, const ResultData& b) {
                      return a.resultSeconds < b.resultSeconds;
                  });

        if (groupList.isEmpty()) continue;

        int leaderTime = groupList.first().resultSeconds;
        int currentRank = 1;
        int prevTime = -1;

        for (int i = 0; i < groupList.size(); ++i) {
            ResultData& data = groupList[i];

            if (data.resultSeconds != prevTime) {
                currentRank = i + 1;
                prevTime = data.resultSeconds;
            }

            data.rank = currentRank;
            data.timeLoss = data.resultSeconds - leaderTime;

            // Обновляем в основном кеше
            if (newCache.contains(data.resultId)) {
                newCache[data.resultId].rank = data.rank;
                newCache[data.resultId].timeLoss = data.timeLoss;
            }
        }

        qDebug() << "Group" << it.key().ageGroup << it.key().gender
                 << "processed, leader time:" << leaderTime;
    }

    // Заменяем кеш
    m_resultsCache = newCache;
    m_cacheValid = true;
    qDebug() << "=== rebuildCache() finished, cached" << m_resultsCache.size() << "results ===";

    // Уведомляем об изменении данных во всех виртуальных колонках
    if (this->rowCount() > 0) {
        QModelIndex topLeft = index(0, m_sourceColumnCount);
        QModelIndex bottomRight = index(this->rowCount() - 1,
                                        m_sourceColumnCount + ResultColumn::VIRTUAL_COLUMN_COUNT - 1);
        if (topLeft.isValid() && bottomRight.isValid()) {
            emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole, Qt::UserRole});
        }
    }

    emit calculationFinished();
}

// Вспомогательные методы
qint64 ResultsProxyModel::getResultIdForRow(int sourceRow) const
{
    if (!sourceModel() || sourceRow < 0 || sourceRow >= sourceModel()->rowCount()) {
        return -1;
    }

    QModelIndex idx = sourceModel()->index(sourceRow, 0);
    return sourceModel()->data(idx).toLongLong();
}

qint64 ResultsProxyModel::getParticipantIdForRow(int sourceRow) const
{
    if (!sourceModel() || sourceRow < 0 || sourceRow >= sourceModel()->rowCount()) {
        return -1;
    }

    QModelIndex idx = sourceModel()->index(sourceRow, 11); // participant_id колонка
    return sourceModel()->data(idx).toLongLong();
}

int ResultsProxyModel::getResultSecondsForRow(int sourceRow) const
{
    if (!sourceModel() || sourceRow < 0 || sourceRow >= sourceModel()->rowCount()) {
        return 0;
    }

    QModelIndex idx = sourceModel()->index(sourceRow, 4); // result_time колонка
    bool ok;
    int seconds = sourceModel()->data(idx).toInt(&ok);

    if (!ok) {
        QString timeStr = sourceModel()->data(idx).toString();
        if (!timeStr.isEmpty()) {
            QTime time = QTime::fromString(timeStr, "hh:mm:ss.zzz");
            if (time.isValid()) {
                seconds = QTime(0,0,0).msecsTo(time) / 1000;
            }
        }
    }

    return seconds;
}

QString ResultsProxyModel::getAgeGroupForRow(int sourceRow) const
{
    if (!sourceModel() || sourceRow < 0 || sourceRow >= sourceModel()->rowCount()) {
        return QString();
    }

    QModelIndex idx = sourceModel()->index(sourceRow, 2); // age_group_name колонка
    return sourceModel()->data(idx).toString().trimmed();
}

QString ResultsProxyModel::getGenderForRow(int sourceRow) const
{
    if (!sourceModel() || sourceRow < 0 || sourceRow >= sourceModel()->rowCount()) {
        return QString();
    }

    QModelIndex idx = sourceModel()->index(sourceRow, 10); // gender колонка
    return sourceModel()->data(idx).toString().trimmed();
}

QString ResultsProxyModel::formatTimeLoss(int seconds) const
{
    if (seconds < 0) return "—";

    if (seconds < 60) {
        return QString("%1 сек").arg(seconds);
    }
    else if (seconds < 3600) {
        int minutes = seconds / 60;
        int secs = seconds % 60;
        return QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        int hours = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        int secs = seconds % 60;
        return QString("%1:%2:%3").arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
}

void ResultsProxyModel::setVisibleColumns(const QList<int>& columns)
{
    qDebug() << "ResultsProxyModel::setVisibleColumns для" << objectName()
             << "устанавливаю:" << columns;

    beginResetModel();

    m_visibleColumns.clear();

    for (int col : columns) {
        m_visibleColumns.insert(col);
    }

    // Всегда показываем виртуальные колонки
    if (m_sourceColumnCount > 0) {
        m_visibleColumns.insert(m_sourceColumnCount + 0);
        m_visibleColumns.insert(m_sourceColumnCount + 1);
        m_visibleColumns.insert(m_sourceColumnCount + 2);
    }

    m_cacheValid = false;

    qDebug() << "  m_visibleColumns теперь:" << m_visibleColumns;

    endResetModel();

    if (sourceModel() && sourceModel()->rowCount() > 0) {
        rebuildCache();
    }
}

bool ResultsProxyModel::filterAcceptsColumn(int sourceColumn, const QModelIndex& sourceParent) const
{
    Q_UNUSED(sourceParent);

    if (m_visibleColumns.isEmpty()) {
        return true;
    }

    return m_visibleColumns.contains(sourceColumn);
}

bool ResultsProxyModel::isColumnVisible(int column) const
{
    if (m_visibleColumns.isEmpty()) {
        return true;
    }
    return m_visibleColumns.contains(column);
}

void ResultsProxyModel::setColumnVisible(int column, bool visible)
{
    if (visible) {
        m_visibleColumns.insert(column);
    } else {
        m_visibleColumns.remove(column);
    }
    invalidateFilter();
}

void ResultsProxyModel::resetModelStructure()
{
    beginResetModel();
    m_cacheValid = false;
    updateSourceColumnCount();
    endResetModel();
}

QList<int> ResultsProxyModel::visibleColumns() const
{
    return m_visibleColumns.values();
}

int ResultsProxyModel::convertTimeToSeconds(const QString& timeStr) const
{
    if (timeStr.isEmpty()) return 0;

    QTime time = QTime::fromString(timeStr, "hh:mm:ss.zzz");
    if (time.isValid()) {
        return QTime(0,0,0).msecsTo(time) / 1000;
    }

    return 0;
}
