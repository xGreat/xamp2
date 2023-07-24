//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSlider>
#include <QPointer>
#include <QMouseEvent>
#include <QVariantAnimation>
#include <QPropertyAnimation>

#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT SeekSlider : public QSlider {
	Q_OBJECT
public:
    explicit SeekSlider(QWidget* parent = nullptr);

	void SetRange(int64_t min, int64_t max);

signals:
	void LeftButtonValueChanged(int64_t value);

protected:
	void mousePressEvent(QMouseEvent* event) override;

	void enterEvent(QEvent* event) override;

	void leaveEvent(QEvent* event) override;

	void wheelEvent(QWheelEvent* event) override;

private:
	void SetValue(int value, bool animate = true);

	int target_ = 0;
	int duration_ = 100;
	int64_t min_ = 0;
	int64_t max_ = 0;
	QVariantAnimation* animation_;
	QEasingCurve easing_curve_ = QEasingCurve(QEasingCurve::Type::InOutCirc);
};

