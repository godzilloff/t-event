#include "EditDialogBase.h"
#include "Document.h"
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>

#include "mainwindow.h"

EditDialogBase::EditDialogBase(Document* document, const QString& tableName, qint64 recordId, QWidget* parent)
    : QDialog(parent)
    , m_document(document)
    , m_tableName(tableName)
    , m_recordId(recordId)
{
    // Устанавливаем заголовок окна
    setDialogTitle(isNewRecord() ?
                       tr("Добавить запись") : tr("Редактировать запись"));
}

EditDialogBase::~EditDialogBase()
{
    emit dialogClosed();
}

void EditDialogBase::setDialogTitle(const QString& title)
{
    setWindowTitle(title);
}

void EditDialogBase::setupButtonBox(QDialogButtonBox* buttonBox)
{
    buttonBox->setStandardButtons(QDialogButtonBox::Ok |
                                  QDialogButtonBox::Cancel |
                                  QDialogButtonBox::Apply);

    QPushButton* applyButton = buttonBox->button(QDialogButtonBox::Apply);
    applyButton->setText(tr("Применить"));

    // Добавляем кнопку удаления для существующих записей
    if (!isNewRecord()) {
        addDeleteButton(buttonBox);
    }

    setupConnections(buttonBox);
}

void EditDialogBase::addDeleteButton(QDialogButtonBox* buttonBox)
{
    QPushButton* deleteButton = new QPushButton(tr("Удалить"));
    deleteButton->setObjectName("deleteButton");
    deleteButton->setStyleSheet("QPushButton { color: red; }");
    buttonBox->addButton(deleteButton, QDialogButtonBox::ActionRole);

    connect(deleteButton, &QPushButton::clicked, this, &EditDialogBase::onDeleteClicked);
}

void EditDialogBase::setupConnections(QDialogButtonBox* buttonBox)
{
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditDialogBase::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &EditDialogBase::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &EditDialogBase::accept);
}

// void EditDialogBase::loadData()
// {
//     if (m_recordId <= 0) {
//         return; // Новая запись, нечего загружать
//     }

//     Document* doc = qobject_cast<Document*>(parent());
//     if (!doc) {
//         qWarning() << "EditDialogBase: Document не найден в parent()";
//         return;
//     }

//     m_originalData = doc->getRecord(m_tableName, m_recordId);

//     qDebug() << "EditDialogBase: Загружены данные для таблицы" << m_tableName
//              << "ID" << m_recordId << "количество полей:" << m_originalData.size();

//     if (m_originalData.isEmpty()) {
//         qWarning() << "EditDialogBase: Пустые данные для записи ID:" << m_recordId;
//     }
// }

void EditDialogBase::loadData()
{
    if (m_recordId <= 0 || !m_document) {
        qDebug() << "EditDialogBase: Нет ID записи или Document";
        return;
    }

    m_originalData = m_document->getRecord(m_tableName, m_recordId);

    qDebug() << "EditDialogBase: Загружены данные для таблицы" << m_tableName
             << "ID" << m_recordId << "количество полей:" << m_originalData.size();

    if (m_originalData.isEmpty()) {
        qWarning() << "EditDialogBase: Пустые данные для записи ID:" << m_recordId;
    }
}

bool EditDialogBase::performSave()
{
    if (!validateForm()) {
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Пожалуйста, заполните все обязательные поля правильно."));
        return false;
    }

    QHash<QString, QVariant> formData = collectFormData();

    // Используем m_document напрямую
    if (!m_document) {
        qWarning() << "EditDialogBase: Document is null!";
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось получить доступ к документу."));
        return false;
    }

    bool success = false;

    if (isNewRecord()) {
        qint64 newId = m_document->insertRecord(m_tableName, formData);
        success = (newId > 0);
        if (success) {
            m_recordId = newId;
        }
    } else {
        success = m_document->updateRecord(m_tableName, m_recordId, formData);
    }

    return success;
}

bool EditDialogBase::saveRecord()
{
    bool success = performSave();
    if (success) {
        emit recordSaved(m_recordId, m_tableName);
    }
    return success;
}

void EditDialogBase::accept()
{
    if (saveRecord()) {
        QDialog::accept();
    } else {
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось сохранить данные в базу."));
    }
}

void EditDialogBase::reject()
{
    QDialog::reject();
}

void EditDialogBase::onDeleteClicked()
{
    if (isNewRecord() || !m_document) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Подтверждение удаления"),
        tr("Вы уверены, что хотите удалить эту запись?\n"
           "Это действие нельзя отменить."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        bool success = m_document->deleteRecord(m_tableName, m_recordId);
        if (success) {
            emit recordDeleted(m_recordId, m_tableName);
            QDialog::accept();
        } else {
            QMessageBox::critical(this, tr("Ошибка"),
                                  tr("Не удалось удалить запись."));
        }
    }
}

