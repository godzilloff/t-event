#include "forminfo.h"
#include "ui_forminfo.h"
#include "Document.h"

#include <QMessageBox>
#include <QDebug>

FormInfo::FormInfo(Document* document, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FormInfo)
    , m_document(document)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &FormInfo::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &FormInfo::reject);
}

FormInfo::~FormInfo()
{
    delete ui;
}

void FormInfo::loadCompetition(qint64 competitionId)
{
    m_competitionId = competitionId;

    if (!m_document || !m_document->isOpen() || competitionId <= 0) {
        qWarning() << "FormInfo::loadCompetition: документ не открыт или невалидный ID";
        return;
    }

    const auto data = m_document->getRecord("competitions", competitionId);
    if (data.isEmpty()) {
        qWarning() << "FormInfo::loadCompetition: запись не найдена, ID =" << competitionId;
        return;
    }

    ui->ed_title    ->setText(data.value("name").toString());
    ui->ed_location ->setText(data.value("location").toString());
    ui->ed_chif     ->setText(data.value("chief_judge").toString());
    ui->ed_secretar ->setText(data.value("chief_secretary").toString());
    // Поле "description" отсутствует в схеме — используем organizations как хранилище
    ui->pl_description->setPlainText(data.value("organizations").toString());
}

bool FormInfo::saveCompetition()
{
    if (!m_document || !m_document->isOpen() || m_competitionId <= 0) {
        emit dataSavedError(tr("Документ не открыт или не задан ID соревнования"));
        return false;
    }

    QHash<QString, QVariant> data;
    data["name"]            = ui->ed_title->text().trimmed();
    data["location"]        = ui->ed_location->text().trimmed();
    data["chief_judge"]     = ui->ed_chif->text().trimmed();
    data["chief_secretary"] = ui->ed_secretar->text().trimmed();
    data["organizations"]   = ui->pl_description->toPlainText();

    if (data["name"].toString().isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Название соревнования не может быть пустым."));
        ui->ed_title->setFocus();
        return false;
    }

    if (!m_document->updateRecord("competitions", m_competitionId, data)) {
        emit dataSavedError(tr("Не удалось сохранить параметры соревнования"));
        return false;
    }

    emit dataSaved();
    return true;
}

void FormInfo::accept()
{
    if (!m_document || !m_document->isOpen() || m_competitionId <= 0) {
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Документ не открыт или не задан ID соревнования."));
        return;
    }
    if (!saveCompetition())
        return;
    QDialog::accept();
}
