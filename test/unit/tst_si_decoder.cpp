#include <QTest>
#include <QDebug>
#include "serial/sportident_decoder.h"

//! [0]
class TestSIDecoder: public QObject
{
    Q_OBJECT

private slots:
    // Базовые тесты
    void testDecode_num1();
    void testDecode_num2();
    void testDecode_num3();

    void testDecode_time1();
    void testDecode_time2();

    void testDecode_CardData1();
};
//! [0]

// SIAC 8026200 (0x7A 78 58)
// жёлтый чип 7205330 (0x6DF1D2)
// красный чип 7010165 (0x6AF775)

//! [1]
// decodeCardNumber
void TestSIDecoder::testDecode_num1()
{
    QByteArray data = QByteArray::fromHex("007A7858");
    quint32 num = SportIdent::DataDecoder::decodeCardNumber(data);
    QVERIFY(num == 8026200);
}

void TestSIDecoder::testDecode_num2()
{
    QByteArray data = QByteArray::fromHex("006DF1D2");
    quint32 num = SportIdent::DataDecoder::decodeCardNumber(data);
    QCOMPARE(num, quint32(0x006DF1D2));
}

void TestSIDecoder::testDecode_num3()
{
    QByteArray data = QByteArray::fromHex("006DF1D2");
    quint32 num = SportIdent::DataDecoder::decodeCardNumber(data);
    QCOMPARE(num, quint32(0x006DF1D2));
}

// decodeTime
void TestSIDecoder::testDecode_time1()
{
    QByteArray ptd = QByteArray::fromHex("8B");
    QByteArray data = QByteArray::fromHex("6977"); // 0x7769=19:29:59
    QDateTime qdt = SportIdent::DataDecoder::decodeTime(data,ptd);
    QCOMPARE(qdt.time(), QTime(19,29,59));
}

void TestSIDecoder::testDecode_time2()
{
    QByteArray ptd = QByteArray::fromHex("0B");
    QByteArray data = QByteArray::fromHex("6412"); // 0B 03 64 12 - Start 0x6412=19:06:58
    QDateTime qdt = SportIdent::DataDecoder::decodeTime(data,ptd);
    QCOMPARE(qdt.time(), QTime(19,06,58));
}

// decodeCardData

void TestSIDecoder::testDecode_CardData1()
{
    QByteArray rawdata = QByteArray::fromHex("AC44BA9BEAEAEAEA0B0464060B0364128B8077691233132E0F7A78580215AE22383032363230303B22536F7672656D6E6E79652074656B686E6F6C6F6769692073706F7274612249494C3B3B3B3B3B3B672E204D6F736B61753B756C2E20416B6164656D696B612042616B756C6576612C20642E382C206B762E35343B313137");
    rawdata.append(QByteArray::fromHex("0B2265D90B2265DC0B2265DE0B2866F30B29698C0B2A6CC10B236E3B0B2B6EF50B2D70DC0B2C71850B2E720F0B2172730B26730E0B2573A30B2774380B2375510B2476020B2976790B64774AEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"));
    SportIdent::CardData cd = SportIdent::DataDecoder::decodeCardData(rawdata,SportIdent::CardType::SIAC);
    QCOMPARE(cd.cardType , SportIdent::CardType::SIAC);
    QCOMPARE(cd.cardNumber , uint32_t(8026200));
    QCOMPARE(cd.punchCount , uint8_t(19));
    QCOMPARE(cd.punches.count() , qsizetype(19));
    QCOMPARE(cd.checkTime.time() , QTime(19,06,46));
    QCOMPARE(cd.startTime.time() , QTime(19,06,58).addMSecs(11));
    QCOMPARE(cd.finishTime.time() , QTime(20,29,29).addMSecs(500));
    //qDebug() << cd.startTime.toString("dd.MM.yyyy HH:mm:ss.zzz");
    //qDebug() << cd.finishTime.toString("dd.MM.yyyy HH:mm:ss.zzz");
}

//! [1]

//! [2]
QTEST_MAIN(TestSIDecoder)
#include "tst_si_decoder.moc"
//! [2]

