#include "AgeGroupEditDialog.h"
#include "Document.h"
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>

AgeGroupEditDialog::AgeGroupEditDialog(Document* document, qint64 recordId, QWidget* parent)
    : EditDialogBase(document, "age_groups", recordId, parent)
{
    setupUi();

    if (!isNewRecord()) {
        loadData();
    }
}

AgeGroupEditDialog::~AgeGroupEditDialog()
{
}

void AgeGroupEditDialog::setupUi()
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
    setMinimumSize(450, 300);
}

void AgeGroupEditDialog::createFormLayout()
{
    // Групповая рамка для формы
    m_groupBox = new QGroupBox(tr("Информация о возрастной группе"), this);
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
    
    // Название группы
    m_labelName = new QLabel(tr("Название группы:"));
    m_editName = new QLineEdit();
    m_editName->setPlaceholderText(tr("Например: М18-20, Ж21-35"));
    m_editName->setMinimumWidth(200);
    m_formLayout->addRow(m_labelName, m_editName);
    
    // Минимальный возраст
    m_labelMinAge = new QLabel(tr("Минимальный возраст:"));
    m_spinMinAge = new QSpinBox();
    m_spinMinAge->setMinimum(0);
    m_spinMinAge->setMaximum(150);
    m_spinMinAge->setSuffix(" лет");
    m_spinMinAge->setSpecialValueText(tr("Не ограничено"));
    m_spinMinAge->setMaximumWidth(120);
    m_formLayout->addRow(m_labelMinAge, m_spinMinAge);
    
    // Максимальный возраст
    m_labelMaxAge = new QLabel(tr("Максимальный возраст:"));
    m_spinMaxAge = new QSpinBox();
    m_spinMaxAge->setMinimum(0);
    m_spinMaxAge->setMaximum(150);
    m_spinMaxAge->setSuffix(" лет");
    m_spinMaxAge->setSpecialValueText(tr("Не ограничено"));
    m_spinMaxAge->setMaximumWidth(120);
    m_formLayout->addRow(m_labelMaxAge, m_spinMaxAge);
    
    // Стоимость
    m_labelPrice = new QLabel(tr("Стоимость участия:"));
    m_spinPrice = new QDoubleSpinBox();
    m_spinPrice->setMinimum(0.0);
    m_spinPrice->setMaximum(100000.0);
    m_spinPrice->setPrefix("₽ ");
    m_spinPrice->setDecimals(2);
    m_spinPrice->setMaximumWidth(150);
    m_formLayout->addRow(m_labelPrice, m_spinPrice);
    
    groupLayout->addLayout(m_formLayout);
    groupLayout->addStretch();
}

void AgeGroupEditDialog::loadData()
{
    EditDialogBase::loadData();
    
    if (m_originalData.isEmpty()) {
        return;
    }
    
    // Заполняем форму данными
    m_editName->setText(m_originalData.value("name").toString());
    m_spinMinAge->setValue(m_originalData.value("min_age", 0).toInt());
    m_spinMaxAge->setValue(m_originalData.value("max_age", 0).toInt());
    m_spinPrice->setValue(m_originalData.value("price", 0.0).toDouble());
}

bool AgeGroupEditDialog::validateForm()
{
    // Проверка обязательных полей
    if (m_editName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), 
            tr("Введите название возрастной группы"));
        m_editName->setFocus();
        return false;
    }
    
    // Проверка корректности возрастных границ
    int minAge = m_spinMinAge->value();
    int maxAge = m_spinMaxAge->value();
    
    if (minAge > 0 && maxAge > 0 && minAge > maxAge) {
        QMessageBox::warning(this, tr("Ошибка"), 
            tr("Минимальный возраст не может быть больше максимального"));
        m_spinMinAge->setFocus();
        return false;
    }
    
    return true;
}

QHash<QString, QVariant> AgeGroupEditDialog::collectFormData()
{
    QHash<QString, QVariant> formdata;
    
    formdata["name"] = m_editName->text().trimmed();
    formdata["min_age"] = (m_spinMinAge->value() > 0) ? m_spinMinAge->value() : QVariant();
    formdata["max_age"] = (m_spinMaxAge->value() > 0) ? m_spinMaxAge->value() : QVariant();
    formdata["price"] = m_spinPrice->value();
    
    return formdata;
}
