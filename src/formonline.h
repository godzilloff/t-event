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
    struct Settings {
        QString url;
        bool onlineEnabled = false;
    };

    explicit FormOnline(QWidget *parent = nullptr);
    ~FormOnline();

    Settings settings() const;

private slots:
    void apply();

private:
    void updateSettings();
    void loadSettingsFromIni();
    void saveSettingsFromIni();

    Ui::FormOnline *ui;
    Settings m_currentSettings;
};

#endif // FORMONLINE_H
