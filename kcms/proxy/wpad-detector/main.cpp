// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include <libproxy/proxy.h>

#include <memory>

namespace std
{
template<>
struct default_delete<pxProxyFactory> {
    void operator()(pxProxyFactory *ptr) const
    {
        px_proxy_factory_free(ptr);
    }
};
} // namespace std

int main()
{
    auto factory = std::unique_ptr<pxProxyFactory>(px_proxy_factory_new());
    std::ignore = px_proxy_factory_get_proxies(factory.get(), "https://www.kde.org");

    return 0;
}
