#ifndef POSTREQUESTSENDER_H
#define POSTREQUESTSENDER_H

#include <QObject>

#include <QCoreApplication>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>

class PostRequestSender : public QObject
{
    Q_OBJECT
public:
    explicit PostRequestSender(QObject *parent = nullptr);
    ~PostRequestSender();

public slots:
    void sendRequest(const QUrl &url, const QJsonObject &jsonData);

signals:
    // httpStatus, тело, заголовок ORGEO-ONLINE-RESPONSE-MESSAGE
    void responseReceived(int httpStatus, const QString& body, const QString& headerMessage);
    void requestFailed(const QString& errorString);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager m_manager;
};

#endif // POSTREQUESTSENDER_H
