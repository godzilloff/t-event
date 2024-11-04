#ifndef GROUP_H
#define GROUP_H


#include <QJsonObject>
#include <QJsonArray>

struct st_rank{
    bool is_active = false;
    QString max_place = "2";
    // "max_time": null,
    // "min_scores": null,
    int percent = 0;
    int qual = 8;
    bool use_scores = false;
};

struct st_group{
    int count_finished = 0;
    int count_person = 0;
    QString course_id ="";
    int first_number = 0;
    QString id = "";
    bool is_any_course = false;
    QString long_name = "";
    QString name;
    int max_age = 0;
    int max_time = 0;
    int max_year = 0;
    int min_age = 0;
    int min_year = 0;
    int order_in_corridor = 0;
    int price = 0;
    int relay_legs = 0;
    int sex = 0;
    int start_corridor = 0;
    int start_interval = 120000;
    QString object = "Group";
};

// "groups": [
// {
//   "__type": null,
//   "count_finished": 0,
//   "count_person": 0,
//   "course_id": "e1d0f44d-a5dd-45b4-830c-5a16ffba1197",
//   "first_number": 0,
//   "id": "9b1bdb80-9ffc-40a7-a577-3a77d35b6870",
//   "is_any_course": false,
//   "long_name": "\u044e\u043d\u0438\u043e\u0440\u044b",
//   "max_age": 0,
//   "max_time": 3600000,
//   "max_year": 1996,
//   "min_age": 0,
//   "min_year": 1999,
//   "name": "\u044e",
//   "object": "Group",
//   "order_in_corridor": 0,
//   "price": 200,

//   "relay_legs": 0,
//   "sex": 0,
//   "start_corridor": 0,
//   "start_interval": 120000

//   "ranking": {
//     "is_active": false,
//     "rank": [
//       {
//         "is_active": false,
//         "max_place": "2",
//         "max_time": null,
//         "min_scores": null,
//         "percent": 0,
//         "qual": 8,
//         "use_scores": false
//       },
//       {
//         "is_active": false,
//         "max_place": "6",
//         "max_time": null,
//         "min_scores": null,
//         "percent": 0,
//         "qual": 7,
//         "use_scores": false
//       },
//       {
//         "is_active": true,
//         "max_place": "0",
//         "max_time": null,
//         "min_scores": null,
//         "percent": 0,
//         "qual": 4,
//         "use_scores": true
//       },
//       {
//         "is_active": true,
//         "max_place": "0",
//         "max_time": null,
//         "min_scores": null,
//         "percent": 0,
//         "qual": 5,
//         "use_scores": true
//       },
//       {
//         "is_active": true,
//         "max_place": "0",
//         "max_time": null,
//         "min_scores": null,
//         "percent": 0,
//         "qual": 6,
//         "use_scores": true
//       },
//       {
//         "is_active": false,
//         "max_place": "0",
//         "max_time": null,
//         "min_scores": null,
//         "percent": 0,
//         "qual": 1,
//         "use_scores": true
//       },
//       {
//         "is_active": false,
//         "max_place": "0",
//         "max_time": null,
//         "min_scores": null,
//         "percent": 0,
//         "qual": 2,
//         "use_scores": true
//       },
//       {
//         "is_active": false,
//         "max_place": "0",
//         "max_time": null,
//         "min_scores": null,
//         "percent": 0,
//         "qual": 3,
//         "use_scores": true
//       }
//     ],
//     "rank_scores": 0
//   },

class group
{
public:
    group();
    group(QString id, QString name);
    group(QJsonObject objJson);
    QJsonObject toJson() const;

    int getCount_finished(){ return data.count_finished;};
    int getCount_person(){ return data.count_person;};
    int getFirst_number(){ return data.first_number;};
    int getOrder_in_corridor(){ return data.order_in_corridor;};
    int getMax_age(){ return data.max_age;};
    int getMax_time(){ return data.max_time;};
    int getMax_year(){ return data.max_year;};
    int getMin_age(){ return data.min_age;};
    int getMin_year(){ return data.min_year;};
    int getPprice(){ return data.price;};
    int getRelay_legs(){ return data.relay_legs;};
    int getSex(){ return data.sex;};
    int getStart_corridor(){ return data.start_corridor;};
    int getStart_interval(){ return data.start_interval;};

    QString getId() { return data.id;};
    QString getName() { return data.name;};
    QString getLongName() { return data.long_name;};
    QString getCourseId(){ return data.course_id;};


private:
    st_group data;
};

#endif // GROUP_H
