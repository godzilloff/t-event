#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QHash>
#include <QVariant>
#include <QList>
#include <QDate>
#include <QString>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();

    bool initialize(const QString& databasePath = "tevent.db");
    bool openDatabase(const QString& databasePath);
    void closeDatabase();

    bool isOpen() const { return m_db.isOpen(); }
    //QSqlDatabase database() const { return m_db; }
    QString databasePath() const { return m_db.databaseName(); }

    template<typename Func>
    auto withDatabase(Func&& func) -> decltype(func(std::declval<QSqlDatabase&>()))
    {
        if (!m_db.isOpen()) {
            throw std::runtime_error("Database is not open");
        }
        return func(m_db);
    }

    void reset(); // Метод для полного сброса состояния
    void setDatabasePath(const QString& path); // Явная установка пути

    // CRUD операции
    bool createRecord(const QString& tableName, const QHash<QString, QVariant>& data, qint64* id = nullptr);
    bool updateRecord(const QString& tableName, qint64 id, const QHash<QString, QVariant>& data);
    bool deleteRecord(const QString& tableName, qint64 id);
    QHash<QString, QVariant> readRecord(const QString& tableName, qint64 id) const;

    // Поиск записей
    QList<qint64> findRecords(const QString& tableName, const QString& field = QString(),
                              const QVariant& value = QVariant()) const;

    // Транзакции
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // Информация о базе
    QStringList tableNames() const;
    QStringList fieldNames(const QString& tableName) const;
    int recordCount(const QString& tableName) const;

    // Управление соревнованиями
    bool createCompetition(const QString& name, const QDate& date);
    qint64 currentCompetitionId() const;
    void setCurrentCompetitionId(qint64 id);

    // Проверка и создание таблиц
    bool checkAndCreateTables();
    void vacuumDatabase();

    // Резервное копирование
    bool backupDatabase(const QString& backupPath);

    // Выполнение произвольного запроса
    bool executeQuery(const QString& sql, const QHash<QString, QVariant>& params = {});
    QList<QHash<QString, QVariant>> executeSelect(const QString& sql, const QHash<QString, QVariant>& params = {});

signals:
    void databaseOpened(const QString& path);
    void databaseClosed();
    void databaseError(const QString& error);

private:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    void createTables();
    void createViews();
    void createIndexes();
    bool tableExists(const QString& tableName) const;
    bool dropTable(const QString& tableName);
    void cleanup();

    QSqlDatabase m_db;
    qint64 m_currentCompetitionId = -1;
    QString m_databasePath;
    QString m_connectionName = "DatabaseManagerConnection";

    Q_DISABLE_COPY(DatabaseManager)
};

#endif // DATABASEMANAGER_H
