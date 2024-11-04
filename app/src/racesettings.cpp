#include "racesettings.h"

#include <QJsonArray>

RaceSettings::RaceSettings() {}

//RaceSettings::RaceSettings(QJsonObject objJson)
void RaceSettings::importFromJson(QJsonObject objJson)
{
    if (const QJsonValue v = objJson["is_corridor_minute_number"]; v.isBool() )
        is_corridor_minute_number = v.toBool();

    if (const QJsonValue v = objJson["is_corridor_order_number"]; v.isBool() )
        is_corridor_order_number = v.toBool();

    if (const QJsonValue v = objJson["is_fixed_number_interval"]; v.isBool() )
        is_fixed_number_interval = v.toBool();

    if (const QJsonValue v = objJson["is_fixed_start_interval"]; v.isBool() )
        is_fixed_start_interval = v.toBool();

    if (const QJsonValue v = objJson["is_mix_groups"]; v.isBool() )
        is_mix_groups = v.toBool();

    if (const QJsonValue v = objJson["is_split_regions"]; v.isBool() )
        is_split_regions = v.toBool();

    if (const QJsonValue v = objJson["is_split_start_groups"]; v.isBool() )
        is_split_start_groups = v.toBool();

    if (const QJsonValue v = objJson["is_split_teams"]; v.isBool() )
        is_split_teams = v.toBool();

    if (const QJsonValue v = objJson["is_start_preparation_draw"]; v.isBool() )
        is_start_preparation_draw = v.toBool();

    if (const QJsonValue v = objJson["is_start_preparation_numbers"]; v.isBool() )
        is_start_preparation_numbers = v.toBool();

    if (const QJsonValue v = objJson["is_start_preparation_reserve"]; v.isBool() )
        is_start_preparation_reserve = v.toBool();

    if (const QJsonValue v = objJson["is_start_preparation_time"]; v.isBool() )
        is_start_preparation_time = v.toBool();

    if (const QJsonValue v = objJson["live_cp_code"]; v.isString() )
        live_cp_code = v.toString(); // QString

    if (const QJsonValue v = objJson["live_cp_enabled"]; v.isBool() )
        live_cp_enabled = v.toBool();

    if (const QJsonValue v = objJson["live_cp_finish_enabled"]; v.isBool() )
        live_cp_finish_enabled = v.toBool();

    if (const QJsonValue v = objJson["live_cp_split_codes"]; v.isString() )
        live_cp_split_codes =  v.toString();//"91,91,92"; // QString

    if (const QJsonValue v = objJson["live_cp_splits_enabled"]; v.isBool() )
        live_cp_splits_enabled = v.toBool();

    if (const QJsonValue v = objJson["live_enabled"]; v.isBool() )
        live_enabled = v.toBool();

    if (const QJsonValue v = objJson["live_results_enabled"]; v.isBool() )
        live_results_enabled = v.toBool();

    if (const QJsonValue v = objJson["numbers_first"]; v.isBool() )
        numbers_first = v.toInt();

    if (const QJsonValue v = objJson["numbers_interval"]; v.isBool() )
        numbers_interval = v.toInt();

    if (const QJsonValue v = objJson["relay_next_number"]; v.isBool() )
        relay_next_number = v.toInt();

    if (const QJsonValue v = objJson["reserve_count"]; v.isBool() )
        reserve_count = v.toInt();

    if (const QJsonValue v = objJson["reserve_percent"]; v.isBool() )
        reserve_percent = v.toInt();

    if (const QJsonValue v = objJson["reserve_prefix"]; v.isString() )
        reserve_prefix =  v.toString(); // QString

    if (const QJsonValue v = objJson["start_first_time"]; v.isBool() )
        start_first_time = v.toInt();

    if (const QJsonValue v = objJson["start_interval"]; v.isBool() )
        start_interval = v.toInt();

    if (const QJsonValue v = objJson["start_one_minute_qty"]; v.isBool() )
        start_one_minute_qty = v.toInt();

    if (const QJsonValue v = objJson["teamwork_host"]; v.isString() )
        teamwork_host =  v.toString();//"192.168.1.2"; // QString

    if (const QJsonValue v = objJson["teamwork_port"]; v.isBool() )
        teamwork_port = v.toInt();

    if (const QJsonValue v = objJson["teamwork_token"]; v.isString() )
        teamwork_token =  v.toString();//"4d647db7"; // QString

    if (const QJsonValue v = objJson["teamwork_type_connection"]; v.isString() )
        teamwork_type_connection =  v.toString();//"client"; // QString

    if (const QJsonValue v = objJson["live_urls"]; v.isArray()) {
        const QJsonArray live_urls_ar = v.toArray();
        live_urls.reserve(live_urls_ar.size());
        for (const QJsonValue &url : live_urls_ar)
            live_urls.append(url.toString());
    }
}

QJsonObject RaceSettings::toJson() const
{
    QJsonObject json;

    json["is_corridor_minute_number"] = is_corridor_minute_number;
    json["is_corridor_order_number"] = is_corridor_order_number;
    json["is_fixed_number_interval"] = is_fixed_number_interval;
    json["is_fixed_start_interval"] = is_fixed_start_interval;
    json["is_mix_groups"] = is_mix_groups;
    json["is_split_regions"] = is_split_regions;
    json["is_split_start_groups"] = is_split_start_groups;
    json["is_split_teams"] = is_split_teams;
    json["is_start_preparation_draw"] = is_start_preparation_draw;
    json["is_start_preparation_numbers"] = is_start_preparation_numbers;
    json["is_start_preparation_reserve"] = is_start_preparation_reserve;
    json["is_start_preparation_time"] = is_start_preparation_time;
    json["live_cp_code"] = live_cp_code;
    json["live_cp_enabled"] = live_cp_enabled;
    json["live_cp_finish_enabled"] = live_cp_finish_enabled;
    json["live_cp_split_codes"] = live_cp_split_codes;// = "91,91,92";
    json["live_cp_splits_enabled"] = live_cp_splits_enabled;
    json["live_enabled"] = live_enabled;
    json["live_results_enabled"] = live_results_enabled;

    json["numbers_first"] = numbers_first;
    json["numbers_interval"] = numbers_interval;
    json["relay_next_number"] = relay_next_number;
    json["reserve_count"] = reserve_count;
    json["reserve_percent"] = reserve_percent;
    json["reserve_prefix"] = reserve_prefix;
    json["start_first_time"] = start_first_time;
    json["start_interval"] = start_interval;
    json["start_one_minute_qty"] = start_one_minute_qty;
    json["teamwork_host"] = teamwork_host;
    json["teamwork_port"] = teamwork_port;
    json["teamwork_token"] = teamwork_token;
    json["teamwork_type_connection"] = teamwork_type_connection;

    QJsonArray live_urls_ar;
    for (const QString &url : live_urls)
        live_urls_ar.append(url);
    json["live_urls"] = live_urls_ar;

    return json;
}

QString RaceSettings::getOnlineUrl()
{
    if (live_urls.size() > 0)
        return live_urls.at(0);
    else return QString{};
}

void RaceSettings::setOnlineUrl(const QString &url)
{
    if (live_urls.size() > 0) live_urls[0] = url;
    else live_urls.push_back(url);
}

void RaceSettings::setOnlineEnable(bool status)
{
    live_enabled = status;
}

bool RaceSettings::getOnlineEnable()
{
    return live_enabled;
}
