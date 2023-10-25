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
	TagEditPage(QWidget* parent, const QList<PlayListEntity>& entities);

signals:
	

private:
	void ReadEmbeddedCover(const PlayListEntity &entity);

	Ui::TagEditPage ui_;
	QList<PlayListEntity> entities_;
};
