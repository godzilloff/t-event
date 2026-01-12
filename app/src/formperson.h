#ifndef FORMPERSON_H
#define FORMPERSON_H

#include <QDialog>

namespace Ui {
class FormPerson;
}

class FormPerson : public QDialog
{
    Q_OBJECT

public:
    explicit FormPerson(QWidget *parent = nullptr);
    ~FormPerson();

signals:
    void requestSave();

private slots:
    void onCheck_numCard(const QString &text);

private:
    Ui::FormPerson *ui;

    QString id;

    void update_organization();
    void update_group();
    void onAccepted();
};

#endif // FORMPERSON_H
