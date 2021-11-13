// SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
// SPDX-License-Identifier: LGPL-2.0-or-later

// clang-format off
#define EXPORT_THUMBNAILER_WITH_JSON(className, json) \
extern "C" \
{\
    Q_DECL_EXPORT ThumbCreator *new_creator()\
    {\
        return new className;\
    }\
}\
\
class KIOPluginForMetaData : public QObject\
{\
    Q_OBJECT\
    Q_PLUGIN_METADATA(IID "blaaa" FILE json)\
};\
