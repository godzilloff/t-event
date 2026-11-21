#ifndef RESULTEDITDIALOG_H
#define RESULTEDITDIALOG_H

#include "EditDialogBase.h"
#include <QDate>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTimeEdit;
class QLabel;
class QFormLayout;
class QVBoxLayout;
class QGroupBox;
QT_END_NAMESPACE

class ResultEditDialog : public EditDialogBase
{
    Q_OBJECT

public:
    explicit ResultEditDialog(Document* document, qint64 recordId = -1,
                              QWidget* parent = nullptr);
    ~ResultEditDialog();

protected:
    void setupUi() override;
    void loadData() override;
    bool validateForm() override;
    QHash<QString, QVariant> collectFormData() override;

private slots:
    void onTimesChanged();

private:
    void createInfoGroup();
    void createEditGroup();
    void recalculateResult();

    // Layouts
    QVBoxLayout* m_mainLayout = nullptr;
    QGroupBox*   m_infoGroup  = nullptr;
    QGroupBox*   m_editGroup  = nullptr;
    QFormLayout* m_infoLayout = nullptr;
    QFormLayout* m_formLayout = nullptr;

    // Read-only
    QLabel* m_lbParticipant = nullptr;
    QLabel* m_lbBib         = nullptr;
    QLabel* m_lbChip        = nullptr;
    QLabel* m_lbAgeGroup    = nullptr;
    QLabel* m_lbDelegation  = nullptr;
    QLabel* m_lbDistance    = nullptr;

    // Editable
    QTimeEdit* m_editStart   = nullptr;
    QTimeEdit* m_editFinish  = nullptr;
    QTimeEdit* m_editResult  = nullptr;   // read-only, считается
    QComboBox* m_comboStatus = nullptr;

    qint64 m_participantId = -1;
    bool   m_updating      = false;
};

#endif // RESULTEDITDIALOG_H
