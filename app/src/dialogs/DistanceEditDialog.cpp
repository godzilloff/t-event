#include "DistanceEditDialog.h"
#include "Document.h"
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>

DistanceEditDialog::DistanceEditDialog(Document* document, qint64 recordId, QWidget* parent)
    : EditDialogBase(document, "distances", recordId, parent)
{
    setupUi();
}

DistanceEditDialog::~DistanceEditDialog()
{
}

void DistanceEditDialog::setupUi()
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
    setMinimumSize(500, 400);
}

void DistanceEditDialog::createFormLayout()
{
    // Групповая рамка для формы
    m_groupBox = new QGroupBox(tr("Информация о дистанции"), this);
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
    
    // Название дистанции
    m_labelName = new QLabel(tr("Название дистанции:"));
    m_editName = new QLineEdit();
    m_editName->setPlaceholderText(tr("Введите название дистанции"));
    m_editName->setMinimumWidth(250);
    m_formLayout->addRow(m_labelName, m_editName);
    
    // Протяженность (км)
    m_labelLength = new QLabel(tr("Протяженность (км):"));
    m_spinLength = new QDoubleSpinBox();
    m_spinLength->setMinimum(0.1);
    m_spinLength->setMaximum(100.0);
    m_spinLength->setSingleStep(0.1);
    m_spinLength->setSuffix(" км");
    m_spinLength->setDecimals(1);
    m_spinLength->setMaximumWidth(120);
    m_formLayout->addRow(m_labelLength, m_spinLength);
    
    // Контрольное время (минуты)
    m_labelControlTime = new QLabel(tr("Контрольное время (мин):"));
    m_spinControlTime = new QSpinBox();
    m_spinControlTime->setMinimum(1);
    m_spinControlTime->setMaximum(1000);
    m_spinControlTime->setSingleStep(5);
    m_spinControlTime->setSuffix(" мин");
    m_spinControlTime->setMaximumWidth(120);
    m_formLayout->addRow(m_labelControlTime, m_spinControlTime);
    
    // Контрольные пункты (JSON)
    m_labelControlPoints = new QLabel(tr("Контрольные пункты:"));
    m_editControlPoints = new QTextEdit();
    m_editControlPoints->setPlaceholderText(
        tr("JSON массив контрольных пунктов\n"
           "Формат: [{\"id\":1,\"name\":\"КП1\",\"order\":1}, ...]\n"
           "Или просто список через запятую: КП1, КП2, КП3"));
    m_editControlPoints->setMaximumHeight(120);
    m_editControlPoints->setAcceptRichText(false);
    
    // Добавляем текстовое поле с меткой
    QVBoxLayout* pointsLayout = new QVBoxLayout();
    pointsLayout->addWidget(m_editControlPoints);
    pointsLayout->setContentsMargins(0, 0, 0, 0);
    
    m_formLayout->addRow(m_labelControlPoints, pointsLayout);
    
    groupLayout->addLayout(m_formLayout);
    groupLayout->addStretch();
}

void DistanceEditDialog::loadData()
{
    EditDialogBase::loadData();
    
    if (m_originalData.isEmpty()) {
        return;
    }
    
    // Заполняем форму данными
    m_editName->setText(m_originalData.value("name").toString());
    m_spinLength->setValue(m_originalData.value("length", 0.0).toDouble());
    m_spinControlTime->setValue(m_originalData.value("control_time", 0).toInt());
    m_editControlPoints->setPlainText(m_originalData.value("control_points").toString());
}

bool DistanceEditDialog::validateForm()
{
    // Проверка обязательных полей
    if (m_editName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), 
            tr("Введите название дистанции"));
        m_editName->setFocus();
        return false;
    }
    
    // Проверка протяженности
    if (m_spinLength->value() <= 0) {
        QMessageBox::warning(this, tr("Ошибка"), 
            tr("Протяженность должна быть больше 0"));
        m_spinLength->setFocus();
        return false;
    }
    
    // Проверка контрольного времени
    if (m_spinControlTime->value() <= 0) {
        QMessageBox::warning(this, tr("Ошибка"), 
            tr("Контрольное время должно быть больше 0"));
        m_spinControlTime->setFocus();
        return false;
    }
    
    return true;
}

QHash<QString, QVariant> DistanceEditDialog::collectFormData()
{
    QHash<QString, QVariant> data;
    
    data["name"] = m_editName->text().trimmed();
    data["length"] = m_spinLength->value();
    data["control_time"] = m_spinControlTime->value();
    data["control_points"] = m_editControlPoints->toPlainText().trimmed();
    
    return data;
}
