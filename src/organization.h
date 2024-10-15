#ifndef ORGANIZATION_H
#define ORGANIZATION_H


#include <QJsonObject>
#include <QJsonArray>

struct st_organization{
    int count_person = 0;
    QString name = "";
    QString id = "";
    QString code = "";
    QString contact = "";
    QString country = "";
    QString region = "";
    int sex = 0;
    QString object = "Organization";
};

// "organizations": [
// {
//   "code": "",
//   "contact": "",
//   "count_person": 0,
//   "country": "",
//   "id": "6f3f498f-4d93-4927-9a47-926a47c12f7c",
//   "name": "\u043f\u0435\u0440\u0432\u0430\u044f",
//   "object": "Organization",
//   "region": "",
//   "sex": 0
// }

class organization
{
public:
    organization();
    organization(QJsonObject objJson);
    QJsonObject toJson() const;

    QString getName(){ return data.name;};
    QString getId(){ return data.id;};
    QString getCode(){ return data.code;};
    QString getContact(){ return data.contact;};
    QString getCountry(){ return data.country;};
    QString getRegion(){ return data.region;};

private:
    st_organization data;
};

#endif // ORGANIZATION_H
