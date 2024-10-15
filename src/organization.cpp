#include "organization.h"

organization::organization() {}

organization::organization(QJsonObject objJson)
{
    data.name = objJson.value("name").toString();
    data.id = objJson.value("id").toString();
    data.code = objJson.value("code").toString();
    data.contact = objJson.value("contact").toString();
    data.country = objJson.value("country").toString();
    data.region = objJson.value("region").toString();

    //qDebug() << objJson.value("name").toString();
}

QJsonObject organization::toJson() const
{
    QJsonObject json;
    json["name"] = data.name;
    json["id"] = data.id;
    json["code"] = data.code;
    json["contact"] = data.contact;
    json["country"] = data.country;
    json["region"] = data.region;
    //json[""] = ;

    return json;
}
