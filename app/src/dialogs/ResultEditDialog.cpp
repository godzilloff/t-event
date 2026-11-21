#include "ResultEditDialog.h"
#include "Document.h"
#include "DatabaseManager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTimeEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>

ResultEditDialog::ResultEditDialog(Document* document, qint64 recordId, QWidget* parent)
    : EditDialogBase(document, "results", recordId, parent)
{
    setupUi();
    if (!isNewRecord())
        loadData();
}

ResultEditDialog::~ResultEditDialog() = default;

void ResultEditDialog::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);

    createInfoGroup();
    createEditGroup();

    auto* bb = new QDialogButtonBox(this);
    setupButtonBox(bb);
    m_mainLayout->addWidget(bb);

    setMinimumSize(520, 480);
    setWindowTitle(isNewRecord() ? tr("Добавить результат")
                                 : tr("Редактировать результат"));
}

void ResultEditDialog::createInfoGroup()
{
    m_infoGroup  = new QGroupBox(tr("Участник"), this);
    m_infoLayout = new QFormLayout(m_infoGroup);
    m_infoLayout->setLabelAlignment(Qt::AlignRight);
    m_infoLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto makeLabel = [this]() {
        auto* l = new QLabel("—", m_infoGroup);
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        return l;
    };

    m_lbParticipant = makeLabel();
    m_lbBib         = makeLabel();
    m_lbChip        = makeLabel();
    m_lbAgeGroup    = makeLabel();
    m_lbDelegation  = makeLabel();
    m_lbDistance    = makeLabel();

    m_infoLayout->addRow(tr("ФИО:"),            m_lbParticipant);
    m_infoLayout->addRow(tr("Номер:"),          m_lbBib);
    m_infoLayout->addRow(tr("Чип:"),            m_lbChip);
    m_infoLayout->addRow(tr("Возрастная гр.:"), m_lbAgeGroup);
    m_infoLayout->addRow(tr("Делегация:"),      m_lbDelegation);
    m_infoLayout->addRow(tr("Дистанция:"),      m_lbDistance);

    m_mainLayout->addWidget(m_infoGroup);
}

void ResultEditDialog::createEditGroup()
{
    m_editGroup  = new QGroupBox(tr("Отметки"), this);
    m_formLayout = new QFormLayout(m_editGroup);
    m_formLayout->setLabelAlignment(Qt::AlignRight);
    m_formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto makeTime = [this](bool readOnly = false) {
        auto* t = new QTimeEdit(m_editGroup);
        t->setDisplayFormat("HH:mm:ss.z");
        if (readOnly) {
            t->setReadOnly(true);
            t->setButtonSymbols(QAbstractSpinBox::NoButtons);
            t->setEnabled(false);
        }
        return t;
    };

    m_editStart  = makeTime();
    m_editFinish = makeTime();
    m_editResult = makeTime(true);

    m_comboStatus = new QComboBox(m_editGroup);
    m_comboStatus->addItems({
        "-", "ОК", "Финишировал", "ДИСКВ.", "Непр. отметка",
        "Сошёл", "Активный", "Неактивный", "ПРЕВЫШЕНО КВ",
        "Проведена жеребьёвка", "Вне конкурса", "Перемещён",
        "Перемещён наверх", "НЕ СТАРТ.", "Не заявлен",
        "Заявка отменена", "Восстановлен"
    });

    m_formLayout->addRow(tr("Старт:"),     m_editStart);
    m_formLayout->addRow(tr("Финиш:"),     m_editFinish);
    m_formLayout->addRow(tr("Результат:"), m_editResult);
    m_formLayout->addRow(tr("Статус:"),    m_comboStatus);

    connect(m_editStart,  &QTimeEdit::timeChanged, this, &ResultEditDialog::onTimesChanged);
    connect(m_editFinish, &QTimeEdit::timeChanged, this, &ResultEditDialog::onTimesChanged);

    m_mainLayout->addWidget(m_editGroup);
}

void ResultEditDialog::loadData()
{
    if (isNewRecord() || !m_document) return;

    auto& db = DatabaseManager::instance();
    QHash<QString, QVariant> params;
    params["id"] = m_recordId;

    auto rows = db.executeSelect(R"(
        SELECT r.id, r.participant_id, r.chip_number,
               r.start_time, r.finish_time, r.result_time, r.status,
               p.full_name, p.bib_number,
               ag.name   AS age_group_name,
               d.name    AS delegation_name,
               dist.name AS distance_name
        FROM results r
        LEFT JOIN participants p  ON r.participant_id = p.id
        LEFT JOIN delegations  d  ON p.delegation_id  = d.id
        LEFT JOIN distances    dist ON p.distance_id  = dist.id
        LEFT JOIN age_groups   ag ON p.age_group_id   = ag.id
        WHERE r.id = :id
    )", params);

    if (rows.isEmpty()) {
        qWarning() << "ResultEditDialog: результат ID" << m_recordId << "не найден";
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Запись результата не найдена."));
        return;
    }

    m_originalData  = rows.first();
    m_participantId = m_originalData.value("participant_id").toLongLong();

    qDebug() << "=== RAW DATA ===";
    qDebug() << "start_time :" << m_originalData.value("start_time")
             << "type:" << m_originalData.value("start_time").typeName()
             << "isNull:" << m_originalData.value("start_time").isNull();
    qDebug() << "finish_time:" << m_originalData.value("finish_time")
             << "type:" << m_originalData.value("finish_time").typeName()
             << "isNull:" << m_originalData.value("finish_time").isNull();
    qDebug() << "================";

    // Инфо-блок
    m_lbParticipant->setText(m_originalData.value("full_name").toString());
    m_lbBib        ->setText(m_originalData.value("bib_number").toString());
    m_lbChip       ->setText(m_originalData.value("chip_number").toString());
    m_lbAgeGroup   ->setText(m_originalData.value("age_group_name").toString());
    m_lbDelegation ->setText(m_originalData.value("delegation_name").toString());
    m_lbDistance   ->setText(m_originalData.value("distance_name").toString());

    // Времена: пробуем разные форматы
    auto parseTime = [](const QVariant& v) -> QTime {
        const QString s = v.toString().trimmed();
        if (s.isEmpty()) return QTime(0, 0, 0);

        // ── Основной формат вашей БД ──
        QDateTime dt = QDateTime::fromString(s, "dd.MM.yyyy HH:mm:ss.zzz");
        if (dt.isValid()) return dt.time();

        // ── Дополнительные (на случай миграции / ручного ввода) ──
        dt = QDateTime::fromString(s, Qt::ISODate);
        if (dt.isValid()) return dt.time();

        QTime t = QTime::fromString(s, "HH:mm:ss.zzz");
        if (t.isValid()) return t;

        t = QTime::fromString(s, "HH:mm:ss");
        if (t.isValid()) return t;

        qWarning() << "ResultEditDialog: не удалось распарсить время:" << s;
        return QTime(0, 0, 0);
    };

    m_updating = true;
    m_editStart ->setTime(parseTime(m_originalData.value("start_time")));
    m_editFinish->setTime(parseTime(m_originalData.value("finish_time")));

    // Статус: если в БД лежит что-то, чего нет в списке — добавляем
    const QString status = m_originalData.value("status").toString();
    int idx = m_comboStatus->findText(status, Qt::MatchFixedString);
    if (idx < 0 && !status.isEmpty()) {
        m_comboStatus->addItem(status);
        idx = m_comboStatus->count() - 1;
    }
    m_comboStatus->setCurrentIndex(idx >= 0 ? idx : 0);
    m_updating = false;

    recalculateResult();
}

void ResultEditDialog::onTimesChanged()
{
    if (!m_updating)
        recalculateResult();
}

void ResultEditDialog::recalculateResult()
{
    const QTime start  = m_editStart ->time();
    const QTime finish = m_editFinish->time();

    m_updating = true;
    if (!start.isValid() || !finish.isValid() || finish < start) {
        m_editResult->setTime(QTime(0, 0, 0));
    } else {
        int secs = start.secsTo(finish);
        if (secs < 0) secs = 0;
        m_editResult->setTime(QTime(0, 0, 0).addSecs(secs));
    }
    m_updating = false;
}

bool ResultEditDialog::validateForm()
{
    if (m_participantId <= 0) {
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Не определён участник результата."));
        return false;
    }
    if (m_editFinish->time() < m_editStart->time()) {
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Время финиша раньше времени старта."));
        m_editFinish->setFocus();
        return false;
    }
    if (m_comboStatus->currentIndex() < 0) {
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Укажите статус."));
        m_comboStatus->setFocus();
        return false;
    }
    return true;
}

QHash<QString, QVariant> ResultEditDialog::collectFormData()
{
    QHash<QString, QVariant> fdata;
    fdata["participant_id"] = m_participantId;

    // Достаём дату из оригинальной строки (формат dd.MM.yyyy HH:mm:ss.zzz),
    // чтобы при изменении времени не потерять дату соревнования.
    auto dateFromOriginal = [this](const QString& key) -> QDate {
        const QString s = m_originalData.value(key).toString().trimmed();
        if (s.isEmpty()) return QDate::currentDate();

        // Основной формат
        QDateTime dt = QDateTime::fromString(s, "dd.MM.yyyy HH:mm:ss.zzz");
        if (dt.isValid()) return dt.date();

        // Fallback на ISO
        dt = QDateTime::fromString(s, Qt::ISODate);
        if (dt.isValid()) return dt.date();

        return QDate::currentDate();
    };

    auto isZeroTime = [](const QTime& t) {
        return !t.isValid() ||
               (t.hour() == 0 && t.minute() == 0 && t.second() == 0 && t.msec() == 0);
    };

    auto buildDateTimeStr = [](const QDate& d, const QTime& t) {
        // Тот же формат, что используется в БД
        return QDateTime(d, t).toString("dd.MM.yyyy HH:mm:ss.zzz");
    };

    const QTime start  = m_editStart ->time();
    const QTime finish = m_editFinish->time();

    if (isZeroTime(start))
        fdata["start_time"] = QVariant();       // NULL
    else
        fdata["start_time"] = buildDateTimeStr(dateFromOriginal("start_time"), start);

    if (isZeroTime(finish))
        fdata["finish_time"] = QVariant();      // NULL
    else
        fdata["finish_time"] = buildDateTimeStr(dateFromOriginal("finish_time"), finish);


    if (isZeroTime(start) || isZeroTime(finish)) {
        fdata["result_time"] = QVariant();     // NULL
    } else {
        const int msecs = qMax(0, start.msecsTo(finish));
        const QTime resultTime = QTime(0, 0, 0).addMSecs(msecs);
        fdata["result_time"] = resultTime.toString("HH:mm:ss.zzz");
    }

    fdata["status"] = m_comboStatus->currentText();

    return fdata;
}
