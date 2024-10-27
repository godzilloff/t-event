#ifndef FORMPREPAR_H
#define FORMPREPAR_H

#include <QDialog>

#include "qsportevent.h"

namespace Ui {
class FormPrepar;
}

class FormPrepar : public QDialog
{
    Q_OBJECT

public:
    explicit FormPrepar(QWidget *parent = nullptr);
    ~FormPrepar();

    void update_sevent(std::shared_ptr<QSportEvent> &pSEvent);

private:
    Ui::FormPrepar *ui;

    std::shared_ptr<QSportEvent> ptrSEvent;

    void onSetBib();

};

#endif // FORMPREPAR_H
