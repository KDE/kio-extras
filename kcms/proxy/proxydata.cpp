// SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "proxydata.h"
#include "proxy.h"

bool ProxyData::isDefaults() const
{
    // Not just m_settings.isDefaults(), since we don't remove the individual proxy settings when changing type no NoProxy
    return m_settings.proxyType() == Proxy::NoProxy;
}
