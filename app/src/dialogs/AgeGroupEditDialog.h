#ifndef AGEGROUPEDITDIALOG_H
#define AGEGROUPEDITDIALOG_H

#include "EditDialogBase.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;
class QFormLayout;
class QVBoxLayout;
class QGroupBox;
QT_END_NAMESPACE

class AgeGroupEditDialog : public EditDialogBase
{
    Q_OBJECT

public:
    explicit AgeGroupEditDialog(Document* document, qint64 recordId, QWidget* parent);
    ~AgeGroupEditDialog();

protected:
    void setupUi() override;
    void loadData() override;
    bool validateForm() override;
    QHash<QString, QVariant> collectFormData() override;

private:
    void createFormLayout();
    
    // Layouts
    QVBoxLayout* m_mainLayout = nullptr;
    QFormLayout* m_formLayout = nullptr;
    
    // Виджеты
    QLineEdit* m_editName = nullptr;
    QSpinBox* m_spinMinAge = nullptr;
    QSpinBox* m_spinMaxAge = nullptr;
    QDoubleSpinBox* m_spinPrice = nullptr;
    
    // Labels
    QLabel* m_labelName;
    QLabel* m_labelMinAge;
    QLabel* m_labelMaxAge;
    QLabel* m_labelPrice;
    
    // Group box
    QGroupBox* m_groupBox = nullptr;
};

#endif // AGEGROUPEDITDIALOG_H
