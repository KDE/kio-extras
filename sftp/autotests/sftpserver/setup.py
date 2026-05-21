# SPDX-FileCopyrightText: 2011-2017 Ruslan Spivak
# SPDX-FileCopyrightText: 2020 Steven Fernandez <steve@lonetwin.net>
#
# SPDX-License-Identifier: MIT


import os

from setuptools import setup, find_packages


classifiers = """\
Intended Audience :: Developers
License :: OSI Approved :: MIT License
Programming Language :: Python
Topic :: Internet :: File Transfer Protocol (FTP)
Operating System :: Unix
"""


def read(*rel_names):
    return open(os.path.join(os.path.dirname(__file__), *rel_names)).read()


setup(
    name='sftpserver',
    version='0.5',
    url='http://github.com/lonetwin/sftpserver',
    license='MIT',
    description='sftpserver - a skeletal SFTP server written using Paramiko',
    author='Steven Fernandez',
    author_email='steve@lonetwin.net',
    packages=find_packages('src'),
    package_dir={'': 'src'},
    install_requires=['setuptools>=0.7', 'paramiko'],
    zip_safe=False,
    entry_points="""\
    [console_scripts]
    sftpserver = sftpserver.__main__:main
    """,
    classifiers=filter(None, classifiers.split('\n')),
    long_description=read('README.rst'),
    extras_require={'test': []}
    )
