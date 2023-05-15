//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListWidget>
#include <QSet>

class TagWidgetItem : public QListWidgetItem {
public:
	TagWidgetItem(const QString& tag, QColor color, QListWidget* parent = nullptr);

	QString GetTag() const;

	bool IsEnable() const;

	void SetEnable(bool enable);

	void Enable();
private:
	bool enabled_ = true;
	QColor color_;
	QString tag_;
};

class TagListView : public QFrame {
	Q_OBJECT
public:
	explicit TagListView(QWidget* parent = nullptr);

	void AddTag(const QString &tag);

	void ClearTag();

signals:
	void TagChanged(const QSet<QString> &tags);

	void TagClear();

private:
	QListWidget* taglist_;
	QSet<QString> tags_;
};

