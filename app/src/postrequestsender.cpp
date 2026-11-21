#include "postrequestsender.h"
#include <QJsonParseError>
#include <QDebug>

PostRequestSender::PostRequestSender(QObject *parent)
    : QObject(parent)
{
    connect(&m_manager, &QNetworkAccessManager::finished,
            this, &PostRequestSender::onReplyFinished);
}
PostRequestSender::~PostRequestSender() = default;

void PostRequestSender::sendRequest(const QUrl &url, const QJsonObject &jsonData)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    QByteArray data = QJsonDocument(jsonData).toJson(QJsonDocument::Compact);
    qDebug() << "POST" << url.toString() << "bytes:" << data.size();
    m_manager.post(request, data);
}

void PostRequestSender::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    const int httpStatus = reply->attribute(
                                    QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    const QString headerMsg = QString::fromUtf8(
        reply->rawHeader("ORGEO-ONLINE-RESPONSE-MESSAGE"));

    if (reply->error() != QNetworkReply::NoError && body.isEmpty()) {
        emit requestFailed(reply->errorString());
        return;
    }

    QString responseText;
    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(body, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject())
        responseText = doc.object().value("response").toString();
    else
        responseText = QString::fromUtf8(body).left(500); // HTML/пустой

    qDebug() << "HTTP" << httpStatus << responseText;
    emit responseReceived(httpStatus, responseText, headerMsg);
}
