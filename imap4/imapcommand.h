#ifndef _IMAPCOMMAND_H
#define _IMAPCOMMAND_H "$Id$"
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

/** @class imapCommand
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
   * @param result the imap command
   * @return none
   */
  void setCommand (const QString &);
  /** 
   * @fn void setParameter (const QString &);
   * @brief set the command parameter(s)
   * @param result the comand parameter(s)
   * @return none
   */
  void setParameter (const QString &);
  /** 
   * @fn const QString getStr ();
   * @brief get a string
   * @return a string
   */
  const QString getStr ();

  static imapCommand *clientNoop ();
  static imapCommand *clientFetch (ulong uid, const QString & fields,
                                   bool nouid = false);
  static imapCommand *clientFetch (ulong fromUid, ulong toUid,
                                   const QString & fields, bool nouid =
                                   false);
  static imapCommand *clientFetch (const QString & sequence,
                                   const QString & fields, bool nouid =
                                   false);
  static imapCommand *clientList (const QString & reference,
                                  const QString & path, bool lsub = false);
  static imapCommand *clientSelect (const QString & path, bool examine =
                                    false);
  static imapCommand *clientClose();
  static imapCommand *clientStatus (const QString & path,
                                    const QString & parameters);
  static imapCommand *clientCopy (const QString & box,
                                  const QString & sequence, bool nouid =
                                  false);
  static imapCommand *clientAppend (const QString & box,
                                    const QString & flags, ulong size);
  static imapCommand *clientCreate (const QString & path);
  static imapCommand *clientDelete (const QString & path);
  static imapCommand *clientSubscribe (const QString & path);
  static imapCommand *clientUnsubscribe (const QString & path);
  static imapCommand *clientExpunge ();
  static imapCommand *clientRename (const QString & src,
                                    const QString & dest);
  static imapCommand *clientSearch (const QString & search, bool nouid =
                                    false);
  static imapCommand *clientStore (const QString & set, const QString & item,
                                   const QString & data, bool nouid = false);
  static imapCommand *clientLogout ();
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
