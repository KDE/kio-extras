# kio-sftp: KIO worker to access servers via SFTP

This KIO worker enables KDE applications to access files stored on hosts that provide access to them via SFTP. SFTP is a subsystem of SSH.


## Usage
To access the files on a host, use the protocol scheme "sftp://" in the address bar or file selector of any KDE application.
Example:

```sftp://example.com/home/user/file.txt```


## Prerequisites
The remote host must have the sftp subsystem enabled. For OpenSSH, this can be accomplished with the following line in `/etc/ssh/sshd_config`:
```Subsystem  sftp  /usr/lib/openssh/sftp-server```
Consult the respective manual for options and/or configuring other ssh servers.


## Caveats

### libssh
The kio-sftp component uses [libssh](https://www.libssh.org/) to provide the sftp implementation. This is a separate project from [OpenSSH](https://www.openssh.org/) and also not to be confused with [libssh2](https://libssh2.org/).

Libssh uses the same config files used by OpenSSH (`/etc/ssh/ssh_config` and `~/.ssh/config`), but unfortunately is not completely compatible with all config options. It may silently ignore unsupported options or implement them slightly different. Most notably:
  - Connection multiplexing is not supported
  - The `ProxyJump` handling is buggy in a lot of scenario's. As a workaround, set the environment variable `OPENSSH_PROXYJUMP=1` in your graphical login session to use OpenSSH for ProxyJump instead of libssh.

If you experience problems with kio-sftp, please try another libssh based sftp client like [yafc](https://github.com/sebastinas/yafc) to try to determine if the problem is within kio-sftp or libssh.

### Permissions
kio-sftp tries to mimic file permissions handling the same as on local filesystems.
  * For newly created files and directories, the owner and group are set to the user used to login to the (remote) host. The requested permissions are 0777 (rwxrwxrwx) for directories and 0666 (rw-rw-rw-) for files.
    The final permissions will be set by the (remote) OS and depends on the ACL's on the (remote) filesystem and the [umask](https://man7.org/linux/man-pages/man2/umask.2.html) of the (remote) user.
  * For copied new files, the owner and group are set to the remote user used to login to the server. The permissions will be set to the permissions of the source file, if possible.
  * For overwritten files (copied or not), the owner, group and permissions of the existing, overwritten, file will be kept.

If the permissions of a new file on the (remote) host are other than expected, check the filesystem's ACL and umask of the user on that host.


## Bugs
Please report all bugs to the [KDE Bugzilla](https://bugs.kde.org) using the product "kio-extras" with the component "SFTP".

Before reporting, make sure the issue is not covered by any of the caveats mentioned above, is not a bug in libssh and there is not already a bug report covering the issue.


# For developers

### Permission types
The code has to cater for multiple API's each having their own types for file permissions. The following list tries to shed some light on them.
  - `mode_t`: This is the (native) POSIX-compatible type used by system calls, libc and libssh. It is usually implemented as an unsigned int. The `mode_t` type includes file-type and permission bits, kio-sftp only uses the latter. The constants of this type have names like `S_IRUSR`, `S_IRWXG`.
  - `int`: This is the type used by KIO to hold permissions. This is basically the same as `mode_t`, but allows for negative values. The only valid negative value is `-1`, which functions as a sentinel value to indicate that [no special permission mode is set](https://api.kde.org/kio-workerbase.html#put). This translates to "use default permissions" in kio-sftp.
  - `std::filesystem::perms`: This is the C++ abstraction of permissions. Again, mostly the same as `mode_t`. It has the added functionality of abstracting away incompatible modes of platforms that have other permission models (windows et al.). It is used in kio-sftp just for this last feature. The constants for this type have names like `perms::owner_read`, `perms::group_all`.
  - `std::optional<std::filesystem::perms>`Same as the previous, but wrapped in a `std::optional`. This is used by kio-sftp to convert from the `int` permissions, by having no value if the `int`-type permission is -1.
  - `QFileDevice::Permissions`: Qt's own variant of permissions flags. Uses hexadecimal variants of posix's octal flags. The constants of this type have names like `QFileDevice::ReadUser`. It is used to set permissions of local files after copying.

So, `int` is used in the KIO API implementation, kio-sftp convert this to an `optional<perms>` for internal use, and the `value()` part of that gets converted to `mode_t` when calling into libc or libssh.

