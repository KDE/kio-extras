/***************************************************************************
                          ksshprocess.h  -  description
                             -------------------
    begin                : Tue Jul 31 2001
    copyright            : (C) 2001 by Lucas Fisher
    email                : ljfisher@iastate.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KSSHPROCESS_H
#define KSSHPROCESS_H

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include <qvaluelist.h>

#include <kdebug.h>

#include "process.h"

#define KSSHPROC 7120

/**
 * Provides version independent access to ssh. Currently supported
 * versions of SSH are:
 *   OpenSSH 2.9p1
 *   OpenSSH 2.9p2
 *   Commercial SSH 3.0.0
 * Other versions of SSH will probably work also.
 *
 * To setup a SSH connection first create a list of options to use and tell
 * KSshProcess about your options. Then start the ssh connection. Once the
 * connection is setup use the  stdin, stdout, stderr, and pty file descriptors
 * to communicate with ssh.
 *
 * @author Lucas Fisher
 *
 * Example: Connect to ssh server on localhost
 *   KSshProcess::SshOpt opt;
 *   KSshProcess::SshOptList options;
 *
 *   opt.opt = KSshProcess::SSH_HOST;
 *   opt.str = "localhost";
 *   options.append(opt);
 *
 *   opt.opt = KSshProcess::SSH_USERNAME;
 *   opt.str = "me";
 *   options.append(opt);
 *
 *   opt.opt = KSshProcess::SSH_PASSWD;
 *   opt.str = "mypassword";
 *   options.append(opt);
 *
 *   KSshProcess ssh;
 *   if( !ssh.setOptions(options) ) {
 *       int err = ssh.error();
 *       // process error
 *        return false;
 *   }
 *
 *   if( !ssh.connect(false) ) {
 *       int err = ssh.error();
 *       switch( err ) {
 *       case KSshProcess::ERR_NEW_HOST_KEY:
 *       case KSshProcess::ERR_DIFF_HOST_KEY:
 *           // ask user whether to continue
 *           break;
 *       default:
 *           // alert user
 *           return;
 *       }
 *
 *       if( continue ) {
 *            if( !ssh.connect(true) ) {
 *                 // alert user
 *                 return;
 *            }
 *       }
 *   }
 *   // We have an open ssh connection to localhost
 *
 */

class KSshProcess {
public:
    /**
     * SSH Option
     *
     * Stores SSH options for use with KSshProcess.
     *
     * SSH options are configured much like UDS entries.
     * Each option is assigned a constant and a string, bool,
     * or number is assigned based on the option.
     *
     * @author Lucas Fisher (ljfisher@iastate.edu)
     */
    class SshOpt {
    public:
        Q_UINT32 opt;
        QString  str;
        Q_INT32  num;
        bool     boolean;
    };

    /**
     * List of SshOptions and associated iterators
     */
    typedef QValueList<SshOpt> SshOptList;
    typedef QValueListIterator<SshOpt> SshOptListIterator;
    typedef QValueListConstIterator<SshOpt> SshOptListConstIterator;

    /**
     * Ssh versions supported by KSshProcess.
     */
    enum SshVersion {
        OPENSSH_2_9P1,
        OPENSSH_2_9P2,
        OPENSSH_2_9,   // if we don't find the previous 2.9 verisons use this
        OPENSSH,       // if we don't match any of the above, use the latest version
        SSH_3_0_0,
        SSH,
        SSH_VER_MAX    // always last
    };	

    /**
     * Initialize a SSH process using the first SSH binary found in the PATH
     */
    KSshProcess();

    /**
     * Initialize a SSH process using the specified SSH binary.
     * @param pathToSsh The fully qualified path name of the ssh binary
     *                  KSshProcess should use to setup a SSH connection.
     */
    KSshProcess(QString pathToSsh);
	~KSshProcess();

    /**
     * Set the ssh binary KSshProcess should use.
     *
     * @param pathToSsh Full path to the ssh binary.
     *
     * @return True if the ssh binary is found and KSshProcess
     *         recognizes the version.
     *
     */
	bool setSshPath(QString pathToSsh);

    /**
     * Get the ssh version.
     *
     * @return  The ssh version or -1 if KSshProcess does not recognize
     *          the ssh version. The returned value corresponds to the
     *          member of the SshVersion enum.
     */
    int version();

    /**
     * Get a string describing the ssh version
     *
     * @return A string describing the ssh version recognized by KSshProcess
     */
    QString versionStr();

    /**
     * Get the last error encountered by KSshProcess.
     *
     * @param msg Set to the error message, if any, outputted by ssh when it is run.
     *
     * @return The error number. See SshError for descriptions.
     */
    int error(QString& msg);

    /**
     * Get the last error encountered by KSshProcess.
     * @return The error number. See SshError for descriptions.
     */
    int error() { return mError; }

    /**
     * Send a signal to the ssh process.
     *
     * @param signal The signal to send to the ssh process. See 'kill -l'
     *               for a list of possible signals.
     *               The default signal is SIGTERM which kills ssh.
     *
     */
    void kill(int signal = SIGTERM);

    /**
     * Print the command line arguments ssh is run with using kdDebug.
     */
    void printArgs();

    /**
     * Set the SSH options.
     *
     * @param opts A list of SshOpt objects specifying the ssh options.
     *
     * @return True if all options are valid. False if unrecognized options
     *         or a required option is missing. Call error()
     *         for details.
     *
     * This must be called before connect().  See SshOptType for a list of
     * supported ssh options.  The required options are SSH_USERNAME, SSH_PASSWD,
     * and SSH_HOST.
     *
     * To reset the saved options, just recall setOptions()
     */
    bool setOptions(const SshOptList& opts);

    /**
     * Create a ssh connection based on the options provided by setOptions().
     *
     * @param acceptHostKey When true KSshProcess will automatically accept
     *                      unrecognized or changed host keys.
     *
     * @return True if the ssh connection is successful. False if the connection
     *         fails.  Call error() to get the reason for the failure.
     */
    bool connect(bool acceptHostKey = false);

    /**
     * Access to standard in and out of the ssh process.
     *
     * @return The file description for stdin and stdout of the ssh process.
     */
    int stdioFd() { return ssh.stdioFd(); }

    /**
     * Access to standard error of the ssh process.
     *
     * @return The file descriptior for stderr of the ssh process.
     */
    int stderrFd() { return ssh.stderrFd(); }

    /**
     * Access the pty to which the ssh process is attached.
     *
     * @return The file descriptor of pty to which ssh is attached.
     */
    int pty() { return ssh.fd(); }
private:
    QString mSshPath;
    int mVersion;
    QString mPassword;
    QString mUsername;
    QString mHost;
    bool mConnected;
    bool mRunning;
    int mPort;
    int mError;
    QString mErrorMsg;
    MyPtyProcess ssh;
    QCStringList mArgs;
    void init();

    static const char * const versionStrs[];
    static const char * const passwdPrompt[];
    static const char * const authSuccessMsg[];
    static const char * const authFailedPrompt[];
    static const char * const hostKeyMissing[];
    static const char * const hostKeyChanged[];
    static const char * const continuePrompt[];

public:
    /**
     * SSH options supported by KSshProcess.  Set SshOpt::opt to one of these
     * values.
     */
    // we cannot do this like UDSAtomType (ORing the type with the name) because
    // we have too many options for ssh and not enough bits.
    enum SshOptType {
        /* Request server to invoke subsystem. (str) */
        SSH_SUBSYSTEM,
        /* Connect to port on the server. (num) */
        SSH_PORT,
        /* Connect to host. (str) */
        SSH_HOST,
        /* connect using this username. (str) */
        SSH_USERNAME,
        /* connect using this password. (str) */
        SSH_PASSWD,
        /* connect using this version of the SSH protocol. num == 1 or 2 */
        SSH_PROTOCOL,
        /* whether to forward X11 connections. (boolean) */
        SSH_FORWARDX11,
        /* whether to do agent forwarding. (boolean) */
        SSH_FORWARDAGENT,
        /* use as escape character. 0 for none  (num) */
        SSH_ESCAPE_CHAR,
        /* command for ssh to perform once it is connected */
        SSH_COMMAND,
        /* Set ssh verbosity. This may be added multiple times. It may also cause KSSHProcess
         * to fail since we don't understand all the debug messages. */
        SSH_VERBOSE,
        /* Set a ssh option as one would find in the ssh_config file
         * The str member should be set to 'optName value' */
        SSH_OPTION,
        /* Set some other option not supported by KSSHProcess. The option should
         * be specified in the str member of SshOpt. Careful with this since
         * not all versions of SSH support the same options. */
        SSH_OTHER,
        SSH_OPT_MAX // always last
    }; // that's all for now

    /**
     * Errors that KSshProcess can encounter.  When a member function returns
     * false, call error() to retrieve one of these error codes.
     */
    enum SshError {
        /* Don't recognize the ssh version */
        ERR_UNKNOWN_VERSION,
        /* Cannot lauch ssh client */
        ERR_CANNOT_LAUNCH,
        /* Interaction with the ssh client failed. This happens when we can't
        find the password prompt or something similar */
        ERR_INTERACT,
        /* Arguments for both a remotely executed subsystem and command were provide.
           Only one or the other may be used */
        ERR_CMD_SUBSYS_CONFLICT,
        /* No password was supplied */
        ERR_NEED_PASSWD,
        /* No usename was supplied */
        ERR_NEED_USERNAME,
        /* Timed out waiting for a response from ssh or the server */
        ERR_TIMED_OUT,
        /* Internal error, probably from a system call */
        ERR_INTERNAL,
        /* ssh was disconnect from the host */
        ERR_DISCONNECTED,
        /* No ssh options have been set. Call setArgs() before calling connect. */
        ERR_NO_OPTIONS,
        /* A host key was received from an unknown host. 
         * Call connect() with the acceptHostKey argument to accept the key. */
        ERR_NEW_HOST_KEY,
        /* A host key different from what is stored in the user's known_hosts file
         * has be received. This is an indication of an attack*/
        ERR_DIFF_HOST_KEY,
        /* An invalid option was found in the SSH option list */
        ERR_INVALID_OPT,
        /* Authentication failed */
        ERR_AUTH_FAILED,
        /* We have no idea what happened */
        ERR_UNKNOWN,
        ERR_MAX
    };

};
#endif
