// sportident_decoder.h

#ifndef SPORTIDENT_DECODER_H
#define SPORTIDENT_DECODER_H

#include "sportident_types.h"
#include "sportident_constants.h"
#include <QByteArray>
#include <QDateTime>
#include <QString>

namespace SportIdent {

class DataDecoder {
public:
    // Декодирование номера карты
    static uint32_t decodeCardNumber(const QByteArray& number) {
        if (number.size() < 4) {
            throw SIException("Invalid card number length");
        }
        
        // Для SI9/SI10: byte0 = серия, bytes1-3 = номер
        uint32_t num = 0;
        num |= static_cast<uint8_t>(number[1]) << 16;
        num |= static_cast<uint8_t>(number[2]) << 8;
        num |= static_cast<uint8_t>(number[3]);
        
        // Проверка диапазонов для определения типа
        if (num >= 1000000 && num <= 1999999) {
            return num; // SI9
        } else if (num >= 2000000 && num <= 2999999) {
            return num; // SI8
        } else if (num >= 7000000 && num <= 9999999) {
            return num; // SI10/SI11
        } else if (num >= 4000000 && num <= 4999999) {
            return num; // pCard
        } else if (num >= 6000000 && num <= 6999999) {
            return num; // tCard
        } else {
            // Для SI5 (устаревший формат)
            uint32_t ret = (static_cast<uint8_t>(number[2]) << 8) | 
                          static_cast<uint8_t>(number[3]);
            if (static_cast<uint8_t>(number[1]) >= 2) {
                ret += static_cast<uint8_t>(number[1]) * 100000;
            }
            return ret;
        }
    }
    
    // Определение типа карты по номеру
    static CardType detectCardType(uint32_t cardNumber) {
        if (cardNumber >= 1000000 && cardNumber <= 1999999) {
            return CardType::SI9;
        } else if (cardNumber >= 2000000 && cardNumber <= 2999999) {
            return CardType::SI8;
        } else if (cardNumber >= 7000000 && cardNumber <= 7999999) {
            return CardType::SI10;
        } else if (cardNumber >= 8000000 && cardNumber <= 8999999) {
            return CardType::SIAC;
        } else if (cardNumber >= 9000000 && cardNumber <= 9999999) {
            return CardType::SI11;
        } else if (cardNumber >= 4000000 && cardNumber <= 4999999) {
            return CardType::SIpCard;
        } else if (cardNumber >= 6000000 && cardNumber <= 6999999) {
            return CardType::SItCard;
        } else if (cardNumber >= 500000 && cardNumber <= 999999) {
            return CardType::SI6;
        } else {
            return CardType::SIAC;//SI5;
        }
    }
    
    // Декодирование времени
    static QDateTime decodeTime(const QByteArray& timeData, 
                                const QByteArray& dayByte = QByteArray(),
                                const QDateTime& reference = QDateTime::currentDateTime(),
                                const QByteArray& subsecondByte = QByteArray())
    {
        
        if (timeData.size() < 2) {
            throw SIException("Invalid time data length");
        }
        
        // Проверка на сброшенное время
        if ((static_cast<uchar>(timeData[0]) == 0xEE)
            && (static_cast<uchar>(timeData[1]) == 0xEE)) {
            return QDateTime();
        }
        
        // 12-часовое время в секундах
        uint32_t seconds = static_cast<uint8_t>(timeData[0]) << 8 |
                          static_cast<uint8_t>(timeData[1]);
        
        QDateTime baseTime = reference;
        int milliseconds = 0;
        
        if (!dayByte.isEmpty()) {
            uint8_t ptd = static_cast<uint8_t>(dayByte[0]);
            
            // AM/PM (бит 0)
            bool isPM = (ptd & 0x01) != 0;
            if (isPM) {
                seconds += 12 * 3600; // Добавляем 12 часов
            }
            
            // День недели (биты 3-1)
            int dayOfWeek = (ptd >> 1) & 0x07;
            if (dayOfWeek == 0) dayOfWeek = 7; // Воскресенье = 7
            
            // Корректировка дня недели
            int currentDayOfWeek = baseTime.date().dayOfWeek();
            int daysToSubtract = (currentDayOfWeek - dayOfWeek) % 7;
            if (daysToSubtract < 0) daysToSubtract += 7;
            
            baseTime = baseTime.addDays(-daysToSubtract);
            
            // Доли секунды из кода контроля (для старта/финиша)
            if (!subsecondByte.isEmpty()) {
                uint8_t msByte = static_cast<uint8_t>(subsecondByte[0]);
                milliseconds = static_cast<int>(msByte * 1000.0 / 256.0);
                baseTime = baseTime.addMSecs(milliseconds);
            }
        } else {
            // Без информации о дне - используем эвристику
            uint32_t currentSeconds = baseTime.time().hour() * 3600 +
                                baseTime.time().minute() * 60 + 
                                baseTime.time().second();
            
            if (seconds < currentSeconds) {
                // Время сегодня
            } else {
                // Время было вчера
                baseTime = baseTime.addDays(-1);
            }
        }
        
        // Создаем время
        QDate date = baseTime.date();
        int hour = seconds / 3600;
        seconds %= 3600;
        int minute = seconds / 60;
        int second = seconds % 60;
        
        return QDateTime(date, QTime(hour, minute, second).addMSecs(milliseconds));
    }
    
    // Декодирование данных карты
    static CardData decodeCardData(const QByteArray& rawData, CardType cardType,
                                  const QDateTime& reference = QDateTime::currentDateTime()) {
        
        CardData cardData;
        
        if (rawData.isEmpty()) {
            throw SIException("Empty card data");
        }
        
        // Определяем структуру в зависимости от типа карты
        Constants::CardOffsets::CardStructure offsets;
        if (cardType == CardType::SI9) {
            offsets = Constants::CardOffsets::SI9;
            cardData.cardType = CardType::SI9;
        } else if (cardType == CardType::SI10 || cardType == CardType::SI11) {
            offsets = Constants::CardOffsets::SI10;
            cardData.cardType = CardType::SI10;
        } else if (cardType == CardType::SIAC) {
            offsets = Constants::CardOffsets::SI10;
            cardData.cardType = CardType::SIAC;
        } else {
            throw SIException("Unsupported card type");
        }
        
        // Декодирование номера карты
        QByteArray cardNumberBytes;
        cardNumberBytes.append('\x00'); // Дополнительный байт для формата
        cardNumberBytes.append(rawData.at(offsets.cn2));
        cardNumberBytes.append(rawData.at(offsets.cn1));
        cardNumberBytes.append(rawData.at(offsets.cn0));
        
        cardData.cardNumber = decodeCardNumber(cardNumberBytes);
        
        // Декодирование времени старта
        if (offsets.std >= 0 && offsets.st >= 0) {
            QByteArray dayByte(1, rawData.at(offsets.std));
            QByteArray subsecondByte = (cardType == CardType::SIAC)
                                           ? QByteArray(1, rawData.at(offsets.std + 1))
                                           : QByteArray(1, 0);
            QByteArray timeBytes = rawData.mid(offsets.st, 2);
            cardData.startTime = decodeTime(timeBytes, dayByte, reference, subsecondByte);
        }
        
        // Декодирование времени финиша
        if (offsets.ftd >= 0 && offsets.ft >= 0) {
            QByteArray dayByte(1, rawData.at(offsets.ftd));
            QByteArray subsecondByte= (cardType == CardType::SIAC)
                                           ? QByteArray(1, rawData.at(offsets.ftd + 1))
                                           : QByteArray(1, 0);
            QByteArray timeBytes = rawData.mid(offsets.ft, 2);
            cardData.finishTime = decodeTime(timeBytes, dayByte, reference, subsecondByte);
        }
        
        // Декодирование времени проверки
        if (offsets.ctd >= 0 && offsets.ct >= 0) {
            QByteArray dayByte(1, rawData.at(offsets.ctd));
            QByteArray timeBytes = rawData.mid(offsets.ct, 2);
            cardData.checkTime = decodeTime(timeBytes, dayByte, reference);
        }
        
        // Чтение количества отметок
        cardData.punchCount = static_cast<uint8_t>(rawData.at(offsets.rc));
        
        // Декодирование отметок
        int punchStart = ((cardType == CardType::SI10)||(cardType == CardType::SIAC)) ? offsets.p1_si10 : offsets.p1;
        int punchesToRead = qMin(cardData.punchCount, static_cast<uint8_t>(offsets.pm));
        
        for (int i = 0; i < punchesToRead; i++) {
            int recordPos = punchStart + i * offsets.pl;
            
            if (recordPos + offsets.pl > rawData.size()) {
                break; // Недостаточно данных
            }
            
            Punch punch;
            
            // Код контрольного пункта
            punch.controlCode = static_cast<uint8_t>(rawData.at(recordPos + offsets.cn));
            
            // Время отметки
            QByteArray dayByte(1, rawData.at(recordPos + offsets.pt));
            QByteArray timeBytes = rawData.mid(recordPos + offsets.pth, 2);
            
            punch.timestamp = decodeTime(timeBytes, dayByte, reference);
            punch.subsecond = 0;
            //qDebug() << punch.timestamp;
            
            cardData.punches.append(punch);
        }
        
        return cardData;
    }
};

} // namespace SportIdent

#endif // SPORTIDENT_DECODER_H
