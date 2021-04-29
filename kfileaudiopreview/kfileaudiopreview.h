/* This file is part of the KDE libraries
   SPDX-FileCopyrightText: 2003 Carsten Pfeiffer <pfeiffer@kde.org>
   SPDX-FileCopyrightText: 2006 Matthias Kretz <kretz@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KFILEAUDIOPREVIEW_H
#define KFILEAUDIOPREVIEW_H

#include <kpreviewwidgetbase.h>
#include <phonon/phononnamespace.h>
#include <QVariantList>

class QCheckBox;
class QUrl;

/**
 * Audio "preview" widget for the file dialog.
 */
class KFileAudioPreview : public KPreviewWidgetBase
{
    Q_OBJECT

public:
    explicit KFileAudioPreview(QWidget *parent = nullptr,
                               const QVariantList &args = QVariantList());
    ~KFileAudioPreview() override;

public Q_SLOTS:
    void showPreview(const QUrl &url) override;
    void clearPreview() override;

private Q_SLOTS:
    void toggleAuto(bool on);
    void stateChanged(Phonon::State newState, Phonon::State oldState);

private:
    QCheckBox *m_autoPlay;

private:
    class Private;
    Private *d;
};

#endif // KFILEAUDIOPREVIEW_H
