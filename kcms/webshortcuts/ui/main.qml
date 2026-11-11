// SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import org.kde.kcmutils as KCMUtils
import org.kde.ki18n

KCMUtils.ScrollViewKCM {
    id: root

    readonly property KI18nContext i18nContext: KI18nContext {
        translationDomain: "kio-extras_kcms"
    }

    function openProviderDialog(row: int, name: string, query: string, keys: string, charset: string): void {
        const dialog = providerDialogComponent.createObject(root, {
            row: row,
            initialName: name,
            initialQuery: query,
            initialKeys: keys,
            initialCharset: charset,
        }) as Kirigami.Dialog;
        dialog.open();
    }

    header: ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Form {
            Layout.fillWidth: true

            Kirigami.FormGroup {
                Kirigami.FormEntry {
                    contentItem: QQC2.CheckBox {
                        text: root.i18nContext.i18nc("@option:check", "Enable web search keywords")
                        checked: kcm.webShortcutsEnabled
                        onToggled: kcm.webShortcutsEnabled = checked

                        KCMUtils.SettingHighlighter {
                            highlight: !kcm.webShortcutsEnabled
                        }
                    }
                    trailingItems: Kirigami.ContextualHelpButton {
                        toolTipText: root.i18nContext.xi18nc("@info:whatsthis",
                            "<para>Web search keywords allow you to quickly search or look up words on the Internet. "
                            + "They work in Plasma's <application>KRunner</application> search, "
                            + "<application>Konqueror</application>'s address bar, and in any other "
                            + "applications that support them. For example, to search for information "
                            + "about the KDE project using the Google search engine, you can simply type "
                            + "<icode>gg:KDE</icode> or <icode>google:KDE</icode>.</para>")
                    }
                }

                Kirigami.FormEntry {
                    contentItem: QQC2.CheckBox {
                        text: root.i18nContext.i18nc("@option:check", "Use preferred keywords only")
                        enabled: kcm.webShortcutsEnabled
                        checked: kcm.usePreferredOnly
                        onToggled: kcm.usePreferredOnly = checked

                        KCMUtils.SettingHighlighter {
                            highlight: kcm.usePreferredOnly
                        }
                    }
                    trailingItems: Kirigami.ContextualHelpButton {
                        toolTipText: root.i18nContext.i18nc("@info:tooltip",
                            "Applications that can only show a limited number of web search keywords (for example, Konqueror when text on a page is selected) will only show the preferred keywords.")
                    }
                }

                Kirigami.FormEntry {
                    contentItem: QQC2.ComboBox {
                        id: defaultProviderCombo

                        Kirigami.FormData.label: root.i18nContext.i18nc("@label:listbox", "Default keyword:")
                        enabled: kcm.webShortcutsEnabled
                        model: kcm.defaultProviderChoices
                        textRole: "text"
                        valueRole: "value"

                        function updateCurrentIndex(): void {
                            currentIndex = indexOfValue(kcm.defaultProvider);
                        }

                        onActivated: kcm.defaultProvider = currentValue
                        onModelChanged: updateCurrentIndex()
                        Component.onCompleted: updateCurrentIndex()

                        Connections {
                            target: kcm
                            function onDefaultProviderChanged(): void {
                                defaultProviderCombo.updateCurrentIndex();
                            }
                        }

                        KCMUtils.SettingHighlighter {
                            highlight: kcm.defaultProvider !== kcm.defaultDefaultProvider
                        }
                    }
                    trailingItems: Kirigami.ContextualHelpButton {
                        toolTipText: root.i18nContext.i18nc("@info:tooltip",
                            "This keyword may be used by applications which automatically search if the input cannot be recognized, for example in Konqueror's address bar. Select “None” to disable this feature.")
                    }
                }

                Kirigami.FormEntry {
                    contentItem: QQC2.ComboBox {
                        id: delimiterCombo
                        Kirigami.FormData.label: root.i18nContext.i18nc("@label:listbox", "Keyword delimiter:")
                        enabled: kcm.webShortcutsEnabled
                        textRole: "text"
                        valueRole: "value"
                        model: [
                            {
                                text: root.i18nContext.i18nc("@item:inlistbox Colon as keyword delimiter", "Colon"),
                                value: ":",
                            },
                            {
                                text: root.i18nContext.i18nc("@item:inlistbox Space as keyword delimiter", "Space"),
                                value: " ",
                            },
                        ]

                        onActivated: kcm.keywordDelimiter = currentValue
                        Component.onCompleted: currentIndex = indexOfValue(kcm.keywordDelimiter)

                        Connections {
                            target: kcm
                            function onKeywordDelimiterChanged(): void {
                                delimiterCombo.currentIndex = delimiterCombo.indexOfValue(kcm.keywordDelimiter);
                            }
                        }

                        KCMUtils.SettingHighlighter {
                            highlight: kcm.keywordDelimiter !== ":"
                        }
                    }
                }
            }
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing
            enabled: kcm.webShortcutsEnabled

            Kirigami.SearchField {
                Layout.fillWidth: true
                onTextChanged: kcm.providersModel.filterText = text
            }

            QQC2.Button {
                text: root.i18nContext.i18nc("@action:button", "Add Keyword…")
                icon.name: "list-add-symbolic"
                onClicked: root.openProviderDialog(-1, "", kcm.clipboardUrl(), "", "")
            }
        }
    }

    view: ListView {
        id: providersList

        enabled: kcm.webShortcutsEnabled
        clip: true
        reuseItems: true
        model: kcm.providersModel

        delegate: QQC2.ItemDelegate {
            id: delegate

            required property var model
            required property int index

            width: ListView.view.width
            hoverEnabled: true
            down: false
            highlighted: false

            QQC2.ToolTip.text: delegate.model.host
            QQC2.ToolTip.visible: delegate.hovered && delegate.model.host.length > 0
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

            onClicked: root.openProviderDialog(delegate.index, delegate.model.name, delegate.model.query, delegate.model.keys.join(","), delegate.model.charset)

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.CheckBox {
                    checked: delegate.model.preferred
                    onToggled: delegate.model.preferred = checked

                    QQC2.ToolTip.text: root.i18nContext.i18nc("@info:tooltip", "Mark this web search keyword as preferred")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    Accessible.name: root.i18nContext.i18nc("@option:check accessible name, %1 is the name of a search provider", "Prefer %1", delegate.model.name)
                }

                Kirigami.Icon {
                    source: delegate.model.iconName
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                }

                KD.TitleSubtitle {
                    Layout.fillWidth: true
                    title: delegate.model.name
                    subtitle: delegate.model.keys.join(", ")
                    selected: delegate.highlighted
                    font: delegate.font
                }

                QQC2.ToolButton {
                    icon.name: "document-edit-symbolic"
                    text: root.i18nContext.i18nc("@action:button", "Edit Keyword…")
                    display: QQC2.AbstractButton.IconOnly
                    onClicked: delegate.clicked()

                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }

                QQC2.ToolButton {
                    icon.name: "edit-delete-symbolic"
                    text: root.i18nContext.i18nc("@action:button", "Delete Keyword")
                    display: QQC2.AbstractButton.IconOnly
                    onClicked: kcm.removeProvider(delegate.index)

                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: providersList.count === 0
            icon.name: "edit-none"
            text: root.i18nContext.i18nc("@info:placeholder", "No web search keywords found")
        }
    }

    Component {
        id: providerDialogComponent

        Kirigami.Dialog {
            id: dialog

            // Row in kcm.providersModel, -1 for a new provider
            required property int row
            required property string initialName
            required property string initialQuery
            required property string initialKeys
            required property string initialCharset

            readonly property bool isNew: row < 0
            readonly property string conflicts: kcm.keywordConflicts(keysField.text, row)
            readonly property bool acceptable: nameField.text.trim().length > 0 && queryField.text.trim().length > 0
                && keysField.text.replace(/[ ,]/g, "").length > 0 && conflicts.length === 0

            title: isNew ? root.i18nContext.i18nc("@title:window", "New Web Search Keyword")
                         : root.i18nContext.i18nc("@title:window", "Edit Web Search Keyword")
            preferredWidth: Kirigami.Units.gridUnit * 30
            padding: Kirigami.Units.largeSpacing

            standardButtons: Kirigami.Dialog.Cancel
            customFooterActions: Kirigami.Action {
                text: dialog.isNew ? root.i18nContext.i18nc("@action:button", "Add") : root.i18nContext.i18nc("@action:button", "Save")
                icon.name: dialog.isNew ? "list-add-symbolic" : "document-save-symbolic"
                enabled: dialog.acceptable
                onTriggered: dialog.accept()
            }

            onAccepted: {
                const charset = charsetCombo.currentIndex > 0 ? charsetCombo.currentText : "";
                if (isNew) {
                    kcm.addProvider(nameField.text, queryField.text, keysField.text, charset);
                } else {
                    kcm.changeProvider(row, queryField.text, keysField.text, charset);
                }
            }
            onClosed: destroy()
            onOpened: (isNew ? nameField : queryField).forceActiveFocus()

            ColumnLayout {
                spacing: Kirigami.Units.largeSpacing

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    type: Kirigami.MessageType.Error
                    visible: dialog.conflicts.length > 0
                    text: dialog.conflicts
                }

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    type: Kirigami.MessageType.Warning
                    visible: queryField.text.length > 0 && !queryField.text.includes("\\{")
                    text: root.i18nContext.i18nc("@info %1 is a placeholder like \\{@}",
                        "The URL does not contain a %1 placeholder for the user query. This means that the same page is always going to be visited, regardless of the text typed in with the keyword.",
                        "\\{@}")
                }

                Kirigami.Form {
                    Layout.fillWidth: true

                    Kirigami.FormGroup {
                        Kirigami.FormEntry {
                            contentItem: QQC2.TextField {
                                id: nameField
                                Kirigami.FormData.label: root.i18nContext.i18nc("@label:textbox", "Name:")
                                enabled: dialog.isNew
                                text: dialog.initialName
                                Accessible.description: root.i18nContext.i18nc("@info:whatsthis", "The human-readable name of the web search keyword")
                            }
                        }

                        Kirigami.FormEntry {
                            contentItem: QQC2.TextField {
                                id: queryField
                                Kirigami.FormData.label: root.i18nContext.i18nc("@label:textbox", "URL:")
                                text: dialog.initialQuery
                                placeholderText: "https://example.org/search?q=\\{@}"
                            }
                            trailingItems: [
                                QQC2.ToolButton {
                                    icon.name: "edit-paste-symbolic"
                                    text: root.i18nContext.i18nc("@action:button", "Insert Query Placeholder")
                                    display: QQC2.AbstractButton.IconOnly
                                    onClicked: {
                                        queryField.insert(queryField.cursorPosition, "\\{@}");
                                        queryField.forceActiveFocus();
                                    }

                                    QQC2.ToolTip.text: text
                                    QQC2.ToolTip.visible: hovered
                                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                                },
                                Kirigami.ContextualHelpButton {
                                    toolTipText: root.i18nContext.i18nc("@info:tooltip",
                                        "<qt>Recommended is \\{@}, since it removes all query variables (name=value) from the resulting string, whereas \\{0} will be substituted with the unmodified query string.<br/>You can use \\{1} ... \\{n} to specify certain words from the query and \\{name} to specify a value given by 'name=value' in the user query.<br/>In addition it is possible to specify multiple references (names, numbers and strings) at once (\\{name1,name2,...,\"string\"}).<br/>The first matching value (from the left) will be used as the substitution value for the resulting URI.<br/>A quoted string can be used as the default value if nothing matches from the left of the reference list.</qt>")
                                }
                            ]
                        }

                        Kirigami.FormEntry {
                            contentItem: QQC2.TextField {
                                id: keysField
                                Kirigami.FormData.label: root.i18nContext.i18nc("@label:textbox", "Keywords:")
                                text: dialog.initialKeys
                                onTextEdited: {
                                    const position = cursorPosition;
                                    text = text.replace(/ /g, ",");
                                    cursorPosition = position;
                                }
                            }
                            trailingItems: Kirigami.ContextualHelpButton {
                                toolTipText: root.i18nContext.i18nc("@info:tooltip",
                                    "<qt>A comma-separated list of keywords. They can be used as a pseudo-URI scheme. For example, the keyword <b>av</b> can be used as in <b>av</b>:<b>my search</b></qt>")
                            }
                        }

                        Kirigami.FormEntry {
                            contentItem: QQC2.ComboBox {
                                id: charsetCombo
                                Kirigami.FormData.label: root.i18nContext.i18nc("@label:listbox", "Character set:")
                                model: [root.i18nContext.i18nc("@item:inlistbox The default character set", "Default")].concat(kcm.availableCharsets)
                                Component.onCompleted: currentIndex = dialog.initialCharset.length === 0 ? 0 : Math.max(0, kcm.availableCharsets.indexOf(dialog.initialCharset) + 1)
                                Accessible.description: root.i18nContext.i18nc("@info:whatsthis", "The character set that will be used to encode your search query")
                            }
                        }
                    }
                }
            }
        }
    }
}
