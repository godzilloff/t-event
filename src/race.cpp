#include "race.h"
#include "functions.h"

race::race(){}

race::~race()
{
    for (int ii=0; ii < groups_.size(); ii++)
        delete groups_[ii];
}


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

int race::addResult(int bib, QByteArray ba)
{
    st_comport_result* const pstres = reinterpret_cast<st_comport_result *>( ba.data() );
    qDebug() << pstres->cardNum;
    qDebug() << pstres->startMs;
    qDebug() << pstres->finishMs;

    int res_ms = pstres->finishMs - pstres->startMs;

    QTime time =QTime::currentTime();

    QJsonObject json;
    //json["assigned_rank"] = data.assigned_rank;
    json["bib"] = bib;
    //json["can_win_count"] = data.can_win_count;
    json["card_number"] = pstres->cardNum;
    json["created_at"] = time.msecsSinceStartOfDay();
    json["credit_time"] = 0;
    //json["days"] = data.days;
    //json["diff"] = data.diff;
    //json["diff_scores"] = data.diff_scores;
    json["finish_msec"] = pstres->finishMs;
    json["finish_time"] = pstres->finishMs;
    json["id"] = QString::number(bib);
    //json["order"] = data.order;
    //json["penalty_laps"] = data.penalty_laps;
    //json["penalty_time"] = data.penalty_time;
    json["person_id"] = this->getBibFromCardNum(pstres->cardNum);
    //json["place"] = data.place;
    json["result"] = QTime::fromMSecsSinceStartOfDay(res_ms).toString("hh:mm:ss.z");
    json["result_msec"] = res_ms;
    json["result_relay"] = QTime::fromMSecsSinceStartOfDay(res_ms).toString("hh:mm:ss.z");
    json["result_relay_msec"] = res_ms;
    //json["scores"] = data.scores;
    //json["scores_rogain"] = data.scores_rogain;
    //json["speed"] = data.speed;
    json["start_msec"] = pstres->startMs;
    json["start_time"] = pstres->startMs;
    json["status"] = 1;
    //json["status_comment"] = data.status_comment;
    //json["system_type"] = data.system_type;
    json["object"] = "ResultManual";

    // */
    result* result_ = new result(json);
    results_.push_back(result_);
    return 0;
}

QString race::addGroupByName(const QString &name)
{
    QString id = getIdGroupByName(name);
    if (id == ""){
        id = QString::number(groups_.size()+1);
        group* gr = new group(id,name);
        groups_.push_back(gr);
    }
    return id;
}

QString race::getIdGroupByName(const QString &name)
{
    for(group* x: groups_){
        if (x->getName() == name)
            return x->getId();
    }
    return QString{};
}

QString race::getIdOrganizationByName(const QString &name)
{
    for(organization* x: organizations_){
        if (x->getName() == name)
            return x->getId();
    }
    return QString{};
}

QString race::getIdOrganizationByNameAndContact(const QString &name,const QString &contact)
{
    for(organization* x: organizations_){
        if ((x->getName() == name)&&(x->getContact() == contact))
            return x->getId();
    }
    return QString{};
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

const st_person* race::getDataPersonId(QString id)
{
    for(person* x: persons_){
        if (x->getId() == id)
            return x->getDataPersonBib();
    }
    return nullptr;
}

const st_person* race::getDataPersonBib(int number)
{
    for(person* x: persons_){
        if (x->getBib() == number)
            return x->getDataPersonBib();
    }
    return nullptr;
}

const st_result* race::getDataResultBib(int number)
{
    for(result* x: results_){
        if (x->getBibResult() == number)
            return x->getDataResult();
    }
    return nullptr;
}

int race::getIndexPerson(int number)
{
    for(person* x: persons_){
        if (x->getBib() == number)
            return x->getBib();
    }
    return -1;
}

void race::addPerson(QString name, QString id_org, QString id_gr, QString qual, QString year, QString comment)
{
    st_person dt;
    splitString(name, dt.surname, dt.name);
    dt.id = QString::number(1000 + persons_.size());
    dt.organization_id = id_org;
    dt.group_id = id_gr;
    dt.year = year.toInt();
    dt.qual = qual.toInt();
    dt.comment = comment;

    person* per = new person(dt);
    persons_.push_back(per);
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

const bool race::isBibFree(int bib)
{
    for(person* x: persons_){
        if (x->getBib() == bib)
            return false;
    }
    return true;
}

const bool race::isCardNumFree(int cardNum)
{
    for(person* x: persons_){
        if (x->getCardNumber() == cardNum)
            return false;
    }
    return true;
}

const bool race::checkingCardNumInPerson(int bib, int cardNum)
{
    for(person* x: persons_){
        if (x->getBib() == bib)
            return (x->getCardNumber() == cardNum);
    }
    return false;
}

const bool race::isResultBibFree(int bib)
{
    for(result* x: results_){
        if (x->getBibResult() == bib)
            return false;
    }
    return true;
}

const bool race::checkingCardNumInResult(int cardNum)
{
    for(result* x: results_){
        if (x->getCardNumber() == cardNum)
            return true;
    }
    return false;
}

const QString race::getPersonInfoFromBib(int bib)
{
    for(person* x: persons_){
        if (x->getBib() == bib){
            return x->getFullName() + " (" + QString::number(bib) + ", "
                   + getNameOrganizationFromBib(bib) + ")";
        }
    }
    return QString{};
}

const QString race::getPersonInfoFromCardNum(int cardNum)
{
    for(person* x: persons_){
        if (x->getCardNumber() == cardNum){
            int bib = x->getBib();
                return x->getFullName() + " (" + QString::number(bib) + ", "
                   + getNameOrganizationFromBib(bib) + ")";
        }
    }
    return QString{};
}

const QString race::getResultFromBib(int bib)
{
    for(result* x: results_){
        if (x->getBibResult() == bib)
            return x->getResultStr();
    }
    return QString{};
}

int race::getBibFromCardNum(int cardNum)
{
    for(person* x: persons_){
        if (x->getCardNumber() == cardNum)
            return x->getBib();
    }
    return -1;
}

int race::getCardNumFromBib(int bib)
{
    for(person* x: persons_){
        if (x->getBib() == bib)
            return x->getCardNumber();
    }
    return -1;
}

int race::setCardNumFromBib(int bib, int cardNum)
{
    for(person* x: persons_){
        if (x->getBib() == bib)
            return x->setCardNum(cardNum);
    }
    return -1;
}

int race::setBibFromCardNum(int cardNum, int bib)
{
    for(result* x: results_){
        if (x->getCardNumber() == cardNum){
            x->setBibResult(bib);
            return 0;
        }
    }
    return -1;
}

int race::clearBibInResult(int cardNum)
{
    for(result* x: results_){
        if (x->getCardNumber() == cardNum){
            x->clearBibResult();
            return 0;
        }
    }
    return -1;
}

QString race::getNameOrganizationFromBib(int number)
{
    for(person* x: persons_){
        if (x->getBib() == number)
            return this->getNameOrganization(x->getOrganizationId());
    }
    return "";
}


QString race::addOrgByName(const QString &name)
{
    QString id = getIdGroupByName(name);
    if (id == ""){
        id = QString::number(organizations_.size()+1);
        organization* org = new organization(id, name);
        organizations_.push_back(org);
    }
    return id;
}

QString race::addOrgByNameAndContact(const QString &name, const QString &contact)
{
    QString id = getIdOrganizationByNameAndContact(name, contact);
    if (id == ""){
        id = QString::number(organizations_.size()+1);
        organization* org = new organization(id,name,contact);
        organizations_.push_back(org);
    }
    return id;
}


// QString race::getNameOrganization(QString id)
// {
//     QString str_name{};
//     auto it = std::find_if(
//         organizations_.begin(),organizations_.end(),
//         [id](organization* x){return x->getId() == id;});
//
//     return it == organizations_.end() ?
//                QString{} : (*it)->getName();
// }

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
    for(group* x: groups_){
        if (x->getId() == id)
            return x->getName();
    }
    return QString{};
}

QString race::getNameCourse(QString id)
{
    for(course* x: courses_){
        if (x->getId() == id)
            return x->getName();
    }
    return QString{};
}

int race::getLengthCourse(QString id)
{
    for(course* x: courses_){
        if (x->getId() == id)
            return x->getLength();
    }
    return 0;
}

int race::getCntKPCourse(QString id)
{
    for(course* x: courses_){
        if (x->getId() == id)
            return x->getCntControll();
    }
    return 0;
}

int race::getClimb(QString id)
{
    for(course* x: courses_){
        if (x->getId() == id)
            return x->getClimb();
    }
    return 0;
}
