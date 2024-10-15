#include "forminfo.h"
#include "ui_forminfo.h"

FormInfo::FormInfo(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FormInfo)
{
    ui->setupUi(this);
}

FormInfo::~FormInfo()
{
    delete ui;
}

void FormInfo::recieveDataFromMain(const st_race &data_)
{
    ui->ed_location->setText(data_.location);
    ui->ed_title->setText(data_.title);
    ui->pl_description->setPlainText(data_.description);
    ui->ed_chif->setText(data_.chief_referee);
    ui->ed_secretar->setText(data_.secretary);
}

void FormInfo::on_buttonBox_accepted()
{
    //
}

