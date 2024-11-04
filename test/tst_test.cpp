#include <QTest>
#include "qsportevent.h"

//! [0]
class TestSportevent: public QObject
{
    Q_OBJECT

private slots:
    void create();
};
//! [0]

//! [1]
void TestSportevent::create()
{
    QSportEvent a;
    QCOMPARE(a.getNameEvent(), "");
}
//! [1]

//! [2]
QTEST_MAIN(TestSportevent)
#include "tst_test.moc"
//! [2]

