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

    void update_sevent(QSportEvent* pSEvent);

public slots:
    void recieveDataFromMain(const st_person* data_);

private:
    Ui::FormPerson *ui;
    QSportEvent* prtSEvent;

    void update_organization();
    void update_group();
};

#endif // FORMPERSON_H
