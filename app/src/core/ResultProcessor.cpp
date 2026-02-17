// ResultProcessor.cpp

#include "ResultProcessor.h"
#include "Document.h"
#include "DatabaseManager.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

ResultProcessor::ResultProcessor(Document* document, QObject* parent)
    : QObject(parent)
    , m_document(document)
{
    Q_ASSERT(m_document);
}

bool ResultProcessor::processCardData(const SportIdent::CardData& cardData, QWidget* parentWidget)
{
    if (!m_document->isOpen()) {
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
        participantId = createOrGetParticipant(cardData, parentWidget);
        if (participantId == -1) {
            emit resultProcessingFailed(tr("Не удалось определить участника"), cardData);
            return false;
        }
    }
    
    // Проверяем, нет ли уже результата для этого участника
    auto& db = DatabaseManager::instance();
    auto existingResults = db.findRecords("results", "participant_id", participantId);
    
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
    auto& db = DatabaseManager::instance();
    auto participants = db.findRecords("participants", "chip_number", 
                                       QString::number(chipNumber));
    
    if (!participants.isEmpty()) {
        m_participantCache[chipNumber] = participants.first();
        return participants.first();
    }
    
    return -1;
}

qint64 ResultProcessor::createOrGetParticipant(const SportIdent::CardData& cardData, 
                                                QWidget* parentWidget)
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
    
    // Ищем участника по bib номеру
    auto& db = DatabaseManager::instance();
    auto participants = db.findRecords("participants", "bib_number", bibNumber);
    
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
