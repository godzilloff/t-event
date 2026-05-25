#ifndef PERSONEDITDIALOG_H
#define PERSONEDITDIALOG_H

//#include "Document.h"
#include "EditDialogBase.h"
#include <QMap>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QDateEdit;
class QTimeEdit;
class QSpinBox;
class QTabWidget;
class QTableView;
class QLabel;
class QFormLayout;
class QVBoxLayout;
class QHBoxLayout;
class QDialogButtonBox;

class Document;
QT_END_NAMESPACE

class QSqlTableModel;

class PersonEditDialog : public EditDialogBase
{
    Q_OBJECT

public:
    explicit PersonEditDialog(Document* document, qint64 recordId = -1,
                              QWidget* parent = nullptr);
    ~PersonEditDialog();

    // Публичные методы для управления диалогом
    bool save() { return saveRecord(); }
    bool validate() { return validateForm(); }

signals:
    void delegationCreated(qint64 id, const QString& name);

protected:
    void setupUi() override;
    void loadData() override;
    bool validateForm() override;
    QHash<QString, QVariant> collectFormData() override;

private slots:
    void onParticipantTypeChanged(int index);
    void updateUiForType();
    void onApplyClicked();

    void onNewDelegationClicked();

private:
    void setupComboBoxes();
    void createMainTab(QWidget* tab);
    QFormLayout* createFormLayout(QWidget* parent);
    void refreshDelegationsCombo();

    // Layouts
    QVBoxLayout* m_mainLayout = nullptr;
    QFormLayout* m_formLayout = nullptr;

    // Виджеты
    QTabWidget* m_tabWidget = nullptr;
    QComboBox* m_comboType = nullptr;
    QLineEdit* m_editName = nullptr;
    QLineEdit* m_editBib = nullptr;
    QLineEdit* m_editChip = nullptr;
    QComboBox* m_comboDelegation = nullptr;
    QComboBox* m_comboDistance = nullptr;
    QComboBox* m_comboAgeGroup = nullptr;
    QComboBox* m_comboGender = nullptr;
    QDateEdit* m_editBirthDate = nullptr;
    QTimeEdit* m_editStartTime = nullptr;
    QSpinBox* m_spinTeamSize = nullptr;
    QPushButton* m_btnNewDelegation = nullptr;

    // Labels
    QLabel* m_labelType;
    QLabel* m_labelName;
    QLabel* m_labelBib;
    QLabel* m_labelChip;
    QLabel* m_labelDelegation;
    QLabel* m_labelDistance;
    QLabel* m_labelAgeGroup;
    QLabel* m_labelBirthDate;
    QLabel* m_labelGender;
    QLabel* m_labelStartTime;
    QLabel* m_labelTeamSize;

    QSqlTableModel* m_delegationsModel = nullptr;
    QSqlTableModel* m_distancesModel = nullptr;
    QSqlTableModel* m_ageGroupsModel = nullptr;

    bool m_isTeam = false;
};

#endif // PERSONEDITDIALOG_H
