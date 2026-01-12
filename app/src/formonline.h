#ifndef FORMONLINE_H
#define FORMONLINE_H

#include <QDialog>

namespace Ui {
class FormOnline;
}

class FormOnline : public QDialog
{
    Q_OBJECT

public:
    explicit FormOnline(QWidget *parent = nullptr);
    ~FormOnline();

public slots:
    void recieveDataFromMain();

private:
    Ui::FormOnline *ui;

    void onAccepted();
};

#endif // FORMONLINE_H
