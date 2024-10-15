#ifndef FORMINFO_H
#define FORMINFO_H

#include <QDialog>

#include "race.h"

namespace Ui {
class FormInfo;
}

class FormInfo : public QDialog
{
    Q_OBJECT

public:
    explicit FormInfo(QWidget *parent = nullptr);
    ~FormInfo();

public slots:
    void recieveDataFromMain(const st_race& data_);

private slots:
    void on_buttonBox_accepted();

private:
    Ui::FormInfo *ui;
};

#endif // FORMINFO_H
