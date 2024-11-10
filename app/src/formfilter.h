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

private:
    Ui::FormFilter *ui;
};

#endif // FORMFILTER_H
