/*
   RFC 2060 (IMAP4rev1) client.

   Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/ 

#include <unistd.h> // For usleep
#include "IMAPClient.h"

//#define IMAP_DEBUG
#undef  IMAP_DEBUG

#ifdef IMAP_DEBUG
#  include <qtextstream.h>
#  include <qfile.h>
#endif

// TODO: authenticate, parse FETCH retval to make it easier to play with.

// IMAP4rev1 (RFC2060) client implementation

#ifdef IMAP_DEBUG
static QFile logfile;

void log(const QString & s)
{
  if (!logfile.isOpen()) {
    logfile.setName("log");
    if (!logfile.open(IO_WriteOnly))
      abort();
  }

  QTextStream t(&logfile);
  t << s;
  logfile.flush();
}
#endif

class IMAPClientPrivate
{
  public:

    QString greeting_;
    QIODevice * device_;
    IMAPClient::State state_;
    ulong commandCount_;
};

IMAPClient::IMAPClient(QIODevice * dev)
{
  d = new IMAPClientPrivate;
  d->device_ = dev;
  d->state_ = NotAuthenticated;
  d->commandCount_ = 0;
  d->greeting_ = response(QString::null)[0];
}

IMAPClient::~IMAPClient()
{
  logout();
  Response::cleanup();
  delete d;
}

  QString
IMAPClient::greeting() const
{
  return d->greeting_;
}

  QString
IMAPClient::capability()
{
  if (!d->device_->isOpen()) {
    qDebug("IMAPClient::capability(): Not connected to server");
    return QString();
  }

  Response r = runCommand("CAPABILITY");

  return r.data()[0];
}

  bool
IMAPClient::noop()
{
  if (!d->device_->isOpen()) {
    qDebug("IMAPClient::noop(): Not connected to server");
    return false;
  }

  Response r = runCommand("NOOP");

  return (r.statusCode() == Response::StatusCodeOk);
}

  bool
IMAPClient::logout()
{
  if (!d->device_->isOpen()) {
    qDebug("IMAPClient::logout(): Not connected to server");
    return false;
  }

  Response r = runCommand("LOGOUT");

  bool ok = (r.statusCode() == Response::StatusCodeOk);

  if (!ok)
    return false;

  d->state_ = Logout;

  return true;
}

  bool
IMAPClient::authenticate(const QString & /* username */, const QString & /* password */, const QString & /* authType */)
{
  qDebug("%s: STUB", __FUNCTION__);

  if (d->state_ < NotAuthenticated) {
    qDebug("IMAPClient::authenticate(): state < NotAuthenticated");
    return false;
  }

  return false;
}

  bool
IMAPClient::login(const QString & username, const QString & password)
{
  if (d->state_ < NotAuthenticated) {
    qDebug("IMAPClient::login(): state < NotAuthenticated");
    return false;
  }

  Response r = runCommand("LOGIN " + username + " " + password);

  bool ok = (r.statusCode() == Response::StatusCodeOk);

  if (ok)
    d->state_ = Authenticated;

  return ok;
}

  bool
IMAPClient::selectMailbox(const QString & name, MailboxInfo & info)
{
  if (d->state_ < Authenticated) {
    qDebug("IMAPClient::selectMailbox(): state < Authenticated");
    return false;
  }

  Response r = runCommand("SELECT " + name);

  bool ok = (r.statusCode() == Response::StatusCodeOk);

  if (!ok)
    return false;

  info = MailboxInfo(r.allData());

  d->state_ = Selected;

  return true;
}

  bool
IMAPClient::examineMailbox(const QString & name, MailboxInfo & info)
{
  if (d->state_ < Authenticated) {
    qDebug("IMAPClient::examineMailbox(): state < Authenticated");
    return false;
  }

  // Is this really just the same thing ?
  return selectMailbox(name, info);
}

  bool
IMAPClient::createMailbox(const QString & name)
{
  if (d->state_ < Authenticated) {
    qDebug("IMAPClient::createMailbox(): state < Authenticated");
    return false;
  }

  Response r = runCommand("CREATE " + name);

  return (r.statusCode() == Response::StatusCodeOk);
}

  bool
IMAPClient::removeMailbox(const QString & name)
{
  if (d->state_ < Authenticated) {
    qDebug("IMAPClient::removeMailbox(): state < Authenticated");
    return false;
  }

  Response r = runCommand("DELETE " + name);

  return (r.statusCode() == Response::StatusCodeOk);
}

  bool
IMAPClient::renameMailbox(const QString & from, const QString & to)
{
  if (d->state_ < Authenticated) {
    qDebug("IMAPClient::renameMailbox(): state < Authenticated");
    return false;
  }

  Response r = runCommand("RENAME " + from + " " + to);

  return (r.statusCode() == Response::StatusCodeOk);
}

  bool
IMAPClient::subscribeMailbox(const QString & name)
{
  if (d->state_ < Authenticated) {
    qDebug("IMAPClient::subscribeMailbox(): state < Authenticated");
    return false;
  }

  Response r = runCommand("SUBSCRIBE " + name);

  return (r.statusCode() == Response::StatusCodeOk);
}

  bool
IMAPClient::unsubscribeMailbox(const QString & name)
{
  if (d->state_ < Authenticated) {
    qDebug("IMAPClient::unsubscribeMailbox(): state < Authenticated");
    return false;
  }

  Response r = runCommand("UNSUBSCRIBE " + name);

  return (r.statusCode() == Response::StatusCodeOk);
}

  bool
IMAPClient::list(
    const QString & ref,
    const QString & wild,
    QValueList<ListResponse> & responseList,
    bool subscribedOnly
)
{
  if (d->state_ < Authenticated) {
    qDebug("IMAPClient::list(): state < Authenticated");
    return false;
  }

  QString cmd = subscribedOnly ? "LSUB" : "LIST";

  Response r = runCommand(cmd + " \"" + ref + "\" " + wild);

  bool ok = (r.statusCode() == Response::StatusCodeOk);

  if (!ok)
    return false;

  responseList.clear();

  QStringList l(r.data());

  QStringList::ConstIterator it(l.begin());

  for (; it != l.end(); ++it)
    responseList.append(ListResponse(*it));

  return true;
}

  bool
IMAPClient::status(
    const QString & mailboxName,
    ulong items,
    StatusInfo & retval
)
{
  if (d->state_ < Authenticated) {
    qDebug("IMAPClient::status(): state < Authenticated");
    return false;
  }

  QString s;

  if (items & StatusInfo::MessageCount)
    s += "MESSAGES ";
  if (items & StatusInfo::RecentCount)
    s += "RECENT ";
  if (items & StatusInfo::NextUID)
    s += "UIDNEXT ";
  if (items & StatusInfo::UIDValidity)
    s += "UIDVALIDITY ";
  if (items & StatusInfo::Unseen)
    s += "UNSEEN ";

  if (s[s.length() - 1] == ' ')
    s.truncate(s.length() - 1);

  Response r = runCommand("STATUS " + mailboxName + " (" + s + ')');

  bool ok = (r.statusCode() == Response::StatusCodeOk);

  if (!ok)
    return false;

  QStringList l(r.data());

  QString resp(l.last());

  retval = StatusInfo(resp);

  return true;
}

  bool
IMAPClient::appendMessage(
    const QString & mailboxName,
    const QString & messageData,
    ulong           flags,
    const QString & date
)
{
  if (d->state_ < Authenticated) {
    qDebug("IMAPClient::appendMessage(): state < Authenticated");
    return false;
  }

  QString s("APPEND " + mailboxName);

  if (0 != flags)
    s += " (" + flagsString(flags) + ")";

  if ("" != date)
    s += " " + date;

  s += "\r\n" + messageData;

  Response r = runCommand(s);

  return (r.statusCode() == Response::StatusCodeOk);
}

  bool
IMAPClient::checkpoint()
{
  if (d->state_ != Selected) {
    qDebug("IMAPClient::checkpoint(): state != Selected");
    return false;
  }

  Response r = runCommand("CHECKPOINT");

  return (r.statusCode() == Response::StatusCodeOk);
}

  bool
IMAPClient::close()
{
  if (d->state_ != Selected) {
    qDebug("IMAPClient::close(): state != Selected");
    return false;
  }

  Response r = runCommand("CLOSE");

  return (r.statusCode() == Response::StatusCodeOk);
}

  bool
IMAPClient::expunge(QValueList<ulong> & ret)
{
  if (d->state_ != Selected) {
    qDebug("IMAPClient::expunge(): state != Selected");
    return false;
  }

  Response r = runCommand("EXPUNGE");

  bool ok = (r.statusCode() == Response::StatusCodeOk);

  if (!ok)
    return false;

  ret.clear();

  QStringList l(r.data());

  QStringList::ConstIterator it(l.begin());

  for (; it != l.end(); ++it) {

    QStringList tokens(QStringList::split(' ', *it));

    if (tokens[0] == "*" && tokens[2] == "EXPUNGE")
      ret << tokens[1].toULong();
  }

  return true;
}

    QValueList<ulong>
IMAPClient::search(
    const QString & spec,
    const QString & charSet,
    bool            usingUID
)
{
  QValueList<ulong> retval;

  if (d->state_ != Selected) {
    qDebug("IMAPClient::search(): state != Selected");
    return retval;
  }

  QString s("SEARCH " + charSet + " " + spec);

  if (usingUID)
    s.prepend("UID ");

  Response r = runCommand(s);

  bool ok = (r.statusCode() == Response::StatusCodeOk);

  if (!ok)
    return retval;

  QString resp(r.data().first());

  QStringList l(QStringList::split(' ', resp));

  if (l.count() < 3 || l[0] != "*" || l[1] != "SEARCH")
    return retval;

  l.remove(l.begin());
  l.remove(l.begin());

  QStringList::ConstIterator it(l.begin());

  for (; it != l.end(); ++it)
    retval << (*it).toULong();

  return retval;
}

  QStringList
IMAPClient::fetch(
    ulong           start,
    ulong           end,
    const QString & spec,
    bool            usingUID
)
{
  QStringList retval;

  if (d->state_ != Selected) {
    qDebug("IMAPClient::fetch(): state != Selected");
    return retval;
  }

  QString s(
      "FETCH " + QString::number(start) + ':' + QString::number(end) +
      ' ' + spec
  );

  if (usingUID)
    s.prepend("UID ");

  Response r = runCommand(s);

  bool ok = (r.statusCode() == Response::StatusCodeOk);

  if (!ok)
    return retval;

  retval = r.data();

  return retval;
}

  bool
IMAPClient::setFlags(
  ulong         start,
  ulong         end,
  FlagSetStyle  style,
  ulong         flags,
  bool          usingUID
)
{
  if (d->state_ != Selected) {
    qDebug("IMAPClient::setFlags(): state != Selected");
    return false;
  }

  QString styleString("FLAGS.SILENT");

  switch (style) {

    case Add:
      styleString.prepend('+');
      break;

    case Remove:
      styleString.prepend('-');
      break;

    case Set:
    default:
      break;
  }

  QString s(
      "STORE " +
      QString::number(start) + ":" + QString::number(end) +
      ' ' + styleString + " (" + flagsString(flags) + ')'
  );

  if (usingUID)
    s.prepend("UID ");

  Response r = runCommand(s);

  return (r.statusCode() == Response::StatusCodeOk);
}

  bool
IMAPClient::copy(ulong start, ulong end, const QString & to, bool usingUID)
{
  if (d->state_ != Selected) {
    qDebug("IMAPClient::copy(): state != Selected");
    return false;
  }

  QString s(
      "COPY " +
      QString::number(start) + ":" + QString::number(end) +
      " " + to
  );

  if (usingUID)
    s.prepend("UID ");

  Response r = runCommand(s);

  return (r.statusCode() == Response::StatusCodeOk);
}

  IMAPClient::Response
IMAPClient::runCommand(const QString & cmd)
{
  if (!d->device_->isOpen()) {
    qDebug("IMAPClient::runCommand(): Socket is not connected");
    return Response("");
  }

  QString id;
  id.sprintf("EMPATH_%08ld", d->commandCount_++);

  QString command(id + " " + cmd + "\r\n");
  d->device_->writeBlock(command.ascii(), command.length());

#ifdef IMAP_DEBUG
  log("> " + command);
#endif

  return Response(response(id));
}

  QStringList
IMAPClient::response(const QString & endIndicator)
{
  QStringList output;
  QString line;

  int max(1024);
  QByteArray buf(max);

  while (true) 
  {
    // Stupidly slow reading, but QIODevice::readLine() seems to be
    // broken when I use a QSocketDevice. It just blocks...
    
    char c('\0');

    int pos(0);

    while (c != '\n' && pos < max - 8)
    {
      c = d->device_->getch();

      if (c != '\r' && c != '\n')
        buf[pos++] = c;
    }

    line = QString::fromUtf8(buf.data(), pos);

#ifdef IMAP_DEBUG
    log("< " + line + "\n");
#endif

    output << line;

    if (line.left(endIndicator.length()) == endIndicator &&
        !!endIndicator)
      break;

    if (!endIndicator && !output.isEmpty())
      break;

    usleep(100);
  }

  return output;
}

  QString
IMAPClient::flagsString(ulong flags) const
{
  QString s;

  if (flags & Seen)
    s += "\\Seen ";
  if (flags & Answered)
    s += "\\Answered ";
  if (flags & Flagged)
    s += "\\Flagged ";
  if (flags & Deleted)
    s += "\\Deleted ";
  if (flags & Draft)
    s += "\\Draft ";
  if (flags & Recent)
    s += "\\Recent ";

  if (s[s.length() - 1] == ' ')
    s.truncate(s.length() - 1);

  return s;
}

QAsciiDict<ulong> * IMAPClient::Response::statusCodeDict_ = 0L;

IMAPClient::Response::Response(const QStringList & data)
  : allData_(data),
    responseType_(ResponseTypeUnknown),
    statusCode_(StatusCodeUnknown)
{
  QString lastLine = allData_.last();

  QStringList tokens(QStringList::split(' ', lastLine));

  QString token0 = tokens[0];

  if (token0 == "*")
    responseType_ = ResponseTypeStatus;
  else if (token0 == "+")
    responseType_ = ResponseTypeContinuationRequest;
  else
    responseType_ = ResponseTypeServerData;

  statusCode_ = _statusCode(tokens[1]);

  data_ = allData_;

  if (data_.count() > 1)
    data_.remove(data_.fromLast());
}

  void
IMAPClient::Response::cleanup()
{
  delete statusCodeDict_;
  statusCodeDict_ = 0;
}

  IMAPClient::Response::ResponseType
IMAPClient::Response::type() const
{
  return responseType_;
}

  IMAPClient::Response::StatusCode
IMAPClient::Response::statusCode() const
{
  return statusCode_;
}

  QStringList
IMAPClient::Response::data() const
{
  return data_;
}

  QStringList
IMAPClient::Response::allData() const
{
  return allData_;
}

  IMAPClient::Response::StatusCode
IMAPClient::Response::_statusCode(const QString & key)
{
  if (0 == statusCodeDict_) {

    statusCodeDict_ = new QAsciiDict<ulong>(23);

    statusCodeDict_->insert("ALERT",          new ulong(StatusCodeAlert));
    statusCodeDict_->insert("NEWNAME",        new ulong(StatusCodeNewName));
    statusCodeDict_->insert("PARSE",          new ulong(StatusCodeParse));
    statusCodeDict_->insert("PERMANENTFLAGS", new ulong(StatusCodePermanentFlags));
    statusCodeDict_->insert("READ-ONLY",      new ulong(StatusCodeReadOnly));
    statusCodeDict_->insert("READ-WRITE",     new ulong(StatusCodeReadWrite));
    statusCodeDict_->insert("TRYCREATE",      new ulong(StatusCodeTryCreate));
    statusCodeDict_->insert("UIDVALIDITY",    new ulong(StatusCodeUIDValidity));
    statusCodeDict_->insert("UNSEEN",         new ulong(StatusCodeUnseen));
    statusCodeDict_->insert("OK",             new ulong(StatusCodeOk));
    statusCodeDict_->insert("NO",             new ulong(StatusCodeNo));
    statusCodeDict_->insert("BAD",            new ulong(StatusCodeBad));
    statusCodeDict_->insert("PREAUTH",        new ulong(StatusCodePreAuth));
    statusCodeDict_->insert("CAPABILITY",     new ulong(StatusCodeCapability));
    statusCodeDict_->insert("LIST",           new ulong(StatusCodeList));
    statusCodeDict_->insert("LSUB",           new ulong(StatusCodeLsub));
    statusCodeDict_->insert("STATUS",         new ulong(StatusCodeStatus));
    statusCodeDict_->insert("SEARCH",         new ulong(StatusCodeSearch));
    statusCodeDict_->insert("FLAGS",          new ulong(StatusCodeFlags));
    statusCodeDict_->insert("EXISTS",         new ulong(StatusCodeExists));
    statusCodeDict_->insert("RECENT",         new ulong(StatusCodeRecent));
    statusCodeDict_->insert("EXPUNGE",        new ulong(StatusCodeExpunge));
    statusCodeDict_->insert("FETCH",          new ulong(StatusCodeFetch));
  }

  if (!key)
    return StatusCodeUnknown;

  ulong * l = statusCodeDict_->find(key.ascii());

  if (0 == l)
    return StatusCodeUnknown;
  else
    return StatusCode(*l);
}

IMAPClient::MailboxInfo::MailboxInfo()
  : count_(0),
    recent_(0),
    unseen_(0),
    uidValidity_(0),
    flags_(0),
    permanentFlags_(0),
    readWrite_(false),
    countAvailable_(false),
    recentAvailable_(false),
    unseenAvailable_(false),
    uidValidityAvailable_(false),
    flagsAvailable_(false),
    permanentFlagsAvailable_(false),
    readWriteAvailable_(false)
{
}

IMAPClient::MailboxInfo::MailboxInfo(const IMAPClient::MailboxInfo & mi)
  : count_(mi.count_),
    recent_(mi.recent_),
    unseen_(mi.unseen_),
    uidValidity_(mi.uidValidity_),
    flags_(mi.flags_),
    permanentFlags_(mi.permanentFlags_),
    readWrite_(mi.readWrite_),
    countAvailable_(mi.countAvailable_),
    recentAvailable_(mi.recentAvailable_),
    unseenAvailable_(mi.unseenAvailable_),
    uidValidityAvailable_(mi.uidValidityAvailable_),
    flagsAvailable_(mi.flagsAvailable_),
    permanentFlagsAvailable_(mi.permanentFlagsAvailable_),
    readWriteAvailable_(mi.readWriteAvailable_)
{
}

  IMAPClient::MailboxInfo &
IMAPClient::MailboxInfo::operator = (const IMAPClient::MailboxInfo & mi)
{
  // Avoid a = a.
  if (this == &mi)
    return *this;

  count_ = mi.count_;
  recent_ = mi.recent_;
  unseen_ = mi.unseen_;
  uidValidity_ = mi.uidValidity_;
  flags_ = mi.flags_;
  permanentFlags_ = mi.permanentFlags_;
  readWrite_ = mi.readWrite_;
  countAvailable_ = mi.countAvailable_;
  recentAvailable_ = mi.recentAvailable_;
  unseenAvailable_ = mi.unseenAvailable_;
  uidValidityAvailable_ = mi.uidValidityAvailable_;
  flagsAvailable_ = mi.flagsAvailable_;
  permanentFlagsAvailable_ = mi.permanentFlagsAvailable_;
  readWriteAvailable_ = mi.readWriteAvailable_;

  return *this;
}

IMAPClient::MailboxInfo::MailboxInfo(const QStringList & list)
  : count_(0),
    recent_(0),
    unseen_(0),
    uidValidity_(0),
    flags_(0),
    permanentFlags_(0),
    readWrite_(false),
    countAvailable_(false),
    recentAvailable_(false),
    unseenAvailable_(false),
    uidValidityAvailable_(false),
    flagsAvailable_(false),
    permanentFlagsAvailable_(false),
    readWriteAvailable_(false)
{
  for (QStringList::ConstIterator it(list.begin()); it != list.fromLast(); ++it)
  {
    QString line(*it);

    QStringList tokens(QStringList::split(' ', line));

    if (tokens[0] != "*")
      continue;

    if (tokens[2] == "EXISTS")
      setCount(tokens[1].toULong());

    else if (tokens[2] == "RECENT")
      setRecent(tokens[1].toULong());

    else if (tokens[1] == "FLAGS")
    {
      int flagsStart  = line.find('(');
      int flagsEnd    = line.find(')');

      if ((-1 != flagsStart) && (-1 != flagsEnd) && flagsStart < flagsEnd)
        setFlags(_flags(line.mid(flagsStart, flagsEnd)));
    }

    else if (tokens[2] == "[UNSEEN")
      setUnseen(tokens[3].left(tokens[3].length() - 2).toULong());

    else if (tokens[2] == "[UIDVALIDITY")
      setUidValidity(tokens[3].left(tokens[3].length() - 2).toULong());

    else if (tokens[2] == "[PERMANENTFLAGS")
    {
      int flagsStart  = line.find('(');
      int flagsEnd    = line.find(')');

      if ((-1 != flagsStart) && (-1 != flagsEnd) && flagsStart < flagsEnd)
        setPermanentFlags(_flags(line.mid(flagsStart, flagsEnd)));
    }
  }

  setReadWrite(-1 != list.last().find("READ-WRITE"));
}

  ulong
IMAPClient::MailboxInfo::_flags(const QString & flagsString) const
{
  ulong flags = 0;

  if (0 != flagsString.contains("\\Seen"))
    flags ^= Seen;
  if (0 != flagsString.contains("\\Answered"))
    flags ^= Answered;
  if (0 != flagsString.contains("\\Flagged"))
    flags ^= Flagged;
  if (0 != flagsString.contains("\\Deleted"))
    flags ^= Deleted;
  if (0 != flagsString.contains("\\Draft"))
    flags ^= Draft;
  if (0 != flagsString.contains("\\Recent"))
    flags ^= Recent;

  return flags;
}


IMAPClient::ListResponse::ListResponse()
  : noInferiors_(false),
    noSelect_(false),
    marked_(false),
    unmarked_(false)
{
}

IMAPClient::ListResponse::ListResponse(const IMAPClient::ListResponse & lr)
  : hierarchyDelimiter_(lr.hierarchyDelimiter_),
    name_(lr.name_),
    noInferiors_(lr.noInferiors_),
    noSelect_(lr.noSelect_),
    marked_(lr.marked_),
    unmarked_(lr.unmarked_)
{
}

  IMAPClient::ListResponse &
IMAPClient::ListResponse::operator = (const IMAPClient::ListResponse & lr)
{
  // Avoid a = a.
  if (this == &lr)
    return *this;

  hierarchyDelimiter_ = lr.hierarchyDelimiter_;
  name_ = lr.name_;
  noInferiors_ = lr.noInferiors_;
  noSelect_ = lr.noSelect_;
  marked_ = lr.marked_;
  unmarked_ = lr.unmarked_;

  return *this;
}

IMAPClient::ListResponse::ListResponse(const QString & s)
  : noInferiors_(false),
    noSelect_(false),
    marked_(false),
    unmarked_(false)
{
  QStringList l(QStringList::split(' ', s));

  if ((l[0] != "*") || (l[1] != "LIST"))
    return;

  int openBrace = s.find('(');
  int closeBrace = s.find(')');

  if ((-1 == openBrace) || (-1 == closeBrace) || (openBrace >= closeBrace))
    return;

  QString attributes(s.mid(openBrace, closeBrace - openBrace));

  noInferiors_  = -1 != attributes.find("\\Noinferiors");
  noSelect_     = -1 != attributes.find("\\Noselect");
  marked_       = -1 != attributes.find("\\Marked");
  unmarked_     = -1 != attributes.find("\\Unmarked");

  int startQuote = s.find("\"");
  int endQuote = s.find("\"", startQuote + 1);

  if ((-1 == startQuote) || (-1 == endQuote) || (startQuote >= endQuote))
    return;

  hierarchyDelimiter_ = s.mid(startQuote + 1, endQuote - startQuote - 1);

  startQuote = s.find("\"", endQuote + 1);
  endQuote = s.find("\"", startQuote + 1);

  name_ = s.mid(startQuote + 1, endQuote - startQuote - 1);
}

IMAPClient::StatusInfo::StatusInfo(const IMAPClient::StatusInfo & si)
  : messageCount_(si.messageCount_),
    recentCount_(si.recentCount_),
    nextUID_(si.nextUID_),
    uidValidity_(si.uidValidity_),
    unseenCount_(si.unseenCount_),
    hasMessageCount_(si.hasMessageCount_),
    hasRecentCount_(si.hasRecentCount_),
    hasNextUID_(si.hasNextUID_),
    hasUIDValidity_(si.hasUIDValidity_),
    hasUnseenCount_(si.hasUnseenCount_)
{
}

  IMAPClient::StatusInfo &
IMAPClient::StatusInfo::operator = (const IMAPClient::StatusInfo & si)
{
  // Avoid a = a.
  if (this == &si)
    return *this;

  messageCount_ = si.messageCount_;
  recentCount_ = si.recentCount_;
  nextUID_ = si.nextUID_;
  uidValidity_ = si.uidValidity_;
  unseenCount_ = si.unseenCount_;
  hasMessageCount_ = si.hasMessageCount_;
  hasRecentCount_ = si.hasRecentCount_;
  hasNextUID_ = si.hasNextUID_;
  hasUIDValidity_ = si.hasUIDValidity_;
  hasUnseenCount_ = si.hasUnseenCount_;

  return *this;
}


IMAPClient::StatusInfo::StatusInfo(const QString & s)
  : messageCount_(0),
    recentCount_(0),
    nextUID_(0),
    uidValidity_(0),
    unseenCount_(0),
    hasMessageCount_(false),
    hasRecentCount_(false),
    hasNextUID_(false),
    hasUIDValidity_(false),
    hasUnseenCount_(false)
{
  int openBrace(s.find('('));
  int closeBrace(s.find(')'));

  if ((-1 != openBrace) || (-1 != closeBrace) || (openBrace <= closeBrace))
    return;

  QString _s(s.mid(openBrace + 1, closeBrace - openBrace));

  QStringList tokens(QStringList::split(' ', _s));

  QStringList::ConstIterator it(tokens.begin());

  for (; it != tokens.end(); ++it) {

    QString s1(*it);
    QString s2(*(++it));

    if (s1 == "MESSAGES")
      setMessageCount(s2.toULong());

    else if (s1 == "RECENT")
      setRecentCount(s2.toULong());

    else if (s1 == "UIDNEXT")
      setNextUID(s2.toULong());

    else if (s1 == "UIDVALIDITY")
      setUIDValidity(s2.toULong());

    else if (s1 == "UNSEEN")
      setUnseenCount(s2.toULong());
  }
}

