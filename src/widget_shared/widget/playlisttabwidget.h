//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QTabWidget>
#include <QMap>
#include <widget/widget_shared_global.h>

class QMouseEvent;

class XAMP_WIDGET_SHARED_EXPORT PlaylistTabWidget : public QTabWidget {
	Q_OBJECT
public:
	explicit PlaylistTabWidget(QWidget* parent = nullptr);

	void AddTab(int32_t playlist_id, const QString &name, QWidget *widget, bool add_db);

	int32_t GetCurrentPlaylistId() const;
signals:
	void CreateNewPlaylist();

	void ClosePlaylist(int32_t playlist_id);

private:	
	void mouseDoubleClickEvent(QMouseEvent* e) override;
};

