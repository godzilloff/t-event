#include "formfilter.h"
#include "ui_formfilter.h"

#include "mainwindow.h"

FormFilter::FormFilter(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FormFilter),
    org(QString{}),group(QString{})
{
    ui->setupUi(this);

    connect(ui->cb_group, &QComboBox::currentIndexChanged, this, &FormFilter::updateStrGroup);
    connect(ui->cb_org, &QComboBox::currentIndexChanged, this, &FormFilter::updateStrOrg);
}

FormFilter::~FormFilter()
{
    delete ui;
}

void FormFilter::update_sevent(std::shared_ptr<QSportEvent> &pSEvent)
{
    ptrSEvent = pSEvent;
}

void FormFilter::update_organization()
{
    QStringList names = ptrSEvent->getNamesOrganization();
    names.push_front(QString{});
    ui->cb_org->clear();
    ui->cb_org->addItems(names);
}

void FormFilter::update_group()
{
    QStringList names = ptrSEvent->getNamesGroup();
    names.push_front(QString{});
    ui->cb_group->clear();
    ui->cb_group->addItems(names);
}

void FormFilter::recieveDataFromMain()
{
    update_organization();
    update_group();
}

void FormFilter::updateStrOrg(int index)
{
    MainWindow *mainWindow = static_cast<MainWindow*>(parent());
    mainWindow->proxyModelPerson->setOrganization(ui->cb_org->currentText());
}

void FormFilter::updateStrGroup(int index)
{
    MainWindow *mainWindow = static_cast<MainWindow*>(parent());
    mainWindow->proxyModelPerson->setGroup(ui->cb_group->currentText());
}
