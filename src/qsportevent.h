#ifndef QSPORTEVENT_H
#define QSPORTEVENT_H

#include <QObject>
#include <QVector>

#include "race.h"

class QSportEvent : public QObject
{
    Q_OBJECT
public:
    explicit QSportEvent(QObject *parent = nullptr);
    ~QSportEvent();

    void addRace();

    void importSportorgJSON(const QString& path);
    void exportSportorgJSON(const QString& path);

    void importCSV(const QString& path);

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
    int getCardNumFromBib(int bib);
    int setCardNumFromBib(int bib, int cardNum);
    int setBibFromCardNum(int cardNum, int bib);
    int clearBibInResult(int cardNum);
    int addResult(int bib, QByteArray ba);

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

};

#endif // QSPORTEVENT_H
