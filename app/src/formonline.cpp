#include "formonline.h"
#include "ui_formonline.h"

#include <QSettings>

FormOnline::FormOnline(QWidget *parent)
    : QDialog(parent), ui(new Ui::FormOnline)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &FormOnline::onAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

FormOnline::~FormOnline() { delete ui; }

void FormOnline::setConfig(const OrgeoConfig& c)
{
    ui->ed_url->setText(c.liveUrl);
    ui->chbox_enable_online->setChecked(c.liveEnabled);
    ui->chbox_results->setChecked(c.resultsEnabled);
    ui->chbox_splits->setChecked(c.splitsEnabled);
    ui->ed_cp_codes->setText(c.cpCodes);
}

OrgeoConfig FormOnline::config()
{
    OrgeoConfig c;
    c.liveUrl = ui->ed_url->text().trimmed();
    c.liveEnabled = ui->chbox_enable_online->isChecked();
    c.resultsEnabled = ui->chbox_results->isChecked();
    c.splitsEnabled = ui->chbox_splits->isChecked();
    c.cpCodes = ui->ed_cp_codes->text().trimmed();
    return c;
}

void FormOnline::onAccepted()
{
    emit configSaved(config());
}
