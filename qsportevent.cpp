#include "qsportevent.h"

#include <QDebug>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QFile>


#include <QMessageBox>

QSportEvent::QSportEvent(QObject *parent)
    : QObject{parent}
{}

QSportEvent::~QSportEvent() {}


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

const bool QSportEvent::checkingCardNumInPerson(int bib, int cardNum)
{
    return races_.at(0)->checkingCardNumInPerson(bib, cardNum);
}

const bool QSportEvent::checkingCardNumInResult(int cardNum)
{
    return races_.at(0)->checkingCardNumInResult(cardNum);
}

const int QSportEvent::getBibFromCardNum(int cardNum)
{
    return races_.at(0)->getBibFromCardNum(cardNum);
}

int QSportEvent::getCardNumFromBib(int bib)
{
    return races_.at(0)->getCardNumFromBib(bib);
}

int QSportEvent::setCardNumFromBib(int bib, int cardNum)
{
    return races_.at(0)->setCardNumFromBib(bib, cardNum);
}

int QSportEvent::addResult(int bib, QByteArray ba)
{
    return races_.at(0)->addResult(bib, ba);
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
                race* race_ = new race(jsonArr.at(0).toObject());
                races_.push_back(race_);
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
