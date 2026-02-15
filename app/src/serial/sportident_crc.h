// sportident_crc.h

#ifndef SPORTIDENT_CRC_H
#define SPORTIDENT_CRC_H

#include <QByteArray>

    namespace SportIdent {

    class CRC16 {
    private:
        static quint16 toInt(const QByteArray& data, int offset) {
            if (offset + 1 < data.size()) {
                return static_cast<quint16>(
                    (static_cast<quint8>(data[offset]) << 8) |
                    static_cast<quint8>(data[offset + 1])
                    );
            } else {
                // Если только один байт остался
                return static_cast<quint16>(static_cast<quint8>(data[offset]) << 8);
            }
        }

    public:

        // Основная функция CRC
        static quint16 calculate(const QByteArray& data) {
            const quint16 CRC_POLYNOM = 0x8005;
            const quint16 CRC_BITF = 0x8000;

            if (data.size() < 1) {
                return 0;
            }

            // Инициализация CRC первыми двумя байтами
            quint16 crc = toInt(data.left(2),0);

            // Подготовка данных: копируем остаток строки начиная с индекса 2
            QByteArray rest = data.mid(2);

            // Добавляем padding до чётной длины + 2 нулевых байта
            if (rest.size() % 2 == 0) {
                rest.append(2,'\0'); // + 00 + 00
            } else {
                rest.append(1,'\0'); // + 00
            }

            // Обработка по 2 байта
            for (int i = 0; i < rest.size(); i += 2) {
                QByteArray chunk = rest.mid(i, 2);
                // Убедимся, что chunk имеет ровно 2 байта (в конце может быть только 1)
                if (chunk.size() < 2) {
                    chunk.resize(2);
                    chunk[1] = '\x00';
                }
                quint16 val = toInt(chunk,0);

                for (int j = 0; j < 16; ++j) {
                    bool crcMsb = (crc & CRC_BITF) != 0;
                    bool valMsb = (val & CRC_BITF) != 0;

                    crc <<= 1;
                    if (valMsb) {
                        crc |= 1; // rotate carry
                    }

                    if (crcMsb) {
                        crc ^= CRC_POLYNOM;
                    }

                    val <<= 1;
                }
            }

            return crc & 0xFFFF;
        }

        static QByteArray calculateBytes(const QByteArray& data) {
            quint16 crc = calculate(data);
            // Возвращаем в порядке big-endian
            QByteArray result(2, '\0');
            result[0] = static_cast<char>((crc >> 8) & 0xFF);
            result[1] = static_cast<char>(crc & 0xFF);
            return result;
        }

        static bool verify(const QByteArray& data, const QByteArray& crc) {
            if (crc.size() != 2) return false;

            // Предполагаем, что CRC передано в порядке big-endian
            quint16 received = static_cast<quint16>(
                (static_cast<quint8>(crc[0]) << 8) |
                static_cast<quint8>(crc[1])
                );

            return calculate(data) == received;
        }

        static bool verify(const QByteArray& data, quint16 crc) {
            return calculate(data) == crc;
        }
    };

} // namespace SportIdent

#endif // SPORTIDENT_CRC_H
