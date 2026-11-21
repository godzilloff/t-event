#ifndef FORMINFO_H
#define FORMINFO_H

#include <QDialog>

class Document;

namespace Ui {
class FormInfo;
}

class FormInfo : public QDialog
{
    Q_OBJECT

public:
    explicit FormInfo(Document* document, QWidget *parent = nullptr);
    ~FormInfo();

    // Загрузить данные соревнования из БД в поля формы
    void loadCompetition(qint64 competitionId);

signals:
    void dataSaved();       // успешное сохранение (MainWindow обновит заголовок)
    void dataSavedError(const QString& error);

public slots:
    void accept() override;

private:
    bool saveCompetition();

    Ui::FormInfo *ui;
    Document* m_document = nullptr;
    qint64    m_competitionId = -1;
};

#endif // FORMINFO_H
