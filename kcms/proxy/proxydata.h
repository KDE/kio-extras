// SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <KCModuleData>

#include "kioslave.h"

class ProxyData : public KCModuleData
{
    using KCModuleData::KCModuleData;
    bool isDefaults() const override;

private:
    ProxySettings m_settings;
};
