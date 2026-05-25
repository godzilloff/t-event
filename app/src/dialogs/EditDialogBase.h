#ifndef EDITDIALOGBASE_H
#define EDITDIALOGBASE_H

#include <QDialog>
#include <QHash>
#include <QVariant>
#include <QDialogButtonBox>

class Document;

class EditDialogBase : public QDialog
{
    Q_OBJECT

public:
    explicit EditDialogBase(Document* document, const QString& tableName,
                            qint64 recordId = -1, QWidget* parent = nullptr);
    virtual ~EditDialogBase();

    qint64 recordId() const { return m_recordId; }
    QString tableName() const { return m_tableName; }
    bool isNewRecord() const { return m_recordId <= 0; }

    bool saveRecord(); // Добавили публичный метод для сохранения

signals:
    // Новые сигналы для уведомления о сохранении
    void recordSaved(qint64 recordId, const QString& tableName);
    void recordDeleted(qint64 recordId, const QString& tableName);
    void dialogClosed();

    void recordUpdated(qint64 recordId, const QString& tableName);

public slots:
    virtual void accept() override;
    virtual void reject() override;

protected:
    virtual void setupUi() = 0;
    virtual void loadData();
    virtual bool validateForm() = 0;
    virtual QHash<QString, QVariant> collectFormData() = 0;

    Document* m_document = nullptr;
    QString m_tableName;
    qint64 m_recordId = -1;
    QHash<QString, QVariant> m_originalData;

    void setDialogTitle(const QString& title);
    void setupButtonBox(QDialogButtonBox* buttonBox);
    void addDeleteButton(QDialogButtonBox* buttonBox); // Кнопка удаления

protected slots:
    virtual void onDeleteClicked(); // Обработчик удаления

private:
    void setupConnections(QDialogButtonBox* buttonBox);
    bool performSave(); // Внутренний метод сохранения
};

#endif // EDITDIALOGBASE_H
