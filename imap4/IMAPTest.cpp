#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include <qsocketdevice.h>
#include <qapplication.h>

#include "IMAPClient.h"

int main(int argc, char ** argv)
{
  QApplication app(argc, argv);

  QString hostname = argc > 1 ? argv[1] : "localhost";
  int port = argc > 2 ? QString(argv[2]).toInt() : 143;

  qDebug("Connecting to %s:%d", hostname.ascii(), port);

  int fd = -1;

  struct sockaddr_in sin;
  struct hostent * he;

  ::memset(&sin, 0, sizeof(sin));

  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;

  qDebug("Looking up host");

  he = ::gethostbyname(hostname.ascii());

  if (NULL == he) {
    qDebug("Couldn't find host `%s'", hostname.ascii());
    return 1;
  }

  qDebug("Found host");

  for (int i = 0; he->h_addr_list[i] != NULL; i++)
  {
    memcpy(&sin.sin_addr, he->h_addr_list[i], he->h_length);
    fd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_IP);

    if (fd >= 0)
    {
      int x =
        ::connect(fd, (struct sockaddr *)(&sin), sizeof(struct sockaddr_in));

      if (0 == x) {
        qDebug("Connected\n");
        break;
      }

      ::close(fd);
      fd = -1;
    }
  }

  if (-1 == fd) {
    qDebug("Couldn't connect to %s:%d", hostname.ascii(), port);
    return 1;
  }

  QSocketDevice socket(fd, QSocketDevice::Stream);
  socket.setBlocking(true);

  IMAPClient m(&socket);

  QString greeting = m.greeting();

  qDebug("Greeting: %s\n", greeting.ascii());

  QString capability = m.capability();

  if (capability.isEmpty()) {
    qDebug("CAPABILITY failed");
    return 1;
  }

  QStringList tokens(QStringList::split(' ', capability));

  if (!tokens.contains("IMAP4rev1")) {
    qDebug("IMAP4rev1 not found in capability response !");
    return 1;
  } else {
    qDebug("IMAP4rev1 found in capability response. Server is OK\n");
  }

  bool ok = m.login("rik", "poker");

  if (!ok) {
    qDebug("LOGIN failed");
    return 1;
  }

  qDebug("LOGIN ok\n");

  QValueList<IMAPClient::ListResponse> responseList;

  ok = m.list("", "*", responseList, false);

  if (!ok) {
    qDebug("LIST failed");
    return 1;
  }

  bool arseExists = false;

  qDebug("Mailbox list:");

  QValueList<IMAPClient::ListResponse>::ConstIterator it(responseList.begin());

  for (; it != responseList.end(); ++it) {
    QString mailboxName = (*it).name();
    qDebug("  `%s'", mailboxName.ascii());
    if (mailboxName == "INBOX.arse")
      arseExists = true;
  }

  qDebug("End of mailbox list\n");

  if (arseExists) {

    qDebug("INBOX.arse exists. Removing...");

    ok = m.removeMailbox("INBOX.arse");

    if (!ok) {
      qDebug("REMOVE INBOX.arse failed");
      return 1;
    }

    qDebug("REMOVE INBOX.arse succeded\n");
  }

  qDebug("(Re)creating INBOX.arse");

  ok = m.createMailbox("INBOX.arse");

  if (!ok) {
    qDebug("CREATE failed");
    return 1;
  }

  qDebug("CREATE INBOX.arse succeded\n");

  IMAPClient::MailboxInfo info;

  ok = m.selectMailbox("INBOX", info);

  if (!ok) {
    qDebug("SELECT failed");
    return 1;
  }

  qDebug("SELECT INBOX succeded. Info follows...");

  qDebug("  Exists: %ld", info.count());
  qDebug("  Recent: %ld", info.recent());
  qDebug("  Unseen: %ld", info.unseen());
  qDebug("  Flags: %s", m.flagsString(info.flags()).ascii());
  qDebug("  PermanentFlags: %s", m.flagsString(info.permanentFlags()).ascii());
  qDebug("  UIDValidity: %ld", info.uidValidity());
  qDebug("  ReadWrite: %s", info.readWrite() ? "yes" : "no");

  qDebug("End of INBOX info\n");

  QValueList<ulong> searchResult =
    m.search(
        "FROM charles@derkarl.org",
        QString::null,
        true
    );

  qDebug("Found %d messages", searchResult.count());

  ulong uid = searchResult[0];

  QStringList bodyInfo = m.fetch(uid, uid, "BODYSTRUCTURE", true);

  qDebug("Body info: %s", bodyInfo[0].ascii());

  QStringList firstMessage = m.fetch(uid, uid, "BODY[TEXT]", true);

  qDebug("%d lines in body", firstMessage.count());

  qDebug("Body follows:");

  // Remove the crap that gets added.
  firstMessage.remove(firstMessage.begin());
  firstMessage.remove(firstMessage.fromLast());

  QStringList::ConstIterator bodyIt(firstMessage.begin());

  for (; bodyIt != firstMessage.end(); ++bodyIt) {
    qDebug((*bodyIt).ascii());
  }

  qDebug("End of body\n");

  ok = m.copy(uid, uid, "INBOX.arse", true);

  if (!ok) {
    qDebug("Couldn't copy message %ld to INBOX.arse", uid);
    return 1;
  }

  qDebug("Copied message to INBOX.arse");

  return 0;
}

