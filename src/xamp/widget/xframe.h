//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QLabel>
#include <QToolButton>

#include <widget/themecolor.h>

class XFrame : public QFrame {
	Q_OBJECT
public:
	explicit XFrame(QWidget* parent = nullptr);

	void SetContentWidget(QWidget* content);

	void SetTitle(const QString& title) const;

	void SetIcon(const QIcon& icon) const;

	QWidget* ContentWidget() const {
		return content_;
	}
signals:
	void CloseFrame();

public slots:
	void OnCurrentThemeChanged(ThemeColor theme_color);

private:
	QLabel* title_frame_label{ nullptr };
	QWidget* content_{ nullptr };
	QToolButton* icon_;
	QToolButton* close_button_{ nullptr };
	QToolButton* max_win_button_{ nullptr };
	QToolButton* min_win_button_{ nullptr };
};