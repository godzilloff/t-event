#ifndef RESULT_H
#define RESULT_H


#include <QJsonObject>
#include <QJsonArray>

//#include "person.h"

struct st_result{
    int assigned_rank = 0;
    int bib = 0;
    int can_win_count = 0;
    int card_number = 0;
    double created_at = -1;//1722189634.093181;
    int credit_time = 0;
    int days = 0;
    int diff = 0;
    int diff_scores = 0;
    //final_result_time": null,
    int finish_msec = -1;
    int finish_time = -1;
    QString id = "";
    int order = 0;
    int penalty_laps = 0;
    int penalty_time = 0;
    QString person_id = "";
    int place = 0;
    QString result = "23:59:59";
    int result_msec = -1;
    QString result_relay = "23:59:59";
    int result_relay_msec = -1;
    int scores = 0;
    int scores_rogain = 0;
    QString speed = "";
    int start_msec = -1;
    int start_time = -1;
    int status = 1;
    QString status_comment = "";
    int system_type = 1;

    QString object = "ResultManual";
};

// "results": [
// {
//   "assigned_rank": 0,
//   "bib": 41,
//   "can_win_count": 0,
//   "card_number": 0,
//   "created_at": 1722189634.093181,
//   "credit_time": 0,
//   "days": 0,
//   "diff": 0,
//   "diff_scores": 0,
//   "final_result_time": null,
//   "finish_msec": 75634093,
//   "finish_time": 75634093,
//   "id": "2e64d1fa-1a54-4e16-b303-a99da9c09981",
//   "object": "ResultManual",
//   "order": 0,
//   "penalty_laps": 0,
//   "penalty_time": 0,
//   "person_id": "a846f2e6-52f9-4fc2-9f67-aa1be089fa0c",
//   "place": 1,
//   "result": "20:44:34",
//   "result_msec": 74674000,
//   "result_relay": "20:44:34",
//   "result_relay_msec": 74674000,
//   "scores": 0,
//   "scores_rogain": 0,
//   "speed": "",
//   "start_msec": 960000,
//   "start_time": 0,
//   "status": 1,
//   "status_comment": "",
//   "system_type": 1,
//   "splits": []
// },

class result
{
public:
    result();
    result(QJsonObject objJson);
    QJsonObject toJson() const;

    int getCardNumber(){ return data.card_number;};
    int getStartMsec(){ return data.start_msec;};
    int getFinishMsec(){ return data.finish_msec;};
    QString getResultStr(){ return data.result;};
    int getResultMsec(){ return data.result_msec;};
    int getBibResult(){return data.bib;};

    //void setItPerson(QVector<person*>::Iterator it_){it = it_;};

private:
    st_result data;

    //QVector<person*>::Iterator it;
};

#endif // RESULT_H
