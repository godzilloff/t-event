#ifndef FORMFILTER_H
#define FORMFILTER_H

#include <QDialog>

namespace Ui {
class FormFilter;
}

class FormFilter : public QDialog
{
    Q_OBJECT

public:
    explicit FormFilter(QWidget *parent = nullptr);
    ~FormFilter();

public slots:
    void recieveDataFromMain();
    void updateStrOrg(int index);
    void updateStrGroup(int index);

private:
    Ui::FormFilter *ui;
    QString org;
    QString group;

    void update_organization();
    void update_group();
};

#endif // FORMFILTER_H
