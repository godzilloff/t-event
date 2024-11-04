#include "formonline.h"
#include "ui_formonline.h"

#include <QSettings>

FormOnline::FormOnline(QWidget *parent)
    : QDialog(parent), ui(new Ui::FormOnline)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &FormOnline::onAccepted);
}

FormOnline::~FormOnline()
{
    delete ui;
}

void FormOnline::update_sevent(std::shared_ptr<QSportEvent> &pSEvent)
{
    ptrSEvent = pSEvent;
}

void FormOnline::recieveDataFromMain()
{
    ui->ed_url->setText(ptrSEvent->getOnlineUrl());
    ui->chbox_enable_online->setChecked(ptrSEvent->getOnlineEnable());
}

void FormOnline::onAccepted()
{
    ptrSEvent->setOnlineEnable(ui->chbox_enable_online->isChecked());
    ptrSEvent->setOnlineUrl(ui->ed_url->text());
}
