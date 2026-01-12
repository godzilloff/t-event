#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include <QUndoStack>
#include <QUndoCommand>
#include <QHash>
#include <QVariant>
#include <QList>

class DatabaseCommand : public QUndoCommand
{
public:
    enum CommandType {
        Insert,
        Update,
        Delete
    };
    
    DatabaseCommand(CommandType type, const QString& tableName, 
                   qint64 id, const QHash<QString, QVariant>& newData = {},
                   const QHash<QString, QVariant>& oldData = {},
                   QUndoCommand* parent = nullptr);
    
    void undo() override;
    void redo() override;
    
private:
    CommandType m_type;
    QString m_tableName;
    qint64 m_id;
    QHash<QString, QVariant> m_newData;
    QHash<QString, QVariant> m_oldData;
    bool m_firstRedo = true;
};

class BatchImportCommand : public QUndoCommand
{
public:
    BatchImportCommand(const QString& tableName,
                       const QList<qint64>& importedIds,
                       const QList<QHash<QString, QVariant>>& importedData,
                       QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QString m_tableName;
    QList<qint64> m_importedIds;
    QList<QHash<QString, QVariant>> m_importedData;
    bool m_firstRedo = true;
};

class UndoStack : public QUndoStack
{
    Q_OBJECT
public:
    static UndoStack& instance();
    
    void pushInsertCommand(const QString& tableName, qint64 id, 
                          const QHash<QString, QVariant>& data);
    void pushUpdateCommand(const QString& tableName, qint64 id,
                          const QHash<QString, QVariant>& oldData,
                          const QHash<QString, QVariant>& newData);
    void pushDeleteCommand(const QString& tableName, qint64 id,
                          const QHash<QString, QVariant>& data);

    void pushBatchImportCommand(const QString& tableName,
                                const QList<qint64>& importedIds,
                                const QList<QHash<QString, QVariant>>& importedData);
    
    bool canUndo() const { return QUndoStack::canUndo(); }
    bool canRedo() const { return QUndoStack::canRedo(); }
    
signals:
    void dataChanged(const QString& tableName, qint64 id);
    
private:
    explicit UndoStack(QObject* parent = nullptr);
    ~UndoStack() = default;
    
    Q_DISABLE_COPY(UndoStack)
};

#endif // UNDOSTACK_H
