#include "ksshprocess.h"
#include <iostream>

int main(int argc, char *argv[]) {

    if( argc < 5 ) {
        cout << "Usage: " << argv[0] << 
            " <ssh path> <host> <username> <password>" << endl;
        return 1;
    }

    KSshProcess ssh(argv[1]);
    cout << ssh.version() << endl;
    
    KSshProcess::SshOptList opts;
    KSshProcess::SshOpt opt;

    opt.opt = KSshProcess::SSH_PORT;
    opt.num = 22;
    opts.append(opt);

    opt.opt = KSshProcess::SSH_HOST;
    opt.str = QString(argv[2]);
    opts.append(opt);

    opt.opt = KSshProcess::SSH_USERNAME;
    opt.str = QString(argv[3]);
    opts.append(opt);

//    opt.opt = KSshProcess::SSH_PASSWD;
//    opt.str = QString(argv[4]);
//   opts.append(opt);

    if( !ssh.setOptions(opts) ) {
        cout << "ksshprocesstest: setOptions failed" << endl;
        return -1;
    }

    ssh.printArgs();

    bool stop = false;
    bool connected;
    char buf[256];
    char c;
    while( !stop && !(connected = ssh.connect()) ) {
        cout << "ksshprocesstest: Error num - " << ssh.error() << endl;
        cout << "ksshprocesstest: Error msg - " << ssh.errorMsg().latin1() << endl;
        switch( ssh.error() ) {
            case KSshProcess::ERR_NEED_PASSWD:
            case KSshProcess::ERR_NEED_PASSPHRASE:
                cout << "Password: ";
                cin >> buf;
                cout << "password is " << buf << endl;
                ssh.setPassword(QString(buf));
                break;
            case KSshProcess::ERR_NEW_HOST_KEY:
            case KSshProcess::ERR_DIFF_HOST_KEY:
                cout << "Accept host key? (y/n): ";
                cin >> c;
                cout << "Answered " << c << endl;
                ssh.acceptHostKey(c == 'y' ? true : false);
                break;
            case KSshProcess::ERR_AUTH_FAILED:
                cout << "ksshprocesstest: auth failed." << endl;
                stop = true;
                break;
            case KSshProcess::ERR_AUTH_FAILED_NEW_KEY:
                cout << "ksshprocesstest: auth failed because of new key." << endl;
                stop = true;
                break;
            case KSshProcess::ERR_AUTH_FAILED_DIFF_KEY:
                cout << "ksshprocesstest: auth failed because of changed key." << endl;
                stop = true;
                break;
            
            case KSshProcess::ERR_INTERACT:
            case KSshProcess::ERR_INTERNAL:
            case KSshProcess::ERR_UNKNOWN:
            case KSshProcess::ERR_INVALID_STATE:
            case KSshProcess::ERR_CANNOT_LAUNCH:
            case KSshProcess::ERR_HOST_KEY_REJECTED:
                cout << "ksshprocesstest: FATAL ERROR" << endl;
                stop = true;
                break;
                
        }
    }

    if( connected ) {
        cout << "ksshprocesstest: Successfully connected to " << argv[2] << endl;
    }
    else {
        cout << "ksshprocesstest: Connect to " << argv[2] << " failed." << endl;
    }
    
}
