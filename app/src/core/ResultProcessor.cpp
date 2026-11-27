// ResultProcessor.cpp

#include "ResultProcessor.h"
#include "Document.h"
#include "DatabaseManager.h"
#include "functions.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include <QCoreApplication>
#include <QDate>
#include <QTime>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

ResultProcessor::ResultProcessor(Document* document, QObject* parent)
    : QObject(parent)
    , m_document(document)
{
    Q_ASSERT(m_document);
}

bool ResultProcessor::processCardData(const SportIdent::CardData& cardData, QWidget* parentWidget)
{
    // Сначала — лог в backup.txt рядом с .exe
    appendCardDataToBackup(cardData);

    if (m_document == nullptr) {
        emit resultProcessingFailed(tr("Документ не открыт"), cardData);
        return false;
    }
    
    if (!cardData.isValid()) {
        emit resultProcessingFailed(tr("Невалидные данные карты"), cardData);
        return false;
    }
    
    // Проверяем, есть ли участник с таким номером карты
    qint64 participantId = findParticipantByChipNumber(cardData.cardNumber);
    
    if (participantId == -1) {
        if (!m_askForMissingParticipant) {
            emit participantNotFound(cardData.cardNumber, cardData);
            return false;
        }
        
        // Участник не найден - спрашиваем пользователя
        QString bib;
        participantId = createOrGetParticipant(cardData, parentWidget, &bib);
        if (participantId == -1) {
            emit resultProcessingFailed(tr("Не удалось определить участника"), cardData);
            return false;
        }
        appendChipBibMapping(cardData.cardNumber, bib);
    }
    
    // Проверяем, нет ли уже результата для этого участника
    const auto& db = DatabaseManager::instance();
    const auto existingResults = db.findRecords("results", "participant_id", participantId);
    
    if (!existingResults.isEmpty()) {
        // Нашли существующий результат
        emit duplicateResultDetected(existingResults.first(), cardData);
        
        // Спрашиваем пользователя, что делать
        if (parentWidget) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                parentWidget,
                tr("Дубликат результата"),
                tr("Для участника уже существует результат.\n"
                   "Заменить существующий результат новым?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            
            if (reply == QMessageBox::No) {
                return false;
            }
            
            // Удаляем старый результат
            m_document->deleteRecord("results", existingResults.first());
        } else {
            return false;
        }
    }
    
    // Создаем новый результат
    qint64 resultId = createResultRecord(cardData, participantId);
    if (resultId == -1) {
        return false;
    }
    
    emit resultProcessed(resultId, cardData);
    return true;
}

qint64 ResultProcessor::findParticipantByChipNumber(uint32_t chipNumber) const
{
    // Проверяем кэш
    if (m_participantCache.contains(chipNumber)) {
        return m_participantCache[chipNumber];
    }
    
    // Ищем в базе данных
    const auto& db = DatabaseManager::instance();
    const auto participants = db.findRecords("participants", "chip_number",
                                       QString::number(chipNumber));
    
    if (!participants.isEmpty()) {
        m_participantCache[chipNumber] = participants.first();
        return participants.first();
    }
    
    return -1;
}

qint64 ResultProcessor::createOrGetParticipant(const SportIdent::CardData& cardData, 
                                                QWidget* parentWidget,
                                                QString* outBib )
{
    if (!parentWidget) {
        return -1;
    }
    
    // Диалог для ввода номера участника
    bool ok;
    QString bibNumber = QInputDialog::getText(
        parentWidget,
        tr("Номер участника"),
        tr("Участник с номером карты %1 не найден.\n"
           "Введите номер участника (bib):").arg(cardData.cardNumber),
        QLineEdit::Normal,
        QString(),
        &ok
    );
    
    if (!ok || bibNumber.isEmpty()) {
        return -1;
    }

    if (outBib)
        *outBib = bibNumber;
    
    // Ищем участника по bib номеру
    const auto& db = DatabaseManager::instance();
    const auto participants = db.findRecords("participants", "bib_number", bibNumber);
    
    if (!participants.isEmpty()) {
        qint64 participantId = participants.first();
        
        // Обновляем номер карты для этого участника
        QHash<QString, QVariant> updateData;
        updateData["chip_number"] = QString::number(cardData.cardNumber);
        
        if (m_document->updateRecord("participants", participantId, updateData)) {
            m_participantCache[cardData.cardNumber] = participantId;
            return participantId;
        }
    }
    
    // Участника нет вообще - предлагаем создать
    QMessageBox::StandardButton reply = QMessageBox::question(
        parentWidget,
        tr("Создать участника?"),
        tr("Участник с номером %1 не найден.\n"
           "Создать нового участника?").arg(bibNumber),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Создаем нового участника
        QHash<QString, QVariant> newParticipant;
        newParticipant["competition_id"] = m_document->competitionId();
        newParticipant["participant_type"] = "individual";
        newParticipant["bib_number"] = bibNumber;
        newParticipant["chip_number"] = QString::number(cardData.cardNumber);
        newParticipant["full_name"] = tr("Участник %1").arg(bibNumber);
        
        qint64 newId = m_document->insertRecord("participants", newParticipant);
        if (newId != -1) {
            m_participantCache[cardData.cardNumber] = newId;
            return newId;
        }
    }
    
    return -1;
}

qint64 ResultProcessor::createResultRecord(const SportIdent::CardData& cardData, 
                                            qint64 participantId)
{
    QHash<QString, QVariant> resultData;
    resultData["participant_id"] = participantId;
    resultData["chip_number"] = QString::number(cardData.cardNumber);
    
    // Сохраняем времена в ISO формате
    if (cardData.startTime.isValid()) {
        //resultData["start_time"] = cardData.startTime.toString(Qt::ISODate);
        resultData["start_time"] = cardData.startTime.toString("dd.MM.yyyy HH:mm:ss.zzz");
    }
    
    if (cardData.finishTime.isValid()) {
        //resultData["finish_time"] = cardData.finishTime.toString(Qt::ISODate);
        resultData["finish_time"] = cardData.finishTime.toString("dd.MM.yyyy HH:mm:ss.zzz");
    }
    
    // Вычисляем результат в секундах
    if (cardData.startTime.isValid() && cardData.finishTime.isValid()) {
        //qint64 resultSeconds = cardData.startTime.secsTo(cardData.finishTime);
        //resultData["result_time"] = resultSeconds;
        qint64 resultMs = cardData.startTime.msecsTo(cardData.finishTime);
        QTime interval = QTime(0, 0, 0).addMSecs(resultMs);
        resultData["result_time"] = interval.toString("hh:mm:ss.zzz");
    }
    
    resultData["status"] = "finished";
    
    // Создаем запись результата
    qint64 resultId = m_document->insertRecord("results", resultData);
    if (resultId == -1) {
        return -1;
    }

    // Сохраняем отметки КП
    saveControlPointTimes(resultId, cardData.punches);
    emit resultReadyForUpload(resultId);
    
    return resultId;
}

void ResultProcessor::saveControlPointTimes(qint64 resultId, 
                                             const QVector<SportIdent::Punch>& punches)
{
    if (punches.isEmpty()) {
        return;
    }
    
    auto& db = DatabaseManager::instance();
    db.beginTransaction();
    
    for (int i = 0; i < punches.size(); ++i) {
        const auto& punch = punches[i];
        
        QHash<QString, QVariant> cpData;
        cpData["result_id"] = resultId;
        cpData["control_point_id"] = punch.controlCode;
        cpData["visit_time"] = punch.timestamp.toString(Qt::ISODate);
        cpData["order_number"] = i + 1;
        
        // Используем прямой доступ к БД, чтобы не создавать отдельные команды undo
        // для каждой отметки (это засорило бы историю)
        DatabaseManager::instance().createRecord("control_point_times", cpData);
    }
    
    db.commitTransaction();
}

PrintResultData ResultProcessor::getPrintDataForParticipant(qint64 participantId) const
{
    PrintResultData data{};

    if (!m_document) {
        return data;
    }

    auto& db = DatabaseManager::instance();

    // Получаем данные участника
    QString sql = R"(
        SELECT
            p.full_name,
            p.bib_number,
            p.chip_number,
            p.start_time,
            ag.name as age_group_name,
            d.name as delegation_name,
            r.result_time,
            r.finish_time
        FROM participants p
        LEFT JOIN age_groups ag ON p.age_group_id = ag.id
        LEFT JOIN delegations d ON p.delegation_id = d.id
        LEFT JOIN results r ON r.participant_id = p.id
        WHERE p.id = :participant_id
    )";

    QHash<QString, QVariant> params;
    params["participant_id"] = participantId;

    auto results = db.executeSelect(sql, params);
    if (!results.isEmpty()) {
        auto& record = results.first();

        data.fullName = record["full_name"].toString();
        data.bibNumber = record["bib_number"].toString();
        data.chipNumber = record["chip_number"].toString();
        data.ageGroup = record["age_group_name"].toString();
        data.delegationName = record["delegation_name"].toString();

        // Времена
        // QString startTimeStr = record["start_time"].toString();
        // if (!startTimeStr.isEmpty()) {
        //     data.startTime = QDateTime::fromString(startTimeStr, Qt::ISODate);
        // }

        // QString finishTimeStr = record["finish_time"].toString();
        // if (!finishTimeStr.isEmpty()) {
        //     data.finishTime = QDateTime::fromString(finishTimeStr, Qt::ISODate);
        // }

        // Парсинг времени с поддержкой обоих форматов
        auto parseDateTime = [](const QString& s) -> QDateTime {
            if (s.isEmpty()) return {};
            QDateTime dt = QDateTime::fromString(s, Qt::ISODate);
            if (!dt.isValid())
                dt = QDateTime::fromString(s, "dd.MM.yyyy HH:mm:ss.zzz");
            return dt;
        };

        data.startTime  = parseDateTime(record["start_time"].toString());
        data.finishTime = parseDateTime(record["finish_time"].toString());

        // result_time хранится как "hh:mm:ss.zzz" — парсим как QTime
        const QString rtStr = record["result_time"].toString();
        if (!rtStr.isEmpty()) {
            QTime rt = QTime::fromString(rtStr, "hh:mm:ss.zzz");
            if (!rt.isValid()) rt = QTime::fromString(rtStr, "hh:mm:ss");
            if (rt.isValid())
                data.resultSeconds = QTime(0, 0, 0).secsTo(rt);
        }
    }

    // Информация о соревновании
    data.competitionName = m_document->competitionName();
    data.competitionDate = QDate::currentDate(); // или из документа

    // Формируем URL для online результатов
    if (!m_onlineResultsBaseUrl.isEmpty()) {
        data.onlineResultsUrl = QString("%1/result/%2")
                                    .arg(m_onlineResultsBaseUrl)
                                    .arg(participantId);
    }

    return data;
}

QString ResultProcessor::formatCardTime(const QDateTime& dt)
{
    if (!dt.isValid())
        return QStringLiteral("00:00:00.0");

    const QTime t = dt.time();
    // Округляем до десятых долей секунды
    int tenths = (t.msec() + 50) / 100;   // 0..10
    int h = t.hour();
    int m = t.minute();
    int s = t.second();
    if (tenths == 10) {
        tenths = 0;
        if (++s == 60) { s = 0; if (++m == 60) { m = 0; if (++h == 24) h = 0; } }
    }

    return QStringLiteral("%1:%2:%3.%4")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'))
        .arg(tenths);
}

void ResultProcessor::appendCardDataToBackup(const SportIdent::CardData& cardData)
{
    const QString path = QCoreApplication::applicationDirPath()
                         + QStringLiteral("/backup.txt");

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Не удалось открыть backup.txt:" << file.errorString();
        return;
    }

    // Все времена — в десятых долях суток, уже округлённые
    const qint64 clearT  = tenthsOfDay(cardData.clearTime);
    const qint64 checkT  = tenthsOfDay(cardData.checkTime);
    const qint64 startT  = tenthsOfDay(cardData.startTime);
    const qint64 finishT = tenthsOfDay(cardData.finishTime);

    // Результат считаем из ОКРУГЛЁННЫХ start/finish,
    // чтобы строка была согласована с выведенными выше отметками.
    qint64 resultT = -1;
    if (startT >= 0 && finishT >= 0 && finishT >= startT)
        resultT = finishT - startT;

    // Время считывания — тоже в десятых, для единообразия
    const QTime now = QTime::currentTime();
    const qint64 nowTenths = (now.hour()   * 36000LL
                              + now.minute() *   600LL
                              + now.second() *    10LL
                              + (now.msec() + 50) / 100);

    QString line;
    line.reserve(160);
    line += QDate::currentDate().toString(QStringLiteral("dd.MM.yyyy"));
    line += QLatin1Char(';');
    line += formatTenths(nowTenths);
    line += QLatin1Char(';');
    line += QString::number(cardData.cardNumber);
    line += QLatin1Char(';');
    line += formatTenths(clearT);
    line += QLatin1Char(';');
    line += formatTenths(checkT);
    line += QLatin1Char(';');
    line += formatTenths(startT);
    line += QLatin1Char(';');
    line += formatTenths(finishT);
    line += QLatin1Char(';');
    line += formatTenths(resultT);
    line += QLatin1Char('\n');

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << line;
}

void ResultProcessor::appendChipBibMapping(uint32_t cardNumber, const QString& bib)
{
    // Отдельный файл — семантически чище, чем мешать с backup.txt
    const QString path = QCoreApplication::applicationDirPath()
                         + QStringLiteral("/chips_bib.txt");

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Не удалось открыть chips_bib.csv:" << file.errorString();
        return;
    }

    // Санитайзинг bib: убираем разделители и переводы строк
    QString safeBib = bib;
    safeBib.remove(QRegularExpression("[;\\r\\n]"));
    safeBib = safeBib.trimmed();

    QString line;
    line.reserve(64);
    line += QDateTime::currentDateTime().toString("dd.MM.yyyy;HH:mm:ss");
    line += QLatin1Char(';');
    line += QString::number(cardNumber);
    line += QLatin1Char(';');
    line += safeBib;
    line += QLatin1Char('\n');

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << line;
}
