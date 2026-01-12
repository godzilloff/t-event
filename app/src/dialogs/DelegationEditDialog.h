#ifndef DELEGATIONEDITDIALOG_H
#define DELEGATIONEDITDIALOG_H

#include "EditDialogBase.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTextEdit;
class QLabel;
class QFormLayout;
class QVBoxLayout;
class QGroupBox;
QT_END_NAMESPACE

class DelegationEditDialog : public EditDialogBase
{
    Q_OBJECT

public:
    explicit DelegationEditDialog(Document* document, qint64 recordId = -1,
                                    QWidget* parent = nullptr);
    ~DelegationEditDialog();

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
    QLineEdit* m_editRepresentative = nullptr;
    QTextEdit* m_editContact = nullptr;
    
    // Labels
    QLabel* m_labelName;
    QLabel* m_labelRepresentative;
    QLabel* m_labelContact;
    
    // Group box
    QGroupBox* m_groupBox = nullptr;
};

#endif // DELEGATIONEDITDIALOG_H
