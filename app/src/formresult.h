#ifndef FORMRESULT_H
#define FORMRESULT_H

#include <QDialog>

#include "result.h"
#include "qsportevent.h"

namespace Ui {
class FormResult;
}

class FormResult : public QDialog
{
    Q_OBJECT

public:
    explicit FormResult(QWidget *parent = nullptr);
    ~FormResult();

    void update_sevent(std::shared_ptr<QSportEvent> &pSEvent);

public slots:
    void recieveDataFromMain(const st_result* data_);

private slots:
    void onCheck_bib(const QString &);

private:
    Ui::FormResult *ui;

    std::shared_ptr<QSportEvent> ptrSEvent;
    const st_result* ptrDataResult;

    void onAccepted();

};

#endif // FORMRESULT_H
