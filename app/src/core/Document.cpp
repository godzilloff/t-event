#include "Document.h"
#include "DatabaseManager.h"
#include "UndoStack.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QStandardPaths>

Document::Document(QObject* parent) : QObject(parent) {}

Document::~Document()
{
    close();
}

bool Document::createNew(const QString& filePath)
{
    if (isOpen()) {
        close();
    }

    QString actualPath = filePath;

    // Если путь не указан, генерируем имя по умолчанию
    if (actualPath.isEmpty()) {
        QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QDir dir(defaultDir);
        actualPath = dir.filePath("Новое соревнование.tevent");
    }

    // Проверяем расширение
    if (!actualPath.endsWith(".tevent") && !actualPath.endsWith(".db")) {
        actualPath += ".tevent";
    }

    // Проверяем, существует ли файл
    if (QFile::exists(actualPath)) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(
            nullptr, // или укажите родительский QWidget, если вызываете из виджета
            QObject::tr("Файл существует"),
            QObject::tr("Файл уже существует:\n%1\n\nПерезаписать?").arg(actualPath),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );

        if (reply == QMessageBox::No) {
            qWarning() << "Пользователь отказался перезаписывать файл:" << actualPath;
            return false;
        }
        // Если Yes — продолжаем выполнение, файл будет перезаписан

        // // Можно запросить подтверждение на перезапись
        // // Пока просто возвращаем ошибку
        // qWarning() << "File already exists:" << actualPath;
        // return false;
    }

    // Создаем новую базу данных
    if (!setupNewDatabase(actualPath)) {
        return false;
    }

    m_filePath = actualPath;
    m_isOpen = true;
    m_isModified = false;

    loadCompetitionInfo();

    emit documentCreated(actualPath);
    emit documentOpened(actualPath);

    return true;
}

bool Document::open(const QString& filePath)
{
    if (isOpen()) {
        close();
    }

    if (!QFile::exists(filePath)) {
        qWarning() << "File does not exist:" << filePath;
        return false;
    }

    // Открываем базу данных
    if (!DatabaseManager::instance().openDatabase(filePath)) {
        return false;
    }

    m_filePath = filePath;
    m_isOpen = true;
    m_isModified = false;

    // Загружаем информацию о соревновании
    if (!loadCompetitionInfo()) {
        qWarning() << "Failed to load competition info from:" << filePath;
        DatabaseManager::instance().closeDatabase();
        m_isOpen = false;
        return false;
    }

    emit documentOpened(filePath);
    return true;
}

bool Document::save()
{
    if (!isOpen()) {
        return false;
    }

    if (m_filePath.isEmpty()) {
        qWarning() << "Не задан путь для сохранения файла!";
        return false;
    }

    // Для SQLite сохранение происходит автоматически
    // Просто помечаем как сохраненное
    m_isModified = false;
    emit documentSaved(m_filePath);
    emit documentModified(false);

    return true;
}

bool Document::saveAs(const QString& filePath)
{
    if (!isOpen()) {
        return false;
    }

    QString actualPath = filePath;
    if (actualPath.isEmpty()) {
        QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QFileInfo info(m_filePath);
        actualPath = QDir(defaultDir).filePath(info.fileName());
    }

    // Проверяем расширение
    if (!actualPath.endsWith(".tevent") && !actualPath.endsWith(".db")) {
        actualPath += ".tevent";
    }

    // Если это другой файл, создаем резервную копию
    if (actualPath != m_filePath && QFile::exists(actualPath)) {
        // Запрашиваем подтверждение перезаписи
        // Пока просто перезаписываем
        QFile::remove(actualPath);
    }

    // Копируем текущую базу данных в новый файл
    QString currentPath = DatabaseManager::instance().databasePath();
    if (currentPath != actualPath) {
        DatabaseManager::instance().closeDatabase();

        if (!QFile::copy(currentPath, actualPath)) {
            // Если копирование не удалось, открываем старую базу
            DatabaseManager::instance().openDatabase(currentPath);
            return false;
        }

        // Открываем новую базу
        if (!DatabaseManager::instance().openDatabase(actualPath)) {
            // Если не удалось открыть новую, пробуем открыть старую
            DatabaseManager::instance().openDatabase(currentPath);
            return false;
        }
    }

    m_filePath = actualPath;
    m_isModified = false;

    emit documentSaved(actualPath);
    emit documentModified(false);

    return true;
}

bool Document::close()
{
    if (!isOpen()) {
        return true;
    }

    // Проверяем на несохраненные изменения
    if (m_isModified) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(
            nullptr, // или укажите родительский QWidget, если вызываете из виджета
            QObject::tr("Сохранение документа"),
            QObject::tr("Документ изменён:\n%1\n\nСохранить изменение на диск?").arg(m_filePath),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );

        if (reply == QMessageBox::No) {
            qWarning() << "Пользователь отказался перезаписывать файл:" << m_filePath;
            return false;
        } else save();
    }

    DatabaseManager::instance().closeDatabase();
    UndoStack::instance().clear();

    m_isOpen = false;
    m_isModified = false;
    m_originalData.clear();
    m_competitionId = -1;
    m_competitionName.clear();

    //emit documentClosed();
    return true;
}

bool Document::setupNewDatabase(const QString& filePath)
{
    // Открываем новую базу данных
    if (!DatabaseManager::instance().openDatabase(filePath)) {
        return false;
    }

    // База уже создана в DatabaseManager::openDatabase()
    // Теперь создаем первое соревнование

    return true;
}

bool Document::loadCompetitionInfo()
{
    auto& db = DatabaseManager::instance();

    // Получаем список соревнований
    QList<qint64> competitions = db.findRecords("competitions");
    if (competitions.isEmpty()) {
        // Создаем новое соревнование по умолчанию
        QFileInfo info(m_filePath);
        QString defaultName = info.baseName();
        if (!createCompetition(defaultName, QDate::currentDate())) {
            return false;
        }
    } else {
        // Используем первое соревнование
        m_competitionId = competitions.first();
        auto compData = db.readRecord("competitions", m_competitionId);
        m_competitionName = compData["name"].toString();
        m_competitionDate = QDate::fromString(compData["date"].toString(), Qt::ISODate);
        db.setCurrentCompetitionId(m_competitionId);
    }

    emit competitionChanged(m_competitionId);
    return true;
}

qint64 Document::insertRecord(const QString& tableName, const QHash<QString, QVariant>& data)
{
    if (!isOpen()) {
        return -1;
    }

    // Добавляем competition_id если его нет
    QHash<QString, QVariant> recordData = data;
    if (!recordData.contains("competition_id") && tableName != "competitions") {
        recordData["competition_id"] = m_competitionId;
    }

    auto& db = DatabaseManager::instance();
    qint64 id = -1;

    if (db.createRecord(tableName, recordData, &id)) {
        // Сохраняем оригинальные данные для undo
        m_originalData[tableName][id] = recordData;

        // Создаем команду undo/redo
        UndoStack::instance().pushInsertCommand(tableName, id, recordData);

        m_isModified = true;
        emit documentModified(true);
        emit recordInserted(tableName, id);

        return id;
    }

    return -1;
}

bool Document::updateRecord(const QString& tableName, qint64 id, const QHash<QString, QVariant>& data)
{
    if (!isOpen() || id <= 0) {
        return false;
    }

    auto& db = DatabaseManager::instance();

    // Получаем старые данные
    QHash<QString, QVariant> oldData;
    if (m_originalData[tableName].contains(id)) {
        oldData = m_originalData[tableName][id];
    } else {
        oldData = db.readRecord(tableName, id);
        m_originalData[tableName][id] = oldData;
    }

    // Обновляем запись
    if (db.updateRecord(tableName, id, data)) {
        // Обновляем кэш
        QHash<QString, QVariant> newData = oldData;
        for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
            newData[it.key()] = it.value();
        }
        m_originalData[tableName][id] = newData;

        // Создаем команду undo/redo
        UndoStack::instance().pushUpdateCommand(tableName, id, oldData, newData);

        m_isModified = true;
        emit documentModified(true);
        emit recordUpdated(tableName, id);

        return true;
    }

    return false;
}

bool Document::deleteRecord(const QString& tableName, qint64 id)
{
    if (!isOpen() || id <= 0) {
        return false;
    }

    auto& db = DatabaseManager::instance();

    // Получаем старые данные
    QHash<QString, QVariant> oldData;
    if (m_originalData[tableName].contains(id)) {
        oldData = m_originalData[tableName][id];
    } else {
        oldData = db.readRecord(tableName, id);
    }

    // Удаляем запись
    if (db.deleteRecord(tableName, id)) {
        // Удаляем из кэша
        m_originalData[tableName].remove(id);

        // Создаем команду undo/redo
        UndoStack::instance().pushDeleteCommand(tableName, id, oldData);

        m_isModified = true;
        emit documentModified(true);
        emit recordDeleted(tableName, id);

        return true;
    }

    return false;
}

QHash<QString, QVariant> Document::getRecord(const QString& tableName, qint64 id) const
{
    qDebug() << "Document::getRecord: таблица =" << tableName << ", ID =" << id;

    if (!isOpen() || id <= 0) {
        qWarning() << "Document не открыт или невалидный ID";
        return {};
    }

    auto data = DatabaseManager::instance().readRecord(tableName, id);

    qDebug() << "Document::getRecord: получено" << data.size() << "полей";
    if (!data.isEmpty()) {
        qDebug() << "Пример данных:" << data.value("full_name").toString();
    }

    return data;
}

int Document::recordCount(const QString& tableName) const
{
    if (!isOpen()) {
        return 0;
    }

    return DatabaseManager::instance().recordCount(tableName);
}

QList<qint64> Document::getAllRecordIds(const QString& tableName) const
{
    if (!isOpen()) {
        return QList<qint64>();
    }

    return DatabaseManager::instance().findRecords(tableName, "competition_id", m_competitionId);
}

bool Document::setCurrentCompetition(qint64 id)
{
    if (!isOpen() || id <= 0) {
        return false;
    }

    auto& db = DatabaseManager::instance();

    // Проверяем, существует ли соревнование
    auto compData = db.readRecord("competitions", id);
    if (compData.isEmpty()) {
        return false;
    }

    m_competitionId = id;
    m_competitionName = compData["name"].toString();
    m_competitionDate = QDate::fromString(compData["date"].toString(), Qt::ISODate);
    db.setCurrentCompetitionId(id);

    emit competitionChanged(id);
    return true;
}

bool Document::createCompetition(const QString& name, const QDate& date)
{
    if (!isOpen()) {
        return false;
    }

    auto& db = DatabaseManager::instance();

    if (!db.createCompetition(name, date)) {
        return false;
    }

    m_competitionId = db.currentCompetitionId();
    m_competitionName = name;
    m_competitionDate = date;

    m_isModified = true;
    emit documentModified(true);
    emit competitionChanged(m_competitionId);

    return true;
}

QString Document::fileName() const
{
    if (m_filePath.isEmpty()) {
        return QString();
    }

    QFileInfo info(m_filePath);
    return info.fileName();
}

QString Document::databasePath() const
{
    return DatabaseManager::instance().databasePath();
}

void Document::setModified(bool modified)
{
    if (m_isModified != modified) {
        m_isModified = modified;
        emit documentModified(modified);
    }
}

bool Document::importFromCsv(const QString& filePath, const QString& tableName)
{
    // TODO: Реализация импорта из CSV
    Q_UNUSED(filePath);
    Q_UNUSED(tableName);
    return false;
}

bool Document::exportToCsv(const QString& filePath, const QString& tableName)
{
    // TODO: Реализация экспорта в CSV
    Q_UNUSED(filePath);
    Q_UNUSED(tableName);
    return false;
}
