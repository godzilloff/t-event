#ifndef FORMPERSON_H
#define FORMPERSON_H

#include <QDialog>

#include "person.h"

#include "qsportevent.h"

namespace Ui {
class FormPerson;
}

class FormPerson : public QDialog
{
    Q_OBJECT

public:
    explicit FormPerson(QWidget *parent = nullptr);
    ~FormPerson();

    void update_sevent(std::shared_ptr<QSportEvent> &pSEvent);

public slots:
    void recieveDataFromMain(const st_person* data_);

private slots:
    void onCheck_numCard(const QString &text);

private:
    Ui::FormPerson *ui;

    std::shared_ptr<QSportEvent> ptrSEvent;
    QString id;

    void update_organization();
    void update_group();
    void onAccepted();
};

#endif // FORMPERSON_H
