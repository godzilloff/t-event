#ifndef RACE_H
#define RACE_H

#include <QString>

#include <QJsonObject>
#include <QJsonArray>

#include "course.h"
#include "group.h"
#include "organization.h"
#include "person.h"
#include "result.h"

//-- данные / "data": {}
//-- дистанции / courses []
//-- группы / groups []
//-- организации / "organizations": []
//-- участники / "persons": []
//-- результаты / "results": []
//-- настройки / "settings": {}

struct st_race{
    QString chief_referee = "";
    QString description = "";
    QString end_datetime = "";
    QString location = "";
    int race_type = 0;
    int relay_leg_count = 0;
    QString secretary = "";
    QString start_datetime = "";
    QString title = "";
    QString url = "";
};

class race
{
public:
    race();
    race(QJsonObject objJson);
    void updateData(QJsonObject objJson);
    QJsonObject toJson() const;

    ~race();

    QString getNameRece(){ return data.title;};

    QVector<person*> *getPtrPersons(){ return &persons_;};
    QVector<course*> *getPtrCources(){ return &courses_;};
    QVector<group*> *getPtrGroups(){ return &groups_;};
    QVector<result*> *getPtrResults(){ return &results_;};
    QVector<organization*> *getPtrOrganizations(){ return &organizations_;};

    const st_race& getDataRace(){return data;};
    const QJsonObject getResultToOnline(const int number);
    int getResultMsec(const int number);

    int getIndexPerson(int number);
    void addPerson(QString name, QString id_org, QString id_gr, QString qual,  QString year, QString comment);

    const st_person* getDataPersonBib(int number);
    const st_result* getDataResultBib(int number);

    QString getNameFromBib(int number);
    QString getSurnameFromBib(int number);
    QString getFullNameFromBib(int number);

    const bool isBibFree(int bib);
    const bool isCardNumFree(int cardNum);
    const bool isResultBibFree(int bib);
    const bool checkingCardNumInPerson(int bib, int cardNum);
    const bool checkingCardNumInResult(int cardNum);
    const QString getPersonInfoFromBib(int bib);
    const QString getPersonInfoFromCardNum(int cardNum);
    const QString getResultFromBib(int bib);
    int getBibFromCardNum(int cardNum);
    int getCardNumFromBib(int bib);
    int setCardNumFromBib(int bib, int cardNum);
    int setBibFromCardNum(int cardNum, int bib);
    int clearBibInResult(int cardNum);
    int addResult(int bib, QByteArray ba);

    QString getIdGroupByName(const QString& name);
    QString getNameGroupFromBib(int number);
    QString addGroupByName(const QString& name);

    QString getIdOrganizationByName(const QString &name);
    QString getIdOrganizationByNameAndContact(const QString &name,const QString &contact);
    QString getNameOrganizationFromBib(int number);
    QString addOrgByName(const QString &name);
    QString addOrgByNameAndContact(const QString &name,const QString &contact);

    QString getNameOrganization(QString id);
    QString getNameGroup(QString id);
    QString getNameCourse(QString id);

    int getLengthCourse(QString id);
    int getCntKPCourse(QString id);
    int getClimb(QString id);

private:
    st_race data;

    QVector<course*> courses_;
    QVector<group*> groups_;
    QVector<organization*> organizations_;
    QVector<person*> persons_;
    QVector<result*> results_;

};

#endif // RACE_H
