#include "my_process.h"

#include <iostream.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

ClientProcess::ClientProcess()
:startingFinished(false)
,m_exited(-1)
{
}

ClientProcess::~ClientProcess()
{
   this->kill();
}

void ClientProcess::kill()
{
   int s(0);
   ::waitpid(pid(),&s,WNOHANG);
   ::kill(pid(), SIGTERM);
   ::waitpid(pid(),&s,0);
};

int ClientProcess::exited()
{
   if (m_exited!=-1)
      return m_exited;
   int s(0);
   if (::waitpid(pid(),&s,WNOHANG)==0)
      return -1;
   if (WIFEXITED(s))
   {
      m_exited=WEXITSTATUS(s);
      return m_exited;
   };
   return -1;
};

int ClientProcess::select(int secs, int usecs, bool* readEvent, bool* writeEvent)
{
   if (readEvent!=0)
      *readEvent=false;
   if (writeEvent!=0)
      *writeEvent=false;

   struct timeval tv;
   tv.tv_sec=secs;
   tv.tv_usec=usecs;

   fd_set readFD;
   FD_ZERO(&readFD);
   if (readEvent!=0)
      FD_SET(fd(),&readFD);

   fd_set writeFD;
   FD_ZERO(&writeFD);
   if (writeEvent!=0)
      FD_SET(fd(),&writeFD);

   int result=::select(fd()+1,&readFD,&writeFD,0,&tv);
   if (result>0)
   {
      if (readEvent!=0)
         *readEvent=FD_ISSET(fd(),&readFD);
      if (writeEvent!=0)
         *writeEvent=FD_ISSET(fd(),&writeFD);
   };
   return result;
};

/*int ClientProcess::exec(const char *passwd)
{    
   //if (m_User.isEmpty())
      //return -1;
   //if (check)
      setTerminal(true);

   // Try to set the default locale to make the parsing of the output
   // of `smbclient' easier.
   putenv("LANG=C");

   QCStringList args;
   //args += "-L localhost";
   args += "-L";
   args += "localhost";
   int ret = PtyProcess::exec("smbclient", args);
   if (ret < 0)
   {
      cerr<<"could not execute smbclient"<<endl;
      return PasswdNotFound;
   }

   ret = ConversePasswd(passwd, TRUE);
   if (ret < 0)
      cerr<< "Conversation with smbclientfailed.\n";

   waitForChild();
   return ret;
}*/

bool ClientProcess::start(const QCString& binary, QCStringList& args)
{    
   setTerminal(true);
   // Try to set the default locale to make the parsing of the output
   // of `smbclient' easier.
   putenv("LANG=C");
   int ret = PtyProcess::exec(binary, args);
   if (ret != 0)
   {
      //cerr<<"could not execute smbclient"<<endl;
      return false;
   }
   return true;
}

/*
 * The tricky thing is to make this work with a lot of different passwd
 * implementations. We _don't_ want implementation specific routines.
 * Return values: -1 = unknown error, 0 = ok, >0 = error code.
 */
/*int ClientProcess::ConversePasswd(const char *passwd,int check)
{
   QCString line;
   bool passwordGiven(FALSE);

   do
   {
      line = readLine();
      if (line.isNull())
      {
         return -1;
      }

      cerr<<"read line: -"<<line<<"-"<<endl;
      if (line.contains("Password:"))
      {
         WaitSlave();
         write(m_Fd, passwd, strlen(passwd));
         write(m_Fd, "\n", 1);
         cerr<<"fed the password into smbclient"<<endl;
         passwordGiven=TRUE;
      };
   } while (!passwordGiven);

   return 0;
}*/

