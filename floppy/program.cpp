
#include "program.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>	
#include <sys/types.h>
#include <sys/socket.h> 
#include <signal.h>

#include <kdebug.h>

Program::Program(const QStringList &args)
:m_pid(0)
,mArgs(args)
,mStarted(false)
{
}

Program::~Program()
{
   if (m_pid!=0)
   {
      this->kill();
   };
}

bool Program::start()
{
   if (mStarted) return false;
   if (pipe(mStdout)==-1) return false;
   if (pipe(mStdin )==-1) return false;
   if (pipe(mStderr )==-1) return false;
   m_pid=fork();

   if (m_pid>0)
   {
      //parent
      ::close(mStdin[0]);
      ::close(mStdout[1]);
      ::close(mStderr[1]);
      mStarted=true;
      return true;
   }
   else if (m_pid==-1)
   {
      //failed
      return false;
   }
   else if (m_pid==0)
   {
      kdDebug(7101)<<"**** mStdin[0]: "<<mStdin[0]<<endl;
      kdDebug(7101)<<"**** mStdin[1]: "<<mStdin[1]<<endl;
      kdDebug(7101)<<"**** mStdout[0]: "<<mStdout[0]<<endl;
      kdDebug(7101)<<"**** mStdout[1]: "<<mStdout[1]<<endl;
      kdDebug(7101)<<"**** mStderr[0]: "<<mStderr[0]<<endl;
      kdDebug(7101)<<"**** mStderr[1]: "<<mStderr[1]<<endl;
      //child
      ::close(0); // close the stdios
      ::close(1);
      ::close(2);

      int fd1=dup(mStdin[0]);
      int fd2=dup(mStdout[1]);
      int fd3=dup(mStderr[1]);

      ::close(mStdin[1]);
      ::close(mStdout[0]);
      ::close(mStderr[0]);

      fcntl(mStdin[0], F_SETFD, FD_CLOEXEC);
      //fcntl(mStdin[1], F_SETFD, FD_CLOEXEC);
      fcntl(mStdout[1], F_SETFD, FD_CLOEXEC);
      fcntl(mStderr[1], F_SETFD, FD_CLOEXEC);

      char **arglist=(char**)malloc((mArgs.count()+1)*sizeof(char*));
      int c=0;

      for (QStringList::Iterator it=mArgs.begin(); it!=mArgs.end(); ++it)
      {
         arglist[c]=(char*)malloc((*it).length()+1);
         strcpy(arglist[c], (*it).latin1());
         c++;
      }
      arglist[mArgs.count()]=0;
      execvp(arglist[0], arglist);
      _exit(-1);
   };
}

bool Program::isRunning()
{
	return mStarted;
}

int Program::select(int secs, int usecs, bool& stdoutReceived, bool& stderrReceived/*, bool& stdinWaiting*/)
{
   struct timeval tv;
   tv.tv_sec=secs;
   tv.tv_usec=usecs;

   fd_set readFDs;
   FD_ZERO(&readFDs);
   FD_SET(stdoutFD(),&readFDs);
   FD_SET(stderrFD(),&readFDs);

   int maxFD=stdoutFD();
   if (stderrFD()>maxFD) maxFD=stderrFD();

   /*fd_set writeFDs;
   FD_ZERO(&writeFDs);
   FD_SET(stdinFD(),&writeFDs);
   if (stdinFD()>maxFD) maxFD=stdinFD();*/
   maxFD++;

   int result=::select(maxFD,&readFDs,/*&writeFDs*/0,0,&tv);
   if (result>0)
   {
      stdoutReceived=FD_ISSET(stdoutFD(),&readFDs);
      stderrReceived=FD_ISSET(stderrFD(),&readFDs);
      //stdinWaiting=(FD_ISSET(stdinFD(),&writeFDs));
   };
   return result;
};

int Program::kill()
{
   if (m_pid==0)
      return -1;
   return ::kill(m_pid, SIGTERM);
};



void Program::closeFDs()
{
   ::close(mStdin[0]);
   ::close(mStdout[0]);
   ::close(mStderr[0]);

   ::close(mStdin[1]);
   ::close(mStdout[1]);
   ::close(mStderr[1]);
};

