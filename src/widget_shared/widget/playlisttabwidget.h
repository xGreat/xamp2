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
	static constexpr QSize kTabIconSize = QSize(32, 32);

	explicit PlaylistTabWidget(QWidget* parent = nullptr);

	int32_t currentPlaylistId() const;

	void saveTabOrder() const;

	void restoreTabOrder();

	void setPlaylistTabIcon(const QIcon &icon);

	void createNewTab(const QString& name, QWidget* widget);

signals:
	void createNewPlaylist();

	void removeAllPlaylist();

private:
	void closeTab(int32_t tab_index);

	bool removePlaylist(int32_t playlist_id);

	void mouseDoubleClickEvent(QMouseEvent* e) override;
};

