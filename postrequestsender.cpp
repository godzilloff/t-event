#include "postrequestsender.h"

PostRequestSender::PostRequestSender(QObject *parent)
    : QObject{parent}, manager(new QNetworkAccessManager(this))
{}

PostRequestSender::~PostRequestSender()
{
    delete manager;
}

void PostRequestSender::sendRequest(const QUrl &url, const QJsonObject &jsonData) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray data = QJsonDocument(jsonData).toJson();
    manager->post(request, data);
}

void PostRequestSender::handleResponse(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QString response = reply->readAll();
        emit responseReceived(response);
        reply->deleteLater();
    } else {
        qDebug() << "Error: " << reply->errorString();
    }
}
