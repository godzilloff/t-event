#include "race.h"

race::race(){}

race::race(QJsonObject objJson)
{
    QJsonArray jsonArrCourses;
    QJsonArray jsonArrGroup;
    QJsonArray jsonArrOrganization;
    QJsonArray jsonArrPerson;
    QJsonArray jsonArrResult;

    updateData(objJson.value("data").toObject());

    jsonArrCourses = objJson.value("courses").toArray();
    for(int ii = 0; ii < jsonArrCourses.count(); ii++){
        course* course_ = new course(jsonArrCourses.at(ii).toObject());
        courses_.push_back(course_);
    }

    jsonArrGroup = objJson.value("groups").toArray();
    for(int ii = 0; ii < jsonArrGroup.count(); ii++){
        group* group_ = new group(jsonArrGroup.at(ii).toObject());
        groups_.push_back(group_);
    }

    jsonArrOrganization = objJson.value("organizations").toArray();
    for(int ii = 0; ii < jsonArrOrganization.count(); ii++){
        organization* organization_ = new organization(jsonArrOrganization.at(ii).toObject());
        organizations_.push_back(organization_);
    }

    jsonArrPerson = objJson.value("persons").toArray();
    for(int ii = 0; ii < jsonArrPerson.count(); ii++){
        person* person_ = new person(jsonArrPerson.at(ii).toObject());
        persons_.push_back(person_);
    }

    qDebug() << " end load persons " ;

    jsonArrResult = objJson.value("results").toArray();
    for(int ii = 0; ii < jsonArrResult.count(); ii++){
        result* result_ = new result(jsonArrResult.at(ii).toObject());
        results_.push_back(result_);
    }
}


// {
//     "persons": [
//         {
//             "ref_id": "550e8400-e29b-41d4-a716-446655440003"
//         },
//         {
//             "ref_id": "15",
//             "bib": "15",
//             "group_name": "ПП2",
//             "name": "Ктото никто",
//             "organization": "Некоманда",
//             "card_number": 87654320,
//             "out_of_competition": false,
//             "start": 30720,
//             "result_ms" : 12000,
//             "result_status" : "OK"
//         }
//     ]
// }

const QJsonObject race::getResultToOnline(const int number)
{
    QJsonObject body;
    for(person* x: persons_){
        if (x->getBib() == number){
            body["ref_id"] = QString::number(number);
            body["bib"] =  QString::number(number);
            body["group_name"] = this->getNameGroup(x->getGroupId());
            body["name"] = x->getFullName();
            body["organization"] = this->getNameOrganization(x->getOrganizationId());
            body["card_number"] = x->getCardNumber();
            //body["country_code"] = "RUS";
            //body["national_code"] = null;
            //body["world_code"] = null;
            body["out_of_competition"] = false;
            //body["start"] = 30720;
            body["result_ms"] = (this->getResultMsec(number) / 10); // Orgeo [ms]=sec*100
            body["result_status"] = "OK";
            return body;
        }
    }
    return body;
}

int race::getResultMsec(const int number)
{
    int res = 0;
    for(result* x: results_){
        if (x->getBibResult() == number)
            return x->getResultMsec() ;
    }
    return res;
}


void race::updateData(QJsonObject objJson)
{
    data.chief_referee = objJson.value("chief_referee").toString();
    data.description = objJson.value("description").toString();
    data.end_datetime = objJson.value("end_datetime").toString();
    data.location = objJson.value("location").toString();
    data.race_type = objJson.value("race_type").toInt();
    data.relay_leg_count = objJson.value("relay_leg_count").toInt();
    data.secretary = objJson.value("secretary").toString();
    data.start_datetime = objJson.value("start_datetime").toString();
    data.title = objJson.value("title").toString();
    data.url = objJson.value("url").toString();
}

QJsonObject race::toJson() const
{
    QJsonObject json;
    json["chief_referee"] = data.chief_referee;
    json["description"] = data.description;
    json["end_datetime"] = data.end_datetime;
    json["location"] = data.location;
    json["race_type"] = data.race_type;
    json["relay_leg_count"] = data.relay_leg_count;
    json["secretary"] = data.secretary;
    json["start_datetime"] = data.start_datetime;
    json["title"] = data.title;
    json["url"] = data.url;

    // QVector<course*> courses_;
    QJsonArray courseArray;
    for (const course *v : courses_)
        courseArray.append(v->toJson());
    json["courses"] = courseArray;

    // QVector<group*> groups_;
    QJsonArray groupArray;
    for (const group *v : groups_)
        groupArray.append(v->toJson());
    json["groups"] = groupArray;

    // QVector<organization*> organizations_;
    QJsonArray organizationArray;
    for (const organization *v : organizations_)
        organizationArray.append(v->toJson());
    json["organizations"] = organizationArray;

    // QVector<person*> persons_;
    QJsonArray personArray;
    for (const person *v : persons_)
        personArray.append(v->toJson());
    json["persons"] = personArray;

    // QVector<result*> results_;
    QJsonArray resultArray;
    for (const result *v : results_)
        resultArray.append(v->toJson());
    json["results"] = resultArray;

    return json;
}

int race::getIndexPerson(int number)
{
    for(person* x: persons_){
        if (x->getBib() == number)
            return x->getBib();
    }
    return -1;
}

QString race::getNameFromBib(int number)
{
    for(person* x: persons_){
        if (x->getBib() == number)
            return x->getName();
    }
    return "";
}

QString race::getSurnameFromBib(int number)
{
    for(person* x: persons_){
        if (x->getBib() == number)
            return x->getSurname();
    }
    return "";
}

QString race::getFullNameFromBib(int number)
{
    for(person* x: persons_){
        if (x->getBib() == number)
            return x->getFullName();
    }
    return "";
}

QString race::getNameOrganizationFromBib(int number)
{
    for(person* x: persons_){
        if (x->getBib() == number)
            return this->getNameOrganization(x->getOrganizationId());
    }
    return "";
}

QString race::getNameOrganization(QString id)
{
    QString str_name = "";
    for(organization* x: organizations_){
        if (x->getId() == id)
            return x->getName();
    }
    return str_name;
}

QString race::getNameGroupFromBib(int number)
{
    for(person* x: persons_){
        if (x->getBib() == number)
            return this->getNameGroup(x->getGroupId());
    }
    return "";
}

QString race::getNameGroup(QString id)
{
    QString str_name = "";
    for(group* x: groups_){
        if (x->getId() == id)
            return x->getName();
    }
    return str_name;
}

QString race::getNameCourse(QString id)
{
    QString str_name = "";
    for(course* x: courses_){
        if (x->getId() == id)
            return x->getName();
    }
    return str_name;
}

int race::getLengthCourse(QString id)
{
    int len = 0;
    for(course* x: courses_){
        if (x->getId() == id)
            return x->getLength();
    }
    return len;
}

int race::getCntKPCourse(QString id)
{
    int len = 0;
    for(course* x: courses_){
        if (x->getId() == id)
            return x->getCntControll();
    }
    return len;
}

int race::getClimb(QString id)
{
    int len = 0;
    for(course* x: courses_){
        if (x->getId() == id)
            return x->getClimb();
    }
    return len;
}
