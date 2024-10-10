#include "formperson.h"
#include "ui_formperson.h"

#include <QTime>
#include <QStringListModel>

FormPerson::FormPerson(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FormPerson)
{
    ui->setupUi(this);
}

FormPerson::~FormPerson()
{
    delete ui;
}

void FormPerson::update_sevent(QSportEvent *pSEvent)
{
    prtSEvent = pSEvent;
}

void FormPerson::recieveDataFromMain(const st_person* data_)
{
    QTime time = QTime::fromMSecsSinceStartOfDay(data_->start_time);
    QDate birth_date_d = QDate::fromString(data_->birth_date, "yyyy-MM-dd");

    update_organization();
    update_group();

    ui->ed_name->setText(data_->name);
    ui->ed_surname->setText(data_->surname);
    ui->ed_bib->setText(QString::number(data_->bib));
    ui->ed_cardNum->setText(QString::number(data_->card_number));
    ui->ted_start->setTime(time);
    ui->cb_qual->setCurrentIndex(data_->qual);
    ui->ed_date->setDate(birth_date_d);
    ui->pled_comment->setPlainText(data_->comment);
    ui->cb_org->setCurrentText(prtSEvent->getNameOrganization(data_->organization_id));
    ui->cb_group->setCurrentText(prtSEvent->getNameGroup(data_->group_id));

}

void FormPerson::update_organization()
{
    QStringList names = prtSEvent->getNamesOrganization();
    ui->cb_org->clear();
    ui->cb_org->addItems(names);
}

void FormPerson::update_group()
{
    QStringList names = prtSEvent->getNamesGroup();
    ui->cb_group->clear();
    ui->cb_group->addItems(names);
}
