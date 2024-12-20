#include "qsportevent.h"

#include <QDebug>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QFile>
#include <QMessageBox>

QSportEvent::QSportEvent(QObject *parent)
    : QObject{parent}
{
    qDebug() << "ctor QSportEvent";
    addRace();
}

QSportEvent::~QSportEvent() {
    qDebug() << "dtor QSportEvent";
    if (races_.size() > 0) delete races_.at(0);
}

void QSportEvent::addRace(){
    race* race_ = new race();
    races_.push_back(race_);
}

const QJsonObject QSportEvent::getResultToOnline(const QString& number){
    int bib = number.toInt();
    return races_.at(0)->getResultToOnline(bib);
}

const QString QSportEvent::getNameOrganization(const QString& id){
    return races_.at(0)->getNameOrganization(id);
}

const QString QSportEvent::getNameGroup(const QString& id){
    return races_.at(0)->getNameGroup(id);
}

const QStringList QSportEvent::getNamesOrganization()
{
    QVector<organization*> *prtOrg = races_.at(0)->getPtrOrganizations();
    QStringList names;

    for (auto *org : *prtOrg) {
        names << org->getName();
    }
    return names;
}

const QStringList QSportEvent::getNamesGroup()
{
    QVector<group*> *prtGroup = races_.at(0)->getPtrGroups();
    QStringList names;

    for (auto *group : *prtGroup) {
        names << group->getName();
    }
    return names;
}

const bool QSportEvent::isBibFree(int bib)
{
    return races_.at(0)->isBibFree(bib);
}

const bool QSportEvent::isCardNumFree(int cardNum)
{
    return races_.at(0)->isCardNumFree(cardNum);
}

const bool QSportEvent::isResultBibFree(int bib)
{
    return races_.at(0)->isResultBibFree(bib);
}

const bool QSportEvent::checkingCardNumInPerson(int bib, int cardNum)
{
    return races_.at(0)->checkingCardNumInPerson(bib, cardNum);
}

const bool QSportEvent::checkingCardNumInResult(int cardNum)
{
    return races_.at(0)->checkingCardNumInResult(cardNum);
}

const QString QSportEvent::getPersonInfoFromBib(int bib)
{
    return races_.at(0)->getPersonInfoFromBib(bib);
}

const QString QSportEvent::getPersonInfoFromCardNum(int cardNum)
{
    return races_.at(0)->getPersonInfoFromCardNum(cardNum);
}

const QString QSportEvent::getResultFromBib(int bib)
{
    return races_.at(0)->getResultFromBib(bib);
}

const int QSportEvent::getBibFromCardNum(int cardNum)
{
    return races_.at(0)->getBibFromCardNum(cardNum);
}


void QSportEvent::setPNameFromId(const QString &id, const QString &name)
{
    races_.at(0)->setPNameFromId(id, name);
}

void QSportEvent::setPSurnameFromId(const QString &id, const QString &surname)
{
    races_.at(0)->setPSurnameFromId(id, surname);
}

void QSportEvent::setPersonComment(const QString &id, const QString &comment)
{
    races_.at(0)->setPersonComment(id, comment);
}

void QSportEvent::setPersonBirthDate(const QString &id, const QString &birthDate)
{
    races_.at(0)->setPersonBirthDate(id, birthDate);
}

int QSportEvent::setPersonStartTime(const QString &id, const int &startTime)
{
    return races_.at(0)->setPersonStartTime(id, startTime);
}

int QSportEvent::getCardNumFromBib(int bib)
{
    return races_.at(0)->getCardNumFromBib(bib);
}

int QSportEvent::setCardNumFromBib(int bib, int cardNum)
{
    return races_.at(0)->setCardNumFromBib(bib, cardNum);
}

int QSportEvent::setCardNumFromId(const QString &id, int cardNum)
{
    return races_.at(0)->setCardNumFromId(id, cardNum);
}

int QSportEvent::setCardNumFromCsv(QString path)
{
    return races_.at(0)->setCardNumFromCsv(path);
}

void QSportEvent::clearAllCardNum()
{
    races_.at(0)->clearAllCardNum(); return;
}

int QSportEvent::setBibFromId(QString id, int bib)
{
    return races_.at(0)->setBibFromId(id, bib);
}

int QSportEvent::setBibInResult(double created, int bib)
{
    return races_.at(0)->setBibInResult(created, bib);
}

int QSportEvent::setPOrgFromNameOrg(const QString &id, const QString &nameOrg)
{
    return races_.at(0)->setPOrgFromNameOrg(id, nameOrg);
}

int QSportEvent::setPGroupFromNameGroup(const QString &id, const QString &nameGroup)
{
    return races_.at(0)->setPGroupFromNameGroup(id, nameGroup);
}

int QSportEvent::clearBibInResult(int cardNum)
{
    return races_.at(0)->clearBibInResult(cardNum);
}

int QSportEvent::setResultStatus(int cardNum, int status)
{
    return races_.at(0)->setResultStatus(cardNum, status);
}

int QSportEvent::setResultStatus(double created, int status)
{
    return races_.at(0)->setResultStatus(created, status);
}

int QSportEvent::addResult(int bib, QByteArray ba)
{
    return races_.at(0)->addResult(bib, ba);
}

QString QSportEvent::getOnlineUrl()
{
    return races_.at(0)->getOnlineUrl();
}

bool QSportEvent::getOnlineEnable()
{
    return races_.at(0)->getOnlineEnable();
}

void QSportEvent::setOnlineUrl(const QString &url)
{
    races_.at(0)->setOnlineUrl(url);
}

void QSportEvent::setOnlineEnable(bool status)
{
    races_.at(0)->setOnlineEnable(status);
}

QJsonObject QSportEvent::toJson() const{
    QJsonObject json;
    json["version"] = "0.5.0.0";
    json["current_race"] = 0;

    // QVector<course*> races_;
    QJsonArray raceArray;
    for (const race *v : races_)
        raceArray.append(v->toJson());
    json["races"] = raceArray;
    return json;
}

st_person QSportEvent::lineToPerson(const QStringList &data, Type_csv tcsv)
{
    switch (tcsv) {
    case QSportEvent::Type_csv::SECRETAR_ST_ONE:
        return lineToPersonFromSecretarStOne(data);
        break;
    case QSportEvent::Type_csv::SECRETAR_ST_TWO:
        return lineToPersonFromSecretarStTwo(data);
        break;
    case QSportEvent::Type_csv::SECRETAR_ST_FOUR:
        return lineToPersonFromSecretarStFour(data);
        break;
    case QSportEvent::Type_csv::ORGEO:
    default:
        if (data.size() == 10)
            return lineToPersonFromOrgeoWithPre(data);
        else
            return lineToPersonFromOrgeoNotPre(data);
        break;
    }
}

st_person QSportEvent::lineToPersonFromSecretarStOne(const QStringList &data)
{
    QString contact = QString{};
    st_person person;
    person.group_id = races_.at(0)->addGroupByName(data[0]);
    person.name = data[1];
    person.organization_id = races_.at(0)->addOrgByNameAndContact(data[2], contact);
    person.qual = data[3].toInt();
    person.bib = data[4].toInt();
    person.year = data[5].toInt();
    person.card_number = data[6].toInt();
    person.comment = data[7]+ " " + data[8];
    return person;
}

st_person QSportEvent::lineToPersonFromSecretarStTwo(const QStringList &data)
{
    QString contact = QString{};
    st_person person;
    person.group_id = races_.at(0)->addGroupByName(data[0]);
    person.name = data[1];
    person.organization_id = races_.at(0)->addOrgByNameAndContact(data[2],contact);
    person.qual = data[3].toInt();
    person.bib = data[4].toInt();
    person.year = data[5].toInt();
    person.card_number = data[6].toInt();
    person.comment = data[7];
    return person;
}

st_person QSportEvent::lineToPersonFromSecretarStFour(const QStringList &data)
{
    QString contact = QString{};
    st_person person;
    person.group_id = races_.at(0)->addGroupByName(data[0]);
    person.name = data[1];
    person.organization_id = races_.at(0)->addOrgByNameAndContact(data[1],contact);
    person.qual = data[3].toInt();
    person.bib = data[4].toInt();
    person.year = data[5].toInt();
    person.card_number = data[6].toInt();
    return person;
}

st_person QSportEvent::lineToPersonFromOrgeoNotPre(const QStringList &data)
{
    QString contact = QString{};
    st_person person;
    person.group_id = races_.at(0)->addGroupByName(data[0]);
    person.name = data[1];
    person.organization_id = races_.at(0)->addOrgByNameAndContact(data[2],contact);
    person.qual = data[3].toInt();
    person.bib = data[4].toInt();
    person.year = data[5].toInt();
    person.card_number = data[6].toInt();
    person.comment = data[7];

    return person;
}

st_person QSportEvent::lineToPersonFromOrgeoWithPre(const QStringList &data)
{
    st_person person;
    person.group_id = races_.at(0)->addGroupByName(data[0]);
    person.name = data[1];
    person.organization_id = races_.at(0)->addOrgByNameAndContact(data[2],data[3]);
    person.qual = data[4].toInt();
    person.bib = data[5].toInt();
    person.year = data[6].toInt();
    person.card_number = data[7].toInt();
    person.comment = data[8];

    return person;
}

void QSportEvent::importSportorgJSON(const QString& path){
    qDebug() << "importSportorgJSON ===="<< path<< Qt::endl ;
    QFile file_input;
    file_input.setFileName(path);

    if ( file_input.open(QIODevice::ReadOnly | QFile::Text) )
    {
        QJsonDocument doc;
        QJsonArray jsonArr;
        QJsonParseError docError;

        doc = QJsonDocument::fromJson(QByteArray(file_input.readAll()), &docError);
        file_input.close();

        if (docError.errorString().toInt() == QJsonParseError::NoError )
        {
            QString str = QJsonValue(doc.object().value("version")).toString();
            qDebug() << "version" << str;

            jsonArr = QJsonValue(doc.object().value("races")).toArray();
            qDebug() << "ready import races: " << jsonArr.count();

            for (qsizetype ii=0; ii < jsonArr.count(); ii++){
                if (ii == 0){
                    races_.at(0)->importFromJson(jsonArr.at(0).toObject());
                } else{
                    race* race_ = new race(jsonArr.at(0).toObject());
                    races_.push_back(race_);
                }
            }
        }
    }
    else
    {
        QMessageBox::information(nullptr, "Ошибка", "Файл не открыт");
    }
}

void QSportEvent::exportSportorgJSON(const QString &path)
{
    QFile saveFile(path);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return;
    }

    QJsonObject gameObject = toJson();
    saveFile.write(QJsonDocument(gameObject).toJson());
    saveFile.close();
}

// Формат csv с сайта orgeo.ru:
//Группа;ФИО;Коллектив;Представитель;Разряд;Номер;Год рождения;Номер чипа;Комментарий;
void QSportEvent::importCSV(const QString& path, Type_csv tcsv){

    QFile file(path);
    if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
        qDebug() << "File not exists";
    } else {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::LastEncoding);

        while (!in.atEnd())
        {
            QString line = in.readLine();
            line = line.trimmed(); // delete space
            QStringList data = line.split(";"); // split string

            if (data.size() > 4){
                for (int i = 0; i < data.size(); i++) {
                    data[i] = data[i].trimmed();   // Обработка данных
                }
                races_.at(0)->addPerson(lineToPerson(data, tcsv));
                //QString id_group = races_.at(0)->addGroupByName(data[0]);
                //QString id_org = races_.at(0)->addOrgByNameAndContact(data[2],data[3]);
                //st_person unit = lineToPerson(data);
                //races_.at(0)->addPerson(unit);
                //races_.at(0)->addPerson(unit.name, id_org,id_group, unit.qual , unit.year, unit.comment);
            }
        }

        file.close();
    }
}
