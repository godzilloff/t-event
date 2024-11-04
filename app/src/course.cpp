#include "course.h"


TcourseControl::TcourseControl(QJsonObject objJson)
{
    data.code = objJson.value("code").toString();
    data.length = objJson.value("length").toInt();
}

QJsonObject TcourseControl::toJson() const
{
    QJsonObject json;
    json["code"] = data.code;
    json["length"] = data.length;
    return json;
}


course::course() {}

course::course(QJsonObject objJson){
    // --- дистанции / courses []:
    //   ---- bib
    //   ---- набор высоты / climb
    //   ---- КП []:
    //     ----- код на карте
    //     ----- код станции
    //     ----- тип КП
    //   ---- стартовый коридор
    //   ---- id
    //   ---- длина дистанции
    //   ---- название
    //   ---- тип бъекта (дистация) / Course

    //qDebug() << objJson.value("name").toString();

    data.name = objJson.value("name").toString();
    data.id = objJson.value("id").toString();

    data.bib = objJson.value("bib").toInt();
    data.climb = objJson.value("climb").toInt();
    data.length = objJson.value("length").toInt();
    data.corridor = objJson.value("corridor").toInt();

    jsonArrCControl = objJson.value("controls").toArray();
    for(int ii = 0; ii < jsonArrCControl.count(); ii++){
        TcourseControl* const ccontrol_ = new TcourseControl(jsonArrCControl.at(ii).toObject());
        ccontrols.push_back(ccontrol_);
    }
}

QJsonObject course::toJson() const
{
    QJsonObject json;
    json["name"] = data.name;
    json["id"] = data.id;
    json["bib"] = data.bib;
    json["climb"] = data.climb;
    json["length"] = data.length;
    json["data"] = data.corridor;
    //json[""] = ;


    //QVector<TcourseControl*> ccontrols;
    QJsonArray ccontrolArray;
    for (const TcourseControl *v : ccontrols)
        ccontrolArray.append(v->toJson());
    json["controls"] = ccontrolArray;

    return json;
}

QString course::getPathCC() const
{
    QString res = "";
    for (const auto& item: ccontrols){
        res += item->getCode() + " ";
    }
    return res;
}
