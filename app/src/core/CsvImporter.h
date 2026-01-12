#ifndef CSVIMPORTER_H
#define CSVIMPORTER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QVariant>

struct CsvRecord {
    QString ageGroup;          // возрастная группа
    QString fullName;          // Фамилия Имя
    QString delegationName;    // Название делегации
    QString sportRank;         // код спортивного разряда
    QString bibNumber;         // номер участника
    QString birthYear;         // год рождения
    QString chipNumber;        // номер чипа электронной отметки
    QString comment;           // комментарий
    
    bool isValid() const;
    QString toString() const;
};

class CsvImporter : public QObject
{
    Q_OBJECT
    
public:
    explicit CsvImporter(QObject* parent = nullptr);
    
    bool importFromFile(const QString& filePath, QList<CsvRecord>& records);
    bool importFromString(const QString& csvData, QList<CsvRecord>& records);
    
    QString lastError() const { return m_lastError; }
    int importedCount() const { return m_importedCount; }
    int errorCount() const { return m_errorCount; }
    
    // Основной метод импорта в базу данных
    bool importToDatabase(const QString& filePath, qint64 competitionId);
    
signals:
    void progressChanged(int current, int total);
    void importFinished(int successCount, int errorCount);
    void errorOccurred(const QString& error);
    
private:
    CsvRecord parseLine(const QString& line);
    bool validateRecord(const CsvRecord& record);
    QString sanitizeString(const QString& str);
    
    int m_importedCount = 0;
    int m_errorCount = 0;
    QString m_lastError;
};

#endif // CSVIMPORTER_H
