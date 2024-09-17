#include "result.h"

result::result() {}

result::result(QJsonObject objJson)
{
    //data.result = objJson.value("result").toString();

    data.assigned_rank = objJson.value("assigned_rank").toInt();
    data.bib = objJson.value("bib").toInt();
    data.can_win_count = objJson.value("can_win_count").toInt();
    data.card_number = objJson.value("card_number").toInt();
    data.created_at = -1.0;//1722189634.093181;         // <------------
    data.credit_time = objJson.value("credit_time").toInt();
    data.days = objJson.value("days").toInt();
    data.diff = objJson.value("diff").toInt();
    data.diff_scores = objJson.value("diff_scores").toInt();
    data.finish_msec = objJson.value("finish_msec").toInt();
    data.finish_time = objJson.value("finish_time").toInt();
    data.id = objJson.value("id").toString();
    data.order = objJson.value("order").toInt();
    data.penalty_laps = objJson.value("penalty_laps").toInt();
    data.penalty_time = objJson.value("penalty_time").toInt();
    data.person_id = objJson.value("person_id").toString();
    data.place = objJson.value("place").toInt();
    data.result = objJson.value("result").toString();
    data.result_msec = objJson.value("result_msec").toInt();
    data.result_relay = objJson.value("result_relay").toString();
    data.result_relay_msec = objJson.value("result_relay_msec").toInt();
    data.scores = objJson.value("scores").toInt();
    data.scores_rogain = objJson.value("scores_rogain").toInt();
    data.speed = objJson.value("speed").toString();
    data.start_msec = objJson.value("start_msec").toInt();
    data.start_time = objJson.value("start_time").toInt();
    data.status = objJson.value("status").toInt();
    data.status_comment = objJson.value("status_comment").toString();
    data.system_type = objJson.value("system_type").toInt();

    data.object = "ResultManual";

    //qDebug() << objJson.value("result").toString();
}

QJsonObject result::toJson() const
{
    QJsonObject json;
    json["assigned_rank"] = data.assigned_rank;
    json["bib"] = data.bib;
    json["can_win_count"] = data.can_win_count;
    json["card_number"] = data.card_number;
    json["created_at"] = data.created_at;
    json["credit_time"] = data.credit_time;
    json["days"] = data.days;
    json["diff"] = data.diff;
    json["diff_scores"] = data.diff_scores;
    json["finish_msec"] = data.finish_msec;
    json["finish_time"] = data.finish_time;
    json["id"] = data.id;
    json["order"] = data.order;
    json["penalty_laps"] = data.penalty_laps;
    json["penalty_time"] = data.penalty_time;
    json["person_id"] = data.person_id;
    json["place"] = data.place;
    json["result"] = data.result;
    json["result_msec"] = data.result_msec;
    json["result_relay"] = data.result_relay;
    json["result_relay_msec"] = data.result_relay_msec;
    json["scores"] = data.scores;
    json["scores_rogain"] = data.scores_rogain;
    json["speed"] = data.speed;
    json["start_msec"] = data.start_msec;
    json["start_time"] = data.start_time;
    json["status"] = data.status;
    json["status_comment"] = data.status_comment;
    json["system_type"] = data.system_type;
    json["object"] = data.object;
    //json[""] = data.;
    //json[""] = ;

    return json;
}
