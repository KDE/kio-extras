/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2006 Matthias Kretz <kretz@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only

*/

#ifndef PHONON_UI_MEDIACONTROLS_H
#define PHONON_UI_MEDIACONTROLS_H

#include <phonon/phononnamespace.h>
#include <QWidget>

namespace Phonon
{
class MediaObject;
class AudioOutput;
class MediaControlsPrivate;

/**
 * \short Simple widget showing buttons to control an MediaObject
 * object.
 *
 * This widget shows the standard player controls. There's at least the
 * play/pause and stop buttons. If the media is seekable it shows a seek-slider.
 * Optional controls include a volume control and a loop control button.
 *
 * \author Matthias Kretz <kretz@kde.org>
 */
class MediaControls : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(MediaControls)
    /**
     * This property holds whether the slider showing the progress of the
     * playback is visible.
     *
     * By default the slider is visible. It is enabled/disabled automatically
     * depending on whether the media can be sought or not.
     */
    Q_PROPERTY(bool seekSliderVisible READ isSeekSliderVisible WRITE setSeekSliderVisible)

    /**
     * This property holds whether the slider controlling the volume is visible.
     *
     * By default the slider is visible if an AudioOutput has been set with
     * setAudioOutput.
     *
     * \see setAudioOutput
     */
    Q_PROPERTY(bool volumeControlVisible READ isVolumeControlVisible WRITE setVolumeControlVisible)

public:
    /**
     * Constructs a media control widget with a \p parent.
     */
    explicit MediaControls(QWidget *parent = nullptr);
    ~MediaControls() override;

    bool isSeekSliderVisible() const;
    bool isVolumeControlVisible() const;

public Q_SLOTS:
    void setSeekSliderVisible(bool isVisible);
    void setVolumeControlVisible(bool isVisible);

    /**
     * Sets the media object to be controlled by this widget.
     */
    void setMediaObject(MediaObject *mediaObject);

    /**
     * Sets the audio output object to be controlled by this widget.
     */
    void setAudioOutput(AudioOutput *audioOutput);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Q_PRIVATE_SLOT(d_func(), void _k_stateChanged(Phonon::State, Phonon::State))
    Q_PRIVATE_SLOT(d_func(), void _k_mediaDestroyed())

    MediaControlsPrivate *const d_ptr;
};

} // namespace Phonon

#endif // PHONON_UI_MEDIACONTROLS_H
