#ifndef QSPORTEVENT_H
#define QSPORTEVENT_H

#include <QObject>
#include <QVector>

#include "race.h"

class QSportEvent : public QObject
{
    Q_OBJECT
public:
    enum Type_csv{
        SECRETAR_ST_ONE = 0,
        SECRETAR_ST_TWO,
        SECRETAR_ST_FOUR,
        ORGEO
    };

    explicit QSportEvent(QObject *parent = nullptr);
    ~QSportEvent();

    void addRace();

    void importSportorgJSON(const QString& path);
    void exportSportorgJSON(const QString& path);

    void importCSV(const QString& path, Type_csv tcsv);

    const QString getNameOrganization(const QString& id);
    const QString getNameGroup(const QString& id);

    const QStringList getNamesOrganization();
    const QStringList getNamesGroup();

    const bool isBibFree(int bib);
    const bool isCardNumFree(int cardNum);
    const bool isResultBibFree(int bib);
    const bool checkingCardNumInPerson(int bib, int cardNum);
    const bool checkingCardNumInResult(int cardNum);
    const QString getPersonInfoFromBib(int bib);
    const QString getPersonInfoFromCardNum(int cardNum);
    const QString getResultFromBib(int bib);
    const int getBibFromCardNum(int cardNum);
    void setPNameFromId(const QString &id, const QString &name);
    void setPSurnameFromId(const QString &id, const QString &surname);
    void setPersonComment(const QString &id, const QString &comment);
    void setPersonBirthDate(const QString &id, const QString &birthDate);
    int setPersonStartTime(const QString &id, const int &startTime);
    int getCardNumFromBib(int bib);
    int setCardNumFromBib(int bib, int cardNum);
    int setCardNumFromId(const QString &id, int cardNum);
    int setCardNumFromCsv(QString path);
    void clearAllCardNum();
    int setBibFromId(QString id, int bib);
    int setBibInResult(double created, int bib);
    int setPOrgFromNameOrg(const QString &id, const QString &nameOrg);
    int setPGroupFromNameGroup(const QString &id, const QString &nameGroup);
    int clearBibInResult(int cardNum);
    int setResultStatus(int cardNum, int status);
    int setResultStatus(double created, int status);
    int addResult(int bib, QByteArray ba);

    QString getOnlineUrl();
    bool getOnlineEnable();
    void setOnlineUrl(const QString &url);
    void setOnlineEnable(bool status);

    QString getNameEvent(){ return races_.at(0)->getNameRece();};
    QVector<race*> getRaces(){return races_;};
    const st_race& getDataRace(){return races_.at(0)->getDataRace();};
    const st_person* getDataPersonId(QString id){return races_.at(0)->getDataPersonId(id);};
    const st_person* getDataPersonBib(int bib){return races_.at(0)->getDataPersonBib(bib);};
    const st_result* getDataResultBib(int bib){return races_.at(0)->getDataResultBib(bib);};

    const QJsonObject getResultToOnline(const QString& number);
    bool empty(){ return races_.size() == 0;};

signals:


private:
    QVector<race*> races_;
    QJsonObject toJson() const;

    st_person lineToPerson(const QStringList &data, Type_csv tcsv);
    st_person lineToPersonFromSecretarStOne(const QStringList &data);
    st_person lineToPersonFromSecretarStTwo(const QStringList &data);
    st_person lineToPersonFromSecretarStFour(const QStringList &data);
    st_person lineToPersonFromOrgeoNotPre(const QStringList &data);
    st_person lineToPersonFromOrgeoWithPre(const QStringList &data);

};

#endif // QSPORTEVENT_H
