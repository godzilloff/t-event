#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QLabel>
#include <QSerialPort>
#include <QMenu>

#include "formonline.h"
#include "settingsdialog.h"
#include "postrequestsender.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class Document;
class SqlTableModel;
class FilterProxyModel;
class QTableView;
class QUndoView;
class CsvImporter;
class PersonEditDialog;
class DelegationEditDialog;
class DistanceEditDialog;
class AgeGroupEditDialog;
class SettingsDialog;
class PostRequestSender;
class FormOnline;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void saveSettings();
    void loadSettings();
    void ui_log_msg(const QString& str);
    Document* document() const { return m_document; }

protected:
    void closeEvent (QCloseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

signals:
    void sendPtrToFilter();
    void sendDataOnlineToDialog();

private slots:
    // COM порт
    // bool ReinitSerialPort();
    // void openSerialPort();
    // void closeSerialPort();
    // void writeData(const QByteArray &data);
    // void readData();
    // void sendDataComport(const QByteArray &data);
    // void handleError(QSerialPort::SerialPortError error);
    // void handleBytesWritten(qint64 bytes);
    // void handleWriteTimeout();

    // Документ
    void onDocumentOpened();
    void onDocumentClosed();
    void onDocumentModified(bool modified);

    // Файловые операции
    void onNewDocument();
    void onOpenDocument();
    bool SaveDocument();
    bool SaveAsDocument();
    void onCloseDocument();

    // Undo/Redo
    // void onUndo();
    // void onRedo();
    void on_act_undo_triggered();
    void on_act_redo_triggered();

    // Работа с записями
    void onAddRecord();
    void onEditRecord();
    void onDeleteRecord();
    void onDoubleClickRecord(const QModelIndex& index);

    // Вкладки и поиск
    void onTabChanged(int index);
    void onSearchTextChanged(const QString& text);

    // Импорт CSV
    // void onCsvImportProgress(int current, int total);
    // void onCsvImportFinished(int successCount, int errorCount);
    // void onCsvImportError(const QString& error);
    void onAct_import_csv_orgeo_ru_triggered();

    // UI элементы (слоты для действий)
    // void on_act_quit_triggered();
    // void on_act_open_triggered();
     void openRecent();
    // void onShowFilter();
    // void onShowPrep();
    // void on_act_info_triggered();
    // void on_act_import_csv_orgeo_ru_triggered();
    // void on_act_show_persons_triggered();
    // void on_act_show_results_triggered();
    // void on_act_show_groups_triggered();
    // void on_act_show_dists_triggered();
    // void on_act_show_orgs_triggered();
    // void on_act_comport_dialogset_triggered();
    // void on_act_online_triggered();
    // void requestOnline(const QString &number);
    // void on_act_save_as_triggered();
    // void onTablePerson_dblclk(const QModelIndex &index);
    // void onTableResult_dblclk(const QModelIndex &index);
    // void saveSE(const QString path);
    void on_act_save_triggered();
    void updateTime();
    // void on_onOpenCsvSecretarStOne_triggered();
    // void on_onOpenCsvSecretarStTwo_triggered();
    // void on_onOpenCsvSecretarStFour_triggered();
    //void on_act_Sync_triggered();

    // Обновление данных
    // void onRecordInserted(const QString& tableName, qint64 id);
    // void onRecordUpdated(const QString& tableName, qint64 id);
    // void onRecordDeleted(const QString& tableName, qint64 id);
    // void onCompetitionChanged(qint64 competitionId);


private:
    // Инициализация
    void initActionsConnections();
    void createActionsAndConnections();
    void createMenus();
    void setupUi();
    void setupTimerStatusBar();
    void setupConnections();
    //void setupConnectionsComport();
    void setupMenuBar();
    void initializeForDocument();
    void closeCurrentDocument();

    // Модели и таблицы
    void setupModels();
    void clearModels();
    void connectModelsToWidgets();
    void setupColumnHeaders(SqlTableModel* model, const QString& tableName);
    void setupHiddenColumns(SqlTableModel* model, const QString& tableName);
    void setupColumnVisibility();
    void setupTableViewHeaders();
    void setupDelegates();
    void updateCompetitionFilters();

    void debugTableStructure(const QString& tableName);

    // Работа с данными
    //void checkAndInitDatabase();
    //void checkViewStructure();
    //void checkFinalTableDisplay();
    //void debugCheckModels();
    //void debugHiddenColumns();
    void refreshAllTables();
    void refreshTable(const QString& tableName);
    int findTabIndexByTableName(const QString& tableName) const;
    bool hasCompetitionIdField(const QString& tableName);

    // Вспомогательные методы
    void showStatusMessage(const QString &message);
    void showWriteError(const QString &message);
    void adjustForCurrentFile(const QString& filePath);
    void updateRecentActionList();
    void updateWindowTitle();
    void updateStatusBar();
    void needSave();
    void notNeedSave();
    bool confirmUnsavedChanges();

    // Импорт CSV
    void importCsvWithUndo(const QString& filePath);
    void showImportResults(int successCount, int errorCount, const QString& details = QString());
    void onCsvImportFinished(int successCount, int errorCount);
    void onCsvImportError(const QString& error);

    // Получение виджетов
    QTableView* getCurrentTableView() const;
    QTableView* getTableViewForTab(int tabIndex) const;
    QString getTableNameForTab(int tabIndex) const;

    // Диалоги
    void showEditDialog(const QString& tableName, qint64 recordId = -1);
    QDialog* createEditDialog(const QString& tableName, qint64 recordId = -1);

    // Открытие документов
    void OpenTEvent(QString fpath);
    void createNewCompetition();

private:
    // UI элементы
    Ui::MainWindow *ui;
    QLabel *statusbar_msg;
    QLabel *clock_time;

    // Диалоги
    FormOnline* ui_online;
    SettingsDialog* ui_com_settings;

    // COM порт
    bool fl_connectedComport;
    SettingsDialog::Settings settingsComport;
    QByteArray dataFromComport;
    qint64 m_bytesToWrite;
    QTimer *comport_timer;
    QTimer *clock_timer;
    QSerialPort *comport;
    PostRequestSender *postSender;

    // Файлы и документы
    const int maxFileNr;
    QMenu* fileMenu;
    QMenu* recentFilesMenu;
    QList<QAction*> recentFileActionList;
    QString currentFilePath;
    bool flag_need_save;
    Document* m_document;
    CsvImporter* m_csvImporter;

    // Модели данных
    QMap<QString, SqlTableModel*> m_tableModels;
    QMap<QString, FilterProxyModel*> m_proxyModels;
    QMap<int, QString> m_tabTableMap;

    // Undo/Redo
    QUndoView* m_undoView;

    // Флаги состояния
    bool m_modelsInitialized;
    bool m_tablesConnected;
    QString m_currentTable;
    bool m_isModified;
};

#endif // MAINWINDOW_H
