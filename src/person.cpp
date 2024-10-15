#include "person.h"

person::person(int bib){
    data.bib = bib;
    //qDebug() << "person::person()\n";
}

person::person(QJsonObject objJson)
{
    data.bib = objJson.value("bib").toInt();
    data.name = objJson.value("name").toString();
    data.surname = objJson.value("surname").toString();
    data.sex = objJson.value("sex").toInt();
    data.card_number = objJson.value("card_number").toInt();
    data.year = objJson.value("year").toInt();
    data.qual = objJson.value("qual").toInt();
    data.comment = objJson.value("comment").toString();

    data.id = objJson.value("id").toString();
    data.group_id = objJson.value("group_id").toString();
    data.organization_id = objJson.value("organization_id").toString();
    data.birth_date = objJson.value("birth_date").toString();
    data.start_time = objJson.value("start_time").toInt();
    data.start_group = objJson.value("start_group").toInt();

    data.national_code = objJson.value("national_code").toString(); // region russia, 77, 76 ...
    data.world_code = objJson.value("world_code").toString();
    //data.object = "Person";

    data.is_out_of_competition = true;
    data.is_paid = false;
    data.is_personal = false;
    data.is_rented_card = false;

    //qDebug() << data.name << " " << data.surname;
    //qDebug() << objJson.value("name").toString() << " " <<
    //    objJson.value("surname").toString();
}

QJsonObject person::toJson() const
{
    QJsonObject json;
    json["bib"] = data.bib;
    json["id"] = data.id;
    json["group_id"] = data.group_id;
    json["organization_id"] = data.organization_id;
    json["card_number"] = data.card_number;
    json["name"] = data.name;
    json["surname"] = data.surname;
    json["sex"] = data.sex;
    json["year"] = data.year;
    json["birth_date"] = data.birth_date;
    json["qual"] = data.qual;
    json["start_time"] = data.start_time;
    json["start_group"] = data.start_group;
    json["national_code"] = data.national_code;
    json["world_code"] = data.world_code;
    json["comment"] = data.comment;
    json["is_out_of_competition"] = data.is_out_of_competition;
    json["is_paid"] = data.is_paid;
    json["is_personal"] = data.is_personal;
    json["is_rented_card"] = data.is_rented_card;
    json["object"] = data.object;

    //json[""] = data.;
    //json[""] = ;

    return json;
}

int person::setCardNum(int cardNum)
{
    data.card_number = cardNum;
    return cardNum;
}


QString getQ(int q){
    QString res = "";
    switch (q) {
    case 0: res = "б/р"; break;
    case 1: res = "Iю"; break;
    case 2: res = "IIю"; break;
    case 3: res = "IIIю"; break;
    case 4: res = "I"; break;
    case 5: res = "II"; break;
    case 6: res = "III"; break;
    case 7: res = "КМС"; break;
    case 8: res = "МС"; break;
    case 9: res = "МСМК"; break;
    case 10: res = "ЗМС"; break;
    default: break;
    }
    return res;
}
