#ifndef DISTANCEEDITDIALOG_H
#define DISTANCEEDITDIALOG_H

#include "EditDialogBase.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QTextEdit;
class QLabel;
class QFormLayout;
class QVBoxLayout;
class QGroupBox;
QT_END_NAMESPACE

class DistanceEditDialog : public EditDialogBase
{
    Q_OBJECT

public:
    explicit DistanceEditDialog(Document* document, qint64 recordId, QWidget* parent);
    ~DistanceEditDialog();

signals:
    void distanceCreated(qint64 id, const QString& name);  // Сигнал для создания

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
    QDoubleSpinBox* m_spinLength = nullptr;
    QSpinBox* m_spinControlTime = nullptr;
    QTextEdit* m_editControlPoints = nullptr;
    
    // Labels
    QLabel* m_labelName;
    QLabel* m_labelLength;
    QLabel* m_labelControlTime;
    QLabel* m_labelControlPoints;
    
    // Group box
    QGroupBox* m_groupBox = nullptr;
};

#endif // DISTANCEEDITDIALOG_H
