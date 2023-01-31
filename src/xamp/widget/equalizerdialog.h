//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>
#include "ui_equalizerdialog.h"

class EqualizerDialog : public QFrame {
    Q_OBJECT
public:
    explicit EqualizerDialog(QWidget *parent = nullptr);

signals:
   void BandValueChange(int band, float value, float Q);

   void PreampValueChange(float value);

private:
    void ParseEqFile();

    void ApplySetting(QString const &name, EQSettings const &settings);

    Ui::EqualizerDialog ui_;
    std::vector<DoubleSlider*> band_sliders_;
    std::vector<QLabel*> band_label_;
    std::vector<QLabel*> band_feq_label_;
};
