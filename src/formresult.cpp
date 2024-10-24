#include "formresult.h"
#include "ui_formresult.h"

#include <QButtonGroup>
#include <QPushButton>

FormResult::FormResult(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FormResult)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &FormResult::onAccepted);
    connect(ui->ed_bib, &QSpinBox::textChanged, this, &FormResult::onCheck_bib);

}

FormResult::~FormResult()
{
    delete ui;
}

void FormResult::update_sevent(std::shared_ptr<QSportEvent> &pSEvent)
{
    ptrSEvent = pSEvent;
}

void FormResult::recieveDataFromMain(const st_result* data_)
{
    ptrDataResult = data_;
    QTime time_create = QTime::fromMSecsSinceStartOfDay(data_->credit_time);
    QTime time_start  = QTime::fromMSecsSinceStartOfDay(data_->start_time);
    QTime time_finish = QTime::fromMSecsSinceStartOfDay(data_->finish_time);
    QTime time_result = QTime::fromMSecsSinceStartOfDay(data_->result_msec);
    //QDate birth_date_d = QDate::fromString(data_->birth_date, "yyyy-MM-dd");

    int temp = data_->bib;
    ui->ed_bib->setValue(temp);
    ui->ed_create->setTime(time_create);
    ui->ed_start->setTime(time_start);
    ui->ed_finish->setTime(time_finish);
    ui->ed_result->setTime(time_result);
}

void FormResult::onCheck_bib(const QString &)
{
    //qDebug() << "on_check_numCard_textChanged ok";
    int bib = ui->ed_bib->value();
    QString bib_txt = QString::number(bib);
    if (ptrSEvent != nullptr){
        if (ptrSEvent->isBibFree(bib))
        {
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            ui->lb_person->setText(bib_txt + " - неизвестен");
        } else {
            QString bibResult{};
            bool stButton = true;

            if (!ptrSEvent->isResultBibFree(bib)){
                if (ptrDataResult->bib != bib) stButton = false;
                bibResult = " (результат: " + ptrSEvent->getResultFromBib(bib) + ")";
            }

            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(stButton);
            ui->lb_person->setText(ptrSEvent->getPersonInfoFromBib(bib) + bibResult);
        }
    }
}

void FormResult::onAccepted()
{
    ptrSEvent->setBibFromCardNum(ptrDataResult->card_number, ui->ed_bib->value());
}
