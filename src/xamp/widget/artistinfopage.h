//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>

class AlbumView;
class QLabel;

class ArtistInfoPage final : public QFrame {
	Q_OBJECT
public:
	explicit ArtistInfoPage(QWidget* parent = nullptr);

	AlbumView* album() const {
		return album_view_;
	}

public slots:
    void onThemeChanged(QColor backgroundColor, QColor color);

	void setArtistId(const QString& artist, const QString& cover_id, int32_t artist_id);

	void setAlbumCount(int32_t album_count);

	void setTracks(int32_t tracks);

	void setTotalDuration(double durations);

private:
	QPixmap getArtistImage(QPixmap const* cover) const;

	int32_t artist_id_{-1};
	QLabel* cover_;
	QLabel* artist_;
	QLabel* tracks_;
	QLabel* albums_;
	QLabel* durations_;
	AlbumView* album_view_;
	QString cover_id_;
};


