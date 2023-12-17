//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <widget/playerorder.h>

#include <QSharedPointer>
#include <QWidget>

class XMainWindow;
class XProgressDialog;
class ProcessIndicator;
class QFileDialog;

struct XAMP_WIDGET_SHARED_EXPORT PlaybackFormat {
    bool is_dsd_file{ false };
    bool enable_sample_rate_convert{ false };
    uint32_t dsd_speed{ 0 };
    uint32_t bit_rate{ 0 };
    DsdModes dsd_mode{ DsdModes::DSD_MODE_PCM };
    AudioFormat file_format;
    AudioFormat output_format;
};

QSharedPointer<ProcessIndicator> makeProcessIndicator(QWidget* widget);

void centerDesktop(QWidget* widget);

void moveToTopWidget(QWidget* source_widget, const QWidget* target_widget);

XAMP_WIDGET_SHARED_EXPORT void centerParent(QWidget* widget);

XAMP_WIDGET_SHARED_EXPORT XMainWindow* getMainWindow();

XAMP_WIDGET_SHARED_EXPORT QString formatSampleRate(const AudioFormat& format);

XAMP_WIDGET_SHARED_EXPORT QString format2String(const PlaybackFormat& playback_format, const QString& file_ext);

XAMP_WIDGET_SHARED_EXPORT QSharedPointer<XProgressDialog> makeProgressDialog(QString const& title,
    QString const& text, 
    QString const& cancel,
    QWidget* parent = nullptr);

XAMP_WIDGET_SHARED_EXPORT PlayerOrder getNextOrder(PlayerOrder cur) noexcept;

XAMP_WIDGET_SHARED_EXPORT AlignPtr<IAudioProcessor> makeR8BrainSampleRateConverter();

XAMP_WIDGET_SHARED_EXPORT AlignPtr<IAudioProcessor> makeSrcSampleRateConverter();

XAMP_WIDGET_SHARED_EXPORT AlignPtr<IAudioProcessor> makeSoxrSampleRateConverter(const QVariantMap& settings);

XAMP_WIDGET_SHARED_EXPORT AlignPtr<IAudioProcessor> makeSampleRateConverter(uint32_t sample_rate);

XAMP_WIDGET_SHARED_EXPORT PlaybackFormat getPlaybackFormat(IAudioPlayer* player);

XAMP_WIDGET_SHARED_EXPORT QString getFileDialogFileExtensions();

XAMP_WIDGET_SHARED_EXPORT QString getExistingDirectory(QWidget* parent);

XAMP_WIDGET_SHARED_EXPORT void getOpenMusicFileName(QWidget* parent, std::function<void(const QString&)>&& action);

XAMP_WIDGET_SHARED_EXPORT void getOpenFileName(QWidget* parent,
    std::function<void(const QString&)>&& action,
    const QString& caption = QString(),
    const QString& dir = QString(),
    const QString& filter = QString());

XAMP_WIDGET_SHARED_EXPORT void getSaveFileName(QWidget* parent,
    std::function<void(const QString&)>&& action,
    const QString& caption = QString(),
    const QString& dir = QString(),
    const QString& filter = QString());

XAMP_WIDGET_SHARED_EXPORT void delay(int32_t seconds);

XAMP_WIDGET_SHARED_EXPORT const QStringList& getTrackInfoFileNameFilter();
