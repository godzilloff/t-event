#include "formprepar.h"
#include "ui_formprepar.h"

#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

#include "mainwindow.h"

FormPrepar::FormPrepar(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FormPrepar)
{
    ui->setupUi(this);

    QObject::connect(ui->btNewBib, &QPushButton::clicked,this, &FormPrepar::onSetBib);
    QObject::connect(ui->btClearCardNum, &QPushButton::clicked,this, &FormPrepar::onClearAllCardNum);
    QObject::connect(ui->btImportCardNum, &QPushButton::clicked,this, &FormPrepar::onSetCardNum);
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
    mainWindow->update_table();
}

void FormPrepar::onClearAllCardNum()
{
    QMessageBox msgBox;
    msgBox.setText("Очистить ВСЕ номера чипов у спортсменов?");
    msgBox.setInformativeText("Ok - очистить\n Отмена - отменить");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int change_clear = msgBox.exec();
    if (change_clear != QMessageBox::Ok)
        return;

    ptrSEvent->clearAllCardNum();
    MainWindow *mainWindow = static_cast<MainWindow*>(parent());
    mainWindow->update_table();
}

void FormPrepar::onSetCardNum()
{
    MainWindow *mainWindow = static_cast<MainWindow*>(parent());
    mainWindow->ui_log_msg("onSetCardNum");
    QString file_name = QFileDialog::getOpenFileName(this, tr("Открыть файл импорта"), QDir::currentPath(), tr("Файл импорта (*.csv)"));

    if (!file_name.isEmpty()){
        mainWindow->ui_log_msg(file_name);
        ptrSEvent->setCardNumFromCsv(file_name);
    }
    mainWindow->update_table();
}
