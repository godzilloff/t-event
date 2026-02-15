//sportident_types.h

#ifndef SPORTIDENT_TYPES_H
#define SPORTIDENT_TYPES_H

#include <QDateTime>
#include <QVector>
#include <QPair>
#include <stdexcept>
#include <cstdint>

namespace SportIdent {

// Типы исключений
class SIException : public std::runtime_error {
public:
    explicit SIException(const std::string& message) : std::runtime_error(message) {}
};

class SITimeoutException : public SIException {
public:
    explicit SITimeoutException(const std::string& message) : SIException(message) {}
};

// Состояния чтения карты
enum class CardReadState {
    Idle,              // Ожидание
    CardDetected,      // Карта обнаружена (C_SI9_DET получен)
    ReadingBlock0,     // Чтение блока 0 отправлен запрос
    AwaitingBlock0,    // Ожидание ответа на блок 0
    DeterminingBlocks, // Определение количества блоков для чтения
    ReadingBlocks,     // Чтение остальных блоков
    AwaitingBlock,     // Ожидание ответа на запрос блока
    ReadingComplete,   // Чтение завершено
    SendingAck,        // Отправка подтверждения
    Error,             // Ошибка
    Acknowledged       // Карта подтверждена (ACK отправлен)
};

// Типы событий станции
enum class StationEvent {
    None,
    CardInserted,      // C_SI9_DET - карта обнаружена
    CardRemoved,       // C_SI_REM - карта извлечена
    BlockDataReceived, // Данные блока получены (C_GET_SI9)
    AckRequired,       // Требуется подтверждение (после чтения)
    ConfigReceived,    // Конфигурация получена (C_GET_SYS_VAL)
    TimeReceived,      // Время получено (C_GET_TIME)
    BeepComplete       // Звуковой сигнал завершен (C_BEEP)
};

// Тип карты
enum class CardType {
    Unknown,
    SI5,
    SI6,
    SI8,
    SI9,
    SI10,
    SI11,
    SIpCard,
    SItCard,
    SIAC
};

// Отметка на контрольном пункте
struct Punch {
    uint32_t controlCode;    // Код контрольного пункта
    QDateTime timestamp;     // Время отметки
    uint8_t subsecond = 0;   // Доля секунды (1/256)
};

// Данные карты
struct CardData {
    uint32_t cardNumber = 0;          // Номер карты
    CardType cardType = CardType::Unknown;
    QDateTime startTime;              // Время старта
    QDateTime finishTime;             // Время финиша
    QDateTime checkTime;              // Время проверки
    QDateTime clearTime;              // Время очистки
    QVector<Punch> punches;           // Отметки
    uint8_t punchCount = 0;           // Количество отметок

    bool isValid() const { return cardNumber > 0; }
};

// Состояние чтения карты
struct ReadSession {
    uint32_t cardNumber = 0;
    CardType cardType = CardType::Unknown;
    CardReadState state = CardReadState::Idle;
    QDateTime detectionTime;
    QByteArray rawData;               // Все собранные данные
    int blocksRead = 0;               // Прочитано блоков
    int blocksToRead = 0;             // Всего блоков для чтения
    int currentBlock = -1;            // Текущий читаемый блок
    CardData decodedData;             // Декодированные данные
    bool acknowledged = false;        // Подтверждение отправлено

    void reset() {
        cardNumber = 0;
        cardType = CardType::Unknown;
        state = CardReadState::Idle;
        rawData.clear();
        blocksRead = 0;
        blocksToRead = 0;
        currentBlock = -1;
        decodedData = CardData();
        acknowledged = false;
    }

    bool isComplete() const {
        return state == CardReadState::ReadingComplete ||
               state == CardReadState::Acknowledged;
    }

    bool inProgress() const {
        return state != CardReadState::Idle &&
               state != CardReadState::Error &&
               !isComplete();
    }
};

// Конфигурация протокола станции
struct StationConfig {
    bool extendedProtocol = true;     // Расширенный протокол
    bool autoSendMode = false;        // Автоотправка
    bool handshakeMode = true;        // Подтверждение получения
    bool passwordAccess = false;      // Доступ по паролю
    bool punchReadout = false;        // Чтение после отметки
    uint8_t operatingMode = 0;        // Режим работы
    uint16_t stationCode = 0;         // Код станции
};

// Информация о станции
struct StationInfo {
    uint32_t serialNumber = 0;        // Серийный номер
    QString firmwareVersion;          // Версия прошивки
    QDate buildDate;                  // Дата сборки
    uint16_t modelId = 0;             // ID модели
    uint8_t memorySize = 0;           // Размер памяти (КБ)
    QDate batteryDate;                // Дата установки батареи
    uint16_t batteryCapacity = 0;     // Емкость батареи (мАч)
    uint8_t stationCode = 0;          // Код станции
};

} // namespace SportIdent

#endif // SPORTIDENT_TYPES_H
