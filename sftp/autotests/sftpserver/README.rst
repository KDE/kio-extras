# SPDX-FileCopyrightText: 2011-2017 Ruslan Spivak
# SPDX-FileCopyrightText: 2020 Steven Fernandez <steve@lonetwin.net>
#
# SPDX-License-Identifier: MIT

sftpserver
==========

``sftpserver`` is a skeletal SFTP server written using `Paramiko`_

This project exists to serve as a starting point / demonstration of how to build
an SFTP server or as something to be used in tests. As such the goal is to *not*
provide a full featured sftp server.

This was initially a simple fork of `@rspivak`'s `sftpserver`_, which in turn
was an adaptation of the code from Paramiko's tests. However, I updated it
further to demonstrate the use of different `threaded` and `forked` modes of
operation after Martin Haack reached out to me with some questions.

...and now this is a minor kio-extras fork. Just needed some tweaks to work for our
integration tests.


Installation
------------
::

    $ pip install git+https://github.com/lonetwin/sftpserver.git@master


Examples
--------
::

    # run sftpserver with defaults (serving current dir, at
    # localhost:3373, in threaded mode, with log level set to DEBUG)

    $ python -m sftpserver

    # run sftpserver with defaults (serving dir /tmp, at
    # localhost:3373, in forked mode, using server key /tmp/test_rsa.key)

    $ sftpserver -r /tmp -k /tmp/test_rsa.key -l DEBUG -m forked


Generating a test private key::

    $ openssl genrsa -out server.key.pem 4096               # generate a private key

Connecting with a Python client to our server::

    >>> import paramiko
    >>> pkey = paramiko.RSAKey.from_private_key_file('/tmp/test_rsa.key')
    >>> transport = paramiko.Transport(('localhost', 3373))
    >>> transport.connect(username='admin', password='admin', pkey=pkey)
    >>> sftp = paramiko.SFTPClient.from_transport(transport)
    >>> sftp.listdir('.')
    ['loop.py', 'stub_sftp.py']


.. _Paramiko: https://www.paramiko.org/
.. _sftpserver: https://github.com/rspivak/sftpserver
