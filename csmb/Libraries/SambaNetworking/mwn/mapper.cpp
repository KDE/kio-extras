/* Name: mapper.cpp

   Description: This file is a part of the libmwn library.

   Author:	Chris Ellison
   Modifications: Oleg Noskov (olegn@corel.com)

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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include "qstring.h"
#include "common.h"
#include "mapper.h"


QApplication *gpApp;


int netmap(QString &szResult, const char* szUNC)
{
	//printf("netmap(%s)\n", szUNC);
  static QString szCliMessage;
  const char* p;
  int nResultCode;
  QString szTempResult;
  szCliMessage.sprintf("netmap %d %u %u %s", getpid(), getuid(), getgid(), szUNC);
				
  int fd = GetServerOpenHandle((const char*)szCliMessage);
  szCliMessage = "";
				
  if (fd >= 0)
  {
    char buf[1024];
    int n;
												
    n = read(fd, buf, sizeof(buf) - 1);
														
    buf[n] = '\0';
    szCliMessage.sprintf("%s", buf);
    close(fd);
  }
  p = (const char*)szCliMessage;
  szTempResult = ExtractWord(p, " ");
  nResultCode = atoi((const char*)szTempResult);
										
  szTempResult = ExtractTail(p);
  szResult = (const char*)szTempResult;
																									
  return (nResultCode);
}


int netmap(char *szResult, size_t n,  const char* szUNC)
{
	//printf("netmap(%s)\n", szUNC);

	static QString szCliMessage;
	const char* p;
	int nResultCode;
	QString szTempResult;
	szCliMessage.sprintf("netmap %d %u %u %s", getpid(), getuid(), getgid(), szUNC);
	
	int fd = GetServerOpenHandle((const char*)szCliMessage);
	szCliMessage = "";
	
	if (fd >= 0)
	{
		char buf[1024];
		int n;

		n = read(fd, buf, sizeof(buf) - 1);
		
		buf[n] = '\0';
		szCliMessage.sprintf("%s", buf);
		close(fd);
	}
	p = (const char*)szCliMessage;
	szTempResult = ExtractWord(p, " ");
	nResultCode = atoi((const char*)szTempResult);

	szTempResult = ExtractTail(p);
	
	if (n >= (strlen((const char*)szTempResult) + 1))
	{
		strncpy(szResult, (const char*)szTempResult, n);
	}
	else
	{
		strcpy(szResult, "The buffer length is not large enough");
		nResultCode = knInvalidBufSize;
	}
	
	//printf("Result=%s\n", (LPCSTR)szResult);

	return nResultCode;
}


int netunmap(QString &szResult, const char* UNC)
{
  //printf("netunmap(%s)\n", UNC);

	static QString szCliMessage;
  const char* p;
  int nResultCode;
  QString szTempResult;
  szCliMessage.sprintf("netunmap %d %u %u %s", getpid(), getuid(), getgid(), UNC);

  int fd = GetServerOpenHandle((const char*)szCliMessage);
  szCliMessage = "";

  if (fd >= 0)
  {
    char buf[1024];
    int n = read(fd, buf, sizeof(buf) -1);

    buf[n] = '\0';
    szCliMessage.sprintf("%s", buf);
    close(fd);
  }
  p = (const char*)szCliMessage;
  szTempResult = ExtractWord(p, " ");
  nResultCode = atoi((const char*)szTempResult);

  szTempResult = ExtractTail(p);
  szResult = (const char*)szTempResult;

  return (nResultCode);
}


int netunmap(char *szResult, size_t n, const char* UNC)
{
  static QString szCliMessage;
  const char* p;
  int nResultCode;
  QString szTempResult;
  szCliMessage.sprintf("netunmap %d %u %u %s", getpid(), getuid(), getgid(), UNC);

  int fd = GetServerOpenHandle((const char*)szCliMessage);
  szCliMessage = "";

  if (fd >= 0)
  {
    char buf[1024];
    int n = read(fd, buf, sizeof(buf) -1);

    buf[n] = '\0';
    szCliMessage.sprintf("%s", buf);
    close(fd);
  }
  p = (const char*)szCliMessage;
  szTempResult = ExtractWord(p, " ");
  nResultCode = atoi((const char*)szTempResult);

  szTempResult = ExtractTail(p);
	if ( n >= (strlen((const char*)szTempResult) + 1) )
	{
	  strncpy(szResult, (const char*)szTempResult, n);
	}
	else
	{
	  strcpy(szResult, "The buffer length is not large enough");
	  nResultCode = knInvalidBufSize;
  }
  return (nResultCode);
}

int NetmapWithMessageLoop(QString &szResult, LPCSTR szUNC, pid_t *pPID /* = NULL */)
{
  szResult.sprintf("netmap %d %u %u %s", NULL == pPID ? getpid() : *pPID, getuid(), getgid(), szUNC);
				
  FILE *f = ServerOpen((LPCSTR)szResult);
	
	char buf[2048];
				
  if (NULL != f)
  {
		if (!WaitWithMessageLoop(f))
			return -1;

    int nn = read(fileno(f), buf, sizeof(buf) -1);
		buf[nn] = '\0';
		
		if (buf[strlen(buf)-1] == '\n')
			buf[strlen(buf)-1] = '\0';
    
    fclose(f);

  	LPCSTR p = &buf[0];
  
    int nResultCode = atoi(ExtractWord(p, " "));
    szResult = ExtractTail(p);
    return nResultCode;																									
  }
  else
    return knNetmapError;
}

