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

signals:
    void responseReceived(const QString &response);

public slots:
    void handleResponse(QNetworkReply *reply);
    void sendRequest(const QUrl &url, const QJsonObject &jsonData);

private:
    QNetworkAccessManager *manager;

};

#endif // POSTREQUESTSENDER_H
