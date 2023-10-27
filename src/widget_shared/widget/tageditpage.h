//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>

#include <ui_tageditpage.h>
#include <widget/widget_shared_global.h>
#include <widget/playlistentity.h>

class XAMP_WIDGET_SHARED_EXPORT TagEditPage : public QFrame {
	Q_OBJECT
public:
	static constexpr QSize kCoverSize{ 600, 600 };

	TagEditPage(QWidget* parent, const QList<PlayListEntity>& entities);

signals:
	

private:
	void ReadEmbeddedCover(const PlayListEntity &entity);

	void SetImageLabel(const QPixmap &image, QSize image_size, size_t image_file_size);

	QString temp_file_path_;
	QPixmap temp_image_;
	QList<PlayListEntity> entities_;
	Ui::TagEditPage ui_;
};
