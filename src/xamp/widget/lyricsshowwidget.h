//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/lrcparser.h>
#include <widget/wheelablewidget.h>

class QDropEvent;

class LyricsShowWidget : public WheelableWidget {
	Q_OBJECT

public:
	explicit LyricsShowWidget(QWidget *parent = nullptr);

	Q_DISABLE_COPY(LyricsShowWidget)

	bool loadLrcFile(const QString &file_path);

	void setCurrentTime(int32_t time, bool is_adding = true);

	void paintItem(QPainter* painter, int32_t index, QRect &rect) override;

	void paintItemMask(QPainter* painter) override;

	void paintBackground(QPainter* painter) override;

	int32_t itemHeight() const override;

	int32_t itemCount() const override;

	void setBackgroundColor(QColor color);

public slots:
	void stop();	

	void setLrc(const QString &lrc);

	void setLrcTime(int32_t length);

	void setLrcFont(const QFont &font);

	void setLrcHightLight(const QColor &color);

	void setLrcColor(const QColor& color);

	void addFullLrc(const QString& lrc, std::chrono::milliseconds duration);

private:
	void dragEnterEvent(QDragEnterEvent* event) override;

	void dragMoveEvent(QDragMoveEvent* event) override;

	void dragLeaveEvent(QDragLeaveEvent* event) override;

	void dropEvent(QDropEvent* event) override;

	void setDefaultLrc();

    void initial();

	void loadLrc(const QString& lrc);

	int32_t pos_;
    int32_t last_lyric_index_;
	float item_precent_;	
	QFont lrc_font_;
	QFont current_mask_font_;
	QColor lrc_color_;
	QColor lrc_hightlight_color_;
	QColor background_color_;
	QString lrc_;
	LrcParser lyric_;	
};
