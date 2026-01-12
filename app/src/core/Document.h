#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QObject>
#include <QString>
#include <QDate>
#include <QHash>
#include <QVariant>

class Document : public QObject
{
    Q_OBJECT

public:
    explicit Document(QObject* parent = nullptr);
    ~Document();

    // Управление документом
    bool createNew(const QString& filePath = QString());
    bool open(const QString& filePath);
    bool save();
    bool saveAs(const QString& filePath);
    bool close();

    // Импорт/экспорт
    bool importFromCsv(const QString& filePath, const QString& tableName);
    bool exportToCsv(const QString& filePath, const QString& tableName);

    // Информация о документе
    bool isModified() const { return m_isModified; }
    bool isOpen() const { return m_isOpen; }
    QString filePath() const { return m_filePath; }
    QString fileName() const;
    QString competitionName() const { return m_competitionName; }
    qint64 competitionId() const { return m_competitionId; }
    QString databasePath() const;

    // Установка состояния изменений
    void setModified(bool modified);

    // CRUD операции с undo/redo
    qint64 insertRecord(const QString& tableName, const QHash<QString, QVariant>& data);
    bool updateRecord(const QString& tableName, qint64 id, const QHash<QString, QVariant>& data);
    bool deleteRecord(const QString& tableName, qint64 id);

    QHash<QString, QVariant> getRecord(const QString& tableName, qint64 id) const;

    // Статистика
    int recordCount(const QString& tableName) const;
    QList<qint64> getAllRecordIds(const QString& tableName) const;

    // Управление соревнованием
    bool setCurrentCompetition(qint64 id);
    bool createCompetition(const QString& name, const QDate& date);

signals:
    void documentCreated(const QString& filePath);
    void documentOpened(const QString& filePath);
    void documentSaved(const QString& filePath);
    void documentClosed();
    void documentModified(bool modified);
    void recordInserted(const QString& tableName, qint64 id);
    void recordUpdated(const QString& tableName, qint64 id);
    void recordDeleted(const QString& tableName, qint64 id);
    void competitionChanged(qint64 competitionId);

private:
    bool setupNewDatabase(const QString& filePath);
    bool loadCompetitionInfo();

    QString m_filePath;
    QString m_competitionName;
    QDate m_competitionDate;
    qint64 m_competitionId = -1;
    bool m_isOpen = false;
    bool m_isModified = false;

    // Кэш для отслеживания изменений
    QHash<QString, QHash<qint64, QHash<QString, QVariant>>> m_originalData;
};

#endif // DOCUMENT_H
