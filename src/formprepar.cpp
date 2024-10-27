#include "formprepar.h"
#include "ui_formprepar.h"

#include <QPushButton>

#include "mainwindow.h"

FormPrepar::FormPrepar(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FormPrepar)
{
    ui->setupUi(this);

    QObject::connect(ui->btNewBib, &QPushButton::clicked,this, &FormPrepar::onSetBib);
}

FormPrepar::~FormPrepar()
{
    delete ui;
}

void FormPrepar::update_sevent(std::shared_ptr<QSportEvent> &pSEvent)
{
    ptrSEvent = pSEvent;
}

void FormPrepar::onSetBib()
{
    MainWindow *mainWindow = static_cast<MainWindow*>(parent());

    int cnt = mainWindow->proxyModelPerson->rowCount();
    QVector<QString> vID;
    vID.reserve(cnt);

    for (int i = 0; i < cnt; i++) {
        QModelIndex index = mainWindow->proxyModelPerson->index(i, TpersonModel::ColNumTablePerson::CID);
        if (index.isValid()) {
            vID.append(mainWindow->proxyModelPerson->data(index,Qt::DisplayRole).toString());
        }
    }

    int ii = ui->spinBox->value();
    for (const QString &id : vID) {
        ptrSEvent->setBibFromId(id, ii++);
    }
}
