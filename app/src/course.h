#ifndef COURSE_H
#define COURSE_H

#include <QJsonObject>
#include <QJsonArray>

struct st_courscontrol{
    QString code = "";          // "31"
    int length = 0;             // 1000
    QString object = "CourseControl";
};


class TcourseControl
{
public:
    TcourseControl();
    TcourseControl(QJsonObject objJson);
    QJsonObject toJson() const;

    QString getCode(){return data.code;};

private:
    st_courscontrol data;
};

struct st_cours{
    int bib = 0;
    int climb = 0;
    int length = 0;
    QString name;
    QString id;
    int corridor = 0;
    QString object = "Course";
};


class course
{
public:
    course();
    course(QJsonObject objJson);
    QJsonObject toJson() const;

    QString getId() { return data.id;};
    QString getName(){ return data.name;};

    int getBib(){ return data.bib;};
    int getClimb(){ return data.climb;};
    int getLength(){ return data.length;};
    //int getCorridor(){ return data.corridor;};

    int getCntControll(){ return ccontrols.size();};
    QString getPathCC() const;

private:
    st_cours data;
    QVector<TcourseControl*> ccontrols;

    QJsonArray jsonArrCControl;
};

// "courses": [
// {
// "bib": 0,
// "climb": 50,
// "corridor": 0,
// "id": "e1d0f44d-a5dd-45b4-830c-5a16ffba1197",
// "length": 3600,
// "name": "\u041c10",
// "object": "Course"
// "controls": [
//     {
//         "code": "31",
//         "length": 58,
//         "object": "CourseControl"
//     },
//     {
//         "code": "65",
//         "length": 98,
//         "object": "CourseControl"
//     },
//     {
//         "code": "85",
//         "length": 85,
//         "object": "CourseControl"
//     },
//     {
//         "code": "61",
//         "length": 65,
//         "object": "CourseControl"
//     }
// ]
// }
// ],


#endif // COURSE_H
