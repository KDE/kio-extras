# SPDX-FileCopyrightText: 2011-2017 Ruslan Spivak
# SPDX-FileCopyrightText: 2020 Steven Fernandez <steve@lonetwin.net>
#
# SPDX-License-Identifier: MIT

__author__ = 'Steven Fernandez <steve@lonetwin.net>'

import argparse
import getpass
import logging
import os
try:
    import pwd
except ImportError:
    pwd = None

import socket
import sys

import paramiko

from sftpserver.stub_sftp import StubSFTPServer, ssh_server

# - Defaults
HOST, PORT = 'localhost', 3373
ROOT = StubSFTPServer.ROOT
LOG_LEVEL = logging.getLevelName(logging.INFO)
MODE = 'threaded'

BACKLOG = 10


def setup_logging(level, mode):
    if mode == 'threaded':
        log_format = logging.BASIC_FORMAT
    else:
        log_format = '%(process)d:' + logging.BASIC_FORMAT

    logging.basicConfig(format=log_format)

    # - setup paramiko logging
    paramiko_logger = logging.getLogger('paramiko')
    paramiko_logger.setLevel(logging.INFO)

    logger = logging.getLogger(__name__)
    logger.setLevel(level)
    return logger


def setup_transport(connection):
    transport = paramiko.Transport(connection)
    transport.add_server_key(StubSFTPServer.KEY)
    transport.set_subsystem_handler('sftp', paramiko.SFTPServer, StubSFTPServer)
    transport.start_server(server=ssh_server)
    return transport


def start_server(host=HOST, port=PORT, root=ROOT, keyfile=None, password=None, level=LOG_LEVEL, mode=MODE):
    logger = setup_logging(level, mode)

    if keyfile is None:
        # Use a fixed key file path so tests can add it to known_hosts
        default_keyfile = os.path.join(os.path.dirname(__file__), '..', '..', 'test_server_key')
        default_pubkey_file = default_keyfile + '.pub'
        if os.path.exists(default_keyfile):
            server_key = paramiko.RSAKey.from_private_key_file(default_keyfile)
            logger.debug('Loaded existing server key from %s', default_keyfile)
        else:
            server_key = paramiko.RSAKey.generate(bits=2048)
            # Save the key for future test runs
            try:
                server_key.write_private_key_file(default_keyfile)
                # Also save the public key in OpenSSH format for known_hosts
                with open(default_pubkey_file, 'w') as f:
                    f.write(f"{server_key.get_name()} {server_key.get_base64()}\n")
                logger.debug('Generated and saved new server key to %s', default_keyfile)
            except Exception as e:
                logger.warning('Could not save server key: %s', e)
    else:
        server_key = paramiko.RSAKey.from_private_key_file(keyfile, password=password)

    StubSFTPServer.ROOT = root
    StubSFTPServer.KEY = server_key

    logger.debug('Serving %s over sftp at %s:%s', root, host, port)

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
    server_socket.bind((host, port))
    server_socket.listen(BACKLOG)

    sessions = []
    while True:
        connection, _ = server_socket.accept()
        if mode == 'forked':
            logger.debug('Starting a new process')
            pid = os.fork()
            if pid == 0:
                try:
                    transport = setup_transport(connection)
                    channel = transport.accept()
                    if os.geteuid() == 0:
                        if pwd is not None:
                            user = pwd.getpwnam(transport.get_username())
                            logger.debug('Dropping privileges, will run as %s', user.pw_name)
                            os.setgid(user.pw_gid)
                            os.setuid(user.pw_uid)
                        else:
                            logger.debug('Dropping privileges is not supported on this platform '
                                         'will run as %s', os.getlogin())

                    transport.join()
                    logger.debug("session for %s has ended. Exiting", user.pw_name)
                except Exception as e:
                    logger.error('Error in child process: %s', e)
                finally:
                    sys.exit()
            else:
                sessions.append(pid)
                pid, _ = os.waitpid(-1, os.WNOHANG)
                if pid:
                    sessions.remove(pid)
        else:
            logger.debug('Starting a new thread')
            try:
                transport = setup_transport(connection)
                channel = transport.accept()
                sessions.append(channel)
            except Exception as e:
                logger.error('Error handling connection: %s', e)
                try:
                    connection.close()
                except:
                    pass

        logger.debug('%s active sessions', len(sessions))


def main():
    usage = """usage: sftpserver [options]"""
    parser = argparse.ArgumentParser(usage=usage)
    parser.add_argument(
        '--host', dest='host', default=HOST,
        help='listen on HOST [default: %(default)s]'
    )
    parser.add_argument(
        '-p', '--port', dest='port', type=int, default=PORT,
        help='listen on PORT [default: %(default)d]'
    )
    parser.add_argument(
        '-l', '--level', dest='level', default=LOG_LEVEL,
        help='Debug level: WARNING, INFO, DEBUG [default: %(default)s]'
    )
    parser.add_argument(
        '-k', '--keyfile', dest='keyfile', metavar='FILE',
        help='Path to private key, for example /tmp/test_rsa.key'
    )
    parser.add_argument(
        '-P', '--password', help='Prompt for keyfile password', action="store_true"
    )

    parser.add_argument(
        '-r', '--root', dest='root', default=ROOT,
        help='Directory to serve as root for the server'
    )
    parser.add_argument(
        '-m', '--mode', default=MODE, const=MODE, nargs='?', choices=('threaded', 'forked'),
        help='Mode to run server in [default: %(default)s]'
    )

    args = parser.parse_args()

    if not os.path.isdir(args.root):
        parser.print_help()
        sys.exit(-1)

    password = None
    if args.password:
        password = getpass.getpass("Password: ")

    start_server(args.host, args.port, args.root, args.keyfile, password, args.level, args.mode)


if __name__ == '__main__':
    main()
