#ifndef FORMINFO_H
#define FORMINFO_H

#include <QDialog>

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

private slots:
    void onButtonBox_accepted();

private:
    Ui::FormInfo *ui;
};

#endif // FORMINFO_H
