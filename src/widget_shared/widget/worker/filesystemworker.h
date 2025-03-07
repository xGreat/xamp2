//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <widget/database.h>
#include <widget/file_system_watcher.h>

class XAMP_WIDGET_SHARED_EXPORT FileSystemWorker final : public QObject {
    Q_OBJECT
public:
    static constexpr size_t kReserveFilePathSize = 1024;

    FileSystemWorker();

    void cancelRequested();

signals:
    void insertDatabase(const ForwardList<TrackInfo>& result, int32_t playlist_id);

    void readFileStart();

    void readCompleted();

    void readFilePath(const QString& file_path);

    void readFileProgress(int32_t progress);

    void foundFileCount(size_t file_count);

public slots:
    void onExtractFile(const QString& file_path, int32_t playlist_id);

    void onSetWatchDirectory(const QString& dir);

private:
    size_t scanPathFiles(const QStringList& file_name_filters,
        int32_t playlist_id,
        const QString& dir);

    bool is_stop_{ false };
    FileSystemWatcher watcher_;
    LoggerPtr logger_;
};

