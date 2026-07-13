// SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Dialogs

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCMUtils
import org.kde.ki18n

import org.kde.private.kcms.proxy as Private

KCMUtils.SimpleKCM {
    id: root

    signal autoDetect

    readonly property KI18nContext i18nContext: KI18nContext {
        translationDomain: "kio-extras_kcms"
    }

    QQC2.ButtonGroup {
        buttons: [noRadio, detectRadio, autoConfigurationRadio, useSystemRadio, manualRadio]
    }

    Kirigami.Form {
        Kirigami.FormEntry {
            contentItem: QQC2.RadioButton {
                id: noRadio
                text: i18nContext.i18nc("@label", "No proxy")
                checked: kcm.proxySettings.proxyType === Private.KCM.NoProxy
                onClicked: if (checked) kcm.proxySettings.proxyType = Private.KCM.NoProxy

                KCMUtils.SettingStateBinding {
                    configObject: kcm.proxySettings
                    settingName: "ProxyType"
                }
            }
        }
        Kirigami.FormEntry {
            contentItem: RowLayout {
                QQC2.RadioButton {
                    id: detectRadio
                    text: i18nContext.i18nc("@label", "Detect proxy configuration automatically")
                    checked: kcm.proxySettings.proxyType === Private.KCM.WPADProxy
                    onClicked: if (checked) kcm.proxySettings.proxyType = Private.KCM.WPADProxy

                    KCMUtils.SettingStateBinding {
                        configObject: kcm.proxySettings
                        settingName: "ProxyType"
                    }
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18nContext.i18nc("@info", "Automatic Configuration, also known as Web Proxy Auto-Discovery (WPAD), is almost never the correct configuration. Enabling it when it is not required may slow down network access and make your network less secure. Your network administrator will know whether enabling it is required.")
                }
            }
        }
        Kirigami.FormEntry {
            contentItem: QQC2.RadioButton {
                id: autoConfigurationRadio
                text: i18nContext.i18nc("@label", "Use proxy auto configuration URL:")
                checked: kcm.proxySettings.proxyType === Private.KCM.PACProxy
                onClicked: if (checked) kcm.proxySettings.proxyType = Private.KCM.PACProxy
                KCMUtils.SettingStateBinding {
                    configObject: kcm.proxySettings
                    settingName: "ProxyType"
                }
            }
        }
        Kirigami.FormEntry {
            visible: autoConfigurationRadio.checked
            contentItem: RowLayout {
                QQC2.TextField {
                    id: pacUrl
                    Layout.fillWidth: true
                    text: kcm.proxySettings.proxyConfigScript
                    onTextChanged: kcm.proxySettings.proxyConfigScript = text
                }
                QQC2.Button {
                    icon.name: "document-open"
                    text: i18nContext.i18nc("@label:button", "Choose file")
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: fileDialog.open()
                    display: QQC2.Button.IconOnly
                }
            }
        }
        Kirigami.FormEntry {
            contentItem: RowLayout {
                QQC2.RadioButton {
                    id: useSystemRadio
                    text: i18nContext.i18nc("@label", "Use system proxy configuration:")
                    Layout.fillWidth: true
                    checked: kcm.proxySettings.proxyType === Private.KCM.EnvVarProxy
                    onClicked: if (checked) kcm.proxySettings.proxyType = Private.KCM.EnvVarProxy
                    KCMUtils.SettingStateBinding {
                        configObject: kcm.proxySettings
                        settingName: "ProxyType"
                    }
                }
                QQC2.Button {
                    text: i18nContext.i18nc("@action:button", "Auto detect")
                    onClicked: root.autoDetect()
                }
            }
        }

        SystemProxyConfiguration {
            label: i18nContext.i18nc("@label", "HTTP proxy:")
            envs: kcm.httpEnvs
            configKey: "httpProxy"
        }

        SystemProxyConfiguration {
            label: i18nContext.i18nc("@label", "SSL proxy:")
            envs: kcm.httpsEnvs
            configKey: "httpsProxy"
        }

        SystemProxyConfiguration {
            label: i18nContext.i18nc("@label", "FTP proxy:")
            envs: kcm.ftpEnvs
            configKey: "ftpProxy"
        }

        SystemProxyConfiguration {
            label: i18nContext.i18nc("@label", "SOCKS proxy:")
            envs: kcm.socksEnvs
            configKey: "socksProxy"
        }

        SystemProxyConfiguration {
            label: i18nContext.i18nc("@label", "Exceptions:")
            envs: kcm.noProxyEnvs
            configKey: "noProxyFor"
        }

        Kirigami.FormEntry {
            visible: useSystemRadio.checked
            contentItem: QQC2.CheckBox {
                id: showEnvValue
                text: i18nContext.i18nc("@label", "Show the value of the environment variables")
            }
        }

        Kirigami.FormEntry {
            contentItem: QQC2.RadioButton {
                id: manualRadio
                text: i18nContext.i18nc("@label", "Use manually specified proxy configuration:")
                checked: kcm.proxySettings.proxyType === Private.KCM.ManualProxy
                onClicked: if (checked) kcm.proxySettings.proxyType = Private.KCM.ManualProxy

                KCMUtils.SettingStateBinding {
                    configObject: kcm.proxySettings
                    settingName: "ProxyType"
                }
            }
        }

        ManualProxyConfiguration {
            id: manualHttp
            visible: manualRadio.checked
            ignoreGlobal: true
            title: i18nContext.i18nc("@title", "HTTP Proxy")
            value: kcm.proxySettings.httpProxy
            settingName: "httpProxy"

            Kirigami.FormEntry {
                contentItem: QQC2.CheckBox {
                    id: useGlobalCheckBox
                    enabled: manualHttp.urlValue.length > 0
                    text: i18nContext.i18nc("@label", "Use this proxy server for all protocols")
                    Component.onCompleted: useGlobalCheckBox.checked = (kcm.proxySettings.httpProxy === kcm.proxySettings.httpsProxy
                        && kcm.proxySettings.httpProxy === kcm.proxySettings.ftpProxy
                        && kcm.proxySettings.httpProxy === kcm.proxySettings.socksProxy)
                }
            }
        }

        ManualProxyConfiguration {
            id: manualSsl
            visible: manualRadio.checked
            title: i18nContext.i18nc("@title", "SSL Proxy")
            value: kcm.proxySettings.httpsProxy
            settingName: "httpsProxy"
        }

        ManualProxyConfiguration {
            id: manualFtp
            visible: manualRadio.checked
            title: i18nContext.i18nc("@title", "FTP Proxy")
            value: kcm.proxySettings.ftpProxy
            settingName: "ftpProxy"
        }

        ManualProxyConfiguration {
            id: manualSocks
            visible: manualRadio.checked
            title: i18nContext.i18nc("@title", "SOCKS Proxy")
            value: kcm.proxySettings.socksProxy
            settingName: "socksProxy"
        }

        Kirigami.FormEntry {
            visible: manualRadio.checked
            contentItem: QQC2.TextField {
                id: exceptions
                Kirigami.FormData.label: i18nc("@label", "Exceptions:")
                text: kcm.proxySettings.noProxyFor
                onTextChanged: kcm.proxySettings.noProxyFor = text

                KCMUtils.SettingStateBinding {
                    configObject: kcm.proxySettings
                    settingName: "NoProxyFor"
                }
            }
        }
        Kirigami.FormEntry {
            visible: manualRadio.checked
            contentItem: QQC2.CheckBox {
                enabled: exceptions.text.length > 0
                checked: kcm.proxySettings.useReverseProxy
                onCheckedChanged: kcm.proxySettings.useReverseProxy = checked
                text: i18nContext.i18nc("@label", "Use proxy settings only for addresses in the exceptions list")

                KCMUtils.SettingStateBinding {
                    configObject: kcm.proxySettings
                    settingName: "UseReverseProxy"
                }
            }
        }
    }

    component ManualProxyConfiguration : Kirigami.FormGroup {
        id: configuration

        property string value
        property bool ignoreGlobal: false
        required property string settingName

        readonly property string urlValue: urlField.text
        readonly property int portValue: portSpinBox.value

        function updateValue(): void  {
            if (manualRadio.checked) {
                if (urlField.text.length > 0) {
                    configuration.value = urlField.text + " " + portSpinBox.value;
                } else {
                    configuration.value = "";
                }
                kcm.proxySettings[configuration.settingName] = value;
            }
        }

        Kirigami.FormEntry {
            contentItem: QQC2.TextField {
                id: urlField
                enabled: !useGlobalCheckBox.checked  || configuration.ignoreGlobal
                Binding {
                    when: useGlobalCheckBox.checked && !configuration.ignoreGlobal
                    urlField.text: manualHttp.urlValue
                }
                text: configuration.value.split(" ")[0]
                onTextChanged: configuration.updateValue()
                Kirigami.FormData.label: i18nContext.i18nc("@label", "Url:")

                KCMUtils.SettingStateBinding {
                    configObject: kcm.proxySettings
                    settingName: configuration.settingName
                }
            }
        }
        Kirigami.FormEntry {
            contentItem: QQC2.SpinBox {
                id: portSpinBox

                enabled: !useGlobalCheckBox.checked || configuration.ignoreGlobal
                Binding {
                    when: useGlobalCheckBox.checked && !configuration.ignoreGlobal
                    portSpinBox.value: manualHttp.portValue
                }
                Kirigami.FormData.label: i18nContext.i18nc("@label", "Port:")
                from: 1
                to: 65535
                value: parseInt(configuration.value.split(" ")[1])
                onValueChanged: configuration.updateValue()

                Connections {
                    target: kcm.proxySettings
                    function onProxyTypeChanged(): void {
                        portSpinBox.valueChanged();
                    }
                }

                KCMUtils.SettingStateBinding {
                    configObject: kcm.proxySettings
                    settingName: configuration.settingName
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        nameFilters: [i18nContext.i18nc("@label", "Proxy auto-configuration files (*.pac)")]
        fileMode: FileDialog.OpenFile
        onAccepted: pacUrl.text = selectedFile.toString().slice("file://".length)
    }

    component SystemProxyConfiguration : Kirigami.FormEntry {
        id: systemProxy

        required property string label
        required property string configKey
        required property list<string> envs

        Component.onCompleted: if (useSystemRadio.checked) {
            textField.value = kcm.proxySettings[configKey]
            textField.resolvedValue = kcm.autoDetectResolve(systemProxy.envs);
        }

        visible: useSystemRadio.checked
        contentItem: QQC2.TextField {
            id: textField
            Kirigami.FormData.label: systemProxy.label
            property string resolvedValue
            property string value

            onTextChanged: if (!showEnvValue.checked) {
                value = text;
                kcm.proxySettings[systemProxy.configKey] = textField.text
            }

            text: showEnvValue.checked ? resolvedValue : value
            enabled: !showEnvValue.checked

            Connections {
                target: kcm.proxySettings
                function onProxyTypeChanged(): void {
                    if (kcm.proxySettings.proxyType === 4) {
                        kcm.proxySettings[systemProxy.configKey] = textField.value
                    }
                }
            }
        }

        Connections {
            target: root
            function onAutoDetect(): void {
                textField.value = kcm.autoDetect(systemProxy.envs);
                textField.resolvedValue = kcm.autoDetectResolve(systemProxy.envs);
            }
        }
    }
}
