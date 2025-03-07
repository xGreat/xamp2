//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QTabWidget>
#include <QTabBar>

#include <widget/database.h>
#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>
#include <widget/playlistpage.h>

class QMouseEvent;
class QPushButton;

class XAMP_WIDGET_SHARED_EXPORT PlaylistTabWidget : public QTabWidget {
	Q_OBJECT
public:
	static constexpr QSize kTabIconSize = QSize(32, 32);

	explicit PlaylistTabWidget(QWidget* parent = nullptr);

	void hidePlusButton();

	int32_t currentPlaylistId() const;

	void saveTabOrder() const;

	void restoreTabOrder();

	void resetAllTabIcon();

	void setPlaylistTabIcon(const QIcon &icon);

	void createNewTab(const QString& name, QWidget* widget);

	void setPlaylistCover(const QPixmap &cover);

	void closeAllTab();

	void setStoreType(StoreType type);

	void reloadAll();

	template <typename F>
	void forEachPlaylist(F &&func) {
		for (int i = 0; i < tabBar()->count(); ++i) {
			auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
			Q_ASSERT(playlist_page != nullptr);
			auto* playlist = playlist_page->playlist();
			func(playlist);
		}
	}

signals:
	void createNewPlaylist();

	void createCloudPlaylist();

	void removeAllPlaylist();

	void reloadAllPlaylist();

	void reloadPlaylist(int32_t tab_index);

	void deletePlaylist(const QString& playlist_id);

public slots:
	void onThemeChangedFinished(ThemeColor theme_color);

private:
	void closeTab(int32_t tab_index);

	bool removePlaylist(int32_t playlist_id);

	void mouseDoubleClickEvent(QMouseEvent* e) override;

	bool eventFilter(QObject* watched, QEvent* event) override;

	StoreType store_type_{ StoreType::LOCAL_STORE };
	QPushButton* plus_button_{ nullptr };
};

