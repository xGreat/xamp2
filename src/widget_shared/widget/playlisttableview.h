//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QTableView>

#include <optional>

#include <widget/playlistentity.h>
#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/playerorder.h>
#include <widget/widget_shared_global.h>

#include <base/rng.h>
#include <base/encodingprofile.h>

class PlayListSqlQueryTableModel;
class PlayListTableFilterProxyModel;

enum PlayListGroup {
	PLAYLIST_GROUP_ALBUM,
	PLAYLIST_GROUP_NONE,
};

class XAMP_WIDGET_SHARED_EXPORT PlayListTableView : public QTableView {
	Q_OBJECT
public:
	static constexpr auto kMinimalEncodingBitRate = 128;
    static constexpr auto kMaxStretchedSize = 500;
	static constexpr auto kMaxPendingPlayListSize = 100;
	static constexpr auto kColumnPlayingWidth = 25;
	static constexpr auto kColumnTrackWidth = 40;
	static constexpr auto kColumnArtistWidth = 300;
	static constexpr auto kColumnDefaultWidth = 120;
	static constexpr auto kColumnDurationWidth = 10;
	static constexpr auto kPendingPlaylistSize = 30;

	static constexpr auto kColumnWidth = 48;
	static constexpr auto kColumnHeight = 46;

	static constexpr auto kCoverSize = QSize(32, 32);

	explicit PlayListTableView(QWidget* parent = nullptr, int32_t playlist_id = 1);

	virtual ~PlayListTableView() override;

	void disableDelete(bool enable = false) {
		enable_delete_ = enable;
	}

	void disableLoadFile(bool enable = false) {
		enable_load_file_ = enable;
	}

	void setPlayListGroup(PlayListGroup group) {
		group_ = group;
	}

	void enableCloudMode(bool mode) {
		cloud_mode_ = mode;
	}

	[[nodiscard]] bool isEnableCloudMode() const {
		return cloud_mode_;
	}

	void setCloudPlaylistId(const QString &playlist_id) {
		cloud_playlist_id_ = playlist_id;
		if (cloud_playlist_id_) {
			cloud_mode_ = true;
		}
	}

	const std::optional<QString>& cloudPlaylistId() const {
		return cloud_playlist_id_;
	}

	void setPlaylistId(const int32_t playlist_id, const QString& column_setting_name);

	[[nodiscard]] int32_t playlistId() const;

	void reload();

	void removePlaying();

	void removeAll();

	void removeItem(const QModelIndex& index);

	void removeSelectItems();

	void setNowPlaying(const QModelIndex& index, bool is_scroll_to = false);

	QModelIndex firstIndex() const;

	QModelIndex shuffleIndex();

	QModelIndex nextIndex(int forward) const;

	std::optional<QModelIndex> selectItem() const;

	QList<PlayListEntity> items() const;

	std::optional<PlayListEntity> selectPlayListEntity() const;

    void play(PlayerOrder order, bool is_plays);

	QModelIndex playOrderIndex(PlayerOrder order);

	void setNowPlayState(PlayingState playing_state);

    void scrollToIndex(const QModelIndex& index);

	void resizeColumn();

	OrderedMap<int32_t, QModelIndex> selectItemIndex() const;

	void append(const QString& file_name);

	QModelIndex hoverIndex() const {
		return model()->index(hover_row_, hover_column_);
	}

	void setHeaderViewHidden(bool enable);

	void setOtherPlaylist(int32_t playlist_id);

	void search(const QString& keyword) const;

signals:
	void updatePlayingState(const PlayListEntity &entity, PlayingState playing_state);

	void playMusic(const PlayListEntity& item, bool is_plays);

    void encodeFlacFile(const PlayListEntity& item);

	void encodeAacFile(const PlayListEntity& item, const EncodingProfile& profile);

	void encodeWavFile(const PlayListEntity& item);

	void downloadFile(const PlayListEntity& entity);

    void readReplayGain(int32_t playlist_id, const QList<PlayListEntity> &entities);

	void editTags(int32_t playlist_id, const QList<PlayListEntity>& entities);

	void updateAlbumCover(const QString &cover_id);

	void addPlaylistItemFinished();

	void extractFile(const QString& file_path, int32_t playlist_id);

	void addToPlaylist(const QString& source_playlist_id, const QString& playlist_id, const std::vector<std::string> &video_ids);

	void removePlaylistItems(const QString& playlist_id, const std::vector<std::string>& video_ids);

	void likeSong(bool like, const PlayListEntity& entity);	

public slots:
	void onPlayIndex(const QModelIndex& index, bool is_play = false);

	void onProcessDatabase(int32_t playlist_id, const QList<PlayListEntity>& entities);

	void onProcessTrackInfo(int32_t total_album, int32_t total_tracks);

	void onThemeColorChanged(QColor backgroundColor, QColor color);

	void onReloadEntity(const PlayListEntity& item);

	void onUpdateReplayGain(int32_t playlistId, 
		const PlayListEntity& entity,
		double track_loudness,
		double album_rg_gain,
		double album_peak,
		double track_rg_gain,
		double track_peak);

private:
	PlayListEntity item(const QModelIndex& index) const;

	void playItem(const QModelIndex& index);

	void pauseItem(const QModelIndex& index);

	bool eventFilter(QObject* obj, QEvent* ev) override;

    void keyPressEvent(QKeyEvent *event) override;

    void resizeEvent(QResizeEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

	void initial();

protected:
	bool cloud_mode_{ false };
	bool enable_delete_{ true };
	bool enable_load_file_{ true };
	int32_t hover_row_{ -1 };
	int32_t hover_column_{ -1 };
	int32_t playlist_id_{ -1 };
	PlayListGroup group_{ PlayListGroup::PLAYLIST_GROUP_NONE };
	std::optional<int32_t> other_playlist_id_;
	std::optional<QString> cloud_playlist_id_;
	QModelIndex play_index_;
    PlayListSqlQueryTableModel* model_;
	PlayListTableFilterProxyModel* proxy_model_;
    QSet<QString> hidden_column_names_;
	PRNG rng_;
	QString column_setting_name_;
	QList<int32_t> always_hidden_columns_;
};


