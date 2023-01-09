//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QModelIndex>

#include <base/spsc_queue.h>
#include <stream/filestream.h>
#include <output_device/idevicestatelistener.h>
#include <player/iplaybackstateadapter.h>

#include <base/align_ptr.h>
#include <base/audioformat.h>
#include <stream/stream.h>
#include <stream/stft.h>

#include <player/playstate.h>

using xamp::base::Errors;
using xamp::base::AlignPtr;
using xamp::base::Exception;
using xamp::base::SpscQueue;
using xamp::base::AudioFormat;
using xamp::base::MakeAlign;

using xamp::player::PlayerState;
using xamp::player::IPlaybackStateAdapter;

using xamp::output_device::DeviceState;
using xamp::stream::FileStream;
using xamp::stream::ComplexValarray;
using xamp::stream::STFT;
using xamp::stream::WindowType;
using xamp::stream::ComplexValarray;

class UIPlayerStateAdapter final
    : public QObject
    , public IPlaybackStateAdapter {
    Q_OBJECT
public:
    explicit UIPlayerStateAdapter(QObject *parent = nullptr);

    void OutputFormatChanged(const AudioFormat output_format, size_t buffer_size) override;

    void OnSampleTime(double stream_time) override;

    void OnStateChanged(PlayerState play_state) override;

    void OnError(const Exception &ex) override;

    void OnDeviceChanged(DeviceState state) override;

    void OnVolumeChanged(float vol) override;

    void OnSamplesChanged(const float* samples, size_t num_buffer_frames) override;

signals:
    void sampleTimeChanged(double stream_time);

    void stateChanged(PlayerState play_state);

    void playbackError(Errors error, const QString &message);

    void deviceChanged(DeviceState state);

    void volumeChanged(float vol);

    void fftResultChanged(ComplexValarray const& result);

private:
    bool enable_spectrum_;
    double last_stream_time_;
    AlignPtr<STFT> stft_;
};
