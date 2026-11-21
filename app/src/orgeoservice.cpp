#include "orgeoservice.h"
#include "DatabaseManager.h"

OrgeoService::OrgeoService(Document* document, PostRequestSender* sender, QObject* parent)
    : QObject(parent), m_document(document), m_sender(sender), m_retryTimer(new QTimer(this))
{
    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, &OrgeoService::processQueue);
    connect(m_sender, &PostRequestSender::responseReceived,
            this, &OrgeoService::onResponseReceived);
    connect(m_sender, &PostRequestSender::requestFailed,
            this, &OrgeoService::onRequestFailed);
}

void OrgeoService::setDocument(Document* document)
{
    m_document = document;

    if (m_document && m_document->isOpen()) {
        loadConfig();   // загрузит из orgeo_configs, выставит m_config, эмитит configChanged()
    } else {
        m_config = OrgeoConfig();  // сброс
        emit configChanged();
    }
    emit logMessage(m_document ? "Orgeo: документ привязан к сервису"
                               : "Orgeo: документ отсоединён");
}

void OrgeoService::loadConfig()
{
    if (!m_document) {
        emit logMessage("Orgeo: loadConfig — документ не привязан");
        return;
    }
    if (!m_document->isOpen()) {
        emit logMessage("Orgeo: loadConfig — документ не открыт");
        return;
    }

    const OrgeoConfig fresh = m_document->getOrgeoConfig();
    if (fresh == m_config) { return; } // ничего не изменилось

    m_config = fresh;
    emit configChanged();
    emit logMessage(QString("Orgeo: конфиг загружен (id=%1, URL='%2', enabled=%3)")
                        .arg(m_config.id).arg(m_config.liveUrl).arg(m_config.liveEnabled));
}

bool OrgeoService::saveConfig(const OrgeoConfig& config)
{
    if (!m_document) {
        emit logMessage("Orgeo: saveConfig — документ не привязан");
        return false;
    }
    if (!m_document->isOpen()) {
        emit logMessage("Orgeo: saveConfig — документ не открыт");
        return false;
    }

    OrgeoConfig mutableConfig = config;                 // lvalue
    const bool ok = m_document->saveOrgeoConfig(mutableConfig);

    if (ok) {
        loadConfig();
        emit configChanged();
        emit logMessage("Orgeo: конфиг сохранён");
    } else {
        emit logMessage("Orgeo: ОШИБКА сохранения конфига");
    }

    return ok;
}

// Очистка Orgeo
QJsonObject OrgeoService::buildClearAllRequest()
{
    // Пустой persons + params.start_list = true → полное стирание базы дня
    QJsonObject req;
    QJsonObject params;
    params["start_list"] = true;
    req["params"]  = params;
    req["persons"] = QJsonArray();      // явно пустой массив
    return req;
}

// Формирование Person (все поля обязательны — API «полного снимка»)
QJsonObject OrgeoService::buildPersonObject(qint64 participantId, qint64 resultId)
{
    QJsonObject person;
    auto& db = DatabaseManager::instance();
    QString sql = R"(
        SELECT p.bib_number, p.full_name, p.chip_number, p.birth_date,
               ag.name AS age_group_name, d.name AS delegation_name,
               r.id AS result_id, r.start_time, r.result_time, r.status
        FROM participants p
        LEFT JOIN age_groups ag ON p.age_group_id = ag.id
        LEFT JOIN delegations d ON p.delegation_id = d.id
        LEFT JOIN results r ON r.participant_id = p.id AND r.id = :result_id
        WHERE p.id = :participant_id
    )";
    QHash<QString, QVariant> params;
    params["participant_id"] = participantId;
    params["result_id"] = resultId > 0 ? resultId : -1;
    auto rows = db.executeSelect(sql, params);
    if (rows.isEmpty()) return person;

    auto& r = rows.first();
    const QString bib = r["bib_number"].toString();
    person["ref_id"]       = bib;
    person["bib"]          = bib;
    person["group_name"]   = r["age_group_name"].toString();
    person["name"]         = r["full_name"].toString();
    person["organization"] = r["delegation_name"].toString();
    person["out_of_competition"] = false;

    // Год рождения (обязательное поле с 08.08.2022)
    const QString bd = r["birth_date"].toString();
    if (!bd.isEmpty()) {
        QDate d = QDate::fromString(bd, Qt::ISODate);
        if (d.isValid()) person["year"] = d.year();
    }

    // Номер чипа
    const QString chip = r["chip_number"].toString();
    if (!chip.isEmpty()) {
        bool ok;
        int num = chip.toInt(&ok);
        if (ok) person["card_number"] = num;
    }

    // Время старта — секунды от начала суток
    const QString startStr = r["start_time"].toString();
    if (!startStr.isEmpty()) {
        QDateTime dt = QDateTime::fromString(startStr, "dd.MM.yyyy HH:mm:ss.zzz");
        if (!dt.isValid()) dt = QDateTime::fromString(startStr, Qt::ISODate);
        if (dt.isValid()) {
            const QTime t = dt.time();
            person["start"] = t.hour()*3600 + t.minute()*60 + t.second();
        }
    }

    // Результат — result_ms в миллисекундах (согласно примеру API: 12000 = 12 c)
    if (resultId > 0) {
        const QString rtStr = r["result_time"].toString();
        if (!rtStr.isEmpty()) {
            QTime rt = QTime::fromString(rtStr, "hh:mm:ss.zzz");
            if (!rt.isValid()) rt = QTime::fromString(rtStr, "hh:mm:ss");
            if (rt.isValid()) {
                person["result_ms"] = (rt.hour()*3600000 + rt.minute()*60000
                                       + rt.second()*1000 + rt.msec()) / 10;
            }
        }
        const QString st = r["status"].toString();
        QString rs = "OK";
        if (st == "dns") rs = "DNS";
        else if (st == "dnf") rs = "DNF";
        else if (st == "dsq") rs = "DSQ";
        person["result_status"] = rs;

        if (m_config.splitsEnabled)
            person["splits"] = buildSplitsArray(resultId);
    }
    return person;
}

QJsonArray OrgeoService::buildSplitsArray(qint64 resultId)
{
    QJsonArray arr;
    auto& db = DatabaseManager::instance();
    QHash<QString, QVariant> p;
    p["result_id"] = resultId;
    auto rows = db.executeSelect(R"(
        SELECT control_point_id, visit_time FROM control_point_times
        WHERE result_id = :result_id ORDER BY order_number)", p);
    for (const auto& r : rows) {
        QJsonObject s;
        s["code"] = r["control_point_id"].toInt();
        QDateTime dt = QDateTime::fromString(r["visit_time"].toString(), Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(r["visit_time"].toString(),
                                       "dd.MM.yyyy HH:mm:ss.zzz");
        if (dt.isValid()) {
            const QTime t = dt.time();
            s["time"] = t.hour()*3600000 + t.minute()*60000
                      + t.second()*1000 + t.msec();
        }
        arr.append(s);
    }
    return arr;
}

QJsonObject OrgeoService::buildStartListRequest()
{
    QJsonObject req;
    QJsonObject params;
    params["start_list"] = true;    // полная перезапись протокола дня
    req["params"] = params;

    QJsonArray persons;
    auto& db = DatabaseManager::instance();
    QHash<QString, QVariant> p;
    p["competition_id"] = m_document->competitionId();
    auto parts = db.executeSelect(
        "SELECT id FROM participants WHERE competition_id = :competition_id "
        "ORDER BY bib_number", p);
    for (const auto& r : parts) {
        QJsonObject per = buildPersonObject(r["id"].toLongLong());
        if (!per.isEmpty()) persons.append(per);
    }
    req["persons"] = persons;
    return req;
}

QJsonObject OrgeoService::buildResultRequest(qint64 resultId)
{
    QJsonObject req;
    QJsonArray persons;
    auto& db = DatabaseManager::instance();
    QHash<QString, QVariant> p;
    p["result_id"] = resultId;
    auto rows = db.executeSelect(
        "SELECT participant_id FROM results WHERE id = :result_id", p);
    if (!rows.isEmpty()) {
        QJsonObject per = buildPersonObject(
            rows.first()["participant_id"].toLongLong(), resultId);
        if (!per.isEmpty()) persons.append(per);
    }
    req["persons"] = persons;
    return req;
}

QJsonObject OrgeoService::buildSplitsRequest(qint64 resultId)
{
    QJsonObject req;
    QJsonArray persons;
    auto& db = DatabaseManager::instance();
    QHash<QString, QVariant> p;
    p["result_id"] = resultId;
    auto rows = db.executeSelect(
        "SELECT participant_id FROM results WHERE id = :result_id", p);
    if (!rows.isEmpty()) {
        QJsonObject per = buildPersonObject(
            rows.first()["participant_id"].toLongLong(), resultId);
        per["splits"] = buildSplitsArray(resultId);
        persons.append(per);
    }
    req["persons"] = persons;
    return req;
}

// --- Отправка ---
void OrgeoService::sendClearAll()
{
    if (!m_config.liveEnabled || m_config.liveUrl.isEmpty()) {
        emit logMessage("Трансляция отключена или URL не задан — очистка невозможна");
        return;
    }

    // Очистка — операция, которая делает дальнейшее состояние «пустым».
    // Сбрасываем очередь: в ней могут висеть результаты, которые
    // бессмысленно отправлять после стирания.
    if (!m_queue.isEmpty()) {
        emit logMessage(QString("Очередь Orgeo: сброшено %1 запрос(ов) "
                                "перед очисткой").arg(m_queue.size()));
        m_queue.clear();
        m_uploading = false;
        m_retryTimer->stop();
        emit queueSizeChanged(0);
    }

    emit logMessage("Отправка команды полной очистки данных дня на Orgeo");
    enqueue(RequestType::ClearAll, buildClearAllRequest());
}

void OrgeoService::sendStartList()
{
    if (!m_config.liveEnabled || m_config.liveUrl.isEmpty()) {
        emit logMessage("Трансляция отключена или URL не задан");
        return;
    }
    QJsonObject payload = buildStartListRequest();
    const int n = payload.value("persons").toArray().size();
    emit logMessage(QString("Стартовый протокол: %1 участников").arg(n));
    enqueue(RequestType::StartList, payload);
}

void OrgeoService::sendResult(qint64 resultId)
{
    if (!m_config.liveEnabled || !m_config.resultsEnabled
        || m_config.liveUrl.isEmpty()) {
        emit logMessage("Orgeo: sendResult пропущен — трансляция "
                        "или отправка результатов отключена");
        return;
    }
    QJsonObject payload = buildResultRequest(resultId);
    if (!payload.value("persons").toArray().isEmpty())
        enqueue(RequestType::Result, payload);
}

void OrgeoService::sendSplits(qint64 resultId)
{
    if (!m_config.liveEnabled || !m_config.resultsEnabled
        || m_config.liveUrl.isEmpty()) {
        emit logMessage("Orgeo: sendSplits пропущен — трансляция "
                        "или отправка результатов отключена");
        return;
    }
    QJsonObject payload = buildSplitsRequest(resultId);
    if (!payload.value("persons").toArray().isEmpty())
        enqueue(RequestType::Splits, payload);
}

void OrgeoService::sendAllPendingResults()
{
    auto& db = DatabaseManager::instance();
    QHash<QString, QVariant> p;
    p["competition_id"] = m_document->competitionId();
    auto rows = db.executeSelect(R"(
        SELECT r.id FROM results r
        JOIN participants p ON p.id = r.participant_id
        WHERE p.competition_id = :competition_id)", p);
    for (const auto& r : rows)
        sendResult(r["id"].toLongLong());
}

// --- Очередь ---
void OrgeoService::enqueue(RequestType type, const QJsonObject& payload)
{
    QueuedRequest req;
    req.type = type;
    req.payload = payload;
    m_queue.enqueue(req);
    emit queueSizeChanged(m_queue.size());
    if (!m_uploading) processQueue();
}

void OrgeoService::processQueue()
{
    if (m_uploading || m_queue.isEmpty()) return;
    if (m_config.liveUrl.isEmpty()) {
        emit logMessage("URL Orgeo не задан");
        return;
    }
    m_uploading = true;
    QueuedRequest& req = m_queue.head();
    const QString tn = requestTypeName(req.type);
    emit uploadStarted(tn);
    emit logMessage(QString("Отправка «%1» (попытка %2)")
                    .arg(tn).arg(req.attempts + 1));
    m_sender->sendRequest(QUrl(m_config.liveUrl), req.payload);
}

void OrgeoService::onResponseReceived(int httpStatus, const QString& body,
                                      const QString& headerMessage)
{
    if (!m_uploading || m_queue.isEmpty()) return;
    QueuedRequest& req = m_queue.head();
    const QString tn = requestTypeName(req.type);

    if (httpStatus == 200) {
        emit logMessage(QString("%1: %2").arg(tn, body));
        emit uploadFinished(tn, true, body);
        m_queue.dequeue();
        m_uploading = false;
        emit queueSizeChanged(m_queue.size());
        if (!m_queue.isEmpty())
            QTimer::singleShot(100, this, &OrgeoService::processQueue);
        return;
    }

    // Ошибка — повтор
    req.attempts++;
    req.lastError = body.isEmpty() ? headerMessage : body;
    m_uploading = false;
    emit logMessage(QString("%1: HTTP %2 — %3")
                    .arg(tn).arg(httpStatus).arg(req.lastError));

    if (req.attempts >= m_maxAttempts) {
        emit logMessage(QString("%1: превышено число попыток").arg(tn));
        emit uploadFinished(tn, false, req.lastError);
        m_queue.dequeue();
        emit queueSizeChanged(m_queue.size());
    } else {
        emit uploadFinished(tn, false, req.lastError);
        scheduleRetry(req.lastError);
    }
}

void OrgeoService::onRequestFailed(const QString& errorString)
{
    if (!m_uploading || m_queue.isEmpty()) return;
    QueuedRequest& req = m_queue.head();
    req.attempts++;
    req.lastError = errorString;
    m_uploading = false;
    emit logMessage("Сетевая ошибка: " + errorString);
    scheduleRetry(errorString);
}

void OrgeoService::scheduleRetry(const QString&)
{
    emit logMessage(QString("Повтор через %1 сек").arg(m_retryIntervalMs / 1000));
    m_retryTimer->start(m_retryIntervalMs);
}

QString OrgeoService::requestTypeName(RequestType t)
{
    switch (t) {
    case RequestType::ClearAll:  return "Очистка данных Orgeo";
    case RequestType::StartList: return "Стартовый протокол";
    case RequestType::Result:    return "Результат";
    case RequestType::Splits:    return "Сплиты";
    }
    return "Запрос";
}
