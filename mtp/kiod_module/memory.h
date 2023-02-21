// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <libmtp.h>

#include <memory>

namespace std
{
// Add a default deleter for LIBMTP_file_t. Means we do not have to repeat the deleter all over the place.
template<>
struct default_delete<LIBMTP_file_t> {
    void operator()(LIBMTP_file_t *ptr) const
    {
        LIBMTP_destroy_file_t(ptr);
    }
};
} // namespace std
