//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <widget/database.h>

class XAMP_WIDGET_SHARED_EXPORT ExtractFileWorker : public QObject {
    Q_OBJECT
public:
    static constexpr size_t kReserveSize = 1024;

    ExtractFileWorker();

signals:
    void InsertDatabase(const ForwardList<TrackInfo>& result, int32_t playlist_id);

    void ReadFileStart();

    void ReadCompleted();

    void ReadFilePath(const QString& file_path);

    void ReadFileProgress(int progress);

    void FoundFileCount(size_t file_count);

    void CalculateEta(uint64_t ms);

    void FromDatabase(int32_t playlist_id, const QList<PlayListEntity>& entity);

public slots:
    void OnExtractFile(const QString& file_path, int32_t playlist_id);

    void OnCancelRequested();

private:
    void ScanPathFiles(const QStringList& file_name_filters,
        int32_t playlist_id,
        const QString& dir);

    bool is_stop_{ true };
    LoggerPtr logger_;
};

