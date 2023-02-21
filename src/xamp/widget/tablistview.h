//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>
#include <QListView>
#include <QStandardItemModel>

#include "thememanager.h"

enum TabIndex {
    TAB_PLAYLIST,
    TAB_FILE_EXPLORER,
    TAB_LYRICS,
    TAB_PODCAST,    
    TAB_ALBUM,
    TAB_ARTIST,
    TAB_CD,
};

class TabListView : public QListView {
    Q_OBJECT
public:
    explicit TabListView(QWidget *parent = nullptr);

    void AddTab(const QString& name, int table_id, const QIcon &icon);

    void AddSeparator();

    QString GetTabName(int table_id) const;

    int32_t GetTabId(const QString &name) const;

signals:
    void ClickedTable(int table_id);

    void TableNameChanged(int table_id, const QString &name);

public slots:
    void OnCurrentThemeChanged(ThemeColor theme_color);

private:
    QStandardItemModel model_;
    QMap<int, QString> names_;
    QMap<QString, int> ids_;
};

