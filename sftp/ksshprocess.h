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
 *   OpenSSH 3.0
 *   OpenSSH 3.1
 *   Commercial SSH 3.0.0
 * Other versions of OpenSSH and commerical SSH will probably work also.
 *
 * To setup a SSH connection first create a list of options to use and tell
 * KSshProcess about your options. Then start the ssh connection. Once the
 * connection is setup use the  stdin, stdout, stderr, and pty file descriptors
 * to communicate with ssh. For a detailed example of how to use, see
 * ksshprocesstest.cpp.
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
 *   KSshProcess ssh;
 *   if( !ssh.setOptions(options) ) {
 *       int err = ssh.error();
 *       // process error
 *        return false;
 *   }
 *
 *   int err;
 *   QString errMsg;
 *   while( !ssh.connect() ) {
 *       err = ssh.error(errMsg);
 *       
 *       switch( err ) {
 *       case KSshProcess::ERR_NEW_HOST_KEY:
 *       case KSshProcess::ERR_DIFF_HOST_KEY:
 *           // ask user to accept key
 *           if( acceptHostKey ) {
 *               ssh.acceptKey(true);
 *           }
 *           break;
 *
 *       case KSshProcess::ERR_NEED_PASSWORD:
 *           // ask user for password
 *           ssh.password(userPassword);
 *           break;
 *       
 *       case KSshProcess::ERR_NEED_KEY_PASSPHRASE:
 *           // ask user for their key passphrase
 *           ssh.keyPassphrase(keyPassphrase);
 *           break;
 *
 *       default:
 *           // somethings wrong, alert user
 *           return;
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
        SSH_VER_MAX,
        UNKNOWN_VER
    };	

    /**
     * SSH options supported by KSshProcess.  Set SshOpt::opt to one of these
     * values.
     */
    // we cannot do this like UDSAtomType (ORing the type with the name) because
    // we have too many options for ssh and not enough bits.
    enum SshOptType {
        /**
         * Request server to invoke subsystem. (str)
         */
        SSH_SUBSYSTEM,
        /**
         * Connect to port on the server. (num)
         */
        SSH_PORT,
        /**
         * Connect to host. (str)
         */
        SSH_HOST,
        /**
         * connect using this username. (str)
         */
        SSH_USERNAME,
        /** 
         * connect using this password. (str)
         */
        SSH_PASSWD,
        /**
         * connect using this version of the SSH protocol. num == 1 or 2
         */
        SSH_PROTOCOL,
        /**
         * whether to forward X11 connections. (boolean)
         */
        SSH_FORWARDX11,
        /**
         * whether to do agent forwarding. (boolean)
         */
        SSH_FORWARDAGENT,
        /**
         * use as escape character. 0 for none  (num)
         */
        SSH_ESCAPE_CHAR,
        /**
         * command for ssh to perform once it is connected (str)
         */
        SSH_COMMAND,
        /**
         * Set ssh verbosity. This may be added multiple times. It may also cause KSSHProcess
         * to fail since we don't understand all the debug messages.
         */
        SSH_VERBOSE,
        /**
         * Set a ssh option as one would find in the ssh_config file
         * The str member should be set to 'optName value'
         */
        SSH_OPTION,
        /**
         * Set some other option not supported by KSSHProcess. The option should
         * be specified in the str member of SshOpt. Careful with this since
         * not all versions of SSH support the same options.
         */
        SSH_OTHER,
        SSH_OPT_MAX // always last
    }; // that's all for now

    /**
     * Errors that KSshProcess can encounter.  When a member function returns
     * false, call error() to retrieve one of these error codes.
     */
    enum SshError {
        /**
         * Don't recognize the ssh version
         */
        ERR_UNKNOWN_VERSION,
        /**
         * Cannot lauch ssh client
         */
        ERR_CANNOT_LAUNCH,
        /**
         * Interaction with the ssh client failed. This happens when we can't
         * find the password prompt or something similar
         */
        ERR_INTERACT,
        /**
         * Arguments for both a remotely executed subsystem and command were provide.
         * Only one or the other may be used
         */
        ERR_CMD_SUBSYS_CONFLICT,
        /**
         * No password was supplied
         */
        ERR_NEED_PASSWD,
        /**
         * No passphrase was supplied.
         */
        ERR_NEED_PASSPHRASE,
        /**
         * No usename was supplied
         */
        ERR_NEED_USERNAME,
        /**
         * Timed out waiting for a response from ssh or the server
         */
        ERR_TIMED_OUT,
        /**
         * Internal error, probably from a system call
         */
        ERR_INTERNAL,
        /**
         * ssh was disconnect from the host
         */
        ERR_DISCONNECTED,
        /**
         * No ssh options have been set. Call setArgs() before calling connect.
         */
        ERR_NO_OPTIONS,
        /**
         * A host key was received from an unknown host. 
         * Call connect() with the acceptHostKey argument to accept the key.
         */
        ERR_NEW_HOST_KEY,
        /**
         * A host key different from what is stored in the user's known_hosts file
         * has be received. This is an indication of an attack
         */
        ERR_DIFF_HOST_KEY,
        /**
         * A new or different host key was rejected by the caller. The ssh
         * connection was terminated and the ssh process killed.
         */
        ERR_HOST_KEY_REJECTED,
        /**
         * An invalid option was found in the SSH option list
         */
        ERR_INVALID_OPT,
        /**
         * SSH accepted host key without prompting user.
         */
        ERR_ACCEPTED_KEY,
        /**
         * Authentication failed
         */
        ERR_AUTH_FAILED,
        /**
         * Authentication failed because a new host key was detected and 
         * SSH is configured with strict host key checking enabled.
         */
        ERR_AUTH_FAILED_NEW_KEY,
        /**
         * Authentication failed because a changed host key was detected and 
         * SSH is configured with strict host key checking enabled.
         */
        ERR_AUTH_FAILED_DIFF_KEY,
        /**
         * The remote host closed the connection for unknown reasons.
         */
        ERR_CLOSED_BY_REMOTE_HOST,
        /**
         * We have no idea what happened
         */
        ERR_UNKNOWN,
        /**
         * The connect state machine entered an invalid state.
         */
        ERR_INVALID_STATE,
        ERR_MAX
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
     * Set the ssh binary KSshProcess should use. This will only affect the
     * next ssh connection attempt using this instance.
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
    SshVersion version();

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

    QString errorMsg() { return mErrorMsg; }

    /**
     * Send a signal to the ssh process. Do not use this to end the
     * ssh connection as it will not correctly reset the internal
     * state of the KSshProcess object.  Use KSshProcess::disconnect()
     * instead.
     *
     * @param signal The signal to send to the ssh process. See 'kill -l'
     *               for a list of possible signals.
     *               The default signal is SIGTERM which kills ssh.
     *
     */
    void kill(int signal = SIGTERM);

    /**
     * The pid of the ssh process started by this instance of KSshProcess.
     * Only valid if KSshProcess::running() returns true;
     * 
     * @return The pid of the running ssh process.
     */
    int pid() { return ssh.pid(); }
    
    /**
     * Whether a ssh connection has been  established with a
     * remote host.  A establish connection means ssh has successfully
     * authenticated with the remote host and user data can be transfered
     * between the local and remote host.  This cannot return
     * true unless the most recent call to KSshProccess::connect() returned true.
     *
     * @return True if a ssh connection has been established with a remote
     *         host. False otherwise.
     */
    bool connected() { return mConnected; }

    /**
     * Whether a ssh process is currently running.  This  only indicates
     * if a ssh process has been started and is still running.  It does not
     * tell if authentication has been successful.  This may return true
     * even if the most recent call to KSshProcess::connect() returned false.
     *
     * @return True if a ssh process started by this instance of KSshProcess
     *         is running. False otherwise.
     */
    bool running() { return mRunning; }
    
    /**
     * Print the command line arguments ssh is run with using kdDebug.
     */
    void printArgs();

    /**
     * Set the SSH options.
     * This must be called before connect().  See SshOptType for a list of
     * supported ssh options.  The required options are SSH_USERNAME 
     * and SSH_HOST.
     *
     * To reset the saved options, just recall setOptions() again with
     * a different options list.
     *
     * @param opts A list of SshOpt objects specifying the ssh options.
     *
     * @return True if all options are valid. False if unrecognized options
     *         or a required option is missing. Call error()
     *         for details.
     *
     */
    bool setOptions(const SshOptList& opts);

    /**
     * Create a ssh connection based on the options provided by setOptions().
     * Sets one of the following error codes on failure:
     * <ul>
     * <li>ERR_NO_OPTIONS</li>
     * <li>ERR_CANNOT_LAUNCH</li>
     * <li>ERR_INVALID_STATE</li>
     * <li>ERR_NEED_PASSWD</li>
     * <li>ERR_AUTH_FAILED</li>
     * <li>ERR_NEW_HOST_KEY</li>
     * <li>ERR_KEY_ACCEPTED</li>
     * <li>ERR_DIFF_HOST_KEY</li>
     * <li>ERR_INTERNAL</li>
     * <li>ERR_INTERACT</li>
     * </ul>
     *
     * @param acceptHostKey When true KSshProcess will automatically accept
     *                      unrecognized or changed host keys.
     *
     * @return True if the ssh connection is successful. False if the connection
     *         fails.  Call error() to get the reason for the failure.
     */
    bool connect();


    /**
     * Disconnect ssh from the host.  This kills the ssh process and
     * resets the internal state of this KSshProcess object. After a 
     * disconnect, the same KSshProcess can be used to connect to a
     * host.
     */
    void disconnect();
    
    /**
     * Call to respond to a ERR_NEW_HOST_KEY or ERR_DIFF_HOST_KEY error.
     * 
     * @param accept True to accept the host key, false to not accept the
     *               host key and kill ssh.
     * 
     */
    void acceptHostKey(bool accept);

    /**
     * Call to respond to a ERR_NEED_PASSWD or ERR_NEED_PASSPHRASE error.
     *
     * @param password The user password to give ssh.
     */
    void setPassword(QString password);
     
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
    /**
     * Path the the ssh binary.
     */
    QString mSshPath;
    
    /**
     * SSH version.  This is an index into the supported SSH 
     * versions array, and the various messages arrays.
     */
    SshVersion mVersion;

    /**
     * User's password.  Zero this out when it is no longer needed.
     */
    QString mPassword;
    
    /**
     * User's username.
     */
    QString mUsername;
    
    /**
     * Name of host we are connecting to.
     */
    QString mHost;

    /**
     * Accept new or changed host keys if true.
     */
    bool mAcceptHostKey;
    
    /**
     * Flag to tell use if we have an open, authenticated ssh
     * session going.
     */
    bool mConnected;
    
    /**
     * Flag to tell us if we have started a ssh process, we use this
     * to make sure we kill ssh before going away.
     */
    bool mRunning;

    /**
     * Save any key fingerprint msg from ssh so we can present
     * it to the caller.
     */
    QString mKeyFingerprint;

    /**
     * The location of the known host key file. We grab this from
     * any error messages ssh prints out.
     */
    QString mKnownHostsFile;

    /**
     * The state of our connect state machine.
     */
    int mConnectState;
    
    /**
     * Port on on which the target ssh server is listening.
     */
    int mPort;

    /**
     * The last error number encountered. This is only valid for the
     * last error.
     */
    SshError mError;

    /**
     * An error message that corresponds to the error number set in
     * mError.  Optional.
     */
    QString mErrorMsg;
    
    /**
     * Interface to the SSH process we ceate.  Handles communication
     * to and from the SSH process using stdin, stdout, stderr, and
     * pty.
     */
    MyPtyProcess ssh;

    /**
     * List of arguments we start SSH with.
     */
    QCStringList mArgs;
    void init();

    QString getLine();
    
    static const char * const versionStrs[];
    static const char * const passwordPrompt[];
    static const char * const passphrasePrompt[];
    static const char * const authSuccessMsg[];
    static const char * const authFailedMsg[];
    static const char * const hostKeyMissingMsg[];
    static const char * const hostKeyChangedMsg[];
    static const char * const continuePrompt[];
    static const char * const hostKeyAcceptedMsg[];
    static const char * const tryAgainMsg[];
    static const char * const hostKeyVerifyFailedMsg[];
    static const char * const connectionClosedMsg[];
    static QRegExp keyFingerprintMsg[];
    static QRegExp knownHostsFileMsg[];
};
#endif
