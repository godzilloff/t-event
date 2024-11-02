#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#include "qsportevent.h"
#include "forminfo.h"
#include "formperson.h"
#include "formprepar.h"
#include "formresult.h"
#include "formonline.h"
#include "settingsdialog.h"

#include "postrequestsender.h"

#include "tpersonmodel.h"
#include "tcoursemodel.h"
#include "torganizationmodel.h"
#include "tgroupmodel.h"
#include "tresultmodel.h"

//#include "tpersonmodel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void createNewSEvent();

    void saveSettings();
    void loadSettings();

    void open_JSON(const QString &path);
    void update_ui_table();
    void update_ui_person();
    void update_ui_result();
    void update_ptr();
    void update_table();

    TpersonModel *modelPerson = nullptr;
    TcourseModel *modelCourse = nullptr;
    TorganizationModel *modelOrganization = nullptr;
    TgroupModel *modelGroup = nullptr;
    TresultModel *modelResult = nullptr;

    TPersonProxyModel* proxyModelPerson = nullptr;
    TResultProxyModel* proxyModelResult = nullptr;

    void ui_log_msg(const QString& str);

signals:
    void sendDataToDialog(const st_race& data_);
    void sendDataPersonToDialog(const st_person* data_);
    void sendDataResultToDialog(const st_result* data_);
    void sendDataOnlineToDialog();


protected:
    void closeEvent (QCloseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    bool ReinitSerialPort();
    void openSerialPort();
    void closeSerialPort();
    //void about();
    void writeData(const QByteArray &data);
    void readData();

    void handleError(QSerialPort::SerialPortError error);
    void handleBytesWritten(qint64 bytes);
    void handleWriteTimeout();

    void on_act_quit_triggered();
    void on_act_open_triggered();

    void openRecent();
    void onShowPrep();

    void on_act_Sportorg_JSON_triggered();
    void on_act_info_triggered();
    void on_act_import_csv_orgeo_ru_triggered();

    void on_act_show_persons_triggered();
    void on_act_show_results_triggered();
    void on_act_show_groups_triggered();
    void on_act_show_dists_triggered();
    void on_act_show_orgs_triggered();

    void on_act_comport_dialogset_triggered();
    void on_act_online_triggered();

    void requestOnline(const QString &number);

    void on_act_save_as_triggered();

    void onTablePerson_dblclk(const QModelIndex &index);
    void onTableResult_dblclk(const QModelIndex &index);

    void on_act_save_triggered();

private:
    void initActionsConnections();
    void showStatusMessage(const QString &message);
    void showWriteError(const QString &message);

private:
    Ui::MainWindow *ui;
    std::shared_ptr<QSportEvent> pSEvent;

    FormInfo* ui_info = nullptr;
    FormPerson* ui_person = nullptr;
    FormPrepar* ui_prepar = nullptr;
    FormResult* ui_result = nullptr;
    FormOnline* ui_online = nullptr;
    SettingsDialog* ui_com_settings = nullptr;

    bool fl_connectedComport = false;
    SettingsDialog::Settings settingsComport;

    QByteArray dataFromComport;
    qint64 m_bytesToWrite = 0;
    QTimer *comport_timer = nullptr;

    QSerialPort *comport = nullptr;
    PostRequestSender *postSender = nullptr;

    const int maxFileNr;
    QMenu* fileMenu;
    QMenu* recentFilesMenu;
    QList<QAction*> recentFileActionList;

    QString currentFilePath;
    bool flag_need_save = false;

    void createActionsAndConnections();
    void createMenus();

    void adjustForCurrentFile(const QString& filePath);
    void updateRecentActionList();
    void updateWindowTitle();
    void needSave();
    void notNeedSave();
};
#endif // MAINWINDOW_H
