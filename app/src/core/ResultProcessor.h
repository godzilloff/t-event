// ResultProcessor.h

#ifndef RESULTPROCESSOR_H
#define RESULTPROCESSOR_H

#include <QObject>
#include <QHash>
#include "serial/sportident_types.h"

class Document;
class QWidget;

class ResultProcessor : public QObject
{
    Q_OBJECT

public:
    explicit ResultProcessor(Document* document, QObject* parent = nullptr);
    
    // Основной метод обработки данных карты
    bool processCardData(const SportIdent::CardData& cardData, QWidget* parentWidget = nullptr);
    
    // Настройки обработки
    void setAutoAssignBib(bool autoAssign) { m_autoAssignBib = autoAssign; }
    bool autoAssignBib() const { return m_autoAssignBib; }
    
    void setAskForMissingParticipant(bool ask) { m_askForMissingParticipant = ask; }
    bool askForMissingParticipant() const { return m_askForMissingParticipant; }

signals:
    void resultProcessed(qint64 resultId, const SportIdent::CardData& cardData);
    void resultProcessingFailed(const QString& reason, const SportIdent::CardData& cardData);
    void participantNotFound(uint32_t cardNumber, const SportIdent::CardData& cardData);
    void duplicateResultDetected(qint64 existingResultId, const SportIdent::CardData& cardData);

private:
    Document* m_document;
    bool m_autoAssignBib = false;           // Автоматически назначать bib по номеру карты?
    bool m_askForMissingParticipant = true;  // Спрашивать если участник не найден?
    
    // Кэш для быстрого поиска участников по номеру карты
    mutable QHash<uint32_t, qint64> m_participantCache;
    
    qint64 findParticipantByChipNumber(uint32_t chipNumber) const;
    qint64 createOrGetParticipant(const SportIdent::CardData& cardData, QWidget* parentWidget);
    qint64 createResultRecord(const SportIdent::CardData& cardData, qint64 participantId);
    void saveControlPointTimes(qint64 resultId, const QVector<SportIdent::Punch>& punches);
};

#endif // RESULTPROCESSOR_H
