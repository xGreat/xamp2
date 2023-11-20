//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>

#include <widget/str_utilts.h>
#include <widget/widget_shared_global.h>
#include <widget/playlistentity.h>

namespace Ui {
	class TagEditPage;
}

class XAMP_WIDGET_SHARED_EXPORT TagEditPage : public QFrame {
	Q_OBJECT
public:
	static constexpr ConstLatin1String kCoverImageFileExt = qTEXT("(*.jpg *.png *.bmp *.jpe *.jpeg *.tif *.tiff)");
	static constexpr QSize kCoverSize{ 600, 600 };

	TagEditPage(QWidget* parent, const QList<PlayListEntity>& entities);

	virtual ~TagEditPage() override;

signals:
	

private:
	void ReadEmbeddedCover(const PlayListEntity &entity);

	void SetImageLabel(const QPixmap &image, QSize image_size, size_t image_file_size);

	void SetCurrentInfo(int32_t index);

	void closeEvent(QCloseEvent* event) override;

	QString temp_file_path_;
	QPixmap temp_image_;
	QList<PlayListEntity> entities_;
	Ui::TagEditPage *ui_;
};
