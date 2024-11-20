#include "formperson.h"
#include "ui_formperson.h"

#include <QTime>
#include <QStringListModel>
#include <QButtonGroup>
#include <QPushButton>

FormPerson::FormPerson(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FormPerson)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &FormPerson::onAccepted);
    connect(ui->ed_cardNum, &QSpinBox::textChanged, this, &FormPerson::onCheck_numCard);

}

FormPerson::~FormPerson()
{
    delete ui;
}

void FormPerson::update_sevent(std::shared_ptr<QSportEvent> &pSEvent)
{
    ptrSEvent = pSEvent;
}

void FormPerson::recieveDataFromMain(const st_person* data_)
{
    id = data_->id;
    QTime time = QTime::fromMSecsSinceStartOfDay(data_->start_time);
    QDate birth_date_d = QDate::fromString(data_->birth_date, "yyyy-MM-dd");

    update_organization();
    update_group();

    ui->ed_name->setText(data_->name);
    ui->ed_surname->setText(data_->surname);
    ui->ed_bib->setValue(data_->bib);
    ui->ed_cardNum->setValue(data_->card_number);
    ui->ted_start->setTime(time);
    ui->cb_qual->setCurrentIndex(data_->qual);
    ui->ed_date->setDate(birth_date_d);
    ui->pled_comment->setPlainText(data_->comment);
    ui->cb_org->setCurrentText(ptrSEvent->getNameOrganization(data_->organization_id));
    ui->cb_group->setCurrentText(ptrSEvent->getNameGroup(data_->group_id));

}

void FormPerson::onCheck_numCard(const QString &text)
{
    //qDebug() << "on_check_numCard_textChanged ok";
    int card = text.toInt();
    if ( (card==0)||(ptrSEvent->isCardNumFree(card)) )
    {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->lb_check_cardNum->setText(text + " - доступен");
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->lb_check_cardNum->setText(text + " - занят " + ptrSEvent->getPersonInfoFromCardNum(card));
    }
}

void FormPerson::onAccepted()
{
    //qDebug() << "Person ok";
    ptrSEvent->setCardNumFromId(id,ui->ed_cardNum->text().toInt());
    ptrSEvent->setPNameFromId(id,ui->ed_name->text());
    ptrSEvent->setPSurnameFromId(id,ui->ed_surname->text());
    ptrSEvent->setBibFromId(id,ui->ed_bib->text().toInt());
    ptrSEvent->setBibFromId(id,ui->ed_bib->text().toInt());
    ptrSEvent->setPOrgFromNameOrg(id,ui->cb_org->currentText());
    ptrSEvent->setPGroupFromNameGroup(id,ui->cb_group->currentText());
    ptrSEvent->setPersonComment(id,ui->pled_comment->toPlainText());
    ptrSEvent->setPersonBirthDate(id,ui->ed_date->date().toString("yyyy-MM-dd"));
    ptrSEvent->setPersonStartTime(id,ui->ted_start->time().msecsSinceStartOfDay());
    emit requestSave();
}

void FormPerson::update_organization()
{
    QStringList names = ptrSEvent->getNamesOrganization();
    ui->cb_org->clear();
    ui->cb_org->addItems(names);
}

void FormPerson::update_group()
{
    QStringList names = ptrSEvent->getNamesGroup();
    ui->cb_group->clear();
    ui->cb_group->addItems(names);
}
