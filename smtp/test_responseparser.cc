#include "response.h"
#include <assert.h>

static const QCString singleLineResponseCRLF = "250 OK\r\n";
static const QCString singleLineResponse     = "250 OK";

static const QCString multiLineResponse[] = {
  "250-ktown.kde.org\r\n",
  "250-STARTTLS\r\n",
  "250-AUTH PLAIN DIGEST-MD5\r\n",
  "250 PIPELINING\r\n"
};
static const unsigned int numMultiLineLines = sizeof multiLineResponse / sizeof *multiLineResponse ;

int main ( int, char** ) {

  KioSMTP::Response r;
  assert( r.isValid() );
  assert( r.lines().empty() );
  assert( r.isWellFormed() );
  assert( r.code() == 0 );
  assert( r.isUnknown() );
  assert( !r.isComplete() );
  assert( !r.isOk() );
  r.parseLine( singleLineResponseCRLF.data(),
	       singleLineResponseCRLF.length() );
  assert( r.isWellFormed() );
  assert( r.isComplete() );
  assert( r.isValid() );
  assert( r.isPositive() );
  assert( r.isOk() );
  assert( r.code() == 250 );
  assert( r.first() == 2 );
  assert( r.second() == 5 );
  assert( r.third() == 0 );
  assert( r.lines().count() == 1 );
  assert( r.lines().front() == "OK" );
  r.parseLine( singleLineResponse.data(),
	       singleLineResponse.length() );
  assert( !r.isValid() );
  r.clear();
  assert( r.isValid() );
  assert( r.lines().empty() );

  r.parseLine( singleLineResponse.data(),
	       singleLineResponse.length() );
  assert( r.isWellFormed() );
  assert( r.isComplete() );
  assert( r.isValid() );
  assert( r.isPositive() );
  assert( r.isOk() );
  assert( r.code() == 250 );
  assert( r.first() == 2 );
  assert( r.second() == 5 );
  assert( r.third() == 0 );
  assert( r.lines().count() == 1 );
  assert( r.lines().front() == "OK" );
  r.parseLine( singleLineResponse.data(),
	       singleLineResponse.length() );
  assert( !r.isValid() );
  r.clear();
  assert( r.isValid() );

  for ( unsigned int i = 0 ; i < numMultiLineLines ; ++i ) {
    r.parseLine( multiLineResponse[i].data(),
		 multiLineResponse[i].length() );
    assert( r.isWellFormed() );
    if ( i < numMultiLineLines-1 )
      assert( !r.isComplete() );
    else
      assert( r.isComplete() );
    assert( r.isValid() );
    assert( r.isPositive() );
    assert( r.code() == 250 );
    assert( r.first() == 2 );
    assert( r.second() == 5 );
    assert( r.third() == 0 );
    assert( r.lines().count() == i + 1 );
  }
  assert( r.lines().back() == "PIPELINING" );
  return 0;
}

#include "response.cc"
