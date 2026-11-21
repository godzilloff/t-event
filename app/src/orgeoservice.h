#ifndef ORGEOSERVICE_H
#define ORGEOSERVICE_H

#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "postrequestsender.h"
#include "Document.h"

class OrgeoService : public QObject
{
    Q_OBJECT
public:
    enum class RequestType { StartList, Result, Splits, ClearAll };

    explicit OrgeoService(Document* document, PostRequestSender* sender,
                          QObject* parent = nullptr);

    void setDocument(Document* document);
    Document* document() const { return m_document; }
    void loadConfig();
    bool saveConfig(const OrgeoConfig& config);
    OrgeoConfig config() const { return m_config; }

    void sendClearAll();
    void sendStartList();
    void sendResult(qint64 resultId);
    void sendSplits(qint64 resultId);
    void sendAllPendingResults();

    int queueSize() const { return m_queue.size(); }
    bool isUploading() const { return m_uploading; }
    bool isEnabled() const { return m_config.liveEnabled; }
    bool isResultsEnabled() const { return m_config.resultsEnabled; }
    bool isSplitsEnabled() const { return m_config.splitsEnabled; }

signals:
    void uploadStarted(const QString& type);
    void uploadFinished(const QString& type, bool success, const QString& message);
    void queueSizeChanged(int size);
    void configChanged();
    void logMessage(const QString& message);

private slots:
    void onResponseReceived(int httpStatus, const QString& body,
                            const QString& headerMessage);
    void onRequestFailed(const QString& errorString);
    void processQueue();

private:
    struct QueuedRequest {
        RequestType type;
        QJsonObject payload;
        int attempts = 0;
        QString lastError;
    };

    QJsonObject buildClearAllRequest();
    QJsonObject buildPersonObject(qint64 participantId, qint64 resultId = -1);
    QJsonArray  buildSplitsArray(qint64 resultId);
    QJsonObject buildStartListRequest();
    QJsonObject buildResultRequest(qint64 resultId);
    QJsonObject buildSplitsRequest(qint64 resultId);

    void enqueue(RequestType type, const QJsonObject& payload);
    void scheduleRetry(const QString& error);
    static QString requestTypeName(RequestType type);

    QPointer<Document> m_document;
    PostRequestSender* m_sender;
    OrgeoConfig m_config;
    QQueue<QueuedRequest> m_queue;
    bool m_uploading = false;
    QTimer* m_retryTimer;
    int m_maxAttempts = 10;
    int m_retryIntervalMs = 10000;   // согласно документации — минимум 10 сек
};

#endif
