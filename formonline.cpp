#include "formonline.h"
#include "ui_formonline.h"

#include <QSettings>

FormOnline::FormOnline(QWidget *parent)
    : QDialog(parent), ui(new Ui::FormOnline)
{
    ui->setupUi(this);

    //connect(ui->buttonBox, &QPushButton::clicked,
    //        this, &SettingsDialog::apply);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &FormOnline::apply);

    loadSettingsFromIni();
}

FormOnline::~FormOnline()
{
    delete ui;
}

FormOnline::Settings FormOnline::settings() const
{
    return m_currentSettings;
}


void FormOnline::loadSettingsFromIni(){
    QSettings settings("set.ini", QSettings::IniFormat);
    settings.beginGroup("online");

    m_currentSettings.url = settings.value("url", QByteArray()).toString();
    m_currentSettings.onlineEnabled = settings.value("enable", QByteArray()).toBool();

    ui->ed_url->setText(m_currentSettings.url);
    ui->chbox_enable_online->setChecked(m_currentSettings.onlineEnabled);

    settings.endGroup();
}

void FormOnline::saveSettingsFromIni()
{
    QSettings settings("set.ini", QSettings::IniFormat);
    settings.beginGroup("online");
    settings.setValue("url", m_currentSettings.url);
    settings.setValue("enable",m_currentSettings.onlineEnabled);
    settings.endGroup();
}

void FormOnline::apply()
{
    updateSettings();
    saveSettingsFromIni();
    hide();
}

void FormOnline::updateSettings()
{
    m_currentSettings.url = ui->ed_url->text();
    m_currentSettings.onlineEnabled = ui->chbox_enable_online->isChecked();
}


