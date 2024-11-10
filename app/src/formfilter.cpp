#include "formfilter.h"
#include "ui_formfilter.h"

FormFilter::FormFilter(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FormFilter)
{
    ui->setupUi(this);
}

FormFilter::~FormFilter()
{
    delete ui;
}
