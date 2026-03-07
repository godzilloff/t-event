#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "Document.h"
#include "SqlTableModel.h"
#include "UndoStack.h"
#include "PersonEditDialog.h"
#include "DelegationEditDialog.h"
#include "DatabaseManager.h"
#include "ResultsProxyModel.h"

#include "dialogs/PersonEditDialog.h"
#include "dialogs/DelegationEditDialog.h"
#include "dialogs/DistanceEditDialog.h"
#include "dialogs/AgeGroupEditDialog.h"

#include "serial/isportident_interface.h"
#include "serial/sportident_station.h"
#include "serial/sportident_types.h"

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
    m_siStation(new SportIdent::SportIdentStation(this)),
    maxFileNr(4),
    flag_need_save(false),
    m_document(nullptr),
    m_csvImporter(new CsvImporter(this)),
    m_modelsInitialized(false),
    m_tablesConnected(false),
    m_isModified(false),
    m_updatingTable(false)
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
    setupConnectionsComport();
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
                        QString str = value.value<QString>();
                        qDebug() << str;
                        //requestOnline(str);
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
            // onRedo(); // Ctrl+Shift+Z = Redo
            on_act_redo_triggered();
        } else {
            // onUndo(); // Ctrl+Z = Undo
            on_act_undo_triggered();
        }
        e->accept();
        return;
    }

    if (e->key() == Qt::Key_Y && e->modifiers() & Qt::ControlModifier) {
        // onRedo(); // Ctrl+Y = Redo
        on_act_redo_triggered();
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
        setupModels(); // Здесь создаются модели и связываются с виджетами
    }

    // 2. СНАЧАЛА настраиваем видимость столбцов (до применения фильтров!)
    qDebug() << "Настраиваю видимость столбцов...";
    setupColumnVisibility();

    if (m_proxyModels.contains("results")) {
        auto* proxy = m_proxyModels["results"];
        qDebug() << "После setupColumnVisibility() - visible columns:"
                 << proxy->visibleColumns();

        // Принудительно показываем виртуальные колонки в tableView
        int totalCols = proxy->columnCount();
        for (int i = 0; i < totalCols; ++i) {
            if (i >= m_tableModels["results"]->columnCount()) {
                // Это виртуальная колонка
                ui->tableResult->setColumnHidden(i, false);
            }
        }
    }

    // 3. Затем применяем фильтры и загружаем данные
    qDebug() << "Применяю фильтры...";
    applyCompetitionFilters(competitionId);

    // 4. Настраиваем делегаты
    setupDelegates();

    // 5. Обновляем UI
    updateWindowTitle();
    updateStatusBar();

    // 6. Принудительно обновляем отображение
    QTimer::singleShot(100, this, [this]() {
        refreshAllTables();

        if (m_proxyModels.contains("results")) {
            auto* resultsProxy = qobject_cast<ResultsProxyModel*>(m_proxyModels["results"]);
            if (resultsProxy) {
                // Сбрасываем модель, чтобы view узнал о новой структуре колонок
                resultsProxy->resetModelStructure();

                // Пересчитываем ранги
                resultsProxy->recalculateRanks();

                // Явно показываем все колонки в tableView
                for (int i = 0; i < resultsProxy->columnCount(); ++i) {
                    ui->tableResult->setColumnHidden(i, false);
                }

                ui->tableResult->setSortingEnabled(false);

                // Принудительно обновляем представление
                ui->tableResult->viewport()->update();
                ui->tableResult->reset();

                qDebug() << "Results model reset and updated, columnCount:" << resultsProxy->columnCount();
            }
        }

        // Убеждаемся, что все колонки видимы в QTableView
        ui->tableResult->resizeColumnsToContents();
    });

    // После всех настроек принудительно обновляем представление результатов
    if (m_proxyModels.contains("results")) {
        auto* resultsProxy = qobject_cast<ResultsProxyModel*>(m_proxyModels["results"]);
        if (resultsProxy) {
            // Сбрасываем модель, чтобы view узнал о новой структуре колонок
            resultsProxy->resetModelStructure();

            // Пересчитываем ранги
            resultsProxy->recalculateRanks();

            // ВАЖНО: ЯВНО ПОКАЗЫВАЕМ ВСЕ КОЛОНКИ
            for (int i = 0; i < resultsProxy->columnCount(); ++i) {
                ui->tableResult->setColumnHidden(i, false);
                qDebug() << "  Column" << i << "hidden:" << ui->tableResult->isColumnHidden(i);
            }

            // Принудительно обновляем представление
            ui->tableResult->reset();

            qDebug() << "Results model reset and updated, columnCount:" << resultsProxy->columnCount();
        }
    }
    // debugTableColumns();

    m_resultProcessor.reset(new ResultProcessor(m_document, this));
    connect(m_resultProcessor.data(), &ResultProcessor::resultProcessed,
            this, &MainWindow::onResultProcessed);

    qDebug() << "=== ИНИЦИАЛИЗАЦИЯ ЗАВЕРШЕНА ===";
}

void MainWindow::applyCompetitionFilters(qint64 competitionId)
{
    qDebug() << "Применяю фильтры competition_id =" << competitionId;

    for (auto it = m_tableModels.begin(); it != m_tableModels.end(); ++it) {
        const QString& tableName = it.key();
        SqlTableModel* model = it.value();

        if (!model) continue;

        // Для таблиц, которые фильтруются по competition_id
        if (tableName == "participants" || tableName == "delegations" ||
            tableName == "distances" || tableName == "age_groups") {

            QString filter = QString("competition_id = %1").arg(competitionId);
            qDebug() << "  Устанавливаю фильтр для" << tableName << ":" << filter;

            model->setFilter(filter);

            // Загружаем данные
            if (!model->select()) {
                qDebug() << "  Ошибка select() для" << tableName << ":"
                         << model->lastError().text();
            } else {
                qDebug() << "  Таблица" << tableName << "загружена, строк:"
                         << model->rowCount();
            }

            // Обновляем прокси
            if (m_proxyModels.contains(tableName)) {
                m_proxyModels[tableName]->invalidate();
            }

        } else if (tableName == "results") {
            // Для результатов фильтр через participant_id
            QString filter = QString(
                                 "participant_id IN (SELECT id FROM participants WHERE competition_id = %1)"
                                 ).arg(competitionId);

            qDebug() << "  Устанавливаю фильтр для results:" << filter;
            model->setFilter(filter);

            if (!model->select()) {
                qDebug() << "  Ошибка select() для results:"
                         << model->lastError().text();
            } else {
                qDebug() << "  Таблица results загружена, строк:"
                         << model->rowCount();
            }

            if (m_proxyModels.contains("results")) {
                m_proxyModels["results"]->invalidate();

                // Пересчитываем места
                if (ResultsProxyModel* resultsProxy =
                    qobject_cast<ResultsProxyModel*>(m_proxyModels["results"])) {

                    // Добавляем отложенный пересчет
                    QTimer::singleShot(100, resultsProxy, [resultsProxy]() {
                        resultsProxy->recalculateRanks();
                    });
                }
            }
        }
    }
}

void MainWindow::onResultProcessed(qint64 resultId, const SportIdent::CardData& cardData)
{
    qDebug() << "Результат обработан, ID:" << resultId;

    if (m_tablesBeingRefreshed.contains("results")) {
        qDebug() << "Уже обновляем результаты, пропускаем";
        return;
    }

    refreshAllTables();

    // Пересчитываем места в прокси-модели
    if (m_proxyModels.contains("results")) {
        if (ResultsProxyModel* resultsProxy = qobject_cast<ResultsProxyModel*>(m_proxyModels["results"])) {
            resultsProxy->recalculateRanks();
        }
    }

    // Находим и выделяем запись
    QTimer::singleShot(30, this, [this, resultId]() {
        highlightResult(resultId);
    });

    updateStatusBar();
    logMessage(tr("Результат для карты %1 успешно добавлен (ID: %2)")
                   .arg(cardData.cardNumber).arg(resultId));
}

void MainWindow::highlightResult(qint64 resultId)
{
    if (!m_proxyModels.contains("results")) return;

    AbstractProxyModel* proxy = m_proxyModels["results"];
    SqlTableModel* sourceModel = m_tableModels["results"];

    for (int row = 0; row < sourceModel->rowCount(); ++row) {
        QModelIndex index = sourceModel->index(row, 0);
        if (sourceModel->data(index).toLongLong() == resultId) {
            QModelIndex sourceIdx = sourceModel->index(row, 0);
            QModelIndex proxyIdx = proxy->mapFromSource(sourceIdx);

            ui->tableResult->selectionModel()->select(
                proxyIdx,
                QItemSelectionModel::Select | QItemSelectionModel::Rows
                );
            ui->tableResult->scrollTo(proxyIdx);
            ui->tabWidget->setCurrentIndex(1);
            break;
        }
    }
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

    clearModels();
    auto& dbManager = DatabaseManager::instance();

    QStringList tables = {"participants", "results", "delegations", "distances", "age_groups"};

    for (const QString& tableName : tables) {
        qDebug() << "Создаю модель для:" << tableName;

        SqlTableModel* model = nullptr;
        AbstractProxyModel* proxy = nullptr;

        dbManager.withDatabase([&](const QSqlDatabase& db) -> bool {
            model = new SqlTableModel(this, db);

            if (tableName == "participants") {
                model->setTable("v_participants_details");
            } else if (tableName == "results") {
                model->setTable("v_results_details");
            } else {
                model->setTable(tableName);
            }

            // Устанавливаем заголовки ДО того, как модель будет заполнена
            setupColumnHeaders(model, tableName);
            setupHiddenColumns(model, tableName);

            model->setEditStrategy(QSqlTableModel::OnManualSubmit);

            return true;
        });

        if (!model) continue;

        // Создаем прокси
        if (tableName == "results") {
            ResultsProxyModel* resultsProxy = new ResultsProxyModel(this);
            resultsProxy->setSourceModel(model);
            proxy = resultsProxy;

            //resultsProxy->setObjectName("ResultsProxy");
            resultsProxy->setObjectName("ResultsProxy_" + tableName); // ВАЖНО!
            qDebug() << "  ResultsProxyModel created, columnCount:" << resultsProxy->columnCount();

            connect(resultsProxy, &ResultsProxyModel::calculationStarted,
                    this, []() { qDebug() << "Пересчет мест..."; });
            connect(resultsProxy, &ResultsProxyModel::calculationFinished,
                    this, []() { qDebug() << "Пересчет мест завершен"; });
        } else {
            FilterProxyModel* filterProxy = new FilterProxyModel(this);
            filterProxy->setSourceModel(model);
            filterProxy->setObjectName("FilterProxy_" + tableName); // ВАЖНО!
            proxy = filterProxy;
        }

        // Сохраняем
        m_tableModels[tableName] = model;
        m_proxyModels[tableName] = proxy;

        // Подключаем сигнал о загрузке данных (для отладки)
        connect(model, &SqlTableModel::dataLoaded, this, [ tableName]() {
            qDebug() << "Данные загружены для таблицы:" << tableName;
        });
    }

    // Связываем с виджетами
    connectModelsToWidgets();

    qDebug() << "Создано моделей:" << m_tableModels.size();
    qDebug() << "=== СОЗДАНИЕ МОДЕЛЕЙ ЗАВЕРШЕНО ===\n";

    // Настройка заголовков для сортировки
    QHeaderView* header = ui->tableResult->horizontalHeader();
    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);  // Показывать индикатор сортировки
    header->setSortIndicator(-1, Qt::AscendingOrder);  // Сброс индикатора

    // Установка политики сортировки
    ui->tableResult->setSortingEnabled(true);
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
            {1, "ФИО участника"},      // из participants
            {2, "Возрастная группа"},    // из age_groups
            {3, "Делегация"},           // из delegations через participants
            {4, "Результат (сек)"},
            {5, "Статус"},
            {6, "Номер (bib)"},        // из participants
            {7, "Номер чипа"},
            {8, "Время старта"},
            {9, "Время финиша"},
            {10, "Пол"},                 // из participants
            {11, "ID участника"},
            {12, "Дистанция"}           // из distances
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
    for (const auto& proxy : std::as_const(m_proxyModels)) {
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
    }
    model->hideColumns(hiddenColumns);
}

void MainWindow::setupColumnVisibility()
{
    qDebug() << "=== setupColumnVisibility() ===";

    for (auto it = m_proxyModels.begin(); it != m_proxyModels.end(); ++it) {
        const QString& tableName = it.key();
        AbstractProxyModel* proxyModel = it.value();

        if (!m_tableModels.contains(tableName)) {
            qDebug() << "  Пропускаю" << tableName << "- нет исходной модели";
            continue;
        }

        QList<int> visibleColumns;

        if (tableName == "participants") {
            visibleColumns = {
                3,  // full_name
                5,  // bib_number
                6,  // chip_number
                9,  // gender
                8,  // birth_date
                11, // delegation_name
                12, // distance_name
                13  // age_group_name
            };
        }
        else if (tableName == "delegations") {
            visibleColumns = {2, 3, 4}; // name, representative, contact
        }
        else if (tableName == "distances") {
            visibleColumns = {2, 3, 4, 5}; // name, length, control_time, control_points
        }
        else if (tableName == "age_groups") {
            visibleColumns = {2, 3, 4, 5}; // name, min_age, max_age, price
        }
        else if (tableName == "results") {


            auto* proxy = m_proxyModels["results"];

            int totalCols = proxy->columnCount();
            qDebug() << "  Results - total columns:" << totalCols;

            // Сначала получаем исходные колонки, которые хотим показать
            // Индексы соответствуют исходной модели v_results_details
            visibleColumns = {
                0,  // ID (скрываем или показываем)
                1,  // ФИО участника
                2,  // Возрастная группа
                3,  // Делегация
                4,  // Результат (сек)
                5,  // Статус (показываем для отладки, потом можно скрыть)
                6,  // Номер (bib)
                7,  // Номер чипа
                8,  // Время старта
                9,  // Время финиша
                10, // Пол
                12  // Дистанция
            };

            qDebug() << "  Results - исходные видимые колонки:" << visibleColumns;

            // Добавляем виртуальные колонки
            if (m_tableModels.contains("results")) {
                int sourceColCount = m_tableModels["results"]->columnCount();
                visibleColumns.append(sourceColCount + 0); // Место
                visibleColumns.append(sourceColCount + 1); // Отставание
                visibleColumns.append(sourceColCount + 2); // % от лидера
            }

            qDebug() << "  Results - total columns with virtual:" << visibleColumns;


            // Виртуальные колонки управляются напрямую в QTableView
            ui->tableResult->setColumnHidden(13, false); // Место
            ui->tableResult->setColumnHidden(14, false); // Отставание
            ui->tableResult->setColumnHidden(15, false); // % от лидера


            proxy->setVisibleColumns(visibleColumns);

            // qDebug() << "  Results - visible columns:" << visibleColumns;
        }

        if (!visibleColumns.isEmpty()) {
            proxyModel->setVisibleColumns(visibleColumns);
            qDebug() << "  Установлена видимость для" << tableName << ":" << visibleColumns;
        }
    }

    qDebug() << "=== setupColumnVisibility() завершен ===";
}

void MainWindow::setupTableViewHeaders()
{
    // Настраиваем порядок столбцов через перемещение заголовков
    if (m_proxyModels.contains("participants")) {
        QTableView* tableView = ui->tablePerson;

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

void MainWindow::onTableDataLoaded(const QString& tableName)
{
    qDebug() << "Данные загружены для таблицы:" << tableName;

    QTableView* tableView1 = nullptr;
    if (tableName == "participants") tableView1 = ui->tablePerson;
    else if (tableName == "results") tableView1 = ui->tableResult;
    else if (tableName == "age_groups") tableView1 = ui->tableGroup;
    else if (tableName == "distances") tableView1 = ui->tableDist;
    else if (tableName == "delegations") tableView1 = ui->tableOrg;

    if (tableView1) {
        // Используем singleShot для отложенного обновления
        QTimer::singleShot(0, tableView1, [tableView1]() {
            tableView1->resizeColumnsToContents();
        });
        tableView1->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    // Специальная обработка для таблицы результатов
    if (tableName == "results" && m_proxyModels.contains("results")) {
        if (ResultsProxyModel* resultsProxy = qobject_cast<ResultsProxyModel*>(m_proxyModels["results"])) {
            // Пересчитываем места только если это действительно нужно
            // и если не было вызвано из рекурсивного обновления
            if (!m_updatingTable) {
                resultsProxy->recalculateRanks();
            }
        }
    }
}

void MainWindow::refreshTable(const QString& tableName)
{
    // Защита от рекурсии
    if (m_tablesBeingRefreshed.contains(tableName)) {
        qDebug() << "Предотвращена рекурсия для таблицы:" << tableName;
        return;
    }

    m_tablesBeingRefreshed.insert(tableName);

    qDebug() << "\n=== ВЫЗОВ refreshTable(" << tableName << ") ===";

    if (!m_document || !m_document->isOpen()) {
        qDebug() << "Документ не открыт, пропускаю обновление таблицы" << tableName;
        m_tablesBeingRefreshed.remove(tableName);
        return;
    }

    if (!m_tableModels.contains(tableName)) {
        qWarning() << "Модель для таблицы" << tableName << "не найдена";
        m_tablesBeingRefreshed.remove(tableName);
        return;
    }

    QSqlTableModel* model = m_tableModels.value(tableName);
    if (!model) {
        qWarning() << "Модель для таблицы" << tableName << "пуста (nullptr)";
        m_tablesBeingRefreshed.remove(tableName);
        return;
    }

    if (!model->database().isOpen()) {
        qWarning() << "База данных для модели" << tableName << "закрыта";
        m_tablesBeingRefreshed.remove(tableName);
        return;
    }

    // Временно отключаем сигнал dataLoaded, чтобы избежать цикла
    model->blockSignals(true);

    bool success = false;
    try {
        success = model->select();
    } catch (const std::exception& e) {
        qCritical() << "Исключение при обновлении таблицы" << tableName << ":" << e.what();
    } catch (...) {
        qCritical() << "Неизвестное исключение при обновлении таблицы" << tableName;
    }

    model->blockSignals(false);

    if (!success) {
        qWarning() << "Ошибка обновления таблицы" << tableName << ":"
                   << model->lastError().text();
    } else {
        qDebug() << "Таблица" << tableName << "успешно обновлена, строк:"
                 << model->rowCount();

        // Явно вызываем обработчик загрузки данных, но с защитой от рекурсии
        if (!m_updatingTable) {
            m_updatingTable = true;
            onTableDataLoaded(tableName);
            m_updatingTable = false;
        }
    }

    // Если эта таблица отображается на текущей вкладке — обновляем вид
    int tabIndex = findTabIndexByTableName(tableName);
    if (tabIndex != -1 && tabIndex == ui->tabWidget->currentIndex()) {
        QTableView* tableView = getTableViewForTab(tabIndex);
        if (tableView) {
            tableView->resizeColumnsToContents();
            tableView->viewport()->update();
        }
    }

    updateStatusBar();

    qDebug() << "=== ЗАВЕРШЕНИЕ refreshTable(" << tableName << ") ===\n";

    m_tablesBeingRefreshed.remove(tableName);
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

    connect(ui->tableResult->horizontalHeader(), &QHeaderView::sectionClicked,
        this, [this](int logicalIndex) {
            if (auto* model = ui->tableResult->model()) {
                model->sort(logicalIndex, ui->tableResult->horizontalHeader()->sortIndicatorOrder());
            }
    });
}

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
    connect(ui->tablePerson, &QTableView::doubleClicked, this, &MainWindow::onDoubleClickRecord,Qt::UniqueConnection);
    connect(ui->tableResult, &QTableView::doubleClicked, this, &MainWindow::onDoubleClickRecord);
    connect(ui->tableGroup, &QTableView::doubleClicked, this, &MainWindow::onDoubleClickRecord);
    connect(ui->tableDist, &QTableView::doubleClicked, this, &MainWindow::onDoubleClickRecord);
    connect(ui->tableOrg, &QTableView::doubleClicked, this, &MainWindow::onDoubleClickRecord);

    // Поиск
    //connect(ui->searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
}

void MainWindow::setupConnectionsComport(){
    // Кнопки
    connect(ui->act_connect_comport, &QAction::triggered, this, &MainWindow::onConnectClicked);

    // Сигналы от станции
    connect(m_siStation.data(), &SportIdent::ISportIdentInterface::stationConnected,
            this, &MainWindow::onStationConnected);
    connect(m_siStation.data(), &SportIdent::ISportIdentInterface::cardDetected,
            this, &MainWindow::onCardDetected);
    connect(m_siStation.data(), &SportIdent::ISportIdentInterface::cardReadComplete,
            this, &MainWindow::onCardReadComplete);
    connect(m_siStation.data(), &SportIdent::ISportIdentInterface::cardRemoved,
            this, &MainWindow::onCardRemoved);
    connect(m_siStation.data(), &SportIdent::ISportIdentInterface::errorOccurred,
            this, &MainWindow::onErrorOccurred);
    connect(m_siStation.data(), &SportIdent::ISportIdentInterface::debugMessage,
            this, &MainWindow::onDebugMessage);
}

void MainWindow::on_act_comport_dialogset_triggered(){
    ui_com_settings->show();
}

void MainWindow::onConnectClicked() {
    if (!fl_connectedComport){
        settingsComport = ui_com_settings->settings();
        QString port = settingsComport.name;
        int baudRate = settingsComport.stringBaudRate.toInt();

        if (m_siStation->connectToStation(port, baudRate)) // Connect
        {
            fl_connectedComport = true;
            ui->act_connect_comport->setIcon(QIcon(":/rec/img/connect.png"));
            ui->act_comport_dialogset->setEnabled(false);
            showStatusMessage(tr("Connected to %1 : %2, %3")
                                  .arg(settingsComport.name, settingsComport.stringBaudRate, settingsComport.stringFlowControl));
        } else {
            fl_connectedComport = false;
            showStatusMessage(tr("Failed to connect to %1").arg(port));
        }
    }
    else {
        // Disconnect
        fl_connectedComport = false;
        ui->act_connect_comport->setIcon(QIcon(":/rec/img/disconnect.png"));
        ui->act_comport_dialogset->setEnabled(true);
        showStatusMessage(tr("Disconnected"));
        m_siStation->disconnectToStation();
        m_siStation->reset();
    }
}

void MainWindow::onCardDetected(uint32_t cardNumber, SportIdent::CardType type) {
    QString typeStr;
    switch (type) {
    case SportIdent::CardType::SI9:  typeStr = "SI9"; break;
    case SportIdent::CardType::SI10: typeStr = "SI10"; break;
    case SportIdent::CardType::SI11: typeStr = "SI11"; break;
    case SportIdent::CardType::SIAC: typeStr = "SIAC"; break;
    default: typeStr = "Unknown"; break;
    }

    logMessage(tr("Обнаружена карта %1 типа %2").arg(cardNumber).arg(typeStr));
}

void MainWindow::onCardRemoved() {
    logMessage(tr("Карта извлечена"));
}

void MainWindow::onCardReadComplete(const SportIdent::CardData& cardData) {
    m_resultProcessor->processCardData(cardData, this);
}

void MainWindow::onStationConnected(const SportIdent::StationInfo& info) {
    logMessage(
        tr("Станция %1 (SN: %2, FW: %3)")
            .arg(info.stationCode)
            .arg(info.serialNumber)
            .arg(info.firmwareVersion)
        );

    logMessage(tr("Станция подключена"));
}

void MainWindow::onErrorOccurred(const QString& errorMessage) {
    m_lastError = errorMessage;
    logMessage(tr("Ошибка: %1").arg(errorMessage));

    QMessageBox::warning(this, tr("Ошибка"), errorMessage);
}

void MainWindow::onDebugMessage(const QString& message) {
    qDebug() << message;
}

void MainWindow::logMessage(const QString& message) {
    QString msg = message;
    showStatusMessage(msg);
}

void MainWindow::showStatusMessage(const QString &message)
{
    statusbar_msg->setText(message);
    ui_log_msg(message);
}

void MainWindow::displayCardData(const SportIdent::CardData& cardData) {
    // Основная информация

    QString typeStr;
    switch (cardData.cardType) {
    case SportIdent::CardType::SI9: typeStr = "SI9"; break;
    case SportIdent::CardType::SI10: typeStr = "SI10"; break;
    case SportIdent::CardType::SI11: typeStr = "SI11"; break;
    case SportIdent::CardType::SIAC: typeStr = "SIAC"; break;
    default: typeStr = "Unknown"; break;
    }
    ui_log_msg(typeStr);
}

void MainWindow::connectModelsToWidgets()
{
    qDebug() << "Связываю модели с виджетами...";

    // Сначала связываем обычные модели
    if (m_proxyModels.contains("participants")) {
        ui->tablePerson->setModel(m_proxyModels["participants"]);
    }
    if (m_proxyModels.contains("age_groups")) {
        ui->tableGroup->setModel(m_proxyModels["age_groups"]);
    }
    if (m_proxyModels.contains("distances")) {
        ui->tableDist->setModel(m_proxyModels["distances"]);
    }
    if (m_proxyModels.contains("delegations")) {
        ui->tableOrg->setModel(m_proxyModels["delegations"]);
    }

    // Results подключаем сразу, без задержки
    if (m_proxyModels.contains("results")) {
        ui->tableResult->setModel(m_proxyModels["results"]);
    }

    // Подгоняем столбцы после установки моделей
    QTimer::singleShot(100, this, [this]() {
        ui->tablePerson->resizeColumnsToContents();
        ui->tableGroup->resizeColumnsToContents();
        ui->tableDist->resizeColumnsToContents();
        ui->tableOrg->resizeColumnsToContents();
        ui->tableResult->resizeColumnsToContents();
    });
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

    if (!m_document || !m_document->isOpen()) {
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
                QHash<QString, QVariant> fdata;
                fdata["competition_id"] = competitionId;
                fdata["name"] = record.delegationName;
                fdata["representative"] = "";
                fdata["contact"] = record.comment; // Используем комментарий как контакт

                qint64 delegationId = -1;
                if (db.createRecord("delegations", fdata, &delegationId)) {
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
                QHash<QString, QVariant> fdata;
                fdata["competition_id"] = competitionId;
                fdata["name"] = record.ageGroup;
                fdata["min_age"] = 0;
                fdata["max_age"] = 99;
                fdata["price"] = 1;

                qint64 ageGroupId = -1;
                if (db.createRecord("age_groups", fdata, &ageGroupId)) {
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
        QHash<QString, QVariant> fdata;
        fdata["competition_id"] = competitionId;
        fdata["delegation_id"] = delegationId;
        fdata["distance_id"] = distanceId;
        fdata["age_group_id"] = ageGroupId;
        fdata["participant_type"] = "individual";
        fdata["full_name"] = record.fullName;
        fdata["bib_number"] = record.bibNumber;

        // Год рождения
        if (!record.birthYear.isEmpty()) {
            bool ok;
            int year = record.birthYear.toInt(&ok);
            if (ok && year >= 1900 && year <= QDate::currentDate().year()) {
                QDate birthDate(year, 1, 1);
                fdata["birth_date"] = birthDate.toString(Qt::ISODate);
            }
        }

        // Номер чипа
        if (!record.chipNumber.isEmpty() && record.chipNumber != "0") {
            fdata["chip_number"] = record.chipNumber;
        }

        // Определяем пол
        QString gender = "Мужской";
        QString ageGroupLower = record.ageGroup.toLower();
        if (ageGroupLower.startsWith("ж-") ||
            ageGroupLower.contains("жен") ||
            ageGroupLower.contains("дев")) {
            gender = "Женский";
        }
        fdata["gender"] = gender;

        // // Спортивный разряд (в комментарий)
        // if (!record.sportRank.isEmpty() && record.sportRank != "0") {
        //     data["team_name"] = QString(tr("Разряд: %1")).arg(record.sportRank);
        // }

        // Создаем участника
        qint64 participantId = -1;
        if (db.createRecord("participants", fdata, &participantId)) {
            importedIds.append(participantId);
            importedData.append(fdata);
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
    // for (FilterProxyModel* proxy : qAsConst(m_proxyModels)) {
    for (auto* proxy : qAsConst(m_proxyModels)) {
        if (proxy) {
            proxy->setSourceModel(nullptr); // Важно: отключаем от исходной модели
            proxy->deleteLater();
        }
    }
    m_proxyModels.clear();

    // Удаляем модели
    for (SqlTableModel* model : qAsConst(m_tableModels)) {
        if (model) {
            model->clear(); // Очищаем данные
            model->deleteLater();
        }
    }
    m_tableModels.clear();

    qDebug() << "Модели очищены";
}


void MainWindow::setupMenuBar()
{
    // Меню уже настроено в createActionsAndConnections()
}

void MainWindow::onDocumentOpened()
{
    qDebug() << "Сигнал: документ открыт";

    // Даем время на завершение открытия БД
    QTimer::singleShot(150, this, [this]() {
        initializeForDocument();
    });

    QTimer::singleShot(500, this, [this]() {
        qDebug() << "=== ПРИНУДИТЕЛЬНОЕ ОБНОВЛЕНИЕ ТАБЛИЦЫ РЕЗУЛЬТАТОВ ===";

        // Перезапрашиваем данные для всех видимых ячеек
        QAbstractItemModel* model = ui->tableResult->model();
        if (model) {
            QModelIndex tl = model->index(0, 13);
            QModelIndex br = model->index(model->rowCount() - 1, 15);
            QMetaObject::invokeMethod(
                model,
                "dataChanged",
                Qt::QueuedConnection,
                Q_ARG(QModelIndex, tl),
                Q_ARG(QModelIndex, br)
                );
        }

        // Принудительная перерисовка
        ui->tableResult->viewport()->update();
    });
}

void MainWindow::onDocumentClosed()
{
    qDebug() << "\n=== ЗАКРЫТИЕ ДОКУМЕНТА ===";

    // Отключаем все сигналы от моделей перед очисткой
    for (auto model : qAsConst(m_tableModels)) {
        if (model) {
            model->disconnect();
        }
    }

    // Сбрасываем фильтры в моделях
    for (auto model : qAsConst(m_tableModels)) {
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
    //ui->act_save->setEnabled(modified);
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

void MainWindow::onCloseDocument()
{
    if (confirmUnsavedChanges()) {
        closeCurrentDocument();
    }
}

void MainWindow::on_act_undo_triggered()
{
    if (UndoStack::instance().canUndo()) {
        UndoStack::instance().undo();
        refreshAllTables(); // Обновляем все таблицы после undo
        updateStatusBar();
    }
}


void MainWindow::on_act_redo_triggered()
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
    qDebug() << "Слот вызван, sender:" << sender();

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

    //FilterProxyModel* proxyModel = m_proxyModels[tableName];
    AbstractProxyModel* proxyModel = m_proxyModels[tableName];

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
    // FilterProxyModel* proxyModel = m_proxyModels[m_currentTable];
    AbstractProxyModel* proxyModel = m_proxyModels[m_currentTable];
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
        title = QString("%1 - %2").arg(m_document->competitionName(),title);

        if (!m_document->filePath().isEmpty()) {
            title = QString("%1 - %2").arg(QFileInfo(m_document->filePath()).fileName(),title);
        }

        if (m_isModified) {
            title += " [*]";
        }
    }

    setWindowTitle(title);
}
