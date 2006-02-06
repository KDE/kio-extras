#include <kio/global.h>
#include <kdebug.h>

#include <qstring.h>
#include <qcstring.h>
#include <qstringlist.h>

//#include <iostream>
//using std::cout;
//using std::endl;

namespace KioSMTP {
  class Response;
};

// fake
class SMTPProtocol {
public:
  SMTPProtocol() { clear(); }

  //
  // public members to control the API emulation below:
  //
  int startTLSReturnCode;
  bool usesSSL;
  bool usesTLS;
  int lastErrorCode;
  QString lastErrorMessage;
  int lastMessageBoxCode;
  QString lastMessageBoxText;
  QByteArray nextData;
  int nextDataReturnCode;
  QStringList caps;
  KIO::MetaData metadata;

  void clear() {
    startTLSReturnCode = 1;
    usesSSL = usesTLS = false;
    lastErrorCode = lastMessageBoxCode = 0;
    lastErrorMessage = lastMessageBoxText.clear();
    nextData.resize( 0 );
    nextDataReturnCode = -1;
    caps.clear();
    metadata.clear();
  }

  //
  // emulated API:
  //
  void parseFeatures( const KioSMTP::Response & ) { /* noop */ }
  int startTLS() {
    if ( startTLSReturnCode == 1 )
      usesTLS = true;
    return startTLSReturnCode;
  }
  bool usingSSL() const { return usesSSL; }
  bool usingTLS() const { return usesTLS; }
  bool haveCapability( const char * cap ) const { return caps.contains( cap ); }
  void error( int id, const QString & msg ) {
    lastErrorCode = id;
    lastErrorMessage = msg;
  }
  void messageBox( int id, const QString & msg, const QString & ) {
    lastMessageBoxCode = id;
    lastMessageBoxText = msg;
  }
  void dataReq() { /* noop */ }
  int readData( QByteArray & ba ) { ba = nextData; return nextDataReturnCode; }
  QString metaData( const QString & key ) const { return metadata[key]; }

};

#define _SMTP_H

#define KIOSMTP_COMPARATORS // for TransactionState::operator==
#include "command.h"
#include "response.h"
#include "transactionstate.h"

#include <assert.h>

using namespace KioSMTP;

static const char * foobarbaz = ".Foo bar baz";
static const unsigned int foobarbaz_len = qstrlen( foobarbaz );

static const char * foobarbaz_dotstuffed = "..Foo bar baz";
static const unsigned int foobarbaz_dotstuffed_len = qstrlen( foobarbaz_dotstuffed );

static const char * foobarbaz_lf = ".Foo bar baz\n";
static const unsigned int foobarbaz_lf_len = qstrlen( foobarbaz_lf );

static const char * foobarbaz_crlf = "..Foo bar baz\r\n";
static const unsigned int foobarbaz_crlf_len = qstrlen( foobarbaz_crlf );

static void checkSuccessfulTransferCommand( bool, bool, bool, bool, bool );

int main( int, char** ) {

  SMTPProtocol smtp;
  Response r;
  TransactionState ts, ts2;

  //
  // EHLO / HELO
  //

  smtp.clear();
  EHLOCommand ehlo( &smtp, "mail.example.com" );
  // flags
  assert( ehlo.closeConnectionOnError() );
  assert( ehlo.mustBeLastInPipeline() );
  assert( !ehlo.mustBeFirstInPipeline() );

  // initial state
  assert( !ehlo.isComplete() );
  assert( !ehlo.doNotExecute( 0 ) );
  assert( !ehlo.needsResponse() );

  // dynamics 1: EHLO succeeds
  assert( ehlo.nextCommandLine( 0 ) == "EHLO mail.example.com\r\n" );
  assert( !ehlo.isComplete() ); // EHLO may fail and we then try HELO
  assert( ehlo.needsResponse() );
  r.clear();
  r.parseLine( "250-mail.example.net\r\n" );
  r.parseLine( "250-PIPELINING\r\n" );
  r.parseLine( "250 8BITMIME\r\n" );
  assert( ehlo.processResponse( r, 0 ) == true );
  assert( ehlo.isComplete() );
  assert( !ehlo.needsResponse() );
  assert( smtp.lastErrorCode == 0 );
  assert( smtp.lastErrorMessage.isNull() );

  // dynamics 2: EHLO fails with "unknown command"
  smtp.clear();
  EHLOCommand ehlo2( &smtp, "mail.example.com" );
  ehlo2.nextCommandLine( 0 );
  r.clear();
  r.parseLine( "500 unknown command\r\n" );
  assert( ehlo2.processResponse( r, 0 ) == true );
  assert( !ehlo2.isComplete() );
  assert( !ehlo2.needsResponse() );
  assert( ehlo2.nextCommandLine( 0 ) == "HELO mail.example.com\r\n" );
  assert( ehlo2.isComplete() );
  assert( ehlo2.needsResponse() );
  r.clear();
  r.parseLine( "250 mail.example.net\r\n" );
  assert( ehlo2.processResponse( r, 0 ) == true );
  assert( !ehlo2.needsResponse() );
  assert( smtp.lastErrorCode == 0 );
  assert( smtp.lastErrorMessage.isNull() );

  // dynamics 3: EHLO fails with unknown response code
  smtp.clear();
  EHLOCommand ehlo3( &smtp, "mail.example.com" );
  ehlo3.nextCommandLine( 0 );
  r.clear();
  r.parseLine( "545 you don't know me\r\n" );
  assert( ehlo3.processResponse( r, 0 ) == false );
  assert( ehlo3.isComplete() );
  assert( !ehlo3.needsResponse() );
  assert( smtp.lastErrorCode == KIO::ERR_UNKNOWN );

  // dynamics 4: EHLO _and_ HELO fail with "command unknown"
  smtp.clear();
  EHLOCommand ehlo4( &smtp, "mail.example.com" );
  ehlo4.nextCommandLine( 0 );
  r.clear();
  r.parseLine( "500 unknown command\r\n" );
  ehlo4.processResponse( r, 0 );
  ehlo4.nextCommandLine( 0 );
  r.clear();
  r.parseLine( "500 unknown command\r\n" );
  assert( ehlo4.processResponse( r, 0 ) == false );
  assert( ehlo4.isComplete() );
  assert( !ehlo4.needsResponse() );
  assert( smtp.lastErrorCode == KIO::ERR_INTERNAL_SERVER );
  
  //
  // STARTTLS
  //

  smtp.clear();
  StartTLSCommand tls( &smtp );
  // flags
  assert( tls.closeConnectionOnError() );
  assert( tls.mustBeLastInPipeline() );
  assert( !tls.mustBeFirstInPipeline() );

  // initial state
  assert( !tls.isComplete() );
  assert( !tls.doNotExecute( 0 ) );
  assert( !tls.needsResponse() );

  // dynamics 1: ok from server, TLS negotiation successful
  ts.clear();
  ts2 = ts;
  assert( tls.nextCommandLine( &ts ) == "STARTTLS\r\n" );
  assert( ts == ts2 );
  assert( tls.isComplete() );
  assert( tls.needsResponse() );
  r.clear();
  r.parseLine( "220 Go ahead" );
  smtp.startTLSReturnCode = 1;
  assert( tls.processResponse( r, &ts ) == true );
  assert( !tls.needsResponse() );
  assert( smtp.lastErrorCode == 0 );
  
  // dynamics 2: NAK from server
  smtp.clear();
  StartTLSCommand tls2( &smtp );
  ts.clear();
  tls2.nextCommandLine( &ts );
  r.clear();
  r.parseLine( "454 TLS temporarily disabled" );
  smtp.startTLSReturnCode = 1;
  assert( tls2.processResponse( r, &ts ) == false );
  assert( !tls2.needsResponse() );
  assert( smtp.lastErrorCode == KIO::ERR_SERVICE_NOT_AVAILABLE );
  
  // dynamics 3: ok from server, TLS negotiation unsuccessful
  smtp.clear();
  StartTLSCommand tls3( &smtp );
  ts.clear();
  tls3.nextCommandLine( &ts );
  r.clear();
  r.parseLine( "220 Go ahead" );
  smtp.startTLSReturnCode = -1;
  assert( tls.processResponse( r, &ts ) == false );
  assert( !tls.needsResponse() );
  
  //
  // AUTH
  //

  smtp.clear();
  QStrIList mechs;
  mechs.append( "PLAIN" );
  smtp.metadata["sasl"] = "PLAIN";
  AuthCommand auth( &smtp, mechs, "user", "pass" );
  // flags
  assert( auth.closeConnectionOnError() );
  assert( auth.mustBeLastInPipeline() );
  assert( !auth.mustBeFirstInPipeline() );

  // initial state
  assert( !auth.isComplete() );
  assert( !auth.doNotExecute( 0 ) );
  assert( !auth.needsResponse() );

  // dynamics 1: TLS, so AUTH should include initial-response:
  smtp.usesTLS = true;
  ts.clear();
  ts2 = ts;
  assert( auth.nextCommandLine( &ts ) == "AUTH PLAIN dXNlcgB1c2VyAHBhc3M=\r\n" );
  assert( auth.isComplete() );
  assert( auth.needsResponse() );
  assert( ts == ts2 );
  r.clear();
  r.parseLine( "250 OK" );

  // dynamics 2: No TLS, so AUTH should not include initial-response:
  smtp.clear();
  smtp.metadata["sasl"] = "PLAIN";
  smtp.usesTLS = false;
  AuthCommand auth2( &smtp, mechs, "user", "pass" );
  ts.clear();
  assert( auth2.nextCommandLine( &ts ) == "AUTH PLAIN\r\n" );
  assert( !auth2.isComplete() );
  assert( auth2.needsResponse() );
  r.clear();
  r.parseLine( "334 Go on" );
  assert( auth2.processResponse( r, &ts ) == true );
  assert( auth2.nextCommandLine( &ts ) == "dXNlcgB1c2VyAHBhc3M=\r\n" );
  assert( auth2.isComplete() );
  assert( auth2.needsResponse() );
  
  // dynamics 3: LOGIN
  smtp.clear();
  smtp.metadata["sasl"] = "LOGIN";
  mechs.clear();
  mechs.append( "LOGIN" );
  AuthCommand auth3( &smtp, mechs, "user", "pass" );
  ts.clear();
  ts2 = ts;
  assert( auth3.nextCommandLine( &ts ) == "AUTH LOGIN\r\n" );
  assert( !auth3.isComplete() );
  assert( auth3.needsResponse() );
  r.clear();
  r.parseLine( "334 VXNlcm5hbWU6" );
  assert( auth3.processResponse( r, &ts ) == true );
  assert( !auth3.needsResponse() );
  assert( auth3.nextCommandLine( &ts ) == "dXNlcg==\r\n" );
  assert( !auth3.isComplete() );
  assert( auth3.needsResponse() );
  r.clear();
  r.parseLine( "334 go on" );
  assert( auth3.processResponse( r, &ts ) == true );
  assert( !auth3.needsResponse() );
  assert( auth3.nextCommandLine( &ts ) == "cGFzcw==\r\n" );
  assert( auth3.isComplete() );
  assert( auth3.needsResponse() );
  r.clear();
  r.parseLine( "250 OK" );
  assert( auth3.processResponse( r, &ts ) == true );
  assert( !auth3.needsResponse() );
  assert( !smtp.lastErrorCode );
  assert( ts == ts2 );

  //
  // MAIL FROM:
  //

  smtp.clear();
  MailFromCommand mail( &smtp, "joe@user.org" );
  // flags
  assert( !mail.closeConnectionOnError() );
  assert( !mail.mustBeLastInPipeline() );
  assert( !mail.mustBeFirstInPipeline() );

  // initial state
  assert( !mail.isComplete() );
  assert( !mail.doNotExecute( 0 ) );
  assert( !mail.needsResponse() );

  // dynamics: success, no size, no 8bit
  ts.clear();
  ts2 = ts;
  assert( mail.nextCommandLine( &ts ) == "MAIL FROM:<joe@user.org>\r\n" );
  assert( ts2 == ts );
  assert( mail.isComplete() );
  assert( mail.needsResponse() );
  r.clear();
  r.parseLine( "250 Ok" );
  assert( mail.processResponse( r, &ts ) == true );
  assert( !mail.needsResponse() );
  assert( ts == ts2 );
  assert( smtp.lastErrorCode == 0 );

  // dynamics: success, size, 8bit, but no SIZE, 8BITMIME caps
  smtp.clear();
  MailFromCommand mail2( &smtp, "joe@user.org", true, 500 );
  ts.clear();
  ts2 = ts;
  assert( mail2.nextCommandLine( &ts ) == "MAIL FROM:<joe@user.org>\r\n" );
  assert( ts == ts2 );

  // dynamics: success, size, 8bit, SIZE, 8BITMIME caps
  smtp.clear();
  MailFromCommand mail3( &smtp, "joe@user.org", true, 500 );
  ts.clear();
  ts2 = ts;
  smtp.caps << "SIZE" << "8BITMIME" ;
  assert( mail3.nextCommandLine( &ts ) == "MAIL FROM:<joe@user.org> BODY=8BITMIME SIZE=500\r\n" );
  assert( ts == ts2 );

  // dynamics: failure
  smtp.clear();
  MailFromCommand mail4( &smtp, "joe@user.org" );
  ts.clear();
  mail4.nextCommandLine( &ts );
  r.clear();
  r.parseLine( "503 Bad sequence of commands" );
  assert( mail4.processResponse( r, &ts ) == false );
  assert( mail4.isComplete() );
  assert( !mail4.needsResponse() );
  assert( ts.failed() );
  assert( !ts.failedFatally() );
  assert( smtp.lastErrorCode == 0 );

  //
  // RCPT TO:
  //

  smtp.clear();
  RcptToCommand rcpt( &smtp, "joe@user.org" );
  // flags
  assert( !rcpt.closeConnectionOnError() );
  assert( !rcpt.mustBeLastInPipeline() );
  assert( !rcpt.mustBeFirstInPipeline() );

  // initial state
  assert( !rcpt.isComplete() );
  assert( !rcpt.doNotExecute( 0 ) );
  assert( !rcpt.needsResponse() );

  // dynamics: success
  ts.clear();
  ts2 = ts;
  assert( rcpt.nextCommandLine( &ts ) == "RCPT TO:<joe@user.org>\r\n" );
  assert( ts == ts2 );
  assert( rcpt.isComplete() );
  assert( rcpt.needsResponse() );
  r.clear();
  r.parseLine( "250 Ok" );
  assert( rcpt.processResponse( r, &ts ) == true );
  assert( !rcpt.needsResponse() );
  assert( ts.atLeastOneRecipientWasAccepted() );
  assert( !ts.haveRejectedRecipients() );
  assert( !ts.failed() );
  assert( !ts.failedFatally() );
  assert( smtp.lastErrorCode == 0 );

  // dynamics: failure
  smtp.clear();
  RcptToCommand rcpt2( &smtp, "joe@user.org" );
  ts.clear();
  rcpt2.nextCommandLine( &ts );
  r.clear();
  r.parseLine( "530 5.7.1 Relaying not allowed!" );
  assert( rcpt2.processResponse( r, &ts ) == false );
  assert( rcpt2.isComplete() );
  assert( !rcpt2.needsResponse() );
  assert( !ts.atLeastOneRecipientWasAccepted() );
  assert( ts.haveRejectedRecipients() );
  assert( ts.rejectedRecipients().count() == 1 );
  assert( ts.rejectedRecipients().front().recipient == "joe@user.org" );
  assert( ts.failed() );
  assert( !ts.failedFatally() );
  assert( smtp.lastErrorCode == 0 );

  // dynamics: success and failure combined
  smtp.clear();
  RcptToCommand rcpt3( &smtp, "info@example.com" );
  RcptToCommand rcpt4( &smtp, "halloween@microsoft.com" );
  RcptToCommand rcpt5( &smtp, "joe@user.org" );
  ts.clear();
  rcpt3.nextCommandLine( &ts );
  r.clear();
  r.parseLine( "530 5.7.1 Relaying not allowed!" );
  rcpt3.processResponse( r, &ts );
  
  rcpt4.nextCommandLine( &ts );
  r.clear();
  r.parseLine( "250 Ok" );
  rcpt4.processResponse( r, &ts );

  rcpt5.nextCommandLine( &ts );
  r.clear();
  r.parseLine( "250 Ok" );
  assert( ts.failed() );
  assert( !ts.failedFatally() );
  assert( ts.haveRejectedRecipients() );
  assert( ts.atLeastOneRecipientWasAccepted() );
  assert( smtp.lastErrorCode == 0 );

  //
  // DATA (init)
  //

  smtp.clear();
  DataCommand data( &smtp );
  // flags
  assert( !data.closeConnectionOnError() );
  assert( data.mustBeLastInPipeline() );
  assert( !data.mustBeFirstInPipeline() );

  // initial state
  assert( !data.isComplete() );
  assert( !data.doNotExecute( 0 ) );
  assert( !data.needsResponse() );

  // dynamics: success
  ts.clear();
  assert( data.nextCommandLine( &ts ) == "DATA\r\n" );
  assert( data.isComplete() );
  assert( data.needsResponse() );
  assert( ts.dataCommandIssued() );
  assert( !ts.dataCommandSucceeded() );
  r.clear();
  r.parseLine( "354 Send data, end in <CR><LF>.<CR><LF>" );
  assert( data.processResponse( r, &ts ) == true );
  assert( !data.needsResponse() );
  assert( ts.dataCommandSucceeded() );
  assert( ts.dataResponse() == r );
  assert( smtp.lastErrorCode == 0 );

  // dynamics: failure
  smtp.clear();
  DataCommand data2( &smtp );
  ts.clear();
  data2.nextCommandLine( &ts );
  r.clear();
  r.parseLine( "551 No valid recipients" );
  assert( data2.processResponse( r, &ts ) == false );
  assert( !data2.needsResponse() );
  assert( !ts.dataCommandSucceeded() );
  assert( ts.dataResponse() == r );
  assert( smtp.lastErrorCode == 0 );

  //
  // DATA (transfer)
  //

  TransferCommand xfer( &smtp, 0 );
  // flags
  assert( !xfer.closeConnectionOnError() );
  assert( !xfer.mustBeLastInPipeline() );
  assert( xfer.mustBeFirstInPipeline() );

  // initial state
  assert( !xfer.isComplete() );
  assert( !xfer.needsResponse() );

  // dynamics 1: DATA command failed
  ts.clear();
  r.clear();
  r.parseLine( "551 no valid recipients" );
  ts.setDataCommandIssued( true );
  ts.setDataCommandSucceeded( false, r );
  assert( xfer.doNotExecute( &ts ) );

  // dynamics 2: some recipients rejected, but not all
  smtp.clear();
  TransferCommand xfer2( &smtp, 0 );
  ts.clear();
  ts.setRecipientAccepted();
  ts.addRejectedRecipient( "joe@user.org", "No relaying allowed" );
  ts.setDataCommandIssued( true );
  r.clear();
  r.parseLine( "354 go on" );
  ts.setDataCommandSucceeded( true, r );
  // ### will change with allow-partial-delivery option:
  assert( xfer.doNotExecute( &ts ) );

  // successful dynamics with all combinations of:
  enum {
    EndInLF = 1,
    PerformDotStuff = 2,
    UngetLast = 4,
    Preloading = 8,
    Error = 16,
    EndOfOptions = 32
  };
  for ( unsigned int i = 0 ; i < EndOfOptions ; ++i )
    checkSuccessfulTransferCommand( i & Error, i & Preloading, i & UngetLast,
				    i & PerformDotStuff, i & EndInLF );

  //
  // NOOP
  //

  smtp.clear();
  NoopCommand noop( &smtp );
  // flags
  assert( !noop.closeConnectionOnError() );
  assert( noop.mustBeLastInPipeline() );
  assert( !noop.mustBeFirstInPipeline() );

  // initial state
  assert( !noop.isComplete() );
  assert( !noop.doNotExecute( &ts ) );
  assert( !noop.needsResponse() );

  // dynamics: success (failure is tested with RSET)
  assert( noop.nextCommandLine( 0 ) == "NOOP\r\n" );
  assert( noop.isComplete() );
  assert( noop.needsResponse() );
  r.clear();
  r.parseLine( "250 Ok" );
  assert( noop.processResponse( r, 0 ) == true );
  assert( noop.isComplete() );
  assert( !noop.needsResponse() );
  assert( smtp.lastErrorCode == 0 );
  assert( smtp.lastErrorMessage.isNull() );

  //
  // RSET
  //

  smtp.clear();
  RsetCommand rset( &smtp );
  // flags
  assert( rset.closeConnectionOnError() );
  assert( !rset.mustBeLastInPipeline() );
  assert( !rset.mustBeFirstInPipeline() );

  // initial state
  assert( !rset.isComplete() );
  assert( !rset.doNotExecute( &ts ) );
  assert( !rset.needsResponse() );

  // dynamics: failure (success is tested with NOOP/QUIT)
  assert( rset.nextCommandLine( 0 ) == "RSET\r\n" );
  assert( rset.isComplete() );
  assert( rset.needsResponse() );
  r.clear();
  r.parseLine( "502 command not implemented" );
  assert( rset.processResponse( r, 0 ) == false );
  assert( rset.isComplete() );
  assert( !rset.needsResponse() );
  assert( smtp.lastErrorCode == 0 ); // an RSET failure isn't worth it, is it?
  assert( smtp.lastErrorMessage.isNull() );

  //
  // QUIT
  //

  smtp.clear();
  QuitCommand quit( &smtp );
  // flags
  assert( quit.closeConnectionOnError() );
  assert( quit.mustBeLastInPipeline() );
  assert( !quit.mustBeFirstInPipeline() );

  // initial state
  assert( !quit.isComplete() );
  assert( !quit.doNotExecute( 0 ) );
  assert( !quit.needsResponse() );

  // dynamics 1: success
  assert( quit.nextCommandLine( 0 ) == "QUIT\r\n" );
  assert( quit.isComplete() );
  assert( quit.needsResponse() );
  r.clear();
  r.parseLine( "221 Goodbye" );
  assert( quit.processResponse( r, 0 ) == true );
  assert( quit.isComplete() );
  assert( !quit.needsResponse() );
  assert( smtp.lastErrorCode == 0 );
  assert( smtp.lastErrorMessage.isNull() );

  // dynamics 2: success
  smtp.clear();
  QuitCommand quit2( &smtp );
  quit2.nextCommandLine( 0 );
  r.clear();
  r.parseLine( "500 unknown command" );
  assert( quit2.processResponse( r, 0 ) == false );
  assert( quit2.isComplete() );
  assert( !quit2.needsResponse() );
  assert( smtp.lastErrorCode == 0 ); // an QUIT failure isn't worth it, is it?
  assert( smtp.lastErrorMessage.isNull() );

  return 0;
}

void checkSuccessfulTransferCommand( bool error, bool preload, bool ungetLast,
				     bool slaveDotStuff, bool mailEndsInNewline ) {
  kDebug() << "   ===== checkTransferCommand( "
	    << error << ", "
	    << preload << ", "
	    << ungetLast << ", "
	    << slaveDotStuff << ", "
	    << mailEndsInNewline << " ) =====" << endl;

  SMTPProtocol smtp;
  if ( slaveDotStuff )
    smtp.metadata["lf2crlf+dotstuff"] = "slave";

  Response r;

  const char * s_pre = slaveDotStuff ?
                         mailEndsInNewline ? foobarbaz_lf : foobarbaz
                       :
                         mailEndsInNewline ? foobarbaz_crlf : foobarbaz_dotstuffed ;
  const unsigned int s_pre_len = qstrlen( s_pre );

  const char * s_post = mailEndsInNewline ? foobarbaz_crlf : foobarbaz_dotstuffed ;
  //const unsigned int s_post_len = qstrlen( s_post );

  TransferCommand xfer( &smtp, preload ? s_post : 0 );

  TransactionState ts;
  ts.setRecipientAccepted();
  ts.setDataCommandIssued( true );
  r.clear();
  r.parseLine( "354 ok" );
  ts.setDataCommandSucceeded( true, r );
  assert( !xfer.doNotExecute( &ts ) );
  if ( preload ) {
    assert( xfer.nextCommandLine( &ts ) == s_post );
    assert( !xfer.isComplete() );
    assert( !xfer.needsResponse() );
    assert( !ts.failed() );
    assert( smtp.lastErrorCode == 0 );
  }
  smtp.nextData.duplicate( s_pre, s_pre_len );
  smtp.nextDataReturnCode = s_pre_len;
  assert( xfer.nextCommandLine( &ts ) == s_post );
  assert( !xfer.isComplete() );
  assert( !xfer.needsResponse() );
  assert( !ts.failed() );
  assert( smtp.lastErrorCode == 0 );
  smtp.nextData.resize( 0 );
  smtp.nextDataReturnCode = 0;
  if ( ungetLast ) {
    xfer.ungetCommandLine( xfer.nextCommandLine( &ts ), &ts );
    assert( !xfer.isComplete() );
    assert( !xfer.needsResponse() );
    assert( !ts.complete() );
    smtp.nextDataReturnCode = -1; // double read -> error
  }
  if ( mailEndsInNewline )
    assert( xfer.nextCommandLine( &ts ) == ".\r\n" );
  else
    assert( xfer.nextCommandLine( &ts ) == "\r\n.\r\n" );
  assert( xfer.isComplete() );
  assert( xfer.needsResponse() );
  assert( !ts.complete() );
  assert( !ts.failed() );
  assert( smtp.lastErrorCode == 0 );
  r.clear();
  if ( error ) {
    r.parseLine( "552 Exceeded storage allocation" );
    assert( xfer.processResponse( r, &ts ) == false );
    assert( !xfer.needsResponse() );
    assert( ts.complete() );
    assert( ts.failed() );
    assert( smtp.lastErrorCode == KIO::ERR_DISK_FULL );
  } else {
    r.parseLine( "250 Message accepted" );
    assert( xfer.processResponse( r, &ts ) == true );
    assert( !xfer.needsResponse() );
    assert( ts.complete() );
    assert( !ts.failed() );
    assert( smtp.lastErrorCode == 0 );
  }
};

#define NDEBUG

#include "command.cc"
#include "response.cc"
#include "transactionstate.cc"
