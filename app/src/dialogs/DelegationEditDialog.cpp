#include "DelegationEditDialog.h"
#include "Document.h"
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QIntValidator>
#include <QRegularExpressionValidator>
#include <QRegularExpression>

DelegationEditDialog::DelegationEditDialog(Document* document, qint64 recordId, QWidget* parent)
    : EditDialogBase(document, "delegations", recordId, parent)
{
    setupUi();
}

DelegationEditDialog::~DelegationEditDialog()
{
}

void DelegationEditDialog::setupUi()
{
    // Основной layout для всего диалога
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // Создаем форму
    createFormLayout();
    
    // Кнопки
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    setupButtonBox(buttonBox);
    m_mainLayout->addWidget(buttonBox);
    
    // Устанавливаем минимальный размер
    setMinimumSize(500, 350);
}

void DelegationEditDialog::createFormLayout()
{
    // Групповая рамка для формы
    m_groupBox = new QGroupBox(tr("Информация о делегации"), this);
    m_mainLayout->addWidget(m_groupBox);
    
    // Layout для группы
    QVBoxLayout* groupLayout = new QVBoxLayout(m_groupBox);
    groupLayout->setContentsMargins(10, 15, 10, 15);
    
    // Form layout для полей
    m_formLayout = new QFormLayout();
    m_formLayout->setContentsMargins(10, 10, 10, 10);
    m_formLayout->setSpacing(15);
    m_formLayout->setLabelAlignment(Qt::AlignRight);
    m_formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    
    // Название делегации
    m_labelName = new QLabel(tr("Название делегации:"));
    m_editName = new QLineEdit();
    m_editName->setPlaceholderText(tr("Введите название делегации"));
    m_editName->setMinimumWidth(250);
    m_formLayout->addRow(m_labelName, m_editName);
    
    // Представитель
    m_labelRepresentative = new QLabel(tr("Представитель:"));
    m_editRepresentative = new QLineEdit();
    m_editRepresentative->setPlaceholderText(tr("ФИО представителя"));
    m_editRepresentative->setMinimumWidth(250);
    m_formLayout->addRow(m_labelRepresentative, m_editRepresentative);
    
    // Контактная информация
    m_labelContact = new QLabel(tr("Контактная информация:"));
    m_editContact = new QTextEdit();
    m_editContact->setPlaceholderText(tr("Телефон, email, адрес..."));
    m_editContact->setMaximumHeight(100);
    m_editContact->setAcceptRichText(false);
    
    // Добавляем текстовое поле с меткой
    QVBoxLayout* contactLayout = new QVBoxLayout();
    contactLayout->addWidget(m_editContact);
    contactLayout->setContentsMargins(0, 0, 0, 0);
    
    m_formLayout->addRow(m_labelContact, contactLayout);
    
    groupLayout->addLayout(m_formLayout);
    groupLayout->addStretch();
}

void DelegationEditDialog::loadData()
{
    EditDialogBase::loadData();
    
    if (m_originalData.isEmpty()) {
        return;
    }
    
    // Заполняем форму данными
    m_editName->setText(m_originalData.value("name").toString());
    m_editRepresentative->setText(m_originalData.value("representative").toString());
    m_editContact->setPlainText(m_originalData.value("contact").toString());
}

bool DelegationEditDialog::validateForm()
{
    // Проверка обязательных полей
    if (m_editName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), 
            tr("Введите название делегации"));
        m_editName->setFocus();
        return false;
    }
    
    // Можно добавить дополнительные проверки
    QString name = m_editName->text().trimmed();
    if (name.length() < 2) {
        QMessageBox::warning(this, tr("Ошибка"), 
            tr("Название делегации должно содержать не менее 2 символов"));
        m_editName->setFocus();
        return false;
    }
    
    return true;
}

QHash<QString, QVariant> DelegationEditDialog::collectFormData()
{
    QHash<QString, QVariant> fdata;
    
    fdata["name"] = m_editName->text().trimmed();
    fdata["representative"] = m_editRepresentative->text().trimmed();
    fdata["contact"] = m_editContact->toPlainText().trimmed();
    
    return fdata;
}
