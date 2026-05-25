// ResultsProxyModel.h
#ifndef RESULTSPROXYMODEL_H
#define RESULTSPROXYMODEL_H

#include "AbstractProxyModel.h"
#include <QHash>
#include <QMap>
#include <QSet>

class ResultsProxyModel : public AbstractProxyModel
{
    Q_OBJECT

public:
    enum class TimePrecision {
        Seconds = 0,      // Без долей: "HH:MM:SS"
        Tenths = 1,       // Десятые: "HH:MM:SS.t"
        Hundredths = 2,   // Сотые: "HH:MM:SS.tt"
        Milliseconds = 3  // Миллисекунды: "HH:MM:SS.ttt"
    };

    explicit ResultsProxyModel(QObject* parent = nullptr);
    ~ResultsProxyModel();

    // QAbstractItemModel
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // QSortFilterProxyModel
    void setSourceModel(QAbstractItemModel* sourceModel) override;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    // AbstractProxyModel
    qint64 recordId(const QModelIndex& index) const override;
    void recalculateRanks() override;
    void setVisibleColumns(const QList<int>& columns) override;
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex& sourceParent) const override;
    QList<int> visibleColumns() const override;
    bool isColumnVisible(int column) const override;
    void setColumnVisible(int column, bool visible) override;

    // Специфичные методы
    int getSourceColumnCount() const { return m_sourceColumnCount; }
    void updateSourceColumnCount();
    void resetModelStructure();

    void setCurrentSortColumn(int column) { m_currentSortColumn = column; }
    void setCurrentSortOrder(Qt::SortOrder order) { m_currentSortOrder = order; }
    bool testLessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
        return lessThan(source_left, source_right);
    }

    int convertTimeToSeconds(const QString& timeStr) const;

    void setTimePrecision(TimePrecision precision);
    TimePrecision timePrecision() const { return m_timePrecision; }

signals:
    void calculationStarted();
    void calculationFinished();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private slots:
    void onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

private:
    struct ResultData {
        qint64 resultId;
        qint64 participantId;
        int resultSeconds;
        QString ageGroup;
        QString gender;
        int rank;
        int timeLoss;

        ResultData() : resultId(-1), participantId(-1), resultSeconds(0), rank(0), timeLoss(0) {}
    };

    struct GroupKey {
        QString ageGroup;
        QString gender;

        bool operator<(const GroupKey& other) const {
            if (ageGroup != other.ageGroup) return ageGroup < other.ageGroup;
            return gender < other.gender;
        }
    };

    // Методы для получения данных
    qint64 getResultIdForRow(int sourceRow) const;
    qint64 getParticipantIdForRow(int sourceRow) const;
    int getResultSecondsForRow(int sourceRow) const;
    QString getAgeGroupForRow(int sourceRow) const;
    QString getGenderForRow(int sourceRow) const;
    bool isVirtualColumn(int column) const { return column >= m_sourceColumnCount; }
    QString formatTimeLoss(int seconds) const;
    void rebuildCache();

    // Данные
    mutable QHash<qint64, ResultData> m_resultsCache;
    mutable bool m_cacheValid;
    int m_sourceColumnCount;
    QSet<int> m_visibleColumns;
    int m_currentSortColumn;
    Qt::SortOrder m_currentSortOrder;

    QString extractTimeFromDateTime(const QString& dateTimeStr) const;
    QString formatTimeWithPrecision(const QTime& time) const;
    TimePrecision m_timePrecision = TimePrecision::Seconds;
};

#endif // RESULTSPROXYMODEL_H
