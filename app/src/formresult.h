#ifndef FORMRESULT_H
#define FORMRESULT_H

#include <QDialog>

namespace Ui {
class FormResult;
}

class FormResult : public QDialog
{
    Q_OBJECT

public:
    explicit FormResult(QWidget *parent = nullptr);
    ~FormResult();

signals:
    void requestSave();

private slots:
    void onCheck_bib(const QString &);

private:
    Ui::FormResult *ui;

    void onAccepted();

};

#endif // FORMRESULT_H
