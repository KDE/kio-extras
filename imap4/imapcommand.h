#ifndef _IMAPCOMMAND_H
#define _IMAPCOMMAND_H
/**********************************************************************
 *
 *   imapcommand.h  - IMAP4rev1 command handler
 *   Copyright (C) 2000 Sven Carstens
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
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   Send comments and bug fixes to
 *
 *********************************************************************/

#include <qstringlist.h>
#include <qstring.h>

/** 
 *  @brief encapulate a IMAP command
 *  @author Svenn Carstens
 *  @date 2000
 *  @todo fix the documentation
 */

class imapCommand
{
public:

   /** 
    * @brief Constructor 
    */
  imapCommand ();
   /** 
    * @fn imapCommand (const QString & command, const QString & parameter);
    * @brief Constructor 
    * @param command Imap command
    * @param parameter Parameters to the command
    * @return none
    */
  imapCommand (const QString & command, const QString & parameter);
  /**
   * @fn bool isComplete ();
   * @brief is it complete?
   * @return whether the command is completed
   */
  bool isComplete ();
  /** 
   * @fn const QString & result ();
   * @brief get the result
   * @return The result
   */
  const QString & result ();
  /** 
   * @fn const QString & resultInfo ();
   * @brief get information about the result
   * @return Information about the result
   */
  const QString & resultInfo ();
  /** 
   * @fn const QString & parameter ();
   * @brief get the parameter
   * @return the parameter
   */
  const QString & parameter ();
  /** 
   * @fn const QString & command ();
   * @brief get the command
   * @return the command
   */
  const QString & command ();
  /** 
   * @fn const QString & id ();
   * @brief get the id
   * @return the id
   */
  const QString & id ();

  /** 
   * @fn void setId (const QString &);
   * @brief set the id
   * @param id the id used by the command
   * @return none
   */
  void setId (const QString &);
  /** 
   * @fn void setComplete ();
   * @brief set the completed state
   * @return none
   */
  void setComplete ();
  /** 
   * @fn void setResult (const QString &);
   * @brief set the completed state
   * @param result the command result
   * @return none
   */
  void setResult (const QString &);
  /** 
   * @fn void setResultInfo (const QString &);
   * @brief set the completed state
   * @param result the command result information
   * @return none
   */
  void setResultInfo (const QString &);
  /** 
   * @fn void setCommand (const QString &);
   * @brief set the command
   * @param command the imap command
   * @return none
   */
  void setCommand (const QString &);
  /** 
   * @fn void setParameter (const QString &);
   * @brief set the command parameter(s)
   * @param parameter the comand parameter(s)
   * @return none
   */
  void setParameter (const QString &);
  /** 
   * @fn const QString getStr ();
   * @brief returns the data to send to the server
   * The function returns the complete data to be sent to
   * the server (\<id\> \<command\> [\<parameter\>])
   * @return the data to send to the server
   * @todo possibly rename function to be clear of it's purpose
   */
  const QString getStr ();

  /** 
   * @fn static imapCommand *clientNoop ();
   * @brief Create a NOOP command
   * @return a NOOP imapCommand
   */
  static imapCommand *clientNoop ();
  /** 
   * @fn static imapCommand *clientFetch (ulong uid, const QString & fields, bool nouid = false);
   * @brief Create a FETCH command
   * @param uid Uid of the message to fetch
   * @param fields options to pass to the server
   * @param nouid Perform a FETCH or UID FETCH command
   * @return a FETCH imapCommand
   * Fetch a single uid
   */
  static imapCommand *clientFetch (ulong uid, const QString & fields,
                                   bool nouid = false);
  /** 
   * @fn static imapCommand *clientFetch (ulong fromUid, ulong toUid, const QString & fields, bool nouid = false);
   * @brief Create a FETCH command
   * @param fromUid start uid of the messages to fetch
   * @param toUid last uid of the messages to fetch
   * @param fields options to pass to the server
   * @param nouid Perform a FETCH or UID FETCH command
   * @return a FETCH imapCommand
   * Fetch a range of uids
   */
  static imapCommand *clientFetch (ulong fromUid, ulong toUid,
                                   const QString & fields, bool nouid =
                                   false);
  /** 
   * @fn static imapCommand *clientFetch (const QString & sequence, const QString & fields, bool nouid = false);
   * @brief Create a FETCH command
   * @param sequence a IMAP FETCH sequence string
   * @param fields options to pass to the server
   * @param nouid Perform a FETCH or UID FETCH command
   * @return a FETCH imapCommand
   * Fetch a range of uids. The other clientFetch functions are just
   * wrappers around this function.
   */
  static imapCommand *clientFetch (const QString & sequence,
                                   const QString & fields, bool nouid =
                                   false);
  /** 
   * @fn static imapCommand *clientList (const QString & reference, const QString & path, bool lsub = false);
   * @brief Create a LIST command
   * @param reference
   * @param path The path to list
   * @param lsub Perform a LIST or a LSUB command
   * @return a LIST imapCommand
   */
  static imapCommand *clientList (const QString & reference,
                                  const QString & path, bool lsub = false);
  /** 
   * @fn static imapCommand *clientSelect (const QString & path, bool examine = false);
   * @brief Create a SELECT command
   * @param path The path to select
   * @param lsub Perform a SELECT or a EXAMINE command
   * @return a SELECT imapCommand
   */
  static imapCommand *clientSelect (const QString & path, bool examine =
                                    false);
  /** 
   * @fn static imapCommand *clientClose();
   * @brief Create a CLOSE command
   * @return a CLOSE imapCommand
   */
  static imapCommand *clientClose();
  /** 
   * @brief Create a STATUS command
   * @param path
   * @param parameters
   * @return a STATUS imapCommand
   */
  static imapCommand *clientStatus (const QString & path,
                                    const QString & parameters);
  /** 
   * @brief Create a COPY command
   * @param box
   * @param sequence
   * @param nouid Perform a COPY or UID COPY command
   * @return a COPY imapCommand
   */
  static imapCommand *clientCopy (const QString & box,
                                  const QString & sequence, bool nouid =
                                  false);
  /** 
   * @brief Create a APPEND command
   * @param box
   * @param flags
   * @param size 
   * @return a APPEND imapCommand
   */
  static imapCommand *clientAppend (const QString & box,
                                    const QString & flags, ulong size);
  /** 
   * @brief Create a CREATE command
   * @param path
   * @return a CREATE imapCommand
   */
  static imapCommand *clientCreate (const QString & path);
  /** 
   * @brief Create a DELETE command
   * @param path
   * @return a DELETE imapCommand
   */
  static imapCommand *clientDelete (const QString & path);
  /** 
   * @brief Create a SUBSCRIBE command
   * @param path
   * @return a SUBSCRIBE imapCommand
   */
  static imapCommand *clientSubscribe (const QString & path);
  /** 
   * @brief Create a UNSUBSCRIBE command
   * @param path
   * @return a UNSUBSCRIBE imapCommand
   */
  static imapCommand *clientUnsubscribe (const QString & path);
  /** 
   * @brief Create a EXPUNGE command
   * @return a EXPUNGE imapCommand
   */
  static imapCommand *clientExpunge ();
  /** 
   * @brief Create a RENAME command
   * @param src Source
   * @param dest Destination
   * @return a RENAME imapCommand
   */
  static imapCommand *clientRename (const QString & src,
                                    const QString & dest);
  /** 
   * @brief Create a SEARCH command
   * @param search
   * @param nouid Perform a UID SEARCH or a SEARCH command
   * @return a SEARCH imapCommand
   */
  static imapCommand *clientSearch (const QString & search, bool nouid =
                                    false);
  /** 
   * @brief Create a STORE command
   * @param set
   * @param item
   * @param data
   * @param nouid Perform a UID STORE or a STORE command
   * @return a STORE imapCommand
   */
  static imapCommand *clientStore (const QString & set, const QString & item,
                                   const QString & data, bool nouid = false);
  /** 
   * @brief Create a LOGOUT command
   * @return a LOGOUT imapCommand
   */
  static imapCommand *clientLogout ();
  /** 
   * @brief Create a STARTTLS command
   * @return a STARTTLS imapCommand
   */
  static imapCommand *clientStartTLS ();
protected:
    QString aCommand;
  QString mId;
  bool mComplete;
  QString aParameter;
  QString mResult;
  QString mResultInfo;

private:
    imapCommand & operator = (const imapCommand &);
};

#endif
