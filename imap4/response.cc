// $Id$
/**********************************************************************
 *
 *   response.cc  - react to responses from the server
 *   Copyright (C) 1999  John Corey
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Send comments and bug fixes to jcorey@fruity.ath.cx
 *
 *********************************************************************/

/*
  References:
    RFC 2060 - Internet Message Access Protocol - Version 4rev1 - December 1996
    RFC 2192 - IMAP URL Scheme - September 1997
    RFC 1731 - IMAP Authentication Mechanisms - December 1994
               (Discusses KERBEROSv4, GSSAPI, and S/Key)
    RFC 2195 - IMAP/POP AUTHorize Extension for Simple Challenge/Response
             - September 1997 (CRAM-MD5 authentication method)
    RFC 2104 - HMAC: Keyed-Hashing for Message Authentication - February 1997

  Supported URLs:
    imap://server/ - Prompt for user/pass, list all folders in home directory
    imap://user:pass@server/ - Uses LOGIN to log in
    imap://user;AUTH=method:pass@server/ - Uses AUTHENTICATE to log in

    imap://server/folder/ - List messages in folder
 */

#include "imap4.h"
#include "base64md5.h"

#include <kprotocolmanager.h>
#include <ksock.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <kio/passdlg.h>
#include <klocale.h>

using namespace KIO;

void IMAP4Protocol::startLoop ()
{
  char buf[1024];
  bool gotGreet = false, isNum=false;
  struct timeval m_tTimeout = {1, 0};
  QString s_buf, s_identifier;
  fd_set FDs;
  while (1) {
    //kdDebug() << "IMAP4: Top of the loop" << endl;
    FD_ZERO(&FDs);
    FD_SET(m_iSock, &FDs);
    QString s_cmd;
    memset(&buf, sizeof(buf), 0);

    if (ReadLine(buf, sizeof(buf)-1) == 0) {
      if (ferror(fp)) {
        kdDebug() << "IMAP4: Error while freading something" << endl;
	break;
      } else {
	while (::select(m_iSock+1, &FDs, 0, 0, &m_tTimeout) ==0) {
	  //kdDebug() << "IMAP4: Sleeping" << endl;
	  sleep(10);
        }
	if (ReadLine(buf, sizeof(buf)-1) == 0) {
	  kdDebug() << "IMAP4: Error while freading something, and yes we already waited on select" << endl;
	  break;
	}
      }  
    }
    kdDebug() << "IMAP4: S: " << buf << endl;
    s_buf=buf;
    if (s_buf.find(" ") == -1) {
      kdDebug() << "IMAP4: Got an invalid response: " << buf << endl;
      break;
    }
    s_identifier=s_buf.mid(0, s_buf.find(" "));
    s_buf.remove(0, s_buf.find(" ")+1);
    //fprintf(stderr,"First token is :%s:\n", s_identifier.data());fflush(stderr);
    if (s_identifier == "*") {
      //debug(QString("IMAP4: Got a star."));
      if (!gotGreet) {
	// We just kinda assume it's the greeting
	gotGreet=true;
	sendNextCommand();
	continue;
      }
      QString s_token;
      if (s_buf.find(" ") == -1) {
        if ( (s_buf != "NO") && (s_buf != "OK") && (s_buf != "BAD")) {
	  kdDebug() << "IMAP4: We got a weird response: " << buf << endl;
	}  
      } else {
	s_token = s_buf.mid(0, s_buf.find(" "));
	s_buf.remove(0, s_buf.find(" ")+1);
	//fprintf(stderr,"Token is:%s:\n", s_token.ascii());fflush(stderr);
	(void)s_token.toInt(&isNum);

	QRegExp r("\r\n");
	s_buf.replace(r, "");

	if (isNum) {
	  if (s_buf.find(" ") != -1) {
	    s_cmd = s_buf.mid(0, s_buf.find(" "));
	    s_buf.remove(0, s_buf.find(" ")+1);
	  } else {
	    s_cmd = s_buf.copy();
	    s_buf = "";
	  } // s_buf.find
	  if (s_cmd == "EXISTS") {
	    kdDebug() << "IMAP4: " << s_token << " messages exist in the current mbox" << endl;
	  } else if (s_cmd == "RECENT") {
	    kdDebug() << "IMAP4: " << s_token << " messages have the recent flag in the current mbox" << endl;
	  } else if (s_cmd == "FETCH") {
	    ;
	  } else {
	    kdDebug() << "IMAP4: Got unknown untokened response :" << buf << ":" << endl;
	  } // s_cmd == EXISTS
	} else if (s_token == "FLAGS") {
	    //debug(QString("IMAP4: ?? Flags are :%1:").arg(s_buf));
        } else if (s_token == "LIST") {
          // Sample: * LIST (\NoInferiors \Marked) "/" "~/.imap/Item"
	  // (Attributes) <hierarchy delimiter> <item name>
	  //debug(QString("IMAP4: S:* LIST %1").arg(s_buf));
	  processList(s_buf);
        } else if (s_token == "LSUB") {
	  // Sample: * LSUB (\NoInferiors \Marked) "/" ~/.imap/Item
	  //debug(QString("IMAP4: S:* LSUB %1").arg(s_buf));
	} else if (s_token == "CAPABILITY")  {
	  capabilities = QStringList::split(" ", s_buf);
          //debug(QString("IMAP4: Found %1 capabilities").arg(capabilities.count()));
        } else if (s_token == "BYE") {
	  kdDebug() << "IMAP4: Server closing..." << endl;
	  imap4_close();
	} else {
/* 	  if (s_buf.find(" ") != -1)
 	    s_cmd = s_buf.mid(0, s_buf.find(" "));
 	  else
 	    s_cmd = s_buf.copy();*/
          serverResponses.append(s_buf);
	  //debug(QString("IMAP4: appending server response: %1").arg(s_buf));
	} // isNum
      }
    } else if (s_identifier == "+") {  // Server awaits further commands from clients
      //debug(QString("IMAP4: Got a plus - %1").arg(s_buf));
      // Really need to save this somewhere for authentication...
      s_buf.remove(0, s_buf.find(" ")+1);  // skip to the key
      authKey = s_buf;
      authState++;
      imap4_login();
//      sendNextCommand();
    } else {
//      debug(QString("IMAP4: Looking for a match - %1").arg(s_identifier));
      pending.first();
      while (pending.current()) {
	if (pending.current()->identifier.copy() == s_identifier){ 
//	  debug(QString("IMAP4: Got a match!  type = %1").arg(pending.current()->type));
	  switch (pending.current()->type) {
	    // Any State
	    case ICMD_NOOP: {
//	      debug("IMAP4: NOOP response");
	      sendNextCommand();
	      break;
	    }
	    case ICMD_CAPABILITY: {
//              debug(QString("IMAP4: CAPABILITY response"));
	      bool imap4v1 = false;
	      for(QStringList::Iterator it=capabilities.begin(); it!=capabilities.end();
	          it++) {
                QString cap = *it;
		if (cap.find("IMAP4rev1", 0, false) != -1)
		  imap4v1 = true;
	      }
	      if (!imap4v1) {
                kdDebug() << "IMAP4: Uh oh, server is not IMAP4rev1 compliant!  Bailing out" << endl;
                error(ERR_UNSUPPORTED_PROTOCOL, "IMAP4rev1");
                return;
	      }
//	      debug("IMAP4: Server is IMAP4rev1 compliant.");
	      imap4_login();
	      sendNextCommand();
	      break;
	    }
	    case ICMD_LOGOUT: {
              //debug(QString("IMAP4: LOGOUT response"));
	      imap4_close();
//	      sendNextCommand();
//              return;
	      break;
	    }

	    // Non-Authenticated State
	    case ICMD_AUTHENTICATE: {
	      //debug(QString("IMAP4: AUTHENTICATE response - %1").arg(s_buf));
              if (s_buf.left(3) == "OK ") {
	        authState = 999;
		kdDebug() << "IMAP4: AUTHENTICATE successfull!" << endl;
		imap4_exec();
	      } else if (s_buf.left(3) == "NO ") {
	        kdDebug() << "IMAP4: AUTHENTICATE failed" << endl;
		error(ERR_ACCESS_DENIED, m_sUser);
                authState = 0;
		return;
	      } else {
	        //debug(QString("IMAP4: BAD AUTHENTICATE error - %1").arg(s_buf));;
		error(ERR_UNSUPPORTED_PROTOCOL, i18n("Error during authentication."));
                authState = 0;
		return;
	      }
              sendNextCommand();
	      break;
	    }
	    case ICMD_LOGIN: {
	      //debug(QString("IMAP4: LOGIN response - %1").arg(s_buf));
              if (s_buf.left(3) == "OK ") {
	        authState = 999;
		kdDebug() << "IMAP4: LOGIN successfull!" << endl;
		imap4_exec();
	      } else if (s_buf.left(3) == "NO ") {
	        kdDebug() << "IMAP4: AUTHENTICATE failed" << endl;
		error(ERR_ACCESS_DENIED, m_sUser);
                authState = 0;
		return;
	      } else {
	        //debug(QString("IMAP4: BAD AUTHENTICATE error - %1").arg(s_buf));;
		error(ERR_UNSUPPORTED_PROTOCOL, "LOGIN");
                authState = 0;
		return;
	      }
	      sendNextCommand();
	      break;
	    }
	    // Authenticated State
	    case ICMD_LIST: {
	      //debug(QString("IMAP4: LIST response: %1").arg(s_buf));
	      if (s_buf.left(3) == "OK ") {
	        kdDebug() << "Finishing..." << endl;
	        finished();
	      } else {
	        //debug(QString("Error during LIST: %1").arg(s_buf));
		warning(s_buf);
	      }
	      break;
	    }
	    case ICMD_LSUB: {
	      //debug(QString("IMAP4: LSUB response: %1").arg(s_buf));
	      break;
	    }
	    case ICMD_SELECT: {
	      // <not tested>
	      kdDebug() << "IMAP4: SELECT response" << endl;
	      if (s_buf.left(3) == "OK ") {
	        s_cmd = s_buf.mid(3, s_buf.length());
	        if (s_cmd.left(11)=="[READ-ONLY]") {
	          kdDebug() << "IMAP4: Warning mbox opened readonly" << endl;
		  s_cmd.remove(0, 12);
	        }
	        kdDebug() << "IMAP4: ICMD_SELECT completed fine" << endl;
	        sendNextCommand();
	      } else {
	        kdDebug() << "IMAP4: ICMD_SELECT failed!" << endl;
	      }
	      break;
	    }
	    default: {
	      break;
	    }
	  } // switch
	}
	pending.next();
      }
    }
  }
}

