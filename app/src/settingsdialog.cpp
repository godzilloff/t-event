// Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QSettings>
#include <QIntValidator>
#include <QLineEdit>
#include <QSerialPortInfo>

static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SettingsDialog),
    m_intValidator(new QIntValidator(0, 4000000, this))
{
    this->resize(this->width(), 382);
    m_ui->setupUi(this);

    m_ui->baudRateBox->setInsertPolicy(QComboBox::NoInsert);

    connect(m_ui->applyButton, &QPushButton::clicked,
            this, &SettingsDialog::apply);
    connect(m_ui->serialPortInfoListBox, &QComboBox::currentIndexChanged,
            this, &SettingsDialog::showPortInfo);
    connect(m_ui->baudRateBox,  &QComboBox::currentIndexChanged,
            this, &SettingsDialog::checkCustomBaudRatePolicy);
    connect(m_ui->serialPortInfoListBox, &QComboBox::currentIndexChanged,
            this, &SettingsDialog::checkCustomDevicePathPolicy);
    connect(m_ui->bt_refresh, &QPushButton::clicked, this, &SettingsDialog::refresh);

    fillPortsParameters();
    fillPortsInfo();

    //updateSettings();
    loadSettingsFromIni();
}

SettingsDialog::~SettingsDialog()
{
    delete m_ui;
}

void SettingsDialog::loadSettingsFromIni(){
    QSettings settings("set.ini", QSettings::IniFormat);
    settings.beginGroup("comport");

    m_currentSettings.name = settings.value("name", QByteArray()).toString();
    m_currentSettings.baudRate = settings.value("baudRate", QByteArray()).toInt();
    m_currentSettings.stringBaudRate = settings.value("stringBaudRate", QByteArray()).toString();
    m_currentSettings.dataBits = QSerialPort::Data8;
    m_currentSettings.stringDataBits = settings.value("stringDataBits", QByteArray()).toString();
    m_currentSettings.parity = QSerialPort::NoParity;
    m_currentSettings.stringParity = settings.value("stringParity", QByteArray()).toString();
    m_currentSettings.stopBits = QSerialPort::OneStop;
    m_currentSettings.stringStopBits = settings.value("stringStopBits", QByteArray()).toString();
    m_currentSettings.flowControl = QSerialPort::NoFlowControl;
    m_currentSettings.stringFlowControl = settings.value("stringFlowControl", QByteArray()).toString();
    m_currentSettings.localEchoEnabled = settings.value("stringFlowControl", QByteArray()).toBool();

    settings.endGroup();
}

void SettingsDialog::saveSettingsFromIni()
{
    QSettings settings("set.ini", QSettings::IniFormat);
    settings.beginGroup("comport");
    settings.setValue("name", m_currentSettings.name);
    settings.setValue("baudRate",m_currentSettings.baudRate);
    settings.setValue("stringBaudRate",m_currentSettings.stringBaudRate);
    settings.setValue("dataBits",m_currentSettings.dataBits);
    settings.setValue("stringDataBits",m_currentSettings.stringDataBits);
    settings.setValue("parity",m_currentSettings.parity);
    settings.setValue("stringParity",m_currentSettings.stringParity);
    settings.setValue("stopBits",m_currentSettings.stopBits);
    settings.setValue("stringStopBits",m_currentSettings.stringStopBits);
    settings.setValue("flowControl",m_currentSettings.flowControl);
    settings.setValue("stringFlowControl",m_currentSettings.stringFlowControl);
    settings.setValue("localEchoEnabled",m_currentSettings.localEchoEnabled);
    settings.endGroup();
}

SettingsDialog::Settings SettingsDialog::settings() const
{
    return m_currentSettings;
}

void SettingsDialog::showPortInfo(int idx)
{
    if (idx == -1)
        return;

    const QString blankString = tr(::blankString);

    const QStringList list = m_ui->serialPortInfoListBox->itemData(idx).toStringList();
    m_ui->descriptionLabel->setText(tr("Описание: %1").arg(list.value(1, blankString)));
    m_ui->manufacturerLabel->setText(tr("Производитель: %1").arg(list.value(2, blankString)));
    m_ui->serialNumberLabel->setText(tr("Серийный номер: %1").arg(list.value(3, blankString)));
    m_ui->locationLabel->setText(tr("Размещение: %1").arg(list.value(4, blankString)));
    m_ui->vidLabel->setText(tr("ИД производителя: %1").arg(list.value(5, blankString)));
    m_ui->pidLabel->setText(tr("ИД изделия: %1").arg(list.value(6, blankString)));
}

void SettingsDialog::apply()
{
    updateSettings();
    saveSettingsFromIni();
    hide();
}

void SettingsDialog::refresh()
{
    fillPortsParameters();
    fillPortsInfo();
}

void SettingsDialog::checkCustomBaudRatePolicy(int idx)
{
    const bool isCustomBaudRate = !m_ui->baudRateBox->itemData(idx).isValid();
    m_ui->baudRateBox->setEditable(isCustomBaudRate);
    if (isCustomBaudRate) {
        m_ui->baudRateBox->clearEditText();
        QLineEdit *edit = m_ui->baudRateBox->lineEdit();
        edit->setValidator(m_intValidator);
    }
}

void SettingsDialog::checkCustomDevicePathPolicy(int idx)
{
    const bool isCustomPath = !m_ui->serialPortInfoListBox->itemData(idx).isValid();
    m_ui->serialPortInfoListBox->setEditable(isCustomPath);
    if (isCustomPath)
        m_ui->serialPortInfoListBox->clearEditText();
}

void SettingsDialog::fillPortsParameters()
{
    m_ui->baudRateBox->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    m_ui->baudRateBox->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    m_ui->baudRateBox->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    m_ui->baudRateBox->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    m_ui->baudRateBox->addItem(tr("Custom"));
    m_ui->baudRateBox->setCurrentIndex(3);

    m_ui->dataBitsBox->addItem(QStringLiteral("5"), QSerialPort::Data5);
    m_ui->dataBitsBox->addItem(QStringLiteral("6"), QSerialPort::Data6);
    m_ui->dataBitsBox->addItem(QStringLiteral("7"), QSerialPort::Data7);
    m_ui->dataBitsBox->addItem(QStringLiteral("8"), QSerialPort::Data8);
    m_ui->dataBitsBox->setCurrentIndex(3);

    m_ui->parityBox->addItem(tr("None"), QSerialPort::NoParity);
    m_ui->parityBox->addItem(tr("Even"), QSerialPort::EvenParity);
    m_ui->parityBox->addItem(tr("Odd"), QSerialPort::OddParity);
    m_ui->parityBox->addItem(tr("Mark"), QSerialPort::MarkParity);
    m_ui->parityBox->addItem(tr("Space"), QSerialPort::SpaceParity);

    m_ui->stopBitsBox->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_OS_WIN
    m_ui->stopBitsBox->addItem(tr("1.5"), QSerialPort::OneAndHalfStop);
#endif
    m_ui->stopBitsBox->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    m_ui->flowControlBox->addItem(tr("None"), QSerialPort::NoFlowControl);
    m_ui->flowControlBox->addItem(tr("RTS/CTS"), QSerialPort::HardwareControl);
    m_ui->flowControlBox->addItem(tr("XON/XOFF"), QSerialPort::SoftwareControl);
}

void SettingsDialog::fillPortsInfo()
{
    m_ui->serialPortInfoListBox->clear();
    const QString blankString = tr(::blankString);
    const auto infos = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &info : infos) {
        QStringList list;
        const QString description = info.description();
        const QString manufacturer = info.manufacturer();
        const QString serialNumber = info.serialNumber();
        const auto vendorId = info.vendorIdentifier();
        const auto productId = info.productIdentifier();
        list << info.portName()
             << (!description.isEmpty() ? description : blankString)
             << (!manufacturer.isEmpty() ? manufacturer : blankString)
             << (!serialNumber.isEmpty() ? serialNumber : blankString)
             << info.systemLocation()
             << (vendorId ? QString::number(vendorId, 16) : blankString)
             << (productId ? QString::number(productId, 16) : blankString);

        m_ui->serialPortInfoListBox->addItem(list.constFirst(), list);
    }

    m_ui->serialPortInfoListBox->addItem(tr("Custom"));
}

void SettingsDialog::updateSettings()
{
    m_currentSettings.name = m_ui->serialPortInfoListBox->currentText();

    if (m_ui->baudRateBox->currentIndex() == 4) {
        m_currentSettings.baudRate = m_ui->baudRateBox->currentText().toInt();
    } else {
        const auto baudRateData = m_ui->baudRateBox->currentData();
        m_currentSettings.baudRate = baudRateData.value<QSerialPort::BaudRate>();
    }
    m_currentSettings.stringBaudRate = QString::number(m_currentSettings.baudRate);

    const auto dataBitsData = m_ui->dataBitsBox->currentData();
    m_currentSettings.dataBits = dataBitsData.value<QSerialPort::DataBits>();
    m_currentSettings.stringDataBits = m_ui->dataBitsBox->currentText();

    const auto parityData = m_ui->parityBox->currentData();
    m_currentSettings.parity = parityData.value<QSerialPort::Parity>();
    m_currentSettings.stringParity = m_ui->parityBox->currentText();

    const auto stopBitsData = m_ui->stopBitsBox->currentData();
    m_currentSettings.stopBits = stopBitsData.value<QSerialPort::StopBits>();
    m_currentSettings.stringStopBits = m_ui->stopBitsBox->currentText();

    const auto flowControlData = m_ui->flowControlBox->currentData();
    m_currentSettings.flowControl = flowControlData.value<QSerialPort::FlowControl>();
    m_currentSettings.stringFlowControl = m_ui->flowControlBox->currentText();

    m_currentSettings.localEchoEnabled = m_ui->localEchoCheckBox->isChecked();
}
