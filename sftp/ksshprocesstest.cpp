#include "ksshprocess.h"

#include <iostream>

void main() {
    KSSHProcess ssh("/usr/bin/ssh");
    cout << ssh.version() << endl;
    
    SshOptList opts;
    SshOpt opt;

    opt.opt = SSH_PORT;
    opt.num = 22;
    opts.append(opt);

    opt.opt = SSH_HOST;
    opt.str = "lcalhost";
    opts.append(opt);

    opt.opt = SSH_USERNAME;
    opt.str = "ljfisher";
    opts.append(opt);

    opt.opt = SSH_PASSWD;
    opt.str = "zap2run";
    opts.append(opt);

    ssh.setArgs(opts);
    ssh.printArgs();

    ssh.connect();
    while(1);
}
