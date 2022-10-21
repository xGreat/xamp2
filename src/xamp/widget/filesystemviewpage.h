//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ui_filesystemviewpage.h>

class PlaylistPage;
class FileSystemModel;

class FileSystemViewPage : public QFrame {
	Q_OBJECT
public:
	explicit FileSystemViewPage(QWidget* parent = nullptr);

	PlaylistPage* playlistPage();

signals:
	void addDirToPlyalist(const QString& url);

private:
	Ui::FileSystemViewPage ui;
	FileSystemModel* dir_model_;
};