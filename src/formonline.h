#ifndef FORMONLINE_H
#define FORMONLINE_H

#include <QDialog>

#include "qsportevent.h"

namespace Ui {
class FormOnline;
}

class FormOnline : public QDialog
{
    Q_OBJECT

public:
    explicit FormOnline(QWidget *parent = nullptr);
    ~FormOnline();

    void update_sevent(std::shared_ptr<QSportEvent> &pSEvent);

public slots:
    void recieveDataFromMain();

private:
    Ui::FormOnline *ui;
    std::shared_ptr<QSportEvent> ptrSEvent;

    void onAccepted();
};

#endif // FORMONLINE_H
