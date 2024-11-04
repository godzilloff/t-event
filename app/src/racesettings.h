#ifndef RACESETTINGS_H
#define RACESETTINGS_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class RaceSettings
{
public:
    RaceSettings();
    //RaceSettings(QJsonObject objJson);
    void importFromJson(QJsonObject objJson);
    QJsonObject toJson() const;

    QString getOnlineUrl();
    bool getOnlineEnable();
    void setOnlineUrl(const QString &url);
    void setOnlineEnable(bool status);

private:
    bool is_corridor_minute_number = false;
    bool is_corridor_order_number = false;
    bool is_fixed_number_interval = true;
    bool is_fixed_start_interval = true;
    bool is_mix_groups = false;
    bool is_split_regions = false;
    bool is_split_start_groups = false;
    bool is_split_teams = false;
    bool is_start_preparation_draw = false;
    bool is_start_preparation_numbers = true;
    bool is_start_preparation_reserve = true;
    bool is_start_preparation_time = false;
    QString live_cp_code = "10";
    bool live_cp_enabled = false;
    bool live_cp_finish_enabled = true;
    QString live_cp_split_codes = "91,91,92";
    bool live_cp_splits_enabled = true;
    bool live_enabled = false;
    bool live_results_enabled = true;
    int numbers_first = 100;
    int numbers_interval = 2;
    int relay_next_number = 1002;
    int reserve_count = 3;
    int reserve_percent = 15;
    QString reserve_prefix = "_\u0420\u0435\u0437\u0435\u0440\u0432";
    int start_first_time = 60000;
    int start_interval = 60000;
    int start_one_minute_qty = 1;
    QString teamwork_host = "192.168.1.2";
    int teamwork_port = 50010;
    QString teamwork_token = "4d647db7";
    QString teamwork_type_connection = "client";

    QVector<QString> live_urls;
};

#endif // RACESETTINGS_H
