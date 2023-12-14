//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/database.h>
#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT FindAlbumCoverWorker : public QObject {
	Q_OBJECT
public:
	FindAlbumCoverWorker();

signals:
	void setAlbumCover(int32_t album_id, const QString& cover_id);

public slots:
	void onFindAlbumCover(int32_t album_id,
		const QString& album,
		const QString& artist,
		const std::wstring& file_path);

	void cancelRequested();

private:
	bool is_stop_{ false };
	PooledDatabasePtr database_ptr_;
};

