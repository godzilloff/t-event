#include "group.h"

group::group() {}

group::group(QString id, QString name) {
    data.long_name = data.name = name;
    data.id = id;
}

group::group(QJsonObject objJson)
{
    data.count_finished = objJson.value("count_finished").toInt();
    data.count_person = objJson.value("count_person").toInt();
    data.course_id = objJson.value("course_id").toString();
    data.first_number = objJson.value("first_number").toInt();
    data.id = objJson.value("id").toString();
    data.is_any_course =  objJson.value("is_any_course").toBool();
    data.long_name = objJson.value("long_name").toString();
    data.name = objJson.value("name").toString();
    data.max_age = objJson.value("max_age").toInt();
    data.max_time = objJson.value("max_time").toInt();
    data.max_year = objJson.value("max_year").toInt();
    data.min_age = objJson.value("min_age").toInt();
    data.min_year = objJson.value("min_year").toInt();
    data.order_in_corridor = objJson.value("order_in_corridor").toInt();
    data.price = objJson.value("price").toInt();
    data.relay_legs = objJson.value("relay_legs").toInt();
    data.sex = objJson.value("sex").toInt();
    data.start_corridor = objJson.value("start_corridor").toInt();
    data.start_interval = objJson.value("start_interval").toInt();

    //qDebug() << objJson.value("name").toString();
}

QJsonObject group::toJson() const
{
    QJsonObject json;
    json["count_finished"] = data.count_finished;
    json["count_person"] = data.count_person;
    json["course_id"] = data.course_id;
    json["first_number"] = data.first_number;
    json["id"] = data.id;
    json["is_any_course"] = data.is_any_course;
    json["long_name"] = data.long_name;
    json["name"] = data.name;
    json["max_age"] = data.max_age;
    json["max_time"] = data.max_time;
    json["max_year"] = data.max_year;
    json["min_age"] = data.min_age;
    json["min_year"] = data.min_year;
    json["order_in_corridor"] = data.order_in_corridor;
    json["price"] = data.price;
    json["relay_legs"] = data.relay_legs;
    json["sex"] = data.sex;
    json["start_corridor"] = data.start_corridor;
    json["start_interval"] = data.start_interval;
    json["object"] = data.object;
    //json[""] = data.;

    return json;
}
