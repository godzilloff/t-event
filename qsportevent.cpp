#include "qsportevent.h"

#include <QDebug>

//#include <QJsonObject>
//#include <QJsonArray>
//#include <QJsonParseError>
//#include <QFile>
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
    file_input.setFileName(path);

    if ( file_input.open(QIODevice::ReadOnly | QFile::Text) )
    {
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
