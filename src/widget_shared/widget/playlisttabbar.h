//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLineEdit>
#include <QTabBar>

class PlaylistTabBar final : public QTabBar {
	Q_OBJECT
public:
	explicit PlaylistTabBar(QWidget* parent = nullptr);
	
signals:
	void textChanged(int32_t index, const QString &text);

public slots:
	void onFinishRename();

private:
	void mouseDoubleClickEvent(QMouseEvent* event) override;

	void focusOutEvent(QFocusEvent* event) override;

	int32_t edited_index_{0};
	QLineEdit* line_edit_{nullptr};
};
