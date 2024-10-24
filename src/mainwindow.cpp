#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QInputDialog>

#include <QSettings>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QTimer>
#include <chrono>
#include <QByteArrayView>
#include <QEvent.h>

#include<QJsonArray>
#include<QJsonDocument>
#include<QJsonObject>


static constexpr std::chrono::seconds kWriteTimeout = std::chrono::seconds{5};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    ui_info(new FormInfo(this)),
    ui_person(new FormPerson(this)),
    ui_result(new FormResult(this)),
    ui_online(new FormOnline(this)),
    ui_com_settings(new SettingsDialog(this)),
    modelPerson (new TpersonModel()),
    modelCourse (new TcourseModel()),
    modelOrganization (new TorganizationModel()),
    modelGroup(new TgroupModel()),
    modelResult (new TresultModel()),
    proxyModelResult (new TResultProxyModel(this)),
    comport_timer(new QTimer(this)),

    comport(new QSerialPort(this)),
    postSender(new PostRequestSender(this)),
    maxFileNr(4)

{
    ui->setupUi(this);
    ui_info->hide();
    ui_person->hide();
    ui_result->hide();
    ui_online->hide();

    initActionsConnections();

    QObject::connect(ui->tablePerson, &QTableView::doubleClicked,this, &MainWindow::onTablePerson_dblclk);
    QObject::connect(ui->tableResult, &QTableView::doubleClicked,this, &MainWindow::onTableResult_dblclk);

    QObject::connect(this, &MainWindow::sendDataToDialog, ui_info, &FormInfo::recieveDataFromMain);
    QObject::connect(this, &MainWindow::sendDataPersonToDialog, ui_person, &FormPerson::recieveDataFromMain);
    QObject::connect(this, &MainWindow::sendDataResultToDialog, ui_result, &FormResult::recieveDataFromMain);

    QObject::connect(modelResult,&QAbstractItemModel::dataChanged,proxyModelResult,&QSortFilterProxyModel::invalidate);

    QObject::connect(comport, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    QObject::connect(comport_timer, &QTimer::timeout, this, &MainWindow::handleWriteTimeout);
    comport_timer->setSingleShot(true);

    QObject::connect(comport, &QSerialPort::readyRead, this, &MainWindow::readData);
    QObject::connect(comport, &QSerialPort::bytesWritten, this, &MainWindow::handleBytesWritten);

    createActionsAndConnections();
    createMenus();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete ui_info;
    delete ui_person;
    delete ui_result;
    delete comport_timer;
    delete comport;

    delete modelPerson;
    delete modelCourse;
    delete modelOrganization;
    delete modelGroup;
    delete modelResult;

    delete proxyModelResult;

    delete fileMenu;
    delete recentFilesMenu;
}

void MainWindow::initActionsConnections()
{
    connect(ui->act_connect_comport, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(ui->act_disconnect_comport, &QAction::triggered, this, &MainWindow::closeSerialPort);
    //connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    //connect(ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
    //connect(ui->actionClear, &QAction::triggered, m_console, &Console::clear);
    //connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    //connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

bool MainWindow::ReinitSerialPort(){
        // Read and set settings
    settingsComport = ui_com_settings->settings();
    comport->setPortName(settingsComport.name);
    comport->setBaudRate(settingsComport.baudRate);
    comport->setDataBits(settingsComport.dataBits);
    comport->setParity(settingsComport.parity);
    comport->setStopBits(settingsComport.stopBits);
    comport->setFlowControl(settingsComport.flowControl);
        // Connect
    return comport->open(QIODevice::ReadWrite);
}

void MainWindow::openSerialPort()
{
    if (!fl_connectedComport){
        // Connect
        if (ReinitSerialPort()) {
            fl_connectedComport = true;
            ui->act_connect_comport->setIcon(QIcon(":/rec/img/connect.png"));
            ui->act_comport_dialogset->setEnabled(false);
            showStatusMessage(tr("Connected to %1 : %2, %3")
                                  .arg(settingsComport.name, settingsComport.stringBaudRate, settingsComport.stringFlowControl));
        } else {
            QMessageBox::critical(this, tr("Error"), comport->errorString());
            showStatusMessage(tr("Open error"));
        }
    }
    else {
        // Disconnect
        fl_connectedComport = false;
        if (comport->isOpen()) comport->close();
        ui->act_connect_comport->setIcon(QIcon(":/rec/img/disconnect.png"));
        ui->act_comport_dialogset->setEnabled(true);
        showStatusMessage(tr("Disconnected"));
    }
}

void MainWindow::closeSerialPort()
{
    if (comport->isOpen()) comport->close();
    //m_console->setEnabled(false);
    ui->act_connect_comport->setEnabled(true);
    ui->act_disconnect_comport->setEnabled(false);
    ui->act_connect_comport->setIcon(QIcon(":/rec/img/connect.png"));
    ui->act_disconnect_comport->setIcon(QIcon(":/rec/img/disconnect.png"));
    ui->act_comport_dialogset->setEnabled(true);
    showStatusMessage(tr("Disconnected"));
}

void MainWindow::writeData(const QByteArray &data)
{
    const qint64 written = comport->write(data);
    if (written == data.size()) {
        m_bytesToWrite += written;
        comport_timer->start(kWriteTimeout);
    } else {
        const QString error = tr("Failed to write all data to port %1.\n"
                                 "Error: %2").arg(comport->portName(),
                                       comport->errorString());
        showWriteError(error);
    }
}

void MainWindow::readData()
{
    dataFromComport.append(comport->readAll());
    //QByteArray subString("\r\n");
    QByteArray subStringStop("#");
    QByteArray subStringStart("$");
    qsizetype indexStop = dataFromComport.indexOf(subStringStop);
    qsizetype indexStart = dataFromComport.indexOf(subStringStart);

    if (indexStop > -1) {
        QByteArray ba("");
        ba = dataFromComport.mid(indexStart+1, 12);
        st_comport_result* m = reinterpret_cast<st_comport_result*>(ba.data());

        int cardNum = m->cardNum;

        if (cardNum < 1){
            QMessageBox::warning(this, "Внимание!", "Чип не инициализирован!");
        }
        else {
            if (pSEvent->checkingCardNumInResult(cardNum)) {
                QMessageBox::warning(this, "Внимание!", "Этот чип уже был считан ранее");
            }
            else {
                int bib = pSEvent->getBibFromCardNum(cardNum);
                if ( bib == -1) {
                    qDebug() << "person not found";

                    bool button = false;
                    bib = QInputDialog::getInt(this, "input","bib",0,0,1000000,1,&button);

                    if (button) {
                        int carrent_cardNum_bib = pSEvent->getCardNumFromBib(bib);

                        if (carrent_cardNum_bib < 1)
                            pSEvent->setCardNumFromBib(bib, cardNum);
                        else {
                            if ((carrent_cardNum_bib > 0)&&(carrent_cardNum_bib != cardNum)){
                                QMessageBox msgBox;
                                msgBox.setText("Заменить номер чипа?");
                                msgBox.setInformativeText("Ok - заменить\n Отмена - оставить без изменений");
                                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                                msgBox.setIcon(QMessageBox::Warning);
                                msgBox.setDefaultButton(QMessageBox::Cancel);
                                int change_cardnum = msgBox.exec();
                                if (change_cardnum == QMessageBox::Ok){
                                    pSEvent->clearBibInResult(carrent_cardNum_bib);
                                    pSEvent->setCardNumFromBib(bib, cardNum);
                                }
                                else bib = -1;
                            }
                        }
                    }
                }

                if (bib != -1){
                    pSEvent->addResult(bib, ba);

                    emit modelResult->dataChanged(modelResult->index(0,0),modelResult->index(0,0));

                    emit proxyModelResult->dataChanged(
                        proxyModelResult->index(0,0),
                        proxyModelResult->index(
                            proxyModelResult->rowCount(),
                            proxyModelResult->columnCount()));
                    //*/
                    emit proxyModelResult->layoutChanged();
                    proxyModelResult->sort(TresultModel::ColNumTableResult::CResult);
                    ui->tableResult->update();
                    ui_log_msg("Считан и добавлен чип " + QString::number(cardNum) +
                               " для спортсмена " + QString::number(bib));
                } else {
                    ui_log_msg("Считан чип, но не добавлен");
                }
            }
        }
        dataFromComport.remove(0,indexStop+2);
    }
    //m_console->putData(data);
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), comport->errorString());
        closeSerialPort();
    }
}

void MainWindow::handleBytesWritten(qint64 bytes)
{
    m_bytesToWrite -= bytes;
    if (m_bytesToWrite == 0)
      comport_timer->stop();
}


void MainWindow::handleWriteTimeout()
{
    const QString error = tr("Write operation timed out for port %1.\n"
                             "Error: %2").arg(comport->portName(),
                                   comport->errorString());
    showWriteError(error);
}

void MainWindow::showStatusMessage(const QString &message)
{
    ui->statusbar->showMessage(message);
    ui_log_msg(message);
}

void MainWindow::showWriteError(const QString &message)
{
    QMessageBox::warning(this, tr("Warning"), message);
}

void MainWindow::saveSettings()
{
    QSettings settings("set.ini", QSettings::IniFormat);
    settings.beginGroup("gui");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("pos",pos());
    settings.endGroup();
}

void MainWindow::loadSettings()
{
    QSettings settings("set.ini", QSettings::IniFormat);
    settings.beginGroup("gui");
    const auto geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty())
        setGeometry(200, 200, 400, 400);
    else
        restoreGeometry(geometry);

    /*const auto position = settings.value("pos", QByteArray()).toByteArray();
    if (!position.isEmpty())
        //setGeometry(200, 200, 400, 400);
    //else
        restorePosition/ (position);*/
    settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    saveSettings();
    QMainWindow::closeEvent(e);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    QWidget* focusedWidget =qApp->focusWidget();

    if ((e->key() == Qt::Key_K)&&(e->modifiers() & Qt::ControlModifier)){
        ui_log_msg("Ctrl+K");
        if (focusedWidget != nullptr){
            ui_log_msg(focusedWidget->objectName());
            if (focusedWidget->objectName() == "tableResult"){
                if (QTableView *table = dynamic_cast<QTableView*>(focusedWidget)) {
                    QModelIndex index = table->currentIndex();
                    int row = index.row();
                    QVariant value = index.model()->data(index.model()->index(row,4), Qt::DisplayRole);
                    if (value.canConvert<QString>()) {
                        QString data = value.value<QString>();
                        //qDebug() << data;
                        requestOnline(data);
                    }
                }
            }
        }
    }
    if ((e->key() == Qt::Key_Insert)){//&&(e->modifiers() & Qt::ControlModifier)){
        ui_log_msg("insert");

        if (focusedWidget != nullptr){
            ui_log_msg(focusedWidget->objectName());
            if (focusedWidget->objectName() == "tablePerson"){
                ui_person->show();
            }
        }
    }
}

void MainWindow::ui_log_msg(const QString& str){
    ui->msg_log->append(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss.zzz") + " -> " + str);
}


void MainWindow::on_act_quit_triggered()
{
    QApplication::quit();
}


void MainWindow::on_act_open_triggered()
{
    ui_log_msg("on_act_open_triggered");
    QString file_name = QFileDialog::getOpenFileName(this, tr("Открыть событие"), QDir::currentPath(), tr("Событие (*.json)"));

    if (!file_name.isEmpty()){
        ui_log_msg(file_name);
        open_JSON(file_name);
    }
}


void MainWindow::on_act_Sportorg_JSON_triggered(){
    ui_log_msg("on_act_Sportorg_JSON_triggered");
    QString file_name = QFileDialog::getOpenFileName(this, "Импорт из Sportorg", QDir::currentPath(), "*.json");

    if (file_name != "") open_JSON(file_name);
}

void MainWindow::open_JSON(const QString &path){
    ui_log_msg("begin parse");
    if (path == "") return;

    pSEvent.reset();
    pSEvent = std::make_shared<QSportEvent>();
    pSEvent->importSportorgJSON(path);

    ui_person->update_sevent(pSEvent);
    ui_result->update_sevent(pSEvent);
    update_ui_table();
    adjustForCurrentFile(path);
}

void MainWindow::update_ui_table(){
    ui_log_msg("update_ui_table");

    //TpersonModel *modelPerson = new TpersonModel();
    modelPerson->reInit(pSEvent);
    ui->tablePerson->setSortingEnabled(true);
    ui->tablePerson->setModel(modelPerson);
    ui->tablePerson->resizeColumnsToContents();
    ui->tablePerson->verticalHeader()->setDefaultSectionSize(20);
    //ui->tablePerson->resizeRowsToContents();

    //TcourseModel *modelCourse = new TcourseModel();
    modelCourse->reInit(pSEvent);
    ui->tableDist->setModel(modelCourse);
    ui->tableDist->resizeColumnsToContents();
    ui->tableDist->verticalHeader()->setDefaultSectionSize(20);

    //TorganizationModel *modelOrganization = new TorganizationModel();
    modelOrganization->reInit(pSEvent);
    ui->tableOrg->setModel(modelOrganization);
    ui->tableOrg->resizeColumnsToContents();
    ui->tableOrg->verticalHeader()->setDefaultSectionSize(20);

    //TgroupModel *modelGroup = new TgroupModel();
    modelGroup->reInit(pSEvent);
    ui->tableGroup->setModel(modelGroup);
    ui->tableGroup->resizeColumnsToContents();
    ui->tableGroup->verticalHeader()->setDefaultSectionSize(20);


    update_ui_result();
}

void MainWindow::update_ui_result(){
    //TresultModel *modelResult = new TresultModel();
    //QSortFilterProxyModel* proxyModelResult = new QSortFilterProxyModel(this);

    modelResult->reInit(pSEvent);
    proxyModelResult->setSourceModel(nullptr);
    proxyModelResult->setSourceModel(modelResult);

    ui->tableResult->setModel(proxyModelResult);
    ui->tableResult->verticalHeader()->setMinimumWidth(25);
    ui->tableResult->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tableResult->setSortingEnabled(true);
    ui->tableResult->sortByColumn(0,Qt::AscendingOrder);
    ui->tableResult->resizeColumnsToContents();
    ui->tableResult->verticalHeader()->setDefaultSectionSize(20);
    proxyModelResult->sort(TresultModel::ColNumTableResult::CResult);

    ui->tableResult->update();
}


void MainWindow::on_act_info_triggered()
{
    if (!pSEvent->empty()) emit sendDataToDialog(pSEvent->getDataRace());
    ui_info->show();
}


void MainWindow::on_act_import_csv_orgeo_ru_triggered()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Импорт заявки", QDir::currentPath(), "*.csv");
    if (file_name == "") return;

    ui_log_msg(file_name);

    QFile file(file_name);
    if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
        qDebug() << "File not exists";
    } else {
        // Создаём поток для извлечения данных из файла
        QTextStream in(&file);
        in.setEncoding(QStringConverter::LastEncoding);
        // Считываем данные до конца файла
        while (!in.atEnd())
        {
            // ... построчно
            QString line = in.readLine();
            // Добавляем в модель по строке с элементами
            QList<QStandardItem *> standardItemsList;
            // учитываем, что строка разделяется точкой с запятой на колонки
            for (QString item : line.split(';')) {
                standardItemsList.append(new QStandardItem(item));
            }
            csvModel->insertRow(csvModel->rowCount(), standardItemsList);
        }
        file.close();
    }
}


void MainWindow::on_act_show_persons_triggered(){
    ui->tabWidget->setCurrentIndex(0);}
void MainWindow::on_act_show_results_triggered(){
    ui->tabWidget->setCurrentIndex(1);}
void MainWindow::on_act_show_groups_triggered(){
    ui->tabWidget->setCurrentIndex(2);}
void MainWindow::on_act_show_dists_triggered(){
    ui->tabWidget->setCurrentIndex(3);}
void MainWindow::on_act_show_orgs_triggered(){
    ui->tabWidget->setCurrentIndex(4);}

void MainWindow::on_act_comport_dialogset_triggered(){
    ui_com_settings->show();
}

void MainWindow::requestOnline(const QString &number)
{
    ui_log_msg("requestOnline");
    ui_log_msg(number);
    QJsonObject requestResultBody = pSEvent->getResultToOnline(number);
    QJsonArray persons;
    QJsonObject bodyrequest;
    persons.append(requestResultBody);
    bodyrequest["persons"] = persons;
    settingsOnline = ui_online->settings();
    postSender->sendRequest(this->settingsOnline.url,bodyrequest);
}


void MainWindow::on_act_online_triggered()
{
    ui_online->show();
}


void MainWindow::on_act_save_as_triggered()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Создать файл T-Event", QDir::currentPath(), "*.json");
    if (file_name == "") return;

    ui_log_msg("Save as file");
    ui_log_msg(file_name);
    pSEvent->exportSportorgJSON(file_name);
}

void MainWindow::onTablePerson_dblclk(const QModelIndex &index){
    ui_log_msg("on_doubleclick_person");
    int row = index.row();

    QString bib = modelPerson->data(modelPerson->index(row, 6),Qt::DisplayRole).toString();
    const st_person* data_ = pSEvent->getDataPersonBib(bib.toInt());
    if (data_ != nullptr){
        emit sendDataPersonToDialog(data_);
        ui_person->show();
    }

}

void MainWindow::onTableResult_dblclk(const QModelIndex &index)
{
    ui_log_msg("on_doubleclick_result");
    int row = index.row();

    QModelIndex index_res = ui->tableResult->currentIndex();

    QString bib = index_res.model()->data(
       index_res.model()->index(row, TresultModel::ColNumTableResult::CBib),Qt::DisplayRole).toString();
    const st_result* data_ = pSEvent->getDataResultBib(bib.toInt());
    if (data_ != nullptr){
        emit sendDataResultToDialog(data_);
        ui_result->show();
    }
}

