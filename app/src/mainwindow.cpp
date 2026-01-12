#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "Document.h"
#include "SqlTableModel.h"
#include "UndoStack.h"
#include "PersonEditDialog.h"
#include "DelegationEditDialog.h"
#include "DatabaseManager.h"

#include "dialogs/PersonEditDialog.h"
#include "dialogs/DelegationEditDialog.h"
#include "dialogs/DistanceEditDialog.h"
#include "dialogs/AgeGroupEditDialog.h"

#include "CsvImporter.h"
#include <QFileDialog>
#include <QProgressDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QStandardPaths>

#include <QInputDialog>

#include <QSettings>
#include <QDate>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QTimer>
#include <QLabel>
//#include <chrono>
#include <QByteArrayView>
#include <QEvent.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QUndoStack>
#include <QUndoView>
#include <QDockWidget>

#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlRelationalDelegate>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStatusBar>
#include <QDebug>

#include <QStandardPaths>

// static const QMap<QString, QString> TABLE_DISPLAY_NAMES = {
//     {"participants", ("Участники")},
//     {"delegations", ("Делегации")},
//     {"distances", ("Дистанции")},
//     {"age_groups", ("Возрастные группы")},
//     {"results", ("Результаты")}
// };

//static constexpr std::chrono::seconds kWriteTimeout = std::chrono::seconds{5};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    statusbar_msg(new QLabel),
    clock_time(new QLabel),

    ui_online(new FormOnline(this)),
    ui_com_settings(new SettingsDialog(this)),

    fl_connectedComport(false),
    comport_timer(new QTimer(this)),
    clock_timer(new QTimer(this)),
    comport(new QSerialPort(this)),
    postSender(new PostRequestSender(this)),
    maxFileNr(4),
    flag_need_save(false),
    m_document(nullptr),
    //m_document(new Document(this)),
    m_csvImporter(new CsvImporter(this)),
    m_modelsInitialized(false),
    m_tablesConnected(false),
    m_isModified(false)
{
    ui->setupUi(this);

    qDebug() << "=== НАЧАЛО ИНИЦИАЛИЗАЦИИ ===";

    // Инициализируем соответствие вкладок и таблиц
    m_tabTableMap = {
        {0, "participants"},      // tab1Person -> participants
        {1, "results"},           // tab2Result -> results
        {2, "age_groups"},        // tab3Group -> age_groups
        {3, "distances"},         // tab4Dist -> distances
        {4, "delegations"}        // tab5Org -> delegations
    };

    qDebug() << "=== КОНСТРУКТОР MainWindow ===";

    // ТОЛЬКО базовая настройка UI
    setupUi();
    setupTimerStatusBar();
    //setupConnections();
    //setupConnectionsComport();
    setupToolbars();
    setupMenuBar();

    createActionsAndConnections();
    createMenus();
    initActionsConnections();

    // Инициализируем undo view
    m_undoView = new QUndoView(&UndoStack::instance(), this);
    QDockWidget* undoDock = new QDockWidget(tr("История изменений"), this);
    undoDock->setWidget(m_undoView);
    addDockWidget(Qt::RightDockWidgetArea, undoDock);

    qDebug() << "=== КОНСТРУКТОР ЗАВЕРШЕН ===\n";
}

MainWindow::~MainWindow()
{
    closeCurrentDocument();

    delete ui;
    delete comport_timer;
    delete comport;

    delete fileMenu;
    delete recentFilesMenu;
}

void MainWindow::closeCurrentDocument()
{
    if (m_document) {
        // Отключаем все соединения с document
        disconnect(m_document, nullptr, this, nullptr);

        // Очищаем модели и состояние
        onDocumentClosed();

        // Удаляем документ
        delete m_document;
        m_document = nullptr;

        // Очищаем DatabaseManager (если это синглтон, который нужно переинициализировать)
        DatabaseManager::instance().closeDatabase();

        // Сбрасываем флаги
        m_isModified = false;
        //m_currentFilePath.clear();
        currentFilePath.clear();
        flag_need_save = false;
    }
}

void MainWindow::updateTime()
{
    QString timeStr = QTime::currentTime().toString("HH:mm:ss");
    clock_time->setText(timeStr);
}

void MainWindow::ui_log_msg(const QString& str){
    ui->msg_log->append(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss.zzz") + " -> " + str);
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

    settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (flag_need_save){
        QString titleBox = "Вопрос";
        QString fileName = (this->currentFilePath != "")? " ["+this->currentFilePath +"]" : "";
        QString question = "Сохранить изменения в файл"+ fileName +"?";
        QMessageBox::StandardButton button =
            QMessageBox::question(this,titleBox,question,QMessageBox::Save | QMessageBox::No | QMessageBox::Cancel );
        switch (button) {
        case QMessageBox::Cancel:
            e->ignore();
            return;
            break;
        case QMessageBox::Save:
            on_act_save_triggered();
            break;
        default:
            break;
        }
    }

    // программа закрывается
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
                        qDebug() << data;
                        //requestOnline(data);
                    }
                }
            }
        }
    }
    if ((e->key() == Qt::Key_Insert)){//&&(e->modifiers() & Qt::ControlModifier)){
        ui_log_msg("insert");

        // if (focusedWidget != nullptr){
        //     ui_log_msg(focusedWidget->objectName());
        //     if (focusedWidget->objectName() == "tablePerson"){
        //         ui_person->show();
        //     }
        // }
    }

    // Undo/Redo через клавиатуру
    if (e->key() == Qt::Key_Z && e->modifiers() & Qt::ControlModifier) {
        if (e->modifiers() & Qt::ShiftModifier) {
            onRedo(); // Ctrl+Shift+Z = Redo
        } else {
            onUndo(); // Ctrl+Z = Undo
        }
        e->accept();
        return;
    }

    if (e->key() == Qt::Key_Y && e->modifiers() & Qt::ControlModifier) {
        onRedo(); // Ctrl+Y = Redo
        e->accept();
        return;
    }

    QMainWindow::keyPressEvent(e);
}

QTableView* MainWindow::getTableViewForTab(int tabIndex) const
{
    switch (tabIndex) {
    case 0: return ui->tablePerson;   // Участники
    case 1: return ui->tableResult;   // Результаты
    case 2: return ui->tableGroup;    // Возрастные группы
    case 3: return ui->tableDist;     // Дистанции
    case 4: return ui->tableOrg;      // Делегации
    default: return nullptr;
    }
}

void MainWindow::initializeForDocument()
{
    if (!m_document || !m_document->isOpen()) {
        qDebug() << "initializeForDocument: документ не открыт";
        return;
    }

    qint64 competitionId = m_document->competitionId();
    qDebug() << "=== ИНИЦИАЛИЗАЦИЯ ДЛЯ ДОКУМЕНТА ===";
    qDebug() << "Competition ID:" << competitionId;

    // 1. Создаем модели (если нужно)
    if (m_tableModels.isEmpty()) {
        qDebug() << "Создаю модели...";
        setupModels(); // В setupModels() теперь встроена установка фильтров и select()
    } else {
        qDebug() << "Модели уже созданы, обновляю фильтры...";

        // Явно обновляем фильтры
        updateCompetitionFilters();
    }

    // 2. Настраиваем делегаты
    setupDelegates();

    // 3. Настраиваем видимость столбцов
    setupColumnVisibility();

    // 4. Обновляем UI
    updateWindowTitle();
    updateStatusBar();

    // 5. Дополнительное обновление через 100мс
    QTimer::singleShot(100, this, [this]() {
        refreshAllTables();
    });

    qDebug() << "=== ИНИЦИАЛИЗАЦИЯ ЗАВЕРШЕНА ===";
}

void MainWindow::setupDelegates()
{
    if (m_proxyModels.contains("participants")) {
        // Используем QSqlRelationalDelegate для автоматического отображения
        // выпадающих списков для связанных полей
        QSqlRelationalDelegate* delegate = new QSqlRelationalDelegate(this);
        ui->tablePerson->setItemDelegate(delegate);

        // Включаем редактирование по двойному клику
        ui->tablePerson->setEditTriggers(QAbstractItemView::DoubleClicked |
                                         QAbstractItemView::EditKeyPressed);
    }
}

bool MainWindow::hasCompetitionIdField(const QString& tableName)
{
    // Список таблиц, которые должны иметь competition_id
    static const QSet<QString> tablesWithCompetitionId = {
        "participants", "delegations", "distances", "age_groups", "competitions"
    };

    return tablesWithCompetitionId.contains(tableName);
}

void MainWindow::setupModels()
{
    qDebug() << "\n=== СОЗДАНИЕ МОДЕЛЕЙ ===";

    // Очищаем старые модели если есть
    clearModels();

    // Получаем базу данных
    auto& dbManager = DatabaseManager::instance();
    QSqlDatabase db = dbManager.database();

    if (!db.isOpen()) {
        qDebug() << "ОШИБКА: База данных не открыта!";
        return;
    }

    // Создаем модели для всех таблиц
    QStringList tables = {"participants", "delegations", "distances", "age_groups", "results"};

    for (const QString& tableName : tables) {
        qDebug() << "Создаю модель для:" << tableName;

        // 1. Создаем SqlTableModel
        SqlTableModel* model = new SqlTableModel(this, db);

        // ВАЖНО: Для participants используем VIEW вместо таблицы!
        if (tableName == "participants") {
            model->setTable("v_participants_details"); // Используем VIEW!
        } else {
            model->setTable(tableName);
        }

        // 2. Настраиваем заголовки столбцов
        setupColumnHeaders(model, tableName);

        // 3. Скрываем ненужные столбцы
        setupHiddenColumns(model, tableName);

        // 4. Устанавливаем стратегию редактирования
        model->setEditStrategy(QSqlTableModel::OnManualSubmit);

        // 5. Создаем FilterProxyModel
        FilterProxyModel* proxy = new FilterProxyModel(this);
        proxy->setSourceModel(model);

        // 6. Сохраняем
        m_tableModels[tableName] = model;
        m_proxyModels[tableName] = proxy;

        // Подключаем сигнал о загрузке данных
        connect(model, &SqlTableModel::dataLoaded, this, [this, tableName, model]() {
            qDebug() << "Данные загружены для таблицы:" << tableName;

            // Определяем соответствующую таблицу
            QTableView* tableView1 = nullptr;
            if (tableName == "participants") tableView1 = ui->tablePerson;
            else if (tableName == "results") tableView1 = ui->tableResult;
            else if (tableName == "age_groups") tableView1 = ui->tableGroup;
            else if (tableName == "distances") tableView1 = ui->tableDist;
            else if (tableName == "delegations") tableView1 = ui->tableOrg;

            if (tableView1 && model->rowCount() > 0) {
                // Ждем один цикл обработки событий
                QTimer::singleShot(0, tableView1, [tableView1]() {
                    tableView1->resizeColumnsToContents();
                    qDebug() << "Столбцы подогнаны для таблицы:" << tableView1->objectName();
                });
                tableView1->setEditTriggers(QAbstractItemView::NoEditTriggers);
            }
        });

        qDebug() << "  Создана модель и прокси для" << tableName;
    }

    // 7. Связываем с виджетами ОДИН РАЗ
    connectModelsToWidgets();

    // 8. Только после связывания устанавливаем фильтры и вызываем select()
    if (m_document && m_document->isOpen()) {
        qint64 competitionId = m_document->competitionId();
        qDebug() << "Устанавливаю фильтры competition_id =" << competitionId;

        for (auto it = m_tableModels.begin(); it != m_tableModels.end(); ++it) {
            if (it.value()) {
                const QString& tableName = it.key();

                // Для таблиц, которые должны фильтроваться по competition_id
                if (tableName == "participants" || tableName == "delegations" ||
                    tableName == "distances" || tableName == "age_groups") {

                    QString filter = QString("competition_id = %1").arg(competitionId);
                    qDebug() << "  Устанавливаю фильтр для" << tableName << ":" << filter;

                    it.value()->setFilter(filter);

                    // НЕМЕДЛЕННО вызываем select() после установки фильтра
                    if (!it.value()->select()) {
                        qDebug() << "Ошибка select() для" << tableName << ":"
                                 << it.value()->lastError().text();
                    } else {
                        qDebug() << "  Таблица" << tableName << "загружена, строк:"
                                 << it.value()->rowCount();
                    }
                } else if (tableName == "results") {
                    // Для results нужно отдельная логика
                    if (!it.value()->select()) {
                        qDebug() << "Ошибка select() для results:"
                                 << it.value()->lastError().text();
                    }
                }
            }
        }
    }

    qDebug() << "Создано моделей:" << m_tableModels.size();
    qDebug() << "=== СОЗДАНИЕ МОДЕЛЕЙ ЗАВЕРШЕНО ===\n";
}

void MainWindow::setupColumnHeaders(SqlTableModel* model, const QString& tableName)
{
    // Сначала получаем структуру модели
    model->select(); // Это нужно чтобы columnCount() работал правильно

    QMap<int, QString> headers;

    if (tableName == "participants") {

        // Проставляем заголовки по индексам
        headers = {
            {0, "ID"},
            {1, "ID соревнования"},
            {2, "Тип участника"},
            {3, "ФИО"},
            {4, "Название команды"},
            {5, "Номер"},
            {6, "Номер чипа"},
            {7, "Время старта"},
            {8, "Дата рождения"},
            {9, "Пол"},
            {10, "Размер команды"},
            {11, "Делегация"},      // delegation_name из VIEW
            {12, "Дистанция"},      // distance_name из VIEW
            {13, "Возрастная группа"}, // age_group_name из VIEW
            {14, "ID делегации"},
            {15, "ID дистанции"},
            {16, "ID возрастной группы"},
            {17, "Создано"},
            {18, "Обновлено"}
        };
    }
    else if (tableName == "delegations") {
        headers = {
            {0, "ID"},
            {1, "ID соревнования"},
            {2, "Название делегации"},
            {3, "Представитель"},
            {4, "Контакт"},
            {5, "Создано"},
            {6, "Обновлено"}
        };
    }
    else if (tableName == "distances") {
        headers = {
            {0, "ID"},
            {1, "ID соревнования"},
            {2, "Название дистанции"},
            {3, "Длина, км"},
            {4, "Контрольное время, мин"},
            {5, "Контрольные пункты"},
            {6, "Создано"},
            {7, "Обновлено"}
        };
    }
    else if (tableName == "age_groups") {
        headers = {
            {0, "ID"},
            {1, "ID соревнования"},
            {2, "Название группы"},
            {3, "Минимальный возраст"},
            {4, "Максимальный возраст"},
            {5, "Стоимость"},
            {6, "Создано"},
            {7, "Обновлено"}
        };
    }
    else if (tableName == "results") {
        headers = {
            {0, "ID"},
            {1, "ID участника"},
            {2, "Номер чипа"},
            {3, "Время старта"},
            {4, "Время финиша"},
            {5, "Результат"},
            {6, "Статус"},
            {7, "Создано"},
            {8, "Обновлено"}
        };
    }

    if (!headers.isEmpty()) {
        model->setColumnHeaders(headers);
    }
}

void MainWindow::updateCompetitionFilters()
{
    if (!m_document || !m_document->isOpen()) {
        return;
    }

    qint64 competitionId = m_document->competitionId();
    qDebug() << "Обновляю фильтры competition_id =" << competitionId;

    for (auto it = m_tableModels.begin(); it != m_tableModels.end(); ++it) {
        if (it.value()) {
            const QString& tableName = it.key();

            // Для таблиц, которые должны фильтроваться по competition_id
            if (tableName == "participants" || tableName == "delegations" ||
                tableName == "distances" || tableName == "age_groups") {

                QString filter = QString("competition_id = %1").arg(competitionId);
                qDebug() << "  Устанавливаю фильтр для" << tableName << ":" << filter;

                // Сначала сбрасываем старый фильтр
                it.value()->setFilter(QString());
                it.value()->select(); // Очищаем данные

                // Устанавливаем новый фильтр
                it.value()->setFilter(filter);

                // Вызываем select() и проверяем результат
                if (!it.value()->select()) {
                    qDebug() << "Ошибка select() для" << tableName << ":"
                             << it.value()->lastError().text();
                } else {
                    qDebug() << "  Таблица" << tableName << "загружена, строк:"
                             << it.value()->rowCount();
                }
            }
        }
    }

    // Обновляем прокси-модели
    for (auto proxy : m_proxyModels) {
        if (proxy) {
            proxy->invalidate();
        }
    }
}

void MainWindow::setupHiddenColumns(SqlTableModel* model, const QString& tableName)
{
    QList<int> hiddenColumns;

    if (tableName == "participants") {
        // Скрываем только действительно ненужные поля
        // НЕ скрываем competition_id - он нужен для фильтрации!
        // НЕ скрываем created_at и updated_at - они могут понадобиться

        hiddenColumns << 14 // delegation_id (дублирует delegation_name)
                      << 15 // distance_id (дублирует distance_name)
                      << 16 // age_group_id (дублирует age_group_name)
                      << 17 // created_at (скрываем если не нужен)
                      << 18; // updated_at (скрываем если не нужен)

        // Если competition_id должен быть скрыт от пользователя:
        // hiddenColumns << 1; // competition_id
    }
    else if (tableName == "delegations") {
        hiddenColumns //<< 0  // ID
                      << 1  // ID соревнования (можно скрыть)
                      << 5  // Создано
                      << 6; // Обновлено
    }
    else if (tableName == "distances") {
        hiddenColumns //<< 0  // ID
                      << 1  // ID соревнования
                      << 6  // Создано
                      << 7; // Обновлено
    }
    else if (tableName == "age_groups") {
        hiddenColumns //<< 0  // ID
                      << 1  // ID соревнования
                      << 6  // Создано
                      << 7; // Обновлено
    }
    else if (tableName == "results") {
        hiddenColumns //<< 0  // ID
                      << 7  // Создано
                      << 8; // Обновлено
    }
    model->hideColumns(hiddenColumns);
}

void MainWindow::setupColumnVisibility()
{
    for (auto it = m_proxyModels.begin(); it != m_proxyModels.end(); ++it) {
        const QString& tableName = it.key();
        FilterProxyModel* proxyModel = it.value();

        if (!m_tableModels.contains(tableName)) continue;

        SqlTableModel* sourceModel = m_tableModels[tableName];
        QList<int> visibleColumns;

        if (tableName == "participants") {
            // Учитываем структуру VIEW v_participants_details
            visibleColumns = {
                0,  // ID
                5,  // bib_number (Номер)
                3,  // full_name (ФИО)
                11, // delegation_name (Делегация)
                9,  // gender (Пол)
                8,  // birth_date (Дата рождения)
                12, // distance_name (Дистанция)
                13, // age_group_name (Возрастная группа)
                6,  // chip_number (Номер чипа)
                7  // start_time (Время старта)
                //2,  // participant_type (Тип участника)
                //4   // team_name (Название команды)
            };
        }
        else if (tableName == "delegations") {
            visibleColumns = {0, 2, 3, 4}; // name, representative, contact
        }
        else if (tableName == "distances") {
            visibleColumns = {0, 2, 3, 4, 5}; // name, length, control_time, control_points
        }
        else if (tableName == "age_groups") {
            visibleColumns = {0, 2, 3, 4, 5}; // name, min_age, max_age, price
        }
        else if (tableName == "results") {
            visibleColumns = {0, 2, 3, 4, 5, 6}; // chip_number, start_time, finish_time, result_time, status
        }

        // Фильтруем невалидные индексы
        visibleColumns.removeIf([](int col) { return col < 0; });

        // Проверяем, что столбцы существуют в модели
        QList<int> validColumns;
        for (int col : visibleColumns) {
            if (col < sourceModel->columnCount()) {
                validColumns.append(col);
            }
        }

        proxyModel->setVisibleColumns(validColumns);
    }
}

void MainWindow::setupTableViewHeaders()
{
    // Настраиваем порядок столбцов через перемещение заголовков
    if (m_proxyModels.contains("participants")) {
        QTableView* tableView = ui->tablePerson;

        // Если используем кастомный SQL запрос, порядок уже задан в запросе
        // Просто скрываем технические столбцы
        for (int i = 11; i <= 18; ++i) { // Скрываем технические столбцы
            tableView->setColumnHidden(i, true);
        }

        // Автоматически подгоняем ширину столбцов
        tableView->resizeColumnsToContents();
    }

    ui->tableResult->resizeColumnsToContents();
    ui->tableDist->resizeColumnsToContents();
    ui->tableGroup->resizeColumnsToContents();
    ui->tableOrg->resizeColumnsToContents();

    // Аналогично для других таблиц
}

void MainWindow::debugTableStructure(const QString& tableName)
{
    if (!m_tableModels.contains(tableName)) {
        qDebug() << "Модель для таблицы" << tableName << "не найдена";
        return;
    }

    // Выполним прямой SQL запрос для сравнения
    auto& dbManager = DatabaseManager::instance();
    QString sql = QString("SELECT * FROM %1 LIMIT 1").arg(tableName);

    QSqlQuery query(dbManager.database());
    if (query.exec(sql) && query.next()) {
        qDebug() << "=== ПРЯМОЙ SQL ЗАПРОС для" << tableName << "===";
        QSqlRecord rec = query.record();
        for (int i = 0; i < rec.count(); ++i) {
            qDebug() << "  Поле" << i << "->" << rec.fieldName(i)
                     << ":" << rec.value(i).toString();
        }
    }

    SqlTableModel* model = m_tableModels[tableName];
    qDebug() << "=== Структура таблицы" << tableName << "===";
    qDebug() << "Количество столбцов:" << model->columnCount();

    for (int i = 0; i < model->columnCount(); ++i) {
        QString fieldName = model->headerData(i, Qt::Horizontal).toString();
        qDebug() << "  Колонка" << i << "->" << fieldName;

        // Проверяем, есть ли отношение
        if (model->relation(i).isValid()) {
            QSqlRelation rel = model->relation(i);
            qDebug() << "    Это связанное поле:";
            qDebug() << "      Таблица:" << rel.tableName();
            qDebug() << "      Ключ:" << rel.indexColumn();
            qDebug() << "      Отображение:" << rel.displayColumn();
        }
    }

    // Проверим несколько первых строк
    int rowsToCheck = qMin(3, model->rowCount());
    qDebug() << "Первые" << rowsToCheck << "строк данных:";

    for (int row = 0; row < rowsToCheck; ++row) {
        qDebug() << "  Строка" << row << ":";
        for (int col = 0; col < model->columnCount(); ++col) {
            QVariant data = model->data(model->index(row, col));
            QString fieldName = model->headerData(col, Qt::Horizontal).toString();
            qDebug() << "    " << fieldName << ":" << data.toString();
        }
    }
}

void MainWindow::refreshAllTables()
{
    qDebug() << "\n=== ВЫЗОВ refreshAllTables() ===";

    if (!m_document->isOpen()) {
        qDebug() << "Документ не открыт, пропускаю обновление";
        return;
    }

    // Обновляем данные во всех моделях
    for (auto it = m_tableModels.begin(); it != m_tableModels.end(); ++it) {
        qDebug() << "Обновляю таблицу:" << it.key();
        try {
            if (it.value() && it.value()->database().isOpen()) {
                if (!it.value()->select()) {
                    qDebug() << "Ошибка обновления" << it.key() << ":"
                             << it.value()->lastError().text();
                } else {
                    qDebug() << "Таблица" << it.key() << "обновлена, строк:"
                             << it.value()->rowCount();
                }
            } else {
                qDebug() << "Модель для" << it.key() << "не инициализирована или БД не открыта";
            }
        } catch (const std::exception& e) {
            qCritical() << "Исключение при обновлении" << it.key() << ":" << e.what();
        } catch (...) {
            qCritical() << "Неизвестное исключение при обновлении" << it.key();
        }
    }

    // Обновляем текущую вкладку
    int currentTab = ui->tabWidget->currentIndex();
    QTableView* tableView = getTableViewForTab(currentTab);
    if (tableView && tableView->model()) {
        tableView->resizeColumnsToContents();
        tableView->viewport()->update();
    }

    updateStatusBar();

    qDebug() << "=== ЗАВЕРШЕНИЕ refreshAllTables() ===\n";
}

void MainWindow::refreshTable(const QString& tableName)
{
    qDebug() << "\n=== ВЫЗОВ refreshTable(" << tableName << ") ===";

    if (!m_document->isOpen()) {
        qDebug() << "Документ не открыт, пропускаю обновление таблицы" << tableName;
        return;
    }

    // Проверяем, есть ли модель для этой таблицы
    if (!m_tableModels.contains(tableName)) {
        qWarning() << "Модель для таблицы" << tableName << "не найдена";
        return;
    }

    QSqlTableModel* model = m_tableModels.value(tableName);
    if (!model) {
        qWarning() << "Модель для таблицы" << tableName << "пуста (nullptr)";
        return;
    }

    if (!model->database().isOpen()) {
        qWarning() << "База данных для модели" << tableName << "закрыта";
        return;
    }

    bool success = false;
    try {
        success = model->select();
    } catch (const std::exception& e) {
        qCritical() << "Исключение при обновлении таблицы" << tableName << ":" << e.what();
        return;
    } catch (...) {
        qCritical() << "Неизвестное исключение при обновлении таблицы" << tableName;
        return;
    }

    if (!success) {
        qWarning() << "Ошибка обновления таблицы" << tableName << ":"
                   << model->lastError().text();
    } else {
        qDebug() << "Таблица" << tableName << "успешно обновлена, строк:"
                 << model->rowCount();
    }

    // Если эта таблица отображается на текущей вкладке — обновляем вид
    int tabIndex = findTabIndexByTableName(tableName); // вам нужно реализовать эту функцию
    if (tabIndex != -1 && tabIndex == ui->tabWidget->currentIndex()) {
        QTableView* tableView = getTableViewForTab(tabIndex);
        if (tableView) {
            tableView->resizeColumnsToContents();
            tableView->viewport()->update();
        }
    }

    updateStatusBar();

    qDebug() << "=== ЗАВЕРШЕНИЕ refreshTable(" << tableName << ") ===\n";
}

int MainWindow::findTabIndexByTableName(const QString& tableName) const
{
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        // Предположим, что вы храните имя таблицы как свойство вкладки или виджета
        QVariant tableProp = ui->tabWidget->widget(i)->property("tableName");
        if (tableProp.isValid() && tableProp.toString() == tableName) {
            return i;
        }
    }
    return -1; // не найдено
}

// void MainWindow::checkAndInitDatabase()
// {
//     qDebug() << "\n=== ПРОВЕРКА И ИНИЦИАЛИЗАЦИЯ БАЗЫ ===";

//     // 1. Проверяем, открыта ли база
//     auto& dbManager = DatabaseManager::instance();
//     if (!dbManager.isOpen()) {
//         qDebug() << "База не открыта. Пробуем инициализировать...";

//         // Пробуем открыть базу по умолчанию
//         if (!dbManager.initialize()) {
//             qDebug() << "Не удалось инициализировать базу!";
//             qDebug() << "Попробуем создать новую...";

//             // Создаем новое соревнование через Document
//             if (m_document->createNew()) {
//                 qDebug() << "Новое соревнование создано!";
//                 qDebug() << "Путь:" << m_document->filePath();
//             } else {
//                 qDebug() << "Не удалось создать соревнование!";
//             }
//         } else {
//             qDebug() << "База успешно открыта";
//         }
//     } else {
//         qDebug() << "База уже открыта";
//         qDebug() << "Путь:" << dbManager.databasePath();
//     }

//     // 2. Проверяем таблицы
//     QStringList tables = dbManager.tableNames();
//     qDebug() << "Таблицы в базе (" << tables.size() << "):";
//     for (const QString& table : tables) {
//         qDebug() << "  - " << table;
//     }

//     // 3. Если таблиц нет, создаем их
//     if (tables.isEmpty()) {
//         qDebug() << "Таблиц нет. Пробуем создать структуру...";
//         dbManager.checkAndCreateTables();

//         tables = dbManager.tableNames();
//         qDebug() << "После создания таблиц (" << tables.size() << "):";
//         for (const QString& table : tables) {
//             qDebug() << "  - " << table;
//         }
//     }

//     qDebug() << "=== КОНЕЦ ПРОВЕРКИ ===\n";
// }

// void MainWindow::checkViewStructure()
// {
//     qDebug() << "=== Начало checkViewStructure ===\n";
//     auto& dbManager = DatabaseManager::instance();

//     // Проверим структуру VIEW
//     QString sql = "SELECT * FROM v_participants_details LIMIT 0";
//     QSqlQuery query(dbManager.database());

//     if (query.exec(sql)) {
//         QSqlRecord rec = query.record();
//         qDebug() << "=== Структура VIEW v_participants_details ===";
//         qDebug() << "Количество столбцов:" << rec.count();

//         for (int i = 0; i < rec.count(); ++i) {
//             qDebug() << "  Колонка" << i << "->" << rec.fieldName(i);
//         }

//         // Проверим одну запись
//         QString dataSql = "SELECT * FROM v_participants_details LIMIT 1";
//         if (query.exec(dataSql) && query.next()) {
//             qDebug() << "=== Пример данных из VIEW ===";
//             for (int i = 0; i < rec.count(); ++i) {
//                 qDebug() << "  " << rec.fieldName(i) << ":" << query.value(i).toString();
//             }
//         }
//     } else {
//         qDebug() << "Ошибка проверки VIEW:" << query.lastError().text();
//     }

//     qDebug() << "=== конец checkViewStructure ===\n";
// }

// void MainWindow::checkFinalTableDisplay()
// {
//     if (!ui->tablePerson || !ui->tablePerson->model()) {
//         qDebug() << "Таблица participants не инициализирована";
//         return;
//     }

//     QAbstractItemModel* model = ui->tablePerson->model();
//     qDebug() << "=== ПРОВЕРКА ОТОБРАЖЕНИЯ В ТАБЛИЦЕ ===";
//     qDebug() << "Столбцов в отображении:" << model->columnCount();
//     qDebug() << "Строк в отображении:" << model->rowCount();

//     // Проверим заголовки
//     qDebug() << "Заголовки столбцов:";
//     for (int i = 0; i < model->columnCount(); ++i) {
//         QString header = model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
//         bool isHidden = ui->tablePerson->isColumnHidden(i);
//         qDebug() << QString("  [%1] '%2': %3")
//                         .arg(i, 2)
//                         .arg(header, -30)
//                         .arg(isHidden ? "скрыт в QTableView" : "видим");
//     }

//     // Проверим первые несколько строк
//     int rowsToCheck = qMin(3, model->rowCount());
//     qDebug() << "Первые" << rowsToCheck << "строк данных в отображении:";

//     for (int row = 0; row < rowsToCheck; ++row) {
//         qDebug() << "  Строка" << row << ":";
//         for (int col = 0; col < qMin(10, model->columnCount()); ++col) { // Первые 10 столбцов
//             if (!ui->tablePerson->isColumnHidden(col)) {
//                 QString header = model->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
//                 QString data = model->data(model->index(row, col), Qt::DisplayRole).toString();
//                 if (!data.isEmpty()) {
//                     qDebug() << QString("    [%1] %2: %3")
//                                     .arg(col, 2)
//                                     .arg(header, -20)
//                                     .arg(data);
//                 }
//             }
//         }
//     }
// }

// void MainWindow::debugCheckModels()
// {
//     qDebug() << "\n=== ПРОВЕРКА МОДЕЛЕЙ ===";

//     // 1. Проверяем таблицы
//     qDebug() << "1. Виджеты таблиц:";
//     qDebug() << "   tablePerson:" << ui->tablePerson;
//     qDebug() << "   tableResult:" << ui->tableResult;
//     qDebug() << "   tableGroup:" << ui->tableGroup;
//     qDebug() << "   tableDist:" << ui->tableDist;
//     qDebug() << "   tableOrg:" << ui->tableOrg;

//     // 2. Проверяем модели
//     qDebug() << "\n2. Модели в m_tableModels:";
//     for (auto it = m_tableModels.constBegin(); it != m_tableModels.constEnd(); ++it) {
//         SqlTableModel* model = it.value();
//         qDebug() << "   " << it.key() << ":";
//         qDebug() << "     Указатель:" << model;
//         if (model) {
//             qDebug() << "     Таблица:" << model->tableName();
//             qDebug() << "     Строк:" << model->rowCount();
//             qDebug() << "     Колонок:" << model->columnCount();
//             qDebug() << "     Ошибка:" << model->lastError().text();
//         }
//     }

//     // 3. Проверяем прокси-модели
//     qDebug() << "\n3. Прокси-модели в m_proxyModels:";
//     for (auto it = m_proxyModels.constBegin(); it != m_proxyModels.constEnd(); ++it) {
//         FilterProxyModel* proxy = it.value();
//         qDebug() << "   " << it.key() << ":";
//         qDebug() << "     Указатель:" << proxy;
//         if (proxy) {
//             qDebug() << "     Исходная модель:" << proxy->sourceModel();
//             qDebug() << "     Строк:" << proxy->rowCount();
//         }
//     }

//     // 4. Проверяем связь виджетов с моделями
//     qDebug() << "\n4. Связь виджетов с моделями:";
//     qDebug() << "   tablePerson модель:" << ui->tablePerson->model();
//     qDebug() << "   tableResult модель:" << ui->tableResult->model();
//     qDebug() << "   tableGroup модель:" << ui->tableGroup->model();
//     qDebug() << "   tableDist модель:" << ui->tableDist->model();
//     qDebug() << "   tableOrg модель:" << ui->tableOrg->model();

//     // 5. Проверяем заголовки
//     qDebug() << "\n5. Заголовки таблиц:";
//     QList<QTableView*> tables = {ui->tablePerson, ui->tableResult, ui->tableGroup,
//                                   ui->tableDist, ui->tableOrg};
//     QStringList names = {"Участники", "Результаты", "Группы", "Дистанции", "Организации"};

//     for (int i = 0; i < tables.size(); ++i) {
//         if (tables[i] && tables[i]->model()) {
//             qDebug() << "   " << names[i] << ":";
//             qDebug() << "     Горизонтальные заголовки:" << tables[i]->horizontalHeader()->count();
//             qDebug() << "     Вертикальные заголовки:" << tables[i]->verticalHeader()->count();
//         }
//     }

//     qDebug() << "=== КОНЕЦ ПРОВЕРКИ ===\n";
// }

// void MainWindow::debugHiddenColumns()
// {
//     if (!m_tableModels.contains("participants")) return;

//     SqlTableModel* model = m_tableModels["participants"];
//     qDebug() << "=== ПРОВЕРКА СКРЫТЫХ СТОЛБЦОВ participants ===";

//     // Получим реальные имена полей из VIEW
//     QStringList realFieldNames = {
//         "id", "competition_id", "participant_type", "full_name", "team_name",
//         "bib_number", "chip_number", "start_time", "birth_date", "gender",
//         "team_size", "delegation_name", "distance_name", "age_group_name",
//         "delegation_id", "distance_id", "age_group_id", "created_at", "updated_at"
//     };

//     for (int i = 0; i < model->columnCount(); ++i) {
//         QString displayHeader = model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
//         QString realFieldName = (i < realFieldNames.size()) ? realFieldNames[i] : "?";
//         bool isHidden = model->m_hiddenColumns.contains(i);

//         qDebug() << QString("  Колонка %1 [%2] -> '%3': %4")
//                         .arg(i, 2)
//                         .arg(realFieldName, -20)
//                         .arg(displayHeader, -30)
//                         .arg(isHidden ? "СКРЫТ" : "ВИДИМ");
//     }
// }

void MainWindow::setupUi()
{
    // Настраиваем таблицы
    QList<QTableView*> tables = {
        ui->tablePerson,
        ui->tableResult,
        ui->tableGroup,
        ui->tableDist,
        ui->tableOrg
    };

    for (QTableView* tableView : tables) {
        if (tableView) {
            tableView->setSelectionMode(QAbstractItemView::SingleSelection);
            tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
            tableView->setAlternatingRowColors(true);
            tableView->setSortingEnabled(true);
            tableView->verticalHeader()->setVisible(false);
            tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

            // Настраиваем заголовки
            QHeaderView* header = tableView->horizontalHeader();
            header->setStretchLastSection(false);
            header->setSectionsMovable(true);
            header->setSectionsClickable(true);
            header->setSectionResizeMode(QHeaderView::Interactive); // Ручное изменение
        }
    }

    ui->tablePerson->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Устанавливаем текущую вкладку
    m_currentTable = m_tabTableMap.value(ui->tabWidget->currentIndex(), "participants");
}

void MainWindow::setupTimerStatusBar(){
    ui->statusbar->addWidget(statusbar_msg, 1);
    ui->statusbar->addPermanentWidget(clock_time, 0);
    clock_timer->setInterval(1000);
    QObject::connect(clock_timer, &QTimer::timeout,this, &MainWindow::updateTime);
    clock_timer->start();
}

void MainWindow::initActionsConnections()
{
    connect(ui->act_new, &QAction::triggered, this, &MainWindow::onNewDocument); // !
    connect(ui->act_open, &QAction::triggered, this, &MainWindow::onOpenDocument);
    //connect(ui->act_filtr, &QAction::triggered, this, &MainWindow::onShowFilter);
    //connect(ui->act_preparation, &QAction::triggered, this, &MainWindow::onShowPrep);
    connect(ui->act_save_as, &QAction::triggered, this, &MainWindow::SaveAsDocument);
    connect(ui->act_save, &QAction::triggered, this, &MainWindow::SaveDocument);

    //connect(ui->act_connect_comport, &QAction::triggered, this, &MainWindow::openSerialPort);
    //connect(ui->act_disconnect_comport, &QAction::triggered, this, &MainWindow::closeSerialPort);

    connect(ui->act_import_csv_orgeo_ru, &QAction::triggered, this, &MainWindow::onAct_import_csv_orgeo_ru_triggered);

    connect(m_csvImporter, &CsvImporter::importFinished,
            this, &MainWindow::onCsvImportFinished);
    connect(m_csvImporter, &CsvImporter::errorOccurred,
            this, &MainWindow::onCsvImportError);
}

// void MainWindow::setupConnections()
// {
//     // Документ
//     connect(m_document, &Document::documentOpened, this, &MainWindow::onDocumentOpened);
//     connect(m_document, &Document::documentClosed, this, &MainWindow::onDocumentClosed);
//     connect(m_document, &Document::documentModified, this, &MainWindow::onDocumentModified);
//     // connect(m_document, &Document::recordInserted, this, &MainWindow::onRecordInserted);
//     // connect(m_document, &Document::recordUpdated, this, &MainWindow::onRecordUpdated);
//     // connect(m_document, &Document::recordDeleted, this, &MainWindow::onRecordDeleted);
//     // connect(m_document, &Document::competitionChanged, this, &MainWindow::onCompetitionChanged);

//     // Вкладки
//     connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

//     // Таблица
//     //connect(ui->tablePerson, &QTableView::doubleClicked, this, &MainWindow::onTablePerson_dblclk);
//     //connect(ui->tableResult, &QTableView::doubleClicked, this, &MainWindow::onTableResult_dblclk);
// }

void MainWindow::setupConnections()
{
    // Проверяем, что документ создан
    if (!m_document) {
        qDebug() << "WARNING: m_document is nullptr in setupConnections()";
        return;
    }

    // Документ
    connect(m_document, &Document::documentOpened, this, &MainWindow::onDocumentOpened);
    connect(m_document, &Document::documentClosed, this, &MainWindow::onDocumentClosed);
    connect(m_document, &Document::documentModified, this, &MainWindow::onDocumentModified);

    // Вкладки
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    // Подключаем двойной клик для всех таблиц
    connect(ui->tablePerson, &QTableView::doubleClicked, this, &MainWindow::onDoubleClickRecord);
    connect(ui->tableResult, &QTableView::doubleClicked, this, &MainWindow::onDoubleClickRecord);
    connect(ui->tableGroup, &QTableView::doubleClicked, this, &MainWindow::onDoubleClickRecord);
    connect(ui->tableDist, &QTableView::doubleClicked, this, &MainWindow::onDoubleClickRecord);
    connect(ui->tableOrg, &QTableView::doubleClicked, this, &MainWindow::onDoubleClickRecord);

    // Поиск
    //connect(ui->searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
}

// void MainWindow::setupConnectionsComport(){
//     QObject::connect(comport, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
//     QObject::connect(comport_timer, &QTimer::timeout, this, &MainWindow::handleWriteTimeout);
//     comport_timer->setSingleShot(true);

//     QObject::connect(comport, &QSerialPort::readyRead, this, &MainWindow::readData);
//     QObject::connect(comport, &QSerialPort::bytesWritten, this, &MainWindow::handleBytesWritten);
// }

// void MainWindow::connectModelsToWidgets()
// {
//     qDebug() << "Связываю модели с виджетами...";

//     if (m_proxyModels.contains("participants")) {
//         ui->tablePerson->setModel(m_proxyModels["participants"]);
//         qDebug() << "  participants → tablePerson";
//     }
//     if (m_proxyModels.contains("results")) {
//         ui->tableResult->setModel(m_proxyModels["results"]);
//         qDebug() << "  results → tableResult";
//     }
//     if (m_proxyModels.contains("age_groups")) {
//         ui->tableGroup->setModel(m_proxyModels["age_groups"]);
//         qDebug() << "  age_groups → tableGroup";
//     }
//     if (m_proxyModels.contains("distances")) {
//         ui->tableDist->setModel(m_proxyModels["distances"]);
//         qDebug() << "  distances → tableDist";
//     }
//     if (m_proxyModels.contains("delegations")) {
//         ui->tableOrg->setModel(m_proxyModels["delegations"]);
//         qDebug() << "  delegations → tableOrg";
//     }
// }

void MainWindow::connectModelsToWidgets()
{
    qDebug() << "Связываю модели с виджетами...";

    if (m_proxyModels.contains("participants")) {
        ui->tablePerson->setModel(m_proxyModels["participants"]);
        // Автоматически подгоняем столбцы после установки модели
        QTimer::singleShot(0, ui->tablePerson, &QTableView::resizeColumnsToContents);
        qDebug() << "  participants > tablePerson";
    }
    if (m_proxyModels.contains("results")) {
        ui->tableResult->setModel(m_proxyModels["results"]);
        QTimer::singleShot(0, ui->tableResult, &QTableView::resizeColumnsToContents);
        qDebug() << "  results > tableResult";
    }
    if (m_proxyModels.contains("age_groups")) {
        ui->tableGroup->setModel(m_proxyModels["age_groups"]);
        QTimer::singleShot(0, ui->tableGroup, &QTableView::resizeColumnsToContents);
        qDebug() << "  age_groups > tableGroup";
    }
    if (m_proxyModels.contains("distances")) {
        ui->tableDist->setModel(m_proxyModels["distances"]);
        QTimer::singleShot(0, ui->tableDist, &QTableView::resizeColumnsToContents);
        qDebug() << "  distances > tableDist";
    }
    if (m_proxyModels.contains("delegations")) {
        ui->tableOrg->setModel(m_proxyModels["delegations"]);
        QTimer::singleShot(0, ui->tableOrg, &QTableView::resizeColumnsToContents);
        qDebug() << "  delegations > tableOrg";
    }
}

void MainWindow::on_act_save_triggered()
{
    if ((!currentFilePath.isEmpty()) && (QFile(currentFilePath).exists()))
        //saveSE(currentFilePath);
        this->SaveDocument();
    else
        //on_act_save_as_triggered();
        this->SaveAsDocument();
}

void MainWindow::onAct_import_csv_orgeo_ru_triggered()
{
    ui_log_msg("onAct_import_csv_orgeo_ru_triggered");

    if (!m_document->isOpen()) {
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Откройте или создайте соревнование перед импортом"));
        return;
    }

    // Получаем путь к файлу
    QString file_Name = QFileDialog::getOpenFileName(this, "Импорт заявки", QDir::currentPath(), "*.csv");
    // if (file_name == "") return;

    // QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    // QString fileName = QFileDialog::getOpenFileName(this,
    //                                                 tr("Импорт заявки из CSV"),
    //                                                 defaultPath,
    //                                                 tr("CSV файлы (*.csv);;Все файлы (*)"));

    if (file_Name.isEmpty()) {
        return;
    }

    // Проверяем расширение
    if (!file_Name.endsWith(".csv", Qt::CaseInsensitive)) {
        QMessageBox::warning(this, tr("Предупреждение"),
                             tr("Рекомендуется использовать файлы с расширением .csv"));
    }

    // Запускаем импорт с поддержкой undo/redo
    importCsvWithUndo(file_Name);
}

void MainWindow::importCsvWithUndo(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось открыть файл:\n%1").arg(file.errorString()));
        return;
    }

    // Читаем весь файл
    QTextStream in(&file);
    QString csvData = in.readAll();
    file.close();

    // Парсим CSV
    QList<CsvRecord> records;
    if (!m_csvImporter->importFromString(csvData, records)) {
        QMessageBox::warning(this, tr("Ошибка импорта"),
                             tr("Не удалось распарсить CSV файл:\n%1").arg(m_csvImporter->lastError()));
        return;
    }

    if (records.isEmpty()) {
        QMessageBox::information(this, tr("Информация"),
                                 tr("В файле не найдено корректных записей"));
        return;
    }

    // Подтверждение импорта
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              tr("Подтверждение импорта"),
                                                              tr("Найдено %1 корректных записей.\n"
                                                                 "Ошибок: %2\n\n"
                                                                 "Продолжить импорт?").arg(records.size()).arg(m_csvImporter->errorCount()),
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Сохраняем оригинальное состояние для undo
    auto& db = DatabaseManager::instance();
    QList<qint64> importedIds;
    QList<QHash<QString, QVariant>> importedData;

    // Начинаем транзакцию
    if (!db.beginTransaction()) {
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось начать транзакцию для импорта"));
        return;
    }

    // Создаем или находим делегации
    QMap<QString, qint64> delegationMap;
    QMap<QString, qint64> ageGroupMap;
    qint64 competitionId = m_document->competitionId();

    // Этап 1: Создание делегаций
    for (int i = 0; i < records.size(); ++i) {
        const CsvRecord& record = records[i];

        if (!delegationMap.contains(record.delegationName)) {
            // Проверяем существование делегации
            QString sql = "SELECT id FROM delegations WHERE name = :name AND competition_id = :competition_id";
            QHash<QString, QVariant> params;
            params["name"] = record.delegationName;
            params["competition_id"] = competitionId;

            auto existing = db.executeSelect(sql, params);
            if (!existing.isEmpty()) {
                delegationMap[record.delegationName] = existing.first()["id"].toLongLong();
            } else {
                // Создаем новую делегацию
                QHash<QString, QVariant> data;
                data["competition_id"] = competitionId;
                data["name"] = record.delegationName;
                data["representative"] = "";
                data["contact"] = record.comment; // Используем комментарий как контакт

                qint64 delegationId = -1;
                if (db.createRecord("delegations", data, &delegationId)) {
                    delegationMap[record.delegationName] = delegationId;
                }
            }
        }
    }

    // Этап 2: Возрастных групп
    for (int i = 0; i < records.size(); ++i) {
        const CsvRecord& record = records[i];

        if (!ageGroupMap.contains(record.delegationName)) {
            // Проверяем существование возрастной группы
            QString sql = "SELECT id FROM age_groups WHERE name = :name AND competition_id = :competition_id";
            QHash<QString, QVariant> params;
            params["name"] = record.ageGroup;
            params["competition_id"] = competitionId;

            auto existing = db.executeSelect(sql, params);
            if (!existing.isEmpty()) {
                ageGroupMap[record.ageGroup] = existing.first()["id"].toLongLong();
            } else {
                // Создаем новую возрастную группу
                QHash<QString, QVariant> data;
                data["competition_id"] = competitionId;
                data["name"] = record.ageGroup;
                data["min_age"] = 0;
                data["max_age"] = 99;
                data["price"] = 1;

                qint64 ageGroupId = -1;
                if (db.createRecord("age_groups", data, &ageGroupId)) {
                    ageGroupMap[record.ageGroup] = ageGroupId;
                }
            }
        }
    }

    // Этап 3: Создание участников
    for (int i = 0; i < records.size(); ++i) {
        const CsvRecord& record = records[i];
        qint64 delegationId = delegationMap.value(record.delegationName, -1);
        qint64 ageGroupId = ageGroupMap.value(record.ageGroup, -1);

        if (delegationId == -1) {
            continue;
        }

        // Находим возрастную группу
        // QString ageGroupSql = "SELECT id FROM age_groups WHERE name = :name AND competition_id = :competition_id";
        // QHash<QString, QVariant> ageGroupParams;
        // ageGroupParams["name"] = record.ageGroup;
        // ageGroupParams["competition_id"] = competitionId;

        // auto ageGroups = db.executeSelect(ageGroupSql, ageGroupParams);
        // //QVariant ageGroupId = ageGroups.isEmpty() ? QVariant() : ageGroups.first()["id"].toLongLong();
        // qint64 ageGroupId = ageGroups.isEmpty() ? -1 : ageGroups.first()["id"].toLongLong();

        // Находим дистанцию (первая в списке)
        QString distanceSql = "SELECT id FROM distances WHERE competition_id = :competition_id LIMIT 1";
        QHash<QString, QVariant> distanceParams;
        distanceParams["competition_id"] = competitionId;

        auto distances = db.executeSelect(distanceSql, distanceParams);
        qint64 distanceId = distances.isEmpty() ? -1 : distances.first()["id"].toLongLong();

        if (distanceId == -1) {
            // Создаем дистанцию по умолчанию если нет
            QHash<QString, QVariant> distanceData;
            distanceData["competition_id"] = competitionId;
            distanceData["name"] = tr("Основная дистанция");
            distanceData["length"] = 5.0;
            distanceData["control_time"] = 60;
            distanceData["control_points"] = "[]";

            if (db.createRecord("distances", distanceData, &distanceId)) {
                // Обновляем модель
                if (m_tableModels.contains("distances")) {
                    m_tableModels["distances"]->select();
                }
            }
        }

        if (distanceId == -1) {
            continue;
        }

        // Подготавливаем данные участника
        QHash<QString, QVariant> data;
        data["competition_id"] = competitionId;
        data["delegation_id"] = delegationId;
        data["distance_id"] = distanceId;
        data["age_group_id"] = ageGroupId;
        data["participant_type"] = "individual";
        data["full_name"] = record.fullName;
        data["bib_number"] = record.bibNumber;

        // Год рождения
        if (!record.birthYear.isEmpty()) {
            bool ok;
            int year = record.birthYear.toInt(&ok);
            if (ok && year >= 1900 && year <= QDate::currentDate().year()) {
                QDate birthDate(year, 1, 1);
                data["birth_date"] = birthDate.toString(Qt::ISODate);
            }
        }

        // Номер чипа
        if (!record.chipNumber.isEmpty() && record.chipNumber != "0") {
            data["chip_number"] = record.chipNumber;
        }

        // Определяем пол
        QString gender = "Мужской";
        QString ageGroupLower = record.ageGroup.toLower();
        if (ageGroupLower.startsWith("ж-") ||
            ageGroupLower.contains("жен") ||
            ageGroupLower.contains("дев")) {
            gender = "Женский";
        }
        data["gender"] = gender;

        // // Спортивный разряд (в комментарий)
        // if (!record.sportRank.isEmpty() && record.sportRank != "0") {
        //     data["team_name"] = QString(tr("Разряд: %1")).arg(record.sportRank);
        // }

        // Создаем участника
        qint64 participantId = -1;
        if (db.createRecord("participants", data, &participantId)) {
            importedIds.append(participantId);
            importedData.append(data);
        }
    }

    if (importedIds.isEmpty()) {
        db.rollbackTransaction();
        QMessageBox::warning(this, tr("Импорт завершен"),
                             tr("Не удалось импортировать ни одного участника"));
        return;
    }

    // Создаем команду undo/redo
    UndoStack::instance().pushBatchImportCommand("participants", importedIds, importedData);

    // Коммитим транзакцию
    if (!db.commitTransaction()) {
        db.rollbackTransaction();
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось завершить импорт"));
        return;
    }

    // Обновляем модели
    if (m_tableModels.contains("participants")) {
        m_tableModels["participants"]->select();
    }
    if (m_tableModels.contains("delegations")) {
        m_tableModels["delegations"]->select();
    }

    // Показываем результаты
    showImportResults(importedIds.size(), records.size() - importedIds.size());

    // Помечаем документ как измененный
    m_document->setModified(true);

    if (importedIds.size() > 0) {
        QTimer::singleShot(100, this, [this]() {
            // Потом обновляем все таблицы
            refreshAllTables();
        });
    }
}

void MainWindow::onCsvImportFinished(int successCount, int errorCount)
{
    showImportResults(successCount, errorCount);
}

void MainWindow::onCsvImportError(const QString& error)
{
    QMessageBox::warning(this, tr("Ошибка импорта"), error);
}

void MainWindow::showImportResults(int successCount, int errorCount, const QString& details)
{
    QString message = tr("Импорт завершен!\n\n"
                         "Успешно импортировано: %1\n"
                         "Ошибок: %2").arg(successCount).arg(errorCount);

    if (!details.isEmpty()) {
        message += "\n\n" + details;
    }

    if (errorCount > 0) {
        message += tr("\n\nОшибки сохранены в лог.");
    }

    QMessageBox::information(this, tr("Результаты импорта"), message);

    // Обновляем статусную строку
    updateStatusBar();
}


// void MainWindow::clearModels()
// {
//     qDebug() << "Очищаем модели...";

//     // Отсоединяем модели от виджетов
//     ui->tablePerson->setModel(nullptr);
//     ui->tableResult->setModel(nullptr);
//     ui->tableGroup->setModel(nullptr);
//     ui->tableDist->setModel(nullptr);
//     ui->tableOrg->setModel(nullptr);

//     // Удаляем прокси-модели
//     for (auto it = m_proxyModels.begin(); it != m_proxyModels.end(); ++it) {
//         if (it.value()) {
//             it.value()->deleteLater();
//         }
//     }
//     m_proxyModels.clear();

//     // Удаляем модели
//     for (auto it = m_tableModels.begin(); it != m_tableModels.end(); ++it) {
//         if (it.value()) {
//             it.value()->deleteLater();
//         }
//     }
//     m_tableModels.clear();

//     // Очищаем Undo stack
//     UndoStack::instance().clear();

//     // Сбрасываем флаги
//     m_modelsInitialized = false;
//     m_tablesConnected = false;

//     qDebug() << "Модели очищены";
// }

void MainWindow::clearModels()
{
    qDebug() << "Очищаем модели...";

    // Отсоединяем модели от виджетов
    ui->tablePerson->setModel(nullptr);
    ui->tableResult->setModel(nullptr);
    ui->tableGroup->setModel(nullptr);
    ui->tableDist->setModel(nullptr);
    ui->tableOrg->setModel(nullptr);

    // Удаляем прокси-модели
    for (FilterProxyModel* proxy : m_proxyModels) {
        if (proxy) {
            proxy->setSourceModel(nullptr); // Важно: отключаем от исходной модели
            proxy->deleteLater();
        }
    }
    m_proxyModels.clear();

    // Удаляем модели
    for (SqlTableModel* model : m_tableModels) {
        if (model) {
            model->clear(); // Очищаем данные
            model->deleteLater();
        }
    }
    m_tableModels.clear();

    qDebug() << "Модели очищены";
}

void MainWindow::setupToolbars()
{
    // Настройка тулбара (если нужно)

    // Создаем тулбар для undo/redo
    QToolBar* editToolbar = addToolBar(tr("Редактирование"));
    editToolbar->setObjectName("editToolbar");

    // Кнопка Undo
    QAction* undoAction = UndoStack::instance().createUndoAction(this);
    undoAction->setIcon(QIcon::fromTheme("edit-undo", QIcon(":/icons/undo.png")));
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setText(tr("Отменить"));
    undoAction->setToolTip(tr("Отменить последнее действие (Ctrl+Z)"));
    editToolbar->addAction(undoAction);

    // Кнопка Redo
    QAction* redoAction = UndoStack::instance().createRedoAction(this);
    redoAction->setIcon(QIcon::fromTheme("edit-redo", QIcon(":/icons/redo.png")));
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setText(tr("Вернуть"));
    redoAction->setToolTip(tr("Вернуть отмененное действие (Ctrl+Y)"));
    editToolbar->addAction(redoAction);

    editToolbar->addSeparator();

    // Добавляем в меню
    ui->menu_2->addAction(undoAction);
    ui->menu_2->addAction(redoAction);

    // Соединяем с нашими слотами (опционально)
    connect(undoAction, &QAction::triggered, this, &MainWindow::onUndo);
    connect(redoAction, &QAction::triggered, this, &MainWindow::onRedo);
}

void MainWindow::setupMenuBar()
{
    // Меню уже настроено в createActionsAndConnections()
}

// void MainWindow::onDocumentOpened()
// {
//     qDebug() << "Сигнал: документ открыт, вызываю initializeForDocument()";
//     initializeForDocument();
// }

// void MainWindow::onDocumentOpened()
// {
//     qDebug() << "Сигнал: документ открыт";

//     // Даем время на завершение открытия БД
//     QTimer::singleShot(100, this, [this]() {
//         initializeForDocument();

//         // После инициализации проверяем age_groups
//         if (m_tableModels.contains("age_groups")) {
//             SqlTableModel* model = m_tableModels["age_groups"];
//             qDebug() << "Проверка age_groups после открытия документа:";
//             qDebug() << "  - Строк в модели:" << model->rowCount();
//             qDebug() << "  - Текущий фильтр:" << model->filter();

//             // Если данные не загрузились, пробуем снова
//             if (model->rowCount() == 0) {
//                 qDebug() << "age_groups пуста, пробую принудительный select...";
//                 model->select();
//             }
//         }
//     });
// }

void MainWindow::onDocumentOpened()
{
    qDebug() << "Сигнал: документ открыт";

    // Даем время на завершение открытия БД
    QTimer::singleShot(150, this, [this]() {
        initializeForDocument();
    });
}

// void MainWindow::onDocumentClosed()
// {
//     qDebug() << "\n=== ЗАКРЫТИЕ ДОКУМЕНТА ===";

//     // Очищаем модели
//     clearModels();

//     // Очищаем все таблицы
//     ui->tablePerson->setModel(nullptr);
//     ui->tableResult->setModel(nullptr);
//     ui->tableGroup->setModel(nullptr);
//     ui->tableDist->setModel(nullptr);
//     ui->tableOrg->setModel(nullptr);

//     // Очищаем Undo stack
//     UndoStack::instance().clear();

//     updateWindowTitle();
//     updateStatusBar();

//     qDebug() << "Документ закрыт\n";
// }

void MainWindow::onDocumentClosed()
{
    qDebug() << "\n=== ЗАКРЫТИЕ ДОКУМЕНТА ===";

    // Отключаем все сигналы от моделей перед очисткой
    for (auto model : m_tableModels) {
        if (model) {
            model->disconnect();
        }
    }

    // Сбрасываем фильтры в моделях
    for (auto model : m_tableModels) {
        if (model) {
            model->setFilter(QString()); // Сбрасываем фильтр
            model->clear(); // Очищаем данные
        }
    }

    // Очищаем модели
    clearModels();

    // Явно устанавливаем nullptr для всех таблиц
    ui->tablePerson->setModel(nullptr);
    ui->tableResult->setModel(nullptr);
    ui->tableGroup->setModel(nullptr);
    ui->tableDist->setModel(nullptr);
    ui->tableOrg->setModel(nullptr);

    // Очищаем кэш QTableView
    ui->tablePerson->reset();
    ui->tableResult->reset();
    ui->tableGroup->reset();
    ui->tableDist->reset();
    ui->tableOrg->reset();

    // Очищаем Undo stack
    UndoStack::instance().clear();

    // Сбрасываем флаги
    m_modelsInitialized = false;
    m_tablesConnected = false;
    m_isModified = false;
    m_currentTable = "participants";

    updateWindowTitle();
    updateStatusBar();

    qDebug() << "Документ закрыт\n";
}

void MainWindow::onDocumentModified(bool modified)
{
    m_isModified = modified;
    ui->act_save->setEnabled(modified);
    updateWindowTitle();
}

void MainWindow::onNewDocument()
{
    if (!confirmUnsavedChanges()) {
        return;
    }

    // Закрываем текущий документ
    closeCurrentDocument();

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    tr("Создать новое соревнование"),
                                                    QDir(defaultPath).filePath("Новое соревнование.tevent"),
                                                    tr("Файлы T-Event (*.tevent);;Файлы SQLite (*.db);;Все файлы (*)"));

    if (!filePath.isEmpty()) {
        // Создаем новый документ
        m_document = new Document(this);

        // Подключаем сигналы ТОЛЬКО после создания документа
        setupConnections();

        if (m_document->createNew(filePath)) {
            adjustForCurrentFile(filePath);

            QTimer::singleShot(100, this, [this]() {
                initializeForDocument();

                QTimer::singleShot(50, this, [this]() {
                    refreshAllTables();
                });
            });
        } else {
            QMessageBox::critical(this, tr("Ошибка"),
                                  tr("Не удалось создать файл: %1").arg(filePath));
            delete m_document;
            m_document = nullptr;
        }
    }
}

void MainWindow::OpenTEvent(QString fpath)
{
    if (!confirmUnsavedChanges()) {
        return;
    }

    // Закрываем текущий документ
    closeCurrentDocument();

    // Создаем новый документ
    m_document = new Document(this);

    // Подключаем сигналы ТОЛЬКО после создания документа
    setupConnections();

    if (m_document->open(fpath)) {
        adjustForCurrentFile(fpath);

        // ДОБАВЛЕНО: ждем пока документ полностью откроется
        QTimer::singleShot(100, this, [this]() {
            initializeForDocument();
        });
    } else {
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось открыть файл: %1").arg(fpath));
        delete m_document;
        m_document = nullptr;
    }
}

void MainWindow::onOpenDocument()
{
    if (!confirmUnsavedChanges()) {
        return;
    }

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    tr("Открыть файл соревнования"),
                                                    defaultPath,
                                                    tr("Файлы T-Event (*.tevent);;Файлы SQLite (*.db);;Все файлы (*)"));

    if (!filePath.isEmpty()) {
        OpenTEvent(filePath);
    }
}

bool MainWindow::SaveDocument()
{
    if (!m_document->isOpen()) {
        return false;
    }

    return m_document->save();
}

bool MainWindow::SaveAsDocument()
{
    if (!m_document->isOpen()) {
        return false;
    }

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    tr("Сохранить соревнование как"),
                                                    QDir(defaultPath).filePath(m_document->fileName()),
                                                    tr("Файлы T-Event (*.tevent);;Файлы SQLite (*.db);;Все файлы (*)"));

    if (!filePath.isEmpty()) {
        return m_document->saveAs(filePath);
    }

    return false;
}

bool MainWindow::confirmUnsavedChanges()
{
    if (m_isModified) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  tr("Нерасохраненные изменения"),
                                                                  tr("Документ содержит несохраненные изменения. Сохранить?"),
                                                                  QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Save) {
            return SaveDocument();
        } else if (reply == QMessageBox::Cancel) {
            return false;
        }
    }

    return true;
}

// void MainWindow::onCloseDocument()
// {
//     if (confirmUnsavedChanges()) {
//         m_document->close();
//     }
// }

void MainWindow::onCloseDocument()
{
    if (confirmUnsavedChanges()) {
        closeCurrentDocument();
    }
}

// void MainWindow::onUndo()
// {
//     UndoStack::instance().undo();
// }

// void MainWindow::onRedo()
// {
//     UndoStack::instance().redo();
// }

void MainWindow::onUndo()
{
    if (UndoStack::instance().canUndo()) {
        UndoStack::instance().undo();
        refreshAllTables(); // Обновляем все таблицы после undo
        updateStatusBar();
    }
}

void MainWindow::onRedo()
{
    if (UndoStack::instance().canRedo()) {
        UndoStack::instance().redo();
        refreshAllTables(); // Обновляем все таблицы после redo
        updateStatusBar();
    }
}

void MainWindow::onAddRecord()
{
    if (!m_document->isOpen()) {
        return;
    }

    // Отображаем диалог для текущей таблицы
    showEditDialog(m_currentTable, -1);
}

// void MainWindow::showEditDialog(const QString& tableName, qint64 recordId)
// {
//     QDialog* dialog = createEditDialog(tableName, recordId);
//     if (dialog) {
//         dialog->setAttribute(Qt::WA_DeleteOnClose);
//         dialog->exec();
//     }
// }

void MainWindow::showEditDialog(const QString& tableName, qint64 recordId)
{
    qDebug() << "showEditDialog: таблица =" << tableName << ", ID =" << recordId;

    QDialog* dialog = nullptr;

    try {
        dialog = createEditDialog(tableName, recordId);

        if (dialog) {
            dialog->setParent(this, Qt::Window);
            dialog->setAttribute(Qt::WA_DeleteOnClose);

            if (EditDialogBase* editDialog = qobject_cast<EditDialogBase*>(dialog)) {
                editDialog->setProperty("document", QVariant::fromValue<Document*>(m_document));

                connect(editDialog, &EditDialogBase::recordSaved,
                        this, [this](qint64 id, const QString& table) {
                            qDebug() << "Запись сохранена:" << table << "ID:" << id;
                            refreshTable(table);
                            updateStatusBar();
                        });

                connect(editDialog, &EditDialogBase::recordDeleted,
                        this, [this](qint64 id, const QString& table) {
                            qDebug() << "Запись удалена:" << table << "ID:" << id;
                            refreshTable(table);
                            updateStatusBar();
                        });
            }

            dialog->show();
            dialog->raise();
            dialog->activateWindow();

            qDebug() << "Диалог успешно открыт";
        }
    } catch (const std::exception& e) {
        qCritical() << "Ошибка при создании диалога:" << e.what();
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось открыть диалог редактирования: %1").arg(e.what()));
    }
}

QDialog* MainWindow::createEditDialog(const QString& tableName, qint64 recordId)
{
    if (!m_document) {
        qWarning() << "MainWindow: Document is null!";
        return nullptr;
    }

    if (tableName == "participants") {
        return new PersonEditDialog(m_document, recordId, this);
    } else if (tableName == "delegations") {
        // return new DelegationEditDialog(m_document, recordId, this);
    } else if (tableName == "distances") {
        // return new DistanceEditDialog(m_document, recordId, this);
    } else if (tableName == "age_groups") {
        // return new AgeGroupEditDialog(m_document, recordId, this);
    }

    qWarning() << "No dialog for table:" << tableName;
    return nullptr;
}

void MainWindow::onEditRecord()
{
    if (!m_document->isOpen()) {
        return;
    }

    QModelIndexList selected = getCurrentTableView()->selectionModel()->selectedRows();

    if (selected.isEmpty()) {
        QMessageBox::information(this, tr("Информация"),
                                 tr("Выберите запись для редактирования"));
        return;
    }

    onDoubleClickRecord(selected.first());
}

void MainWindow::onDoubleClickRecord(const QModelIndex& index)
{
    if (!m_document || !m_document->isOpen() || !index.isValid()) {
        return;
    }

    // Получаем текущую вкладку для определения таблицы
    int currentTab = ui->tabWidget->currentIndex();
    QString tableName = m_tabTableMap.value(currentTab, "participants");

    qDebug() << "Двойной клик по таблице:" << tableName
             << "строка:" << index.row() << "столбец:" << index.column();

    // Получаем прокси-модель для текущей таблицы
    if (!m_proxyModels.contains(tableName)) {
        qWarning() << "Не найдена прокси-модель для таблицы:" << tableName;
        return;
    }

    FilterProxyModel* proxyModel = m_proxyModels[tableName];

    // Преобразуем индекс из прокси в исходную модель
    QModelIndex sourceIndex = proxyModel->mapToSource(index);

    // Получаем ID записи из исходной модели
    if (!m_tableModels.contains(tableName)) {
        qWarning() << "Не найдена модель для таблицы:" << tableName;
        return;
    }

    SqlTableModel* sourceModel = m_tableModels[tableName];
    qint64 recordId = -1;

    if (sourceIndex.isValid()) {  // Получаем ID из первой колонки
        recordId = sourceModel->data(sourceModel->index(sourceIndex.row(), 0)).toLongLong();
    }

    if (recordId <= 0) {
        qWarning() << "Не удалось получить ID записи для таблицы:" << tableName;
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Не удалось определить запись для редактирования"));
        return;
    }

    qDebug() << "Открываю диалог для редактирования записи ID:" << recordId
             << "таблица:" << tableName;

    // Открываем соответствующий диалог редактирования
    showEditDialog(tableName, recordId);
}

void MainWindow::onDeleteRecord()
{
    if (!m_document->isOpen()) {
        return;
    }

    // Получаем текущую таблицу и виджет
    QTableView* currentTableView = getCurrentTableView();
    if (!currentTableView) {
        return;
    }

    QModelIndexList selected = currentTableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::information(this, tr("Информация"),
                                 tr("Выберите запись для удаления"));
        return;
    }

    // Используем метод recordId() из прокси-модели
    FilterProxyModel* proxyModel = m_proxyModels[m_currentTable];
    if (!proxyModel) {
        return;
    }

    QModelIndex proxyIndex = selected.first();
    qint64 recordId = proxyModel->recordId(proxyIndex);

    if (recordId <= 0) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              tr("Подтверждение удаления"),
                                                              tr("Вы уверены, что хотите удалить выбранную запись?"),
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_document->deleteRecord(m_currentTable, recordId);
    }
}

QTableView* MainWindow::getCurrentTableView() const
{
    switch (ui->tabWidget->currentIndex()) {
    case 0: return ui->tablePerson;   // Участники
    case 1: return ui->tableResult;   // Результаты
    case 2: return ui->tableGroup;    // Возрастные группы
    case 3: return ui->tableDist;     // Дистанции
    case 4: return ui->tableOrg;      // Делегации
    default: return nullptr;
    }
}

void MainWindow::onTabChanged(int index)
{
    if (!m_document->isOpen()) {
        return;
    }

    m_currentTable = m_tabTableMap.value(index, "participants");
    updateStatusBar();
}

void MainWindow::onSearchTextChanged(const QString& text)
{
    if (m_proxyModels.contains(m_currentTable)) {
        m_proxyModels[m_currentTable]->setTextFilter(text);
    }
}

void MainWindow::updateStatusBar()
{
    QString status;

    if (m_document->isOpen()) {
        int participantCount = 0;
        if (m_tableModels.contains("participants") && m_tableModels["participants"]) {
            participantCount = m_tableModels["participants"]->rowCount();
        }

        status = tr("Соревнование: %1 | Участников: %2")
                     .arg(m_document->competitionName())
                     .arg(participantCount);

        if (m_isModified) {
            status += " [*]";
        }
    } else {
        status = tr("Документ не открыт");
    }

    statusBar()->showMessage(status);
}

void MainWindow::updateWindowTitle()
{
    QString title = tr("Спортивное соревнование");

    if (m_document->isOpen()) {
        title = QString("%1 - %2").arg(m_document->competitionName()).arg(title);

        if (!m_document->filePath().isEmpty()) {
            title = QString("%1 - %2").arg(QFileInfo(m_document->filePath()).fileName()).arg(title);
        }

        if (m_isModified) {
            title += " [*]";
        }
    }

    setWindowTitle(title);
}
