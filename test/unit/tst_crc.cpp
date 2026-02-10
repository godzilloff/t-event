#include <QTest>
#include <QDebug>
#include "serial/sportident_crc.h"

//! [0]
class TestCRC: public QObject
{
    Q_OBJECT

private slots:
    // Базовые тесты
    void testCalculate_empty();
    void testCalculate_singleByte();
    void testCalculate_twoBytes();
    void testCalculate_VeryLong();

    // Тестовые данные из спецификации
    void testKnownValues();

    // Тесты verify
    void testVerify();
    // void testVerify_withCrc();

    // Пограничные случаи
    void testEdgeCases();

    // Производительность (опционально)
    void benchmarkCalculate();
};
//! [0]

//! [1]
void TestCRC::testCalculate_empty()
{
    QByteArray data;
    quint16 crc = SportIdent::CRC16::calculate(data);
    QCOMPARE(crc, quint16(0));
}

void TestCRC::testCalculate_singleByte()
{
    QByteArray data = QByteArray::fromHex("FF");
    quint16 crc = SportIdent::CRC16::calculate(data);
    // Проверяем, что не падает и возвращает осмысленное значение
    QVERIFY(crc != 0xFFFF); // не должно быть ошибки
}

void TestCRC::testCalculate_twoBytes()
{
    QByteArray data = QByteArray::fromHex("1234");
    quint16 crc = SportIdent::CRC16::calculate(data);
    QCOMPARE(crc, quint16(0xECBB)); // Это пример, подставьте реальное значение
}

void TestCRC::testCalculate_VeryLong()
{
    QByteArray data = QByteArray::fromHex("EF83000F000DB3DD9AEAEAEAEA0B045A2B0B035A3F0B016A540797082C0F6DF1D2040FFF4B373230353333303B4B6F73696C6F7620496C79613B4D3B31393838303631363B4D545543493B676F647A696C6C6F6666406D61696C2E72753B2B37393035353135343838303B4D6F733F6F773B4D6F733F6F773B3134313032313B5275737369");
    quint16 crc = SportIdent::CRC16::calculate(data);
    QCOMPARE(crc, quint16(0x997F)); // Это пример, подставьте реальное значение
}

void TestCRC::testKnownValues()
{
    // Тестируем ваш пример
    QByteArray data = QByteArray::fromHex("83027101");
    quint16 crc = SportIdent::CRC16::calculate(data);
    QCOMPARE(crc, quint16(0x1A14));

    // Проверяем байтовое представление
    QByteArray crcBytes = SportIdent::CRC16::calculateBytes(data);
    QCOMPARE(crcBytes.toHex().toUpper(), QByteArray("1A14"));

    // Можно добавить больше известных значений
    // Например, из документации SportIdent
    // QByteArray test2 = QByteArray::fromHex("01020304");
    // quint16 crc2 = SportIdent::CRC16::calculate(test2);
    // Замените на ожидаемое значение
    // QCOMPARE(crc2, quint16(0xXXXX));
}

void TestCRC::testVerify()
{
    QByteArray data = QByteArray::fromHex("E806000F0F6AF775");
    QByteArray correctCrc = QByteArray::fromHex("7EA6");
    QByteArray wrongCrc = QByteArray::fromHex("1234");

    QVERIFY(SportIdent::CRC16::verify(data, correctCrc));
    QVERIFY(!SportIdent::CRC16::verify(data, wrongCrc));

    // Тест с quint16
    QVERIFY(SportIdent::CRC16::verify(data, quint16(0x7EA6)));
    QVERIFY(!SportIdent::CRC16::verify(data, quint16(0x1234)));
}

void TestCRC::testEdgeCases()
{
    // Длинные данные
    QByteArray longData(1024, '\xAA');
    quint16 crc = SportIdent::CRC16::calculate(longData);
    QVERIFY(crc != 0); // Не должно быть нулем для ненулевых данных

    // Данные с нечетной длиной
    // QByteArray oddData = QByteArray::fromHex("010203");
    // quint16 crc2 = SportIdent::CRC16::calculate(oddData);
    // Проверяем, что функция не падает
    // QVERIFY(true);

    // Проверяем инволютивность: CRC от данных с правильным CRC должно давать 0
    // (если CRC работает как checksum)
    // QByteArray data = QByteArray::fromHex("12345678");
    // QByteArray crcBytes = SportIdent::CRC16::calculateBytes(data);
    // QByteArray dataWithCrc = data + crcBytes;
    // QCOMPARE(SportIdent::CRC16::calculate(dataWithCrc), quint16(0));
}

void TestCRC::benchmarkCalculate()
{
    QByteArray data(1024, '\xAA');

    QBENCHMARK {
        SportIdent::CRC16::calculate(data);
    }
}

//! [1]

//! [2]
QTEST_MAIN(TestCRC)
#include "tst_crc.moc"
//! [2]

