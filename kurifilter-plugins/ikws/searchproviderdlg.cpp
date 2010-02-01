/*
 * Copyright (c) 2000 Malte Starostik <malte@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "searchproviderdlg.h"
#include "searchprovider.h"

#include <kapplication.h>
#include <kcharsets.h>
#include <kmessagebox.h>


SearchProviderDialog::SearchProviderDialog(SearchProvider *provider, QWidget *parent)
    : KDialog( parent )
    , m_provider(provider)
{
    setModal(true);
    setButtons( Ok | Cancel );

    m_dlg.setupUi(mainWidget());

    showButtonSeparator(true);

    m_dlg.leQuery->setMinimumWidth(kapp->fontMetrics().averageCharWidth() * 50);

    connect(m_dlg.leName,      SIGNAL(textChanged(QString)), SLOT(slotChanged()));
    connect(m_dlg.leQuery,     SIGNAL(textChanged(QString)), SLOT(slotChanged()));
    connect(m_dlg.leShortcut,  SIGNAL(textChanged(QString)), SLOT(slotChanged()));

    // Data init
    QStringList charsets = KGlobal::charsets()->availableEncodingNames();
    charsets.prepend(i18n("Default"));
    m_dlg.cbCharset->addItems(charsets);
    if (m_provider)
    {
        setPlainCaption(i18n("Modify Search Provider"));
        m_dlg.leName->setText(m_provider->name());
        m_dlg.leQuery->setText(m_provider->query());
        m_dlg.leShortcut->setText(m_provider->keys().join(","));
        m_dlg.cbCharset->setCurrentIndex(m_provider->charset().isEmpty() ? 0 : charsets.indexOf(m_provider->charset()));
        m_dlg.leName->setEnabled(false);
        m_dlg.leQuery->setFocus();
    }
    else
    {
        setPlainCaption(i18n("New Search Provider"));
        m_dlg.leName->setFocus();
        enableButton(Ok, false);
    }
}

void SearchProviderDialog::slotChanged()
{
    enableButton(Ok, !(m_dlg.leName->text().isEmpty()
                       || m_dlg.leShortcut->text().isEmpty()
                       || m_dlg.leQuery->text().isEmpty()));
}


void SearchProviderDialog::slotButtonClicked(int button) {
    if (button == KDialog::Ok) {
        if ((m_dlg.leQuery->text().indexOf("\\{") == -1)
            && KMessageBox::warningContinueCancel(0,
                i18n("The URI does not contain a \\{...} placeholder for the user query.\n"
                    "This means that the same page is always going to be visited, "
                    "regardless of what the user types."),
                QString(), KGuiItem(i18n("Keep It"))) == KMessageBox::Cancel) {
            return;
        }
        
        if (!m_provider)
            m_provider = new SearchProvider;
        m_provider->setName(m_dlg.leName->text().trimmed());
        m_provider->setQuery(m_dlg.leQuery->text().trimmed());
        m_provider->setKeys(m_dlg.leShortcut->text().trimmed().toLower().split(',', QString::SkipEmptyParts));
        m_provider->setCharset(m_dlg.cbCharset->currentIndex() ? m_dlg.cbCharset->currentText() : QString());
        KDialog::accept();
    } else {
        KDialog::slotButtonClicked(button);
    }
}

#include "searchproviderdlg.moc"
