// ResultColumns.h

// ResultColumns.h
#ifndef RESULTCOLUMNS_H
#define RESULTCOLUMNS_H

#include <Qt>
#include <QMetaEnum>

// Пространство имен для колонок результатов
namespace ResultColumn {
    Q_NAMESPACE // Для возможности использовать Q_ENUM_NS

    enum class Source {
        ID = 0,
        FullName = 1,
        AgeGroupName = 2,
        DelegationName = 3,
        ResultTime = 4,
        Status = 5,
        BibNumber = 6,
        ChipNumber = 7,
        StartTime = 8,
        FinishTime = 9,
        Gender = 10,
        ParticipantId = 11,
        DistanceName = 12
    };
    Q_ENUM_NS(Source)

    enum class Virtual {
        Rank = 0,      // Место
        TimeLoss = 1,  // Отставание
        Percent = 2    // Процент от лидера
    };
    Q_ENUM_NS(Virtual)

    // Вспомогательные функции
    inline const char* sourceColumnName(Source col) {
        const QMetaEnum me = QMetaEnum::fromType<Source>();
        return me.valueToKey(static_cast<int>(col));
    }

    inline const char* virtualColumnName(Virtual col) {
        const QMetaEnum me = QMetaEnum::fromType<Virtual>();
        return me.valueToKey(static_cast<int>(col));
    }

    // Константы
    constexpr int SOURCE_COLUMN_COUNT = 13;  // Количество колонок в источнике
    constexpr int VIRTUAL_COLUMN_COUNT = 3;   // Количество виртуальных колонок
    constexpr int VIRTUAL_COLUMN_OFFSET = 1000;
}

#endif // RESULTCOLUMNS_H
