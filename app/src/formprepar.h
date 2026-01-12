#ifndef FORMPREPAR_H
#define FORMPREPAR_H

#include <QDialog>

namespace Ui {
class FormPrepar;
}

class FormPrepar : public QDialog
{
    Q_OBJECT

public:
    explicit FormPrepar(QWidget *parent = nullptr);
    ~FormPrepar();

private:
    Ui::FormPrepar *ui;

    void onSetBib();
    void onClearAllCardNum();
    void onSetCardNum();

};

#endif // FORMPREPAR_H
