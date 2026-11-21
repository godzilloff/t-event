#ifndef FORMONLINE_H
#define FORMONLINE_H

#include <QDialog>
#include "Document.h"

namespace Ui {
class FormOnline;
}

class FormOnline : public QDialog
{
    Q_OBJECT

public:
    explicit FormOnline(QWidget* parent = nullptr);
    ~FormOnline();
    void setConfig(const OrgeoConfig& config);
    OrgeoConfig config();
signals:
    void configSaved(const OrgeoConfig& config);

private slots:
    void onAccepted();

private:
    Ui::FormOnline* ui;

};

#endif // FORMONLINE_H
