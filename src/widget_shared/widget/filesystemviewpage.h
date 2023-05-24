//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ui_filesystemviewpage.h>
#include <widget/widget_shared_global.h>

class PlaylistPage;
class FileSystemModel;

class XAMP_WIDGET_SHARED_EXPORT FileSystemViewPage : public QFrame {
	Q_OBJECT
public:
	explicit FileSystemViewPage(QWidget* parent = nullptr);

	PlaylistPage* playlistPage();

signals:
	void addDirToPlaylist(const QString& url);

	void ExtractFile(const QString& file_path, int32_t playlist_id, bool is_podcast_mode);

private:
	class XAMP_WIDGET_SHARED_EXPORT DirFirstSortFilterProxyModel;

	Ui::FileSystemViewPage ui;
	FileSystemModel* dir_model_;
	DirFirstSortFilterProxyModel* dir_first_sort_filter_;
};