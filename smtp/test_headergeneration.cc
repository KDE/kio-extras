#include "request.h"

//#include <iostream>

//using std::cout;
//using std::endl;

int main( int , char ** ) {
  static QCString expected =
    "From: mutz@kde.org\r\n"
    "Subject: missing subject\r\n"
    "To: joe@user.org,\r\n"
    "\tvalentine@14th.february.org\r\n"
    "Cc: boss@example.com\r\n"
    "\n"
    "From: Marc Mutz <mutz@kde.org>\r\n"
    "Subject: missing subject\r\n"
    "To: joe@user.org,\r\n"
    "\tvalentine@14th.february.org\r\n"
    "Cc: boss@example.com\r\n"
    "\n"
    "From: \"Mutz, Marc\" <mutz@kde.org>\r\n"
    "Subject: missing subject\r\n"
    "To: joe@user.org,\r\n"
    "\tvalentine@14th.february.org\r\n"
    "Cc: boss@example.com\r\n"
    "\n"
    "From: =?utf-8?b?TWFyYyBNw7Z0eg==?= <mutz@kde.org>\r\n"
    "Subject: missing subject\r\n"
    "To: joe@user.org,\r\n"
    "\tvalentine@14th.february.org\r\n"
    "Cc: boss@example.com\r\n"
    "\n"
    "From: mutz@kde.org\r\n"
    "Subject: =?utf-8?b?QmzDtmRlcyBTdWJqZWN0?=\r\n"
    "To: joe@user.org,\r\n"
    "\tvalentine@14th.february.org\r\n"
    "Cc: boss@example.com\r\n"
    "\n"
    "From: Marc Mutz <mutz@kde.org>\r\n"
    "Subject: =?utf-8?b?QmzDtmRlcyBTdWJqZWN0?=\r\n"
    "To: joe@user.org,\r\n"
    "\tvalentine@14th.february.org\r\n"
    "Cc: boss@example.com\r\n"
    "\n"
    "From: \"Mutz, Marc\" <mutz@kde.org>\r\n"
    "Subject: =?utf-8?b?QmzDtmRlcyBTdWJqZWN0?=\r\n"
    "To: joe@user.org,\r\n"
    "\tvalentine@14th.february.org\r\n"
    "Cc: boss@example.com\r\n"
    "\n"
    "From: =?utf-8?b?TWFyYyBNw7Z0eg==?= <mutz@kde.org>\r\n"
    "Subject: =?utf-8?b?QmzDtmRlcyBTdWJqZWN0?=\r\n"
    "To: joe@user.org,\r\n"
    "\tvalentine@14th.february.org\r\n"
    "Cc: boss@example.com\r\n"
    "\n";

  KioSMTP::Request request;
  QCString result;

  request.setEmitHeaders( true );
  request.setFromAddress( "mutz@kde.org" );
  request.addTo( "joe@user.org" );
  request.addTo( "valentine@14th.february.org" );
  request.addCc( "boss@example.com" );

  result += request.headerFields() + '\n';
  result += request.headerFields( "Marc Mutz" ) + '\n';
  result += request.headerFields( "Mutz, Marc" ) + '\n';
  result += request.headerFields( "Marc Mötz" ) + '\n';

  request.setSubject( "Blödes Subject" );

  result += request.headerFields() + '\n';
  result += request.headerFields( "Marc Mutz" ) + '\n';
  result += request.headerFields( "Mutz, Marc" ) + '\n';
  result += request.headerFields( "Marc Mötz" ) + '\n';

  //cout << "Result:\n" << result.data() << endl;

  return result == expected ? 0 : 1 ;
}

#include "request.cc"

