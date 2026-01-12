#include "DatabaseManager.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDate>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QUuid>
#include <QStandardPaths>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::initialize(const QString& databasePath)
{
    QString actualPath = databasePath;

    // Если путь не указан, используем стандартное расположение
    if (actualPath.isEmpty() || actualPath == ":memory:") {
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(appDataDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        actualPath = dir.filePath("tevent.db");
    }

    return openDatabase(actualPath);
}

bool DatabaseManager::openDatabase(const QString& databasePath)
{
    if (m_db.isOpen()) {
        closeDatabase();
    }

    m_databasePath = databasePath;

    // Проверяем, существует ли файл
    bool fileExists = QFile::exists(databasePath);

    m_db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
    m_db.setDatabaseName(databasePath);

    if (!m_db.open()) {
        qCritical() << "Cannot open database:" << m_db.lastError().text();
        emit databaseError(m_db.lastError().text());
        return false;
    }

    // Настройки SQLite для производительности
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        qWarning() << "Failed to enable foreign keys:" << query.lastError().text();
    }

    // // Оптимизации для работы с большими объемами данных
    // query.exec("PRAGMA journal_mode = WAL");
    // query.exec("PRAGMA synchronous = NORMAL");
    // query.exec("PRAGMA cache_size = -20000"); // 20MB кэша
    // query.exec("PRAGMA temp_store = MEMORY");
    // query.exec("PRAGMA mmap_size = 268435456"); // 256MB mmap

    // Оптимизации для работы с большими объемами данных
    query.exec("PRAGMA journal_mode = DELETE");
    query.exec("PRAGMA synchronous = FULL");
    query.exec("PRAGMA cache_size = -20000"); // 20MB кэша
    query.exec("PRAGMA mmap_size = 268435456"); // 256MB mmap

    // Создаем таблицы если файла не было
    if (!fileExists) {
        createTables();
        createViews();
        createIndexes();
        qDebug() << "Created new database at:" << databasePath;
    } else {
        // Проверяем и обновляем структуру если нужно
        checkAndCreateTables();
    }

    emit databaseOpened(databasePath);
    return true;
}

void DatabaseManager::closeDatabase()
{
    if (m_db.isOpen()) {
        // Оптимизация перед закрытием
        QSqlQuery query(m_db);
        query.exec("PRAGMA optimize");

        m_db.close();
        emit databaseClosed();
    }
}

bool DatabaseManager::checkAndCreateTables()
{
    QStringList requiredTables = {
        "competitions", "delegations", "age_groups",
        "distances", "participants", "team_members",
        "results", "control_point_times"
    };

    bool allTablesExist = true;

    for (const QString& table : requiredTables) {
        if (!tableExists(table)) {
            qWarning() << "Table" << table << "does not exist, recreating database...";
            allTablesExist = false;
            break;
        }
    }

    if (!allTablesExist) {
        // Создаем все таблицы заново
        createTables();
        createViews();
        createIndexes();
        return true;
    }

    return allTablesExist;
}

void DatabaseManager::createTables()
{
    QSqlQuery query(m_db);

    // Таблица соревнований
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS competitions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            date TEXT NOT NULL,
            location TEXT,
            chief_judge TEXT,
            chief_secretary TEXT,
            organizations TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )");

    // Таблица делегаций
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS delegations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            competition_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            representative TEXT,
            contact TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (competition_id) REFERENCES competitions(id) ON DELETE CASCADE,
            UNIQUE(competition_id, name)
        )
    )");

    // Таблица возрастных групп
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS age_groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            competition_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            min_age INTEGER,
            max_age INTEGER,
            price REAL DEFAULT 0.0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (competition_id) REFERENCES competitions(id) ON DELETE CASCADE
        )
    )");

    // Таблица дистанций
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS distances (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            competition_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            length REAL DEFAULT 0.0,
            control_time INTEGER DEFAULT 0,
            control_points TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (competition_id) REFERENCES competitions(id) ON DELETE CASCADE
        )
    )");

    // Участники
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS participants (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            competition_id INTEGER NOT NULL,
            delegation_id INTEGER,
            distance_id INTEGER NOT NULL,
            age_group_id INTEGER,

            participant_type TEXT DEFAULT 'individual',

            bib_number TEXT UNIQUE,
            chip_number TEXT,
            start_time TEXT,

            full_name TEXT,
            birth_date TEXT,
            gender TEXT,

            team_name TEXT,
            team_size INTEGER DEFAULT 1,

            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,

            FOREIGN KEY (competition_id) REFERENCES competitions(id) ON DELETE CASCADE,
            FOREIGN KEY (delegation_id) REFERENCES delegations(id),
            FOREIGN KEY (distance_id) REFERENCES distances(id),
            FOREIGN KEY (age_group_id) REFERENCES age_groups(id),

            CHECK (
                (participant_type = 'individual' AND full_name IS NOT NULL) OR
                (participant_type = 'team' AND team_name IS NOT NULL)
            )
        )
    )");

    // Члены команды
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS team_members (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            participant_id INTEGER NOT NULL,
            full_name TEXT NOT NULL,
            birth_date TEXT,
            gender TEXT,
            bib_number TEXT,
            chip_number TEXT UNIQUE,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (participant_id) REFERENCES participants(id) ON DELETE CASCADE
        )
    )");

    // Результаты
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            participant_id INTEGER NOT NULL,
            chip_number TEXT NOT NULL,
            start_time TEXT,
            finish_time TEXT,
            result_time INTEGER,
            status TEXT DEFAULT 'active',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (participant_id) REFERENCES participants(id) ON DELETE CASCADE
        )
    )");

    // Времена прохождения контрольных пунктов
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS control_point_times (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            result_id INTEGER NOT NULL,
            control_point_id INTEGER NOT NULL,
            visit_time TEXT NOT NULL,
            order_number INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (result_id) REFERENCES results(id) ON DELETE CASCADE
        )
    )");
}

void DatabaseManager::createViews()
{
    QSqlQuery query(m_db);

    query.exec(R"(
        CREATE VIEW IF NOT EXISTS v_participants_details AS
        SELECT
            p.id,
            p.competition_id,
            p.participant_type,
            p.full_name,
            p.team_name,
            p.bib_number,
            p.chip_number,
            p.start_time,
            p.birth_date,
            p.gender,
            p.team_size,
            d.name as delegation_name,
            dist.name as distance_name,
            ag.name as age_group_name,
            p.delegation_id,
            p.distance_id,
            p.age_group_id,
            p.created_at,
            p.updated_at
        FROM participants p
        LEFT JOIN delegations d ON p.delegation_id = d.id
        LEFT JOIN distances dist ON p.distance_id = dist.id
        LEFT JOIN age_groups ag ON p.age_group_id = ag.id
    )");
}

void DatabaseManager::createIndexes()
{
    QSqlQuery query(m_db);

    query.exec("CREATE INDEX IF NOT EXISTS idx_participants_competition ON participants(competition_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_participants_delegation ON participants(delegation_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_participants_distance ON participants(distance_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_participants_bib ON participants(bib_number)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_participants_chip ON participants(chip_number)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_results_participant ON results(participant_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_control_times_result ON control_point_times(result_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_delegations_competition ON delegations(competition_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_distances_competition ON distances(competition_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_age_groups_competition ON age_groups(competition_id)");
}

bool DatabaseManager::createRecord(const QString& tableName, const QHash<QString, QVariant>& data, qint64* id)
{
    if (!m_db.isOpen() || tableName.isEmpty() || data.isEmpty()) {
        return false;
    }

    QStringList fieldNames = data.keys();
    QStringList placeholders;
    for (int i = 0; i < fieldNames.size(); ++i) {
        placeholders.append("?");
    }

    QString sql = QString("INSERT INTO %1 (%2) VALUES (%3)")
                      .arg(tableName)
                      .arg(fieldNames.join(", "))
                      .arg(placeholders.join(", "));

    QSqlQuery query(m_db);
    query.prepare(sql);

    for (const QString& field : fieldNames) {
        query.addBindValue(data[field]);
    }

    if (!query.exec()) {
        qWarning() << "Failed to create record in" << tableName << ":" << query.lastError().text();
        emit databaseError(query.lastError().text());
        return false;
    }

    if (id) {
        *id = query.lastInsertId().toLongLong();
    }

    return true;
}

bool DatabaseManager::updateRecord(const QString& tableName, qint64 id, const QHash<QString, QVariant>& data)
{
    if (!m_db.isOpen() || tableName.isEmpty() || data.isEmpty() || id <= 0) {
        return false;
    }

    QStringList updates;
    QStringList fieldNames = data.keys();

    for (const QString& field : fieldNames) {
        updates.append(field + " = ?");
    }

    QString sql = QString("UPDATE %1 SET %2, updated_at = CURRENT_TIMESTAMP WHERE id = ?")
                      .arg(tableName)
                      .arg(updates.join(", "));

    QSqlQuery query(m_db);
    query.prepare(sql);

    for (const QString& field : fieldNames) {
        query.addBindValue(data[field]);
    }

    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to update record in" << tableName << ":" << query.lastError().text();
        emit databaseError(query.lastError().text());
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool DatabaseManager::deleteRecord(const QString& tableName, qint64 id)
{
    if (!m_db.isOpen() || tableName.isEmpty() || id <= 0) {
        return false;
    }

    QString sql = QString("DELETE FROM %1 WHERE id = ?").arg(tableName);
    QSqlQuery query(m_db);
    query.prepare(sql);
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to delete record from" << tableName << ":" << query.lastError().text();
        emit databaseError(query.lastError().text());
        return false;
    }

    return query.numRowsAffected() > 0;
}

QHash<QString, QVariant> DatabaseManager::readRecord(const QString& tableName, qint64 id) const
{
    QHash<QString, QVariant> result;

    if (!m_db.isOpen() || tableName.isEmpty() || id <= 0) {
        return result;
    }

    QString sql = QString("SELECT * FROM %1 WHERE id = ?").arg(tableName);
    QSqlQuery query(m_db);
    query.prepare(sql);
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        QSqlRecord record = query.record();
        for (int i = 0; i < record.count(); ++i) {
            result[record.fieldName(i)] = query.value(i);
        }
    }

    return result;
}

QList<qint64> DatabaseManager::findRecords(const QString& tableName, const QString& field,
                                           const QVariant& value) const
{
    QList<qint64> result;

    if (!m_db.isOpen() || tableName.isEmpty()) {
        return result;
    }

    QString sql;
    if (field.isEmpty()) {
        sql = QString("SELECT id FROM %1").arg(tableName);
    } else {
        sql = QString("SELECT id FROM %1 WHERE %2 = ?").arg(tableName).arg(field);
    }

    QSqlQuery query(m_db);
    query.prepare(sql);

    if (!field.isEmpty()) {
        query.addBindValue(value);
    }

    if (query.exec()) {
        while (query.next()) {
            result.append(query.value(0).toLongLong());
        }
    } else {
        qWarning() << "Failed to find records:" << query.lastError().text();
    }

    return result;
}

bool DatabaseManager::beginTransaction()
{
    if (!m_db.isOpen()) {
        return false;
    }

    return m_db.transaction();
}

bool DatabaseManager::commitTransaction()
{
    if (!m_db.isOpen()) {
        return false;
    }

    return m_db.commit();
}

bool DatabaseManager::rollbackTransaction()
{
    if (!m_db.isOpen()) {
        return false;
    }

    return m_db.rollback();
}

QStringList DatabaseManager::tableNames() const
{
    if (!m_db.isOpen()) {
        return QStringList();
    }

    return m_db.tables(QSql::Tables);
}

QStringList DatabaseManager::fieldNames(const QString& tableName) const
{
    QStringList names;

    if (!m_db.isOpen() || tableName.isEmpty()) {
        return names;
    }

    QSqlRecord record = m_db.record(tableName);
    for (int i = 0; i < record.count(); ++i) {
        names.append(record.fieldName(i));
    }

    return names;
}

int DatabaseManager::recordCount(const QString& tableName) const
{
    if (!m_db.isOpen() || tableName.isEmpty()) {
        return 0;
    }

    QString sql = QString("SELECT COUNT(*) FROM %1").arg(tableName);
    QSqlQuery query(m_db);

    if (query.exec(sql) && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

bool DatabaseManager::createCompetition(const QString& name, const QDate& date)
{
    if (!m_db.isOpen()) {
        return false;
    }

    QHash<QString, QVariant> data;
    data["name"] = name;
    data["date"] = date.toString(Qt::ISODate);

    qint64 id = -1;
    if (createRecord("competitions", data, &id)) {
        m_currentCompetitionId = id;
        return true;
    }

    return false;
}

qint64 DatabaseManager::currentCompetitionId() const
{
    return m_currentCompetitionId;
}

void DatabaseManager::setCurrentCompetitionId(qint64 id)
{
    m_currentCompetitionId = id;
}

bool DatabaseManager::tableExists(const QString& tableName) const
{
    if (!m_db.isOpen() || tableName.isEmpty()) {
        return false;
    }

    QString sql = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'")
                      .arg(tableName);

    QSqlQuery query(m_db);
    if (query.exec(sql)) {
        return query.next();
    }

    return false;
}

bool DatabaseManager::dropTable(const QString& tableName)
{
    if (!m_db.isOpen() || tableName.isEmpty()) {
        return false;
    }

    QString sql = QString("DROP TABLE IF EXISTS %1").arg(tableName);
    QSqlQuery query(m_db);

    return query.exec(sql);
}

void DatabaseManager::vacuumDatabase()
{
    if (!m_db.isOpen()) {
        return;
    }

    QSqlQuery query(m_db);
    query.exec("VACUUM");
}

bool DatabaseManager::backupDatabase(const QString& backupPath)
{
    if (!m_db.isOpen()) {
        return false;
    }

    // Закрываем текущее соединение
    QString originalPath = m_db.databaseName();
    m_db.close();

    // Копируем файл
    bool success = QFile::copy(originalPath, backupPath);

    // Открываем заново
    m_db.open();

    return success;
}

bool DatabaseManager::executeQuery(const QString& sql, const QHash<QString, QVariant>& params)
{
    if (!m_db.isOpen() || sql.isEmpty()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(sql);

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.bindValue(":" + it.key(), it.value());
    }

    if (!query.exec()) {
        qWarning() << "Failed to execute query:" << query.lastError().text();
        emit databaseError(query.lastError().text());
        return false;
    }

    return true;
}

QList<QHash<QString, QVariant>> DatabaseManager::executeSelect(const QString& sql, const QHash<QString, QVariant>& params)
{
    QList<QHash<QString, QVariant>> result;

    if (!m_db.isOpen() || sql.isEmpty()) {
        return result;
    }

    QSqlQuery query(m_db);
    query.prepare(sql);

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.bindValue(":" + it.key(), it.value());
    }

    if (query.exec()) {
        while (query.next()) {
            QHash<QString, QVariant> record;
            QSqlRecord sqlRecord = query.record();
            for (int i = 0; i < sqlRecord.count(); ++i) {
                record[sqlRecord.fieldName(i)] = query.value(i);
            }
            result.append(record);
        }
    } else {
        qWarning() << "Failed to execute select:" << query.lastError().text();
    }

    return result;
}

void DatabaseManager::reset()
{
    cleanup(); // Закрыть базу, очистить кэши и т.д.
    m_currentCompetitionId = -1;
    m_db = QSqlDatabase(); // Сброс объекта базы данных
}

void DatabaseManager::cleanup()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

