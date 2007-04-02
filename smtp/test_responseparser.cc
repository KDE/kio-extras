#include "test_responseparser.h"
#include "response.h"

#include <qtest_kde.h>
#include <assert.h>

QTEST_KDEMAIN_CORE( ResponseParserTest )

static const QByteArray singleLineResponseCRLF = "250 OK\r\n";
static const QByteArray singleLineResponse     = "250 OK";

static const QByteArray multiLineResponse[] = {
  "250-ktown.kde.org\r\n",
  "250-STARTTLS\r\n",
  "250-AUTH PLAIN DIGEST-MD5\r\n",
  "250 PIPELINING\r\n"
};
static const unsigned int numMultiLineLines = sizeof multiLineResponse / sizeof *multiLineResponse ;

void ResponseParserTest::testResponseParser()
{
  KioSMTP::Response r;
  QVERIFY( r.isValid() );
  QVERIFY( r.lines().empty() );
  QVERIFY( r.isWellFormed() );
  QCOMPARE( r.code(), 0u );
  QVERIFY( r.isUnknown() );
  QVERIFY( !r.isComplete() );
  QVERIFY( !r.isOk() );
  r.parseLine( singleLineResponseCRLF.data(), singleLineResponseCRLF.length() );
  QVERIFY( r.isWellFormed() );
  QVERIFY( r.isComplete() );
  QVERIFY( r.isValid() );
  QVERIFY( r.isPositive() );
  QVERIFY( r.isOk() );
  QCOMPARE( r.code(), 250u );
  QCOMPARE( r.errorCode(), 0 );
  QCOMPARE( r.first(), 2u );
  QCOMPARE( r.second(), 5u );
  QCOMPARE( r.third(), 0u );
  QCOMPARE( r.lines().count(), 1 );
  QCOMPARE( r.lines().front(), QByteArray("OK") );
  r.parseLine( singleLineResponse.data(), singleLineResponse.length() );
  QVERIFY( !r.isValid() );
  r.clear();
  QVERIFY( r.isValid() );
  QVERIFY( r.lines().empty() );

  r.parseLine( singleLineResponse.data(), singleLineResponse.length() );
  QVERIFY( r.isWellFormed() );
  QVERIFY( r.isComplete() );
  QVERIFY( r.isValid() );
  QVERIFY( r.isPositive() );
  QVERIFY( r.isOk() );
  QCOMPARE( r.code(), 250u );
  QCOMPARE( r.first(), 2u );
  QCOMPARE( r.second(), 5u );
  QCOMPARE( r.third(), 0u );
  QCOMPARE( r.lines().count(), 1 );
  QCOMPARE( r.lines().front(), QByteArray("OK") );
  r.parseLine( singleLineResponse.data(), singleLineResponse.length() );
  QVERIFY( !r.isValid() );
  r.clear();
  QVERIFY( r.isValid() );

  for ( unsigned int i = 0 ; i < numMultiLineLines ; ++i ) {
    r.parseLine( multiLineResponse[i].data(), multiLineResponse[i].length() );
    QVERIFY( r.isWellFormed() );
    if ( i < numMultiLineLines-1 )
      QVERIFY( !r.isComplete() );
    else
      QVERIFY( r.isComplete() );
    QVERIFY( r.isValid() );
    QVERIFY( r.isPositive() );
    QCOMPARE( r.code(), 250u );
    QCOMPARE( r.first(), 2u );
    QCOMPARE( r.second(), 5u );
    QCOMPARE( r.third(), 0u );
    QCOMPARE( r.lines().count(), (int)i + 1 );
  }
  QCOMPARE( r.lines().back(), QByteArray("PIPELINING") );

  r.clear();
  r.parseLine( "230", 3 );
  QVERIFY( r.isValid() );
  QVERIFY( r.isWellFormed() ); // even though it isn't ;-)
  QCOMPARE( r.code(), 230u );
  QCOMPARE( r.lines().count(), 1 );
  QVERIFY( r.lines().front().isNull() );

  r.clear();
  r.parseLine( "230\r\n", 5 );
  QVERIFY( r.isValid() );
  QVERIFY( r.isWellFormed() ); // even though it isn't ;-)
  QCOMPARE( r.code(), 230u );
  QCOMPARE( r.lines().count(), 1 );
  QVERIFY( r.lines().front().isNull() );

  r.clear();
  r.parseLine( " 23 ok", 6 );
  QVERIFY( !r.isValid() );
  QVERIFY( !r.isWellFormed() );
}

#include "test_responseparser.moc"
#include "response.cc"
