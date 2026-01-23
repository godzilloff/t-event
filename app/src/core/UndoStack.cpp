#include "UndoStack.h"
#include "DatabaseManager.h"
#include <QDebug>

DatabaseCommand::DatabaseCommand(CommandType type, const QString& tableName,
                               qint64 id, const QHash<QString, QVariant>& newData,
                               const QHash<QString, QVariant>& oldData,
                               QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_type(type)
    , m_tableName(tableName)
    , m_id(id)
    , m_newData(newData)
    , m_oldData(oldData)
{
    if (m_type == Insert && !m_newData.contains("id")) {
        m_newData["id"] = m_id; // фиксируем id для redo
    }

    switch (type) {
    case Insert:
        if (tableName == "participants") setText(QObject::tr("Добавлен(а) %1").arg(newData["full_name"].toString()));
        else setText(QObject::tr("Insert into %1").arg(tableName));
        break;
    case Update:
        if (tableName == "participants") setText(QObject::tr("Обновлён(а) %1").arg(newData["full_name"].toString()));
        else setText(QObject::tr("Update %1").arg(tableName));
        break;
    case Delete:
        if (tableName == "participants") setText(QObject::tr("Удален(а) %1").arg(oldData["full_name"].toString()));
        else setText(QObject::tr("Delete from %1").arg(tableName));
        break;
    }
}

void DatabaseCommand::undo()
{
    auto& db = DatabaseManager::instance();
    
    switch (m_type) {
    case Insert:
        // Отмена вставки = удаление
        db.deleteRecord(m_tableName, m_id);
        break;
    case Update:
        // Отмена обновления = восстановление старых данных
        db.updateRecord(m_tableName, m_id, m_oldData);
        break;
    case Delete:
        // Отмена удаления = вставка старых данных
        db.createRecord(m_tableName, m_oldData);
        break;
    }
}

void DatabaseCommand::redo()
{
    // При первом выполнении redo команда уже была выполнена
    if (m_firstRedo) {
        m_firstRedo = false;
        return;
    }
    
    auto& db = DatabaseManager::instance();
    
    switch (m_type) {
    case Insert:
        db.createRecord(m_tableName, m_newData);
        break;
    case Update:
        db.updateRecord(m_tableName, m_id, m_newData);
        break;
    case Delete:
        db.deleteRecord(m_tableName, m_id);
        break;
    }
}

UndoStack& UndoStack::instance()
{
    static UndoStack instance;
    return instance;
}

UndoStack::UndoStack(QObject* parent) : QUndoStack(parent) {}

void UndoStack::pushInsertCommand(const QString& tableName, qint64 id,
                                 const QHash<QString, QVariant>& data)
{
    push(new DatabaseCommand(DatabaseCommand::Insert, tableName, id, data));
}

void UndoStack::pushUpdateCommand(const QString& tableName, qint64 id,
                                 const QHash<QString, QVariant>& oldData,
                                 const QHash<QString, QVariant>& newData)
{
    push(new DatabaseCommand(DatabaseCommand::Update, tableName, id, newData, oldData));
}

void UndoStack::pushDeleteCommand(const QString& tableName, qint64 id,
                                 const QHash<QString, QVariant>& data)
{
    push(new DatabaseCommand(DatabaseCommand::Delete, tableName, id, {}, data));
}

// ===================

BatchImportCommand::BatchImportCommand(const QString& tableName,
                                       const QList<qint64>& importedIds,
                                       const QList<QHash<QString, QVariant>>& importedData,
                                       QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_tableName(tableName)
    , m_importedIds(importedIds)
    , m_importedData(importedData)
{
    setText(QObject::tr("Импорт %1 записей").arg(importedIds.size()));
}

void BatchImportCommand::undo()
{
    auto& db = DatabaseManager::instance();

    // Удаляем импортированные записи
    for (qint64 id : m_importedIds) {
        db.deleteRecord(m_tableName, id);
    }
}

void BatchImportCommand::redo()
{
    // При первом выполнении redo команда уже была выполнена
    if (m_firstRedo) {
        m_firstRedo = false;
        return;
    }

    auto& db = DatabaseManager::instance();

    // Восстанавливаем удаленные записи
    for (int i = 0; i < m_importedIds.size(); ++i) {
        auto data = m_importedData[i];
        data["id"] = m_importedIds[i];

        qint64 newId = -1;
        bool ok = db.createRecord(m_tableName, data, &newId);
        if (!ok) {
            qDebug() << "  Restoring record with id =" << m_importedIds[i] << ", data keys:" << data.keys();
            qWarning() << "Failed to restore record with id" << m_importedIds[i];
        } else if (newId != m_importedIds[i]) {
            qWarning() << "ID mismatch! Expected:" << m_importedIds[i] << "Got:" << newId;
        }
    }
}


void UndoStack::pushBatchImportCommand(const QString& tableName,
                                       const QList<qint64>& importedIds,
                                       const QList<QHash<QString, QVariant>>& importedData)
{
    push(new BatchImportCommand(tableName, importedIds, importedData));
}
