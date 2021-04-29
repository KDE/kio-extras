/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 Matthias Kretz <kretz@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only

*/

#ifndef PHONON_MEDIACONTROLS_P_H
#define PHONON_MEDIACONTROLS_P_H

#include "mediacontrols.h"
#define TRANSLATION_DOMAIN "kfileaudiopreview5"
#include <klocalizedstring.h>
#include <phonon/volumeslider.h>
#include <phonon/seekslider.h>
#include <QToolButton>
#include <QBoxLayout>
#include <QStyle>

namespace Phonon
{
class MediaControlsPrivate
{
    Q_DECLARE_PUBLIC(MediaControls)
protected:
    MediaControlsPrivate(MediaControls *parent)
        : q_ptr(parent),
          layout(parent),
          playButton(parent),
          pauseButton(parent),
          seekSlider(parent),
          volumeSlider(parent),
          media(nullptr)
    {
        int size = parent->style()->pixelMetric(QStyle::PM_ToolBarIconSize);
        QSize iconSize(size, size);
        playButton.setIconSize(iconSize);
        playButton.setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
        playButton.setToolTip(i18n("start playback"));
        playButton.setAutoRaise(true);

        pauseButton.setIconSize(iconSize);
        pauseButton.setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
        pauseButton.setToolTip(i18n("pause playback"));
        pauseButton.hide();
        pauseButton.setAutoRaise(true);

        seekSlider.setIconVisible(false);

        volumeSlider.setOrientation(Qt::Horizontal);
        volumeSlider.setMaximumWidth(80);
        volumeSlider.hide();

        layout.setContentsMargins(0, 0, 0, 0);
        layout.setSpacing(0);
        layout.addWidget(&playButton);
        layout.addWidget(&pauseButton);
        layout.addWidget(&seekSlider, 1);
        layout.addWidget(&volumeSlider);
    }

    MediaControls *q_ptr;
    QHBoxLayout layout;
    QToolButton playButton;
    QToolButton pauseButton;
    SeekSlider seekSlider;
    VolumeSlider volumeSlider;
    MediaObject *media;

private:
    void _k_stateChanged(Phonon::State, Phonon::State);
    void _k_mediaDestroyed();
    void updateVolumeSliderVisibility();
};
} // namespace Phonon

#endif // PHONON_MEDIACONTROLS_P_H
