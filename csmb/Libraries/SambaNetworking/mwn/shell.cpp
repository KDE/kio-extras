/* Name: shell.cpp

   Description: This file is a part of the libmwn library.

   Authors:	Oleg Noskov (olegn@corel.com)
            Aleksei Rousskikh (alekseir@corel.com),


   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.


*/


/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#include <stdio.h>
#include <stdlib.h>

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <grp.h>

#if defined (_HPUX_SOURCE)
#define _TERMIOS_INCLUDED
#include <bsdtty.h>
#endif

#ifdef HAVE_SYS_STROPTS_H
#include <sys/stropts.h>
#define _NEW_TTY_CTRL
#endif

#include <assert.h>
#include <time.h>
#include <signal.h>
#include <qintdict.h>
#include <sys/wait.h>

#include "shell.h"
#include "acttask.h"
#include "PromptDialog.h"
#include "qapplication.h"
#include "errno.h"

static char ptynam[] = "/dev/ptyxx";
static char ttynam[] = "/dev/ttyxx";

BOOL gbPseudoTerminalDone;
int gnPseudoTerminalReturnCode;
BOOL gbTryAgain = FALSE;
static BOOL gbCancelled;

void CPseudoTerminal::setSize(int lines, int columns)
{
	struct winsize wsize;
  
	if (m_fd < 0)
		return;
  
	wsize.ws_row = (unsigned short)lines;
  wsize.ws_col = (unsigned short)columns;
  
	ioctl(m_fd, TIOCSWINSZ, (char *)&wsize);
}

int CPseudoTerminal::run(QStrList & args, const char* term)
{
	pid_t pid = fork();
  
	if (pid <  0)
		return -1; 
  
	if (pid == 0)
		makeShell(ttynam, args, term);
  
	FILE *f = fdopen(m_fd, "r+");
	
	if (NULL != f)
	{
		setbuf(f, NULL);

		gnPseudoTerminalReturnCode = -1024;
WaitAgain:;
		
		while (!feof(f) && !gbCancelled)
		{
			if (!WaitWithMessageLoop(f))
				break; // stopped by user
			
			if (!DataReceived(f))
			{
				waitpid(pid, &gnPseudoTerminalReturnCode, 0);
				//printf("Code = %d\n", gnPseudoTerminalReturnCode);
				break;
			}
		}

		if (-1024 == gnPseudoTerminalReturnCode &&
				!feof(f) &&
				waitpid(pid, &gnPseudoTerminalReturnCode, WNOHANG) <= 0)
		{
			//printf("Wait again!\n");
			goto WaitAgain;
		}

		//printf("Code = %d\n", gnPseudoTerminalReturnCode);

		if (256 == gnPseudoTerminalReturnCode)
			gbTryAgain = TRUE;

		fclose(f);
	}

	if (gbCancelled)
		return -1024; // Cancelled by user
	
	return 0;
}

void CPseudoTerminal::makeShell(const char* dev, QStrList & args, 
	const char* term)
{
	int sig; char* t;
  
	// open and set all standard files to master/slave tty
  int tt = open(dev, O_RDWR | O_EXCL);
#if defined(SVR4) || defined(__SVR4)
  ioctl(tt, I_PUSH, "ptem");
  ioctl(tt, I_PUSH, "ldterm");
#endif

  //reset signal handlers for child process
  
	for (sig = 1; sig < NSIG; sig++)
		signal(sig,SIG_DFL);
 	
	for (int i = 0; i < getdtablesize(); i++)
	{
		if (i != tt && i != m_fd)
			close(i);
	}
	
  dup2(tt, fileno(stdin));
  dup2(tt, fileno(stdout));
  dup2(tt, fileno(stderr));
	
	if (tt > 2)
		close(tt);

	if (setsid() < 0)
		perror("failed to set process group");

#if defined(TIOCSCTTY)  
	ioctl(0, TIOCSCTTY, 0);
#endif  
	
	int pgrp = getpid();                  
	
	ioctl(0, TIOCSPGRP, (char *)&pgrp);   
	
	setpgid(0,0);                        
  
	close(open(dev, O_WRONLY, 0));       
	
	setpgid(0,0);                        
	
  close(m_fd);
	
  setuid(getuid());
	setgid(getgid());

	if (term && term[0])
		setenv("TERM",term,1);
	
  unsigned int i;
  
	char **argv = (char**)malloc(sizeof(char*)*(args.count()+1));
  
	for (i = 0; i<args.count(); i++)
		argv[i] = strdup(args.at(i));
  
	argv[i] = 0L;

  // setup for login shells
  
	char *f = argv[0];
	
  t = strrchr( argv[0], '/' );
  t = strdup(t);
  *t = '-';
  argv[0] = t;

	execvp(f, argv);
	
	perror("exec failed");
	exit(1); // oops
}

int openShell()
{
	int ptyfd = -1; 
	char *s3, *s4;
  static char ptyc3[] = "pqrstuvwxyzabcde";
  static char ptyc4[] = "0123456789abcdef";

  // Find master pty

	for (s3 = ptyc3; *s3 != 0; s3++) 
  {
    for (s4 = ptyc4; *s4 != 0; s4++) 
    {
      ptynam[8] = ttynam[8] = *s3;
      ptynam[9] = ttynam[9] = *s4;
      
			if ((ptyfd = open(ptynam,O_RDWR)) >= 0) 
      {
        if (geteuid() == 0 || access(ttynam,R_OK|W_OK) == 0)
					break;
        
				close(ptyfd);
				
				ptyfd = -1;
      }
    }
    
		if (ptyfd >= 0)
			break;
  }
  
	if (ptyfd < 0)
		exit(1); // can't open pseudo terminal
  
	fcntl(ptyfd, F_SETFL, O_NDELAY);

  return ptyfd;
}

CPseudoTerminal::CPseudoTerminal(LPCSTR OperationDescription,
																 char *pResultString, 
																 int nResultSize) :
	m_OperationDescription(OperationDescription)
{
	m_pResultString = pResultString;
	m_nResultSize = nResultSize;

	m_fd = openShell();
}

CPseudoTerminal::~CPseudoTerminal()
{
	close(m_fd);
}

void CPseudoTerminal::send_byte(char c)
{ 
	write(m_fd, &c, 1);
}

void CPseudoTerminal::send_string(const char* s)
{
	write(m_fd,s,strlen(s));
}

void CPseudoTerminal::send_bytes(const char* s, int len)
{
	write(m_fd, s, len);
}

BOOL CPseudoTerminal::DataReceived(FILE *f)
{ 
	char buf[4096];
	LPCSTR IncorrectPasswordMessage = "-su: ";
	QString msg;

	buf[0] = '\0';

	LPSTR pBuf = &buf[0];

ReadMore:;

	int n = read(m_fd, pBuf, sizeof(buf)-1-(pBuf-&buf[0]));
	
  if (-1 == n && EAGAIN == errno)
    goto ReadMore;
  
	if (-1 == n)
	{
    return FALSE;
	}
	
	pBuf[n] = '\0';

	if (*pBuf == '\0')
		strcpy(pBuf, "\n");

	pBuf += strlen(pBuf);

	static char savedpwd[1024];

	if (!strncmp("Password:", buf, 9 ))
	{
		char pwd[1024];
		
		if (gbTryAgain)
		{
      if (!CPromptDialog::Prompt(LoadString(knPASSWORD_INVALID), LoadString(knPASSWORD), savedpwd, sizeof(savedpwd)-1))
				goto Cancelled;
			
			strcat(savedpwd,"\n");
			send_string(savedpwd);
			fgets(buf, sizeof(buf)-1, f);	// should be empty
			memset(savedpwd, 0xdd, sizeof(savedpwd));
			gbTryAgain = FALSE;
		}
		else
		{
			msg = m_OperationDescription;

			if (!msg.isEmpty())
				msg += ":\n";
			
			msg += LoadString(knSUPER_USER_RIGHTS);

			if (CPromptDialog::Prompt(msg, LoadString(knPASSWORD), pwd, sizeof(pwd)-1))
			{
				strcat(pwd,"\n");
				send_string(pwd);
				fgets(buf, sizeof(buf)-1, f);	// should be empty
				memset(pwd, 0xdd, sizeof(pwd));
			}
			else
			{
Cancelled:;
				gbCancelled = TRUE;
				close(m_fd);
				return FALSE;
			}
		}
	}
	else
	{
		if (pBuf[-1] != '\n')
			goto ReadMore;

		pBuf = &buf[0];
		
		while (*pBuf == '\r' || *pBuf == '\n')
			pBuf++;

		if (!strncmp(pBuf, IncorrectPasswordMessage, strlen(IncorrectPasswordMessage)) ||
				(NULL != m_pResultString && !strncmp(m_pResultString, IncorrectPasswordMessage, strlen(IncorrectPasswordMessage))))
		{
			//gbTryAgain = TRUE;
		}
		else
		{
			if (NULL != m_pResultString &&
					(int)(strlen(buf) + strlen(m_pResultString)) < m_nResultSize)
				strcat(m_pResultString, buf);
		}
	}

	return TRUE;
}

void CPseudoTerminal::DataWritten(int)
{
	written();
}

int SuperUserExecute(LPCSTR OperationDescription, LPCSTR Command, char *ResultString, int nResultSize)
{
	CPseudoTerminal *term;
	
	QStrList args;		 
	char TempFileName[] = "/tmp/_mwnXXXXXX";
	mkstemp(TempFileName);

	args.append("/bin/su");
	args.append("-c");
	
	QString s;
	s.sprintf("\"\"%s;/bin/echo $?>%s\"\"", Command, TempFileName);
	args.append(s);

DoAgain:;
	
	if (NULL != ResultString)
		*ResultString = '\0'; // prepare result buffer
	
	term = new CPseudoTerminal(OperationDescription, ResultString, nResultSize);

	gbPseudoTerminalDone = FALSE;
	gbCancelled = FALSE;

	term->run(args, NULL);
	
	if (gbCancelled)
		return -1024; // Cancelled by user
	
	if (gbTryAgain)
		goto DoAgain;

	FILE *f = fopen(TempFileName, "r");
	
	if (NULL == f)
		return -1;
	
	char buf[20];
	fgets(buf, sizeof(buf)-1, f);
  
	fclose(f);
	unlink(TempFileName);

	return atoi(buf);
}

