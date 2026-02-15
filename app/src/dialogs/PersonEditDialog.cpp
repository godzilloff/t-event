// PersonEditDialog.cpp
#include "PersonEditDialog.h"
#include "DatabaseManager.h"
#include "Document.h"
#include "qsqlerror.h"
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QTimeEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QIntValidator>
#include <QDate>
#include <QTime>
#include <QGroupBox>

// PersonEditDialog::PersonEditDialog(qint64 recordId, QWidget* parent)
//     : EditDialogBase("participants", recordId, parent)
// {
//     setupUi();
//     setupComboBoxes();

//     connect(m_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &PersonEditDialog::onParticipantTypeChanged);

//     updateUiForType();

//     // Загружаем данные после инициализации UI
//     if (!isNewRecord()) {
//         loadData();
//     }
// }

// PersonEditDialog::PersonEditDialog(qint64 recordId, QWidget* parent)
//     : PersonEditDialog(nullptr, recordId, parent) // Делегируем другому конструктору
// {
// }

PersonEditDialog::PersonEditDialog(Document* document, qint64 recordId, QWidget* parent)
    : EditDialogBase(document, "participants", recordId, parent)
    , m_comboType(nullptr)
    , m_editName(nullptr)
    , m_editBib(nullptr)
    , m_editChip(nullptr)
    , m_comboDelegation(nullptr)
    , m_comboDistance(nullptr)
    , m_comboAgeGroup(nullptr)
    , m_comboGender(nullptr)
    , m_editBirthDate(nullptr)
    , m_editStartTime(nullptr)
    , m_spinTeamSize(nullptr)
    , m_labelType(nullptr)
    , m_labelName(nullptr)
    , m_labelBib(nullptr)
    , m_labelChip(nullptr)
    , m_labelDelegation(nullptr)
    , m_labelDistance(nullptr)
    , m_labelAgeGroup(nullptr)
    , m_labelBirthDate(nullptr)
    , m_labelGender(nullptr)
    , m_labelStartTime(nullptr)
    , m_labelTeamSize(nullptr)
    , m_delegationsModel(nullptr)
    , m_distancesModel(nullptr)
    , m_ageGroupsModel(nullptr)
    , m_isTeam(false)
{
    qDebug() << "PersonEditDialog создан с Document:" << document
             << "ID записи:" << recordId;

    // СНАЧАЛА создаем UI
    setupUi();
    setupComboBoxes();

    // Потом загружаем данные
    if (!isNewRecord()) {
        loadData();
    }

    // Настраиваем соединения
    connect(m_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PersonEditDialog::onParticipantTypeChanged);

    updateUiForType();
}

PersonEditDialog::~PersonEditDialog()
{
}

void PersonEditDialog::setupUi()
{
    // Основной layout для всего диалога
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);

    // Tab widget для вкладок
    m_tabWidget = new QTabWidget(this);

    // Основная вкладка
    QWidget* mainTab = new QWidget();
    createMainTab(mainTab);
    m_tabWidget->addTab(mainTab, tr("Основное"));

    m_mainLayout->addWidget(m_tabWidget);

    // Кнопки
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    setupButtonBox(buttonBox);

    // Переименовываем кнопку "Apply" в "Сохранить"
    QPushButton* applyButton = buttonBox->button(QDialogButtonBox::Apply);
    applyButton->setText(tr("Сохранить"));
    connect(applyButton, &QPushButton::clicked, this, &PersonEditDialog::onApplyClicked);

    m_mainLayout->addWidget(buttonBox);

    // Устанавливаем минимальный размер
    setMinimumSize(600, 500);
}

QFormLayout* PersonEditDialog::createFormLayout(QWidget* parent)
{
    QFormLayout* formLayout = new QFormLayout(parent);
    formLayout->setContentsMargins(10, 10, 10, 10);
    formLayout->setSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    return formLayout;
}

void PersonEditDialog::createMainTab(QWidget* tab)
{
    QVBoxLayout* tabLayout = new QVBoxLayout(tab);
    tabLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* groupBox = new QGroupBox(tr("Информация об участнике"), tab);
    m_formLayout = createFormLayout(groupBox);

    // Тип участника
    m_labelType = new QLabel(tr("Тип:"));
    m_comboType = new QComboBox();
    m_comboType->addItem(tr("Индивидуальный"));
    m_comboType->addItem(tr("Команда"));
    m_formLayout->addRow(m_labelType, m_comboType);

    // ФИО / Название команды
    m_labelName = new QLabel(tr("ФИО участника:"));
    m_editName = new QLineEdit();
    m_editName->setMinimumWidth(200);
    m_formLayout->addRow(m_labelName, m_editName);

    // Номер участника
    m_labelBib = new QLabel(tr("Номер участника:"));
    m_editBib = new QLineEdit();
    m_editBib->setValidator(new QIntValidator(1, 99999, this));
    m_editBib->setMaximumWidth(100);
    m_formLayout->addRow(m_labelBib, m_editBib);

    // Номер чипа
    m_labelChip = new QLabel(tr("Номер чипа:"));
    m_editChip = new QLineEdit();
    m_editChip->setValidator(new QIntValidator(1, 999999, this));
    m_editChip->setMaximumWidth(120);
    m_formLayout->addRow(m_labelChip, m_editChip);

    // Делегация
    m_labelDelegation = new QLabel(tr("Делегация:"));
    m_comboDelegation = new QComboBox();
    m_comboDelegation->setMinimumWidth(200);
    m_formLayout->addRow(m_labelDelegation, m_comboDelegation);

    // Дистанция
    m_labelDistance = new QLabel(tr("Дистанция:"));
    m_comboDistance = new QComboBox();
    m_comboDistance->setMinimumWidth(200);
    m_formLayout->addRow(m_labelDistance, m_comboDistance);

    // Возрастная группа
    m_labelAgeGroup = new QLabel(tr("Возрастная группа:"));
    m_comboAgeGroup = new QComboBox();
    m_comboAgeGroup->setMinimumWidth(150);
    m_formLayout->addRow(m_labelAgeGroup, m_comboAgeGroup);

    // Дата рождения
    m_labelBirthDate = new QLabel(tr("Дата рождения:"));
    m_editBirthDate = new QDateEdit();
    m_editBirthDate->setCalendarPopup(true);
    m_editBirthDate->setDate(QDate::currentDate().addYears(-20));
    m_editBirthDate->setMaximumDate(QDate::currentDate());
    m_editBirthDate->setDisplayFormat("dd.MM.yyyy");
    m_editBirthDate->setMaximumWidth(120);
    m_formLayout->addRow(m_labelBirthDate, m_editBirthDate);

    // Пол
    m_labelGender = new QLabel(tr("Пол:"));
    m_comboGender = new QComboBox();
    m_comboGender->addItem(tr("Мужской"));
    m_comboGender->addItem(tr("Женский"));
    m_comboGender->setMaximumWidth(120);
    m_formLayout->addRow(m_labelGender, m_comboGender);

    // Время старта
    m_labelStartTime = new QLabel(tr("Время старта:"));
    m_editStartTime = new QTimeEdit();
    m_editStartTime->setDisplayFormat("HH:mm:ss");
    m_editStartTime->setMaximumWidth(100);
    m_formLayout->addRow(m_labelStartTime, m_editStartTime);

    // Размер команды
    m_labelTeamSize = new QLabel(tr("Размер команды:"));
    m_spinTeamSize = new QSpinBox();
    m_spinTeamSize->setMinimum(1);
    m_spinTeamSize->setMaximum(10);
    m_spinTeamSize->setValue(1);
    m_spinTeamSize->setMaximumWidth(80);
    m_formLayout->addRow(m_labelTeamSize, m_spinTeamSize);

    groupBox->setLayout(m_formLayout);
    tabLayout->addWidget(groupBox);
    tabLayout->addStretch();
}

void PersonEditDialog::setupComboBoxes()
{
    if (!m_document) {
        qWarning() << "PersonEditDialog: Document is null!";
        return;
    }

    auto& dbManager = DatabaseManager::instance();
    qint64 competitionId = m_document->competitionId();

    qDebug() << "PersonEditDialog: Настраиваю ComboBox для competition_id:" << competitionId;

    try {
        // Делегации
        dbManager.withDatabase([this, competitionId](const QSqlDatabase& db) {
            m_delegationsModel = new QSqlTableModel(this, db);
            m_delegationsModel->setTable("delegations");

            if (competitionId > 0) {
                QString filter = QString("competition_id = %1").arg(competitionId);
                m_delegationsModel->setFilter(filter);
                qDebug() << "PersonEditDialog: Установлен фильтр для делегаций:" << filter;
            }

            m_delegationsModel->setSort(m_delegationsModel->fieldIndex("name"), Qt::AscendingOrder);

            if (!m_delegationsModel->select()) {
                qWarning() << "PersonEditDialog: Ошибка загрузки делегаций:"
                           << m_delegationsModel->lastError().text();
            } else {
                qDebug() << "PersonEditDialog: Загружено делегаций:" << m_delegationsModel->rowCount();
            }

            m_comboDelegation->setModel(m_delegationsModel);
            m_comboDelegation->setModelColumn(m_delegationsModel->fieldIndex("name"));
            m_comboDelegation->setCurrentIndex(-1);
        });

        // Дистанции
        dbManager.withDatabase([this, competitionId](const QSqlDatabase& db) {
            m_distancesModel = new QSqlTableModel(this, db);
            m_distancesModel->setTable("distances");

            if (competitionId > 0) {
                QString filter = QString("competition_id = %1").arg(competitionId);
                m_distancesModel->setFilter(filter);
                qDebug() << "PersonEditDialog: Установлен фильтр для дистанций:" << filter;
            }

            m_distancesModel->setSort(m_distancesModel->fieldIndex("name"), Qt::AscendingOrder);

            if (!m_distancesModel->select()) {
                qWarning() << "PersonEditDialog: Ошибка загрузки дистанций:"
                           << m_distancesModel->lastError().text();
            } else {
                qDebug() << "PersonEditDialog: Загружено дистанций:" << m_distancesModel->rowCount();
            }

            m_comboDistance->setModel(m_distancesModel);
            m_comboDistance->setModelColumn(m_distancesModel->fieldIndex("name"));
            m_comboDistance->setCurrentIndex(-1);
        });

        // Возрастные группы
        dbManager.withDatabase([this, competitionId](const QSqlDatabase& db) {
            m_ageGroupsModel = new QSqlTableModel(this, db);
            m_ageGroupsModel->setTable("age_groups");

            if (competitionId > 0) {
                QString filter = QString("competition_id = %1").arg(competitionId);
                m_ageGroupsModel->setFilter(filter);
                qDebug() << "PersonEditDialog: Установлен фильтр для возрастных групп:" << filter;
            }

            m_ageGroupsModel->setSort(m_ageGroupsModel->fieldIndex("name"), Qt::AscendingOrder);

            if (!m_ageGroupsModel->select()) {
                qWarning() << "PersonEditDialog: Ошибка загрузки возрастных групп:"
                           << m_ageGroupsModel->lastError().text();
            } else {
                qDebug() << "PersonEditDialog: Загружено возрастных групп:" << m_ageGroupsModel->rowCount();
            }

            m_comboAgeGroup->setModel(m_ageGroupsModel);
            m_comboAgeGroup->setModelColumn(m_ageGroupsModel->fieldIndex("name"));
            m_comboAgeGroup->setCurrentIndex(-1);
        });

    } catch (const std::exception& e) {
        qWarning() << "PersonEditDialog: Ошибка при настройке ComboBox:" << e.what();
    }
}

void PersonEditDialog::loadData()
{
    if (isNewRecord()) {
        return; // Для новой записи не загружаем
    }

    // Вызываем базовый метод для получения данных
    EditDialogBase::loadData();

    if (m_originalData.isEmpty()) {
        qWarning() << "PersonEditDialog: Не удалось загрузить данные для записи ID:" << m_recordId;
        return;
    }

    qDebug() << "PersonEditDialog: Загружаю данные для записи ID:" << m_recordId;
    qDebug() << "Original data:" << m_originalData;

    // Заполняем форму данными
    QString participantType = m_originalData.value("participant_type", "individual").toString();
    m_isTeam = (participantType == "team");

    // Устанавливаем тип участника
    if (m_comboType) {
        m_comboType->setCurrentIndex(m_isTeam ? 1 : 0);
        qDebug() << "Установлен тип участника:" << participantType << "(isTeam:" << m_isTeam << ")";
    }

    // Имя/название команды
    if (m_editName) {
        if (m_isTeam) {
            m_editName->setText(m_originalData.value("team_name").toString());
        } else {
            m_editName->setText(m_originalData.value("full_name").toString());
        }
        qDebug() << "Установлено имя:" << m_editName->text();
    }

    if (m_editBib) {
        m_editBib->setText(m_originalData.value("bib_number").toString());
        qDebug() << "Установлен номер:" << m_editBib->text();
    }

    if (m_editChip) {
        m_editChip->setText(m_originalData.value("chip_number").toString());
        qDebug() << "Установлен номер чипа:" << m_editChip->text();
    }

    // Дата рождения
    if (m_editBirthDate && !m_isTeam) {
        QString birthDateStr = m_originalData.value("birth_date").toString();
        if (!birthDateStr.isEmpty()) {
            QDate date = QDate::fromString(birthDateStr, Qt::ISODate);
            if (date.isValid()) {
                m_editBirthDate->setDate(date);
                qDebug() << "Установлена дата рождения:" << birthDateStr;
            }
        }
    }

    // Пол
    if (m_comboGender && !m_isTeam) {
        QString gender = m_originalData.value("gender").toString();
        if (!gender.isEmpty()) {
            int index = m_comboGender->findText(gender);
            if (index >= 0) {
                m_comboGender->setCurrentIndex(index);
                qDebug() << "Установлен пол:" << gender;
            }
        }
    }

    // Размер команды
    if (m_spinTeamSize && m_isTeam) {
        m_spinTeamSize->setValue(m_originalData.value("team_size", 1).toInt());
        qDebug() << "Установлен размер команды:" << m_spinTeamSize->value();
    }

    // Делегация
    if (m_comboDelegation && m_delegationsModel) {
        qint64 delegationId = m_originalData.value("delegation_id").toLongLong();
        qDebug() << "Delegation ID from data:" << delegationId;

        if (delegationId > 0) {
            // Ищем делегацию в модели
            for (int i = 0; i < m_delegationsModel->rowCount(); ++i) {
                qint64 modelDelegationId = m_delegationsModel->data(
                                                                 m_delegationsModel->index(i, 0)).toLongLong();

                if (modelDelegationId == delegationId) {
                    m_comboDelegation->setCurrentIndex(i);
                    QString delegationName = m_delegationsModel->data(
                                                                   m_delegationsModel->index(i, m_delegationsModel->fieldIndex("name"))).toString();
                    qDebug() << "Найдена делегация:" << delegationName << "ID:" << delegationId;
                    break;
                }
            }
        }
    }

    // Дистанция
    if (m_comboDistance && m_distancesModel) {
        qint64 distanceId = m_originalData.value("distance_id").toLongLong();
        qDebug() << "Distance ID from data:" << distanceId;

        if (distanceId > 0) {
            for (int i = 0; i < m_distancesModel->rowCount(); ++i) {
                qint64 modelDistanceId = m_distancesModel->data(
                                                             m_distancesModel->index(i, 0)).toLongLong();

                if (modelDistanceId == distanceId) {
                    m_comboDistance->setCurrentIndex(i);
                    QString distanceName = m_distancesModel->data(
                                                               m_distancesModel->index(i, m_distancesModel->fieldIndex("name"))).toString();
                    qDebug() << "Найдена дистанция:" << distanceName << "ID:" << distanceId;
                    break;
                }
            }
        }
    }

    // Возрастная группа
    if (m_comboAgeGroup && m_ageGroupsModel) {
        qint64 ageGroupId = m_originalData.value("age_group_id").toLongLong();
        qDebug() << "Age Group ID from data:" << ageGroupId;

        if (ageGroupId > 0) {
            for (int i = 0; i < m_ageGroupsModel->rowCount(); ++i) {
                qint64 modelAgeGroupId = m_ageGroupsModel->data(
                                                             m_ageGroupsModel->index(i, 0)).toLongLong();

                if (modelAgeGroupId == ageGroupId) {
                    m_comboAgeGroup->setCurrentIndex(i);
                    QString ageGroupName = m_ageGroupsModel->data(
                                                               m_ageGroupsModel->index(i, m_ageGroupsModel->fieldIndex("name"))).toString();
                    qDebug() << "Найдена возрастная группа:" << ageGroupName << "ID:" << ageGroupId;
                    break;
                }
            }
        }
    }

    // Время старта
    if (m_editStartTime) {
        QString startTime = m_originalData.value("start_time").toString();
        if (!startTime.isEmpty()) {
            QTime time = QTime::fromString(startTime, "HH:mm:ss");
            if (time.isValid()) {
                m_editStartTime->setTime(time);
                qDebug() << "Установлено время старта:" << startTime;
            }
        }
    }

    updateUiForType();
    qDebug() << "PersonEditDialog: Данные успешно загружены";
}

bool PersonEditDialog::validateForm()
{
    if (m_editName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"),
                             m_isTeam ? tr("Введите название команды") : tr("Введите ФИО участника"));
        m_editName->setFocus();
        return false;
    }

    if (m_editBib->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Введите номер участника"));
        m_editBib->setFocus();
        return false;
    }

    if (m_comboDistance->currentIndex() < 0) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Выберите дистанцию"));
        m_comboDistance->setFocus();
        return false;
    }

    return true;
}

QHash<QString, QVariant> PersonEditDialog::collectFormData()
{
    QHash<QString, QVariant> fdata;

    fdata["participant_type"] = m_isTeam ? "team" : "individual";

    if (m_isTeam) {
        fdata["team_name"] = m_editName->text().trimmed();
        fdata["team_size"] = m_spinTeamSize->value();
        fdata["full_name"] = QVariant();
        fdata["birth_date"] = QVariant();
        fdata["gender"] = QVariant();
    } else {
        fdata["full_name"] = m_editName->text().trimmed();
        fdata["birth_date"] = m_editBirthDate->date().toString(Qt::ISODate);
        fdata["gender"] = m_comboGender->currentText();
        fdata["team_name"] = QVariant();
        fdata["team_size"] = QVariant();
    }

    fdata["bib_number"] = m_editBib->text().trimmed();

    if (m_editChip->text().length() > 0)
        fdata["chip_number"] = m_editChip->text().trimmed();

    if (m_comboDelegation->currentIndex() >= 0) {
        qint64 delegationId = m_delegationsModel->data(
                                                    m_delegationsModel->index(m_comboDelegation->currentIndex(), 0)).toLongLong();
        fdata["delegation_id"] = delegationId;
    } else {
        fdata["delegation_id"] = QVariant(); // NULL
    }

    if (m_comboDistance->currentIndex() >= 0) {
        qint64 distanceId = m_distancesModel->data(
                                                m_distancesModel->index(m_comboDistance->currentIndex(), 0)).toLongLong();
        fdata["distance_id"] = distanceId;
    } else {
        fdata["distance_id"] = QVariant(); // NULL
    }

    if (m_comboAgeGroup->currentIndex() >= 0) {
        qint64 ageGroupId = m_ageGroupsModel->data(
                                                m_ageGroupsModel->index(m_comboAgeGroup->currentIndex(), 0)).toLongLong();
        fdata["age_group_id"] = ageGroupId;
    } else {
        fdata["age_group_id"] = QVariant(); // NULL
    }

    if (!m_editStartTime->time().isNull() && m_editStartTime->time().isValid() && (m_editStartTime->time().msecsSinceStartOfDay() > 0 ) ) {
        fdata["start_time"] = m_editStartTime->time().toString("HH:mm:ss");
    } else {
        fdata["start_time"] = QVariant(); // NULL
    }

    return fdata;
}

void PersonEditDialog::onParticipantTypeChanged(int index)
{
    m_isTeam = (index == 1);
    updateUiForType();
}

void PersonEditDialog::updateUiForType()
{
    bool isTeam = m_isTeam;

    // Обновляем подписи
    m_labelName->setText(isTeam ? tr("Название команды:") : tr("ФИО участника:"));

    // Показываем/скрываем соответствующие поля
    m_labelBirthDate->setVisible(!isTeam);
    m_editBirthDate->setVisible(!isTeam);
    m_labelGender->setVisible(!isTeam);
    m_comboGender->setVisible(!isTeam);
    m_labelTeamSize->setVisible(isTeam);
    m_spinTeamSize->setVisible(isTeam);
}

void PersonEditDialog::onApplyClicked()
{
    if (saveRecord()) {
        // Если это новая запись, меняем заголовок
        if (isNewRecord()) {
            setDialogTitle(tr("Редактировать запись"));

            // Меняем доступность кнопок
            QDialogButtonBox* buttonBox = findChild<QDialogButtonBox*>();
            if (buttonBox) {
                QPushButton* deleteButton = buttonBox->findChild<QPushButton*>("deleteButton");
                if (!deleteButton) {
                    addDeleteButton(buttonBox);
                }
            }
        }

        // Сообщаем пользователю
        QMessageBox::information(this, tr("Сохранение"),
                                 tr("Данные успешно сохранены"));
    }
}
