//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QSqlQueryModel>
#include <QStyledItemDelegate>
#include <QPushButton>

#include <widget/widget_shared.h>

#include <widget/albumentity.h>
#include <widget/playlistentity.h>

class QLabel;
class ClickableLabel;
class AlbumViewPage;
class AlbumPlayListTableView;
class PlaylistPage;

class AlbumViewStyledDelegate final : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit AlbumViewStyledDelegate(QObject* parent = nullptr);

	void setTextColor(QColor color);

	void clearImageCache();
signals:
	void enterAlbumView(const QModelIndex& index) const;

	void showAlbumOpertationMenu(const QModelIndex& index, const QPoint &pt) const;

protected:
	bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex& idx) const override;
private:
	QColor text_color_;
	QPoint mouse_point_;
	QPixmap mask_image_;
	QScopedPointer<QPushButton> more_album_opt_button_;
	QScopedPointer<QPushButton> play_button_;
	mutable LruCache<QString, QPixmap> image_cache_;
};

class AlbumViewPage final : public QFrame {
	Q_OBJECT
public:
	explicit AlbumViewPage(QWidget* parent = nullptr);

	void setPlaylistMusic(const QString &album, int32_t album_id, const QString& cover_id);

	ClickableLabel* artist() {
		return artist_;
	}

	PlaylistPage* playlistPage() {
		return page_;
	}

signals:
	void clickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void leaveAlbumView() const;

private:
	ClickableLabel* artist_;
	PlaylistPage* page_;
};

class AlbumView final : public QListView {
	Q_OBJECT
public:
	explicit AlbumView(QWidget* parent = nullptr);

	void update();

	AlbumViewPage* albumViewPage() {
		return page_;
	}

	void enablePage(bool enable);

signals:
    void addPlaylist(const Vector<int32_t> &music_ids, const Vector<PlayListEntity> &entities);

	void clickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void clickedAlbum(const QString& album, int32_t album_id, const QString& cover_id);

	void removeAll();

	void loadCompleted();

public slots:
	void refreshOnece();

	void setFilterByArtistId(int32_t artist_id);

	void hideWidget();

	void onSearchTextChanged(const QString& text);

    void onThemeChanged(QColor backgroundColor, QColor color);

    void append(const QString& file_name);

    void processMeatadata(int64_t dir_last_write_time, const ForwardList<Metadata> &medata);

	void showOperationMenu(const QPoint& pt);

	void showAlbumViewMenu(const QPoint& pt);

private:
	void resizeEvent(QResizeEvent* event) override;

	bool enable_page_{true};
	AlbumViewPage* page_;
	AlbumViewStyledDelegate* styled_delegate_;
	QSqlQueryModel model_;
};

