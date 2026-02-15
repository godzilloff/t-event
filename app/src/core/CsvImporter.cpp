#include "CsvImporter.h"
#include "DatabaseManager.h"
//#include "Document.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

CsvImporter::CsvImporter(QObject* parent) : QObject(parent) {}

bool CsvRecord::isValid() const
{
    return !fullName.isEmpty() && 
           !delegationName.isEmpty() && 
           !bibNumber.isEmpty() &&
           !ageGroup.isEmpty();
}

QString CsvRecord::toString() const
{
    return QString("%1 | %2 | %3 | №%4")
        .arg(fullName,delegationName,ageGroup,bibNumber);
}

// Формат csv с сайта orgeo.ru:
//Группа;ФИО;Коллектив;Представитель;Разряд;Номер;Год рождения;Номер чипа;Комментарий;
CsvRecord CsvImporter::parseLine(const QString& line)
{
    CsvRecord record;
    
    // Разделяем строку по точке с запятой
    QStringList parts = line.split(';');
    
    if (parts.size() >= 8) {
        record.ageGroup = sanitizeString(parts.value(0));
        record.fullName = sanitizeString(parts.value(1));
        record.delegationName = sanitizeString(parts.value(2));
        record.sportRank = sanitizeString(parts.value(3));
        record.bibNumber = sanitizeString(parts.value(4));
        record.birthYear = sanitizeString(parts.value(5));
        record.chipNumber = sanitizeString(parts.value(6));
        record.comment = sanitizeString(parts.value(7));
    }
    
    return record;
}

bool CsvImporter::validateRecord(const CsvRecord& record)
{
    // Проверяем обязательные поля
    if (record.fullName.isEmpty()) {
        m_lastError = tr("Отсутствует ФИО участника");
        return false;
    }
    
    if (record.delegationName.isEmpty()) {
        m_lastError = tr("Отсутствует название делегации");
        return false;
    }
    
    if (record.bibNumber.isEmpty()) {
        m_lastError = tr("Отсутствует номер участника");
        return false;
    }
    
    // Проверяем формат номера участника
    bool ok;
    record.bibNumber.toInt(&ok);
    if (!ok) {
        m_lastError = tr("Некорректный номер участника: %1").arg(record.bibNumber);
        return false;
    }
    
    // Проверяем год рождения
    if (!record.birthYear.isEmpty()) {
        int year = record.birthYear.toInt(&ok);
        if (!ok || year < 1900 || year > QDate::currentDate().year()) {
            m_lastError = tr("Некорректный год рождения: %1").arg(record.birthYear);
            return false;
        }
    }
    
    // Проверяем номер чипа
    if (!record.chipNumber.isEmpty()) {
        record.chipNumber.toInt(&ok);
        if (!ok && record.chipNumber != "0") {
            m_lastError = tr("Некорректный номер чипа: %1").arg(record.chipNumber);
            return false;
        }
    }
    
    return true;
}

QString CsvImporter::sanitizeString(const QString& str)
{
    QString result = str.trimmed();
    
    // Убираем лишние пробелы
    result = result.simplified();
    
    // Убираем кавычки если есть
    if (result.startsWith('"') && result.endsWith('"')) {
        result = result.mid(1, result.length() - 2);
    }
    
    return result;
}

bool CsvImporter::importFromFile(const QString& filePath, QList<CsvRecord>& records)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = tr("Не удалось открыть файл: %1").arg(file.errorString());
        return false;
    }
    
    QTextStream in(&file);
    
    int lineNumber = 0;
    records.clear();
    m_importedCount = 0;
    m_errorCount = 0;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNumber++;
        
        // Пропускаем пустые строки
        if (line.trimmed().isEmpty()) {
            continue;
        }
        
        CsvRecord record = parseLine(line);
        
        if (validateRecord(record)) {
            records.append(record);
            m_importedCount++;
        } else {
            qWarning() << tr("Ошибка в строке %1: %2").arg(lineNumber).arg(m_lastError);
            m_errorCount++;
        }
        
        emit progressChanged(lineNumber, -1);
    }
    
    file.close();
    
    return m_importedCount > 0;
}

bool CsvImporter::importFromString(const QString& csvData, QList<CsvRecord>& records)
{
    QStringList lines = csvData.split('\n', Qt::SkipEmptyParts);
    
    records.clear();
    m_importedCount = 0;
    m_errorCount = 0;
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty()) {
            continue;
        }
        
        CsvRecord record = parseLine(line);
        
        if (validateRecord(record)) {
            records.append(record);
            m_importedCount++;
        } else {
            qWarning() << tr("Ошибка в строке %1: %2").arg(i + 1).arg(m_lastError);
            m_errorCount++;
        }
        
        emit progressChanged(i + 1, lines.size());
    }
    
    return m_importedCount > 0;
}

bool CsvImporter::importToDatabase(const QString& filePath, qint64 competitionId)
{
    QList<CsvRecord> records;
    if (!importFromFile(filePath, records)) {
        m_lastError = tr("Не удалось загрузить данные из CSV файла");
        return false;
    }
    
    auto& db = DatabaseManager::instance();
    
    // Начинаем транзакцию
    if (!db.beginTransaction()) {
        m_lastError = tr("Не удалось начать транзакцию");
        return false;
    }
    
    int successCount = 0;
    
    // Сначала создаем или находим делегации
    QMap<QString, qint64> delegationMap; // Название делегации -> ID
    
    for (const CsvRecord& record : records) {
        if (!delegationMap.contains(record.delegationName)) {
            // Проверяем, существует ли уже такая делегация
            QString sql = "SELECT id FROM delegations WHERE name = :name AND competition_id = :competition_id";
            QHash<QString, QVariant> params;
            params["name"] = record.delegationName;
            params["competition_id"] = competitionId;
            
            auto existing = db.executeSelect(sql, params);
            if (!existing.isEmpty()) {
                delegationMap[record.delegationName] = existing.first()["id"].toLongLong();
            } else {
                // Создаем новую делегацию
                QHash<QString, QVariant> data;
                data["competition_id"] = competitionId;
                data["name"] = record.delegationName;
                data["representative"] = "";
                data["contact"] = "";
                
                qint64 delegationId = -1;
                if (db.createRecord("delegations", data, &delegationId)) {
                    delegationMap[record.delegationName] = delegationId;
                } else {
                    qWarning() << tr("Не удалось создать делегацию: %1").arg(record.delegationName);
                }
            }
        }
    }
    
    // Теперь импортируем участников
    for (const CsvRecord& record : records) {
        qint64 delegationId = delegationMap.value(record.delegationName, -1);
        if (delegationId == -1) {
            qWarning() << tr("Не найдена делегация для участника: %1").arg(record.fullName);
            continue;
        }
        
        // Находим возрастную группу по названию
        QString ageGroupSql = "SELECT id FROM age_groups WHERE name = :name AND competition_id = :competition_id";
        QHash<QString, QVariant> ageGroupParams;
        ageGroupParams["name"] = record.ageGroup;
        ageGroupParams["competition_id"] = competitionId;
        
        auto ageGroups = db.executeSelect(ageGroupSql, ageGroupParams);
        qint64 ageGroupId = ageGroups.isEmpty() ? -1 : ageGroups.first()["id"].toLongLong();
        
        // Находим дистанцию (по умолчанию первая дистанция)
        QString distanceSql = "SELECT id FROM distances WHERE competition_id = :competition_id LIMIT 1";
        QHash<QString, QVariant> distanceParams;
        distanceParams["competition_id"] = competitionId;
        
        auto distances = db.executeSelect(distanceSql, distanceParams);
        qint64 distanceId = distances.isEmpty() ? -1 : distances.first()["id"].toLongLong();
        
        if (distanceId == -1) {
            qWarning() << tr("Не найдена дистанция для соревнования");
            continue;
        }
        
        // Создаем участника
        QHash<QString, QVariant> data;
        data["competition_id"] = competitionId;
        data["delegation_id"] = delegationId;
        data["distance_id"] = distanceId;
        data["age_group_id"] = ageGroupId;
        data["participant_type"] = "individual";
        data["full_name"] = record.fullName;
        data["bib_number"] = record.bibNumber;
        
        // Год рождения
        if (!record.birthYear.isEmpty()) {
            int year = record.birthYear.toInt();
            QDate birthDate(year, 1, 1); // 1 января указанного года
            data["birth_date"] = birthDate.toString(Qt::ISODate);
        }
        
        // Номер чипа
        if (!record.chipNumber.isEmpty() && record.chipNumber != "0") {
            data["chip_number"] = record.chipNumber;
        }
        
        // Определяем пол по возрастной группе
        QString gender = "Мужской";
        if (record.ageGroup.startsWith("ж-") || 
            record.ageGroup.contains("ЖЕН", Qt::CaseInsensitive) ||
            record.ageGroup.contains("ДЕВ", Qt::CaseInsensitive)) {
            gender = "Женский";
        }
        data["gender"] = gender;
        
        qint64 participantId = -1;
        if (db.createRecord("participants", data, &participantId)) {
            successCount++;
        } else {
            qWarning() << tr("Не удалось создать участника: %1").arg(record.fullName);
        }
    }
    
    if (successCount > 0) {
        db.commitTransaction();
        m_importedCount = successCount;
        emit importFinished(successCount, records.size() - successCount);
        return true;
    } else {
        db.rollbackTransaction();
        m_lastError = tr("Не удалось импортировать ни одного участника");
        return false;
    }
}
