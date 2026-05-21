/*
 * SPDX-FileCopyrightText: 2026 Ian Monroe <imonroe@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 * LD_PRELOAD mock for getpwuid_r, ultimately for overriding known hosts file for libssh
 */

#include <dlfcn.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>

#include <algorithm>
#include <string_view>

using GetpwuidRFunc = int (*)(uid_t, struct passwd *, char *, size_t, struct passwd **);
static GetpwuidRFunc real_getpwuid_r = nullptr;

static thread_local char home_buf[4096];

extern "C" {

__attribute__((visibility("default"))) int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result)
{
    if (!real_getpwuid_r) {
        *reinterpret_cast<void **>(&real_getpwuid_r) = dlsym(RTLD_NEXT, "getpwuid_r");
        if (!real_getpwuid_r) {
            *reinterpret_cast<void **>(&real_getpwuid_r) = dlsym(RTLD_NEXT, "__getpwuid_r");
        }
        if (!real_getpwuid_r) {
            *result = nullptr;
            return ENOENT;
        }
    }

    if (const int rc = real_getpwuid_r(uid, pwd, buf, buflen, result); rc != 0 || *result == nullptr) {
        return rc;
    }

    if (const char *env_val = std::getenv("KIO_SFTP_TEST_HOME"); env_val && uid == getuid()) {
        const std::string_view fake_home{env_val};
        const std::size_t len = std::min(fake_home.size(), sizeof(home_buf) - 1);
        std::copy_n(fake_home.begin(), len, home_buf);
        home_buf[len] = '\0';
        pwd->pw_dir = home_buf;
    }

    return 0;
}

} // extern "C"
