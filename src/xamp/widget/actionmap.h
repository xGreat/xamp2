//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <map>
#include <QPoint>
#include <QScopedPointer>
#include <QPropertyAnimation>
#include <QMenu>
#include <QEvent>

#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#endif

#include "thememanager.h"

class XMenu : public QMenu {
public:
	explicit XMenu(QWidget* object = nullptr)
		: QMenu(object) {
	}

#if defined(Q_OS_WIN)
	bool event(QEvent* evt) override {
		if (evt->type() == QEvent::WinIdChange) {
			win32::setAccentPolicy(winId());
			win32::addDwmMenuShadow(winId());
		}
		return QMenu::event(evt);
	}
#endif
};

template <typename Type, typename F = std::function<void()>>
class ActionMap {
public:
	using MapType = std::map<QAction*, F>;

	class SubMenu {
	public:
		SubMenu(const QString& menu_name, XMenu* menu, MapType& action_map)
			: action_group_(new QActionGroup(submenu_.get()))
			, action_map_(action_map) {
			submenu_.reset(menu->addMenu(menu_name));
            qTheme.setMenuStyle(submenu_.get());
		}

		template <typename Callable>
		QAction* addAction(const QString& menu_name,
			Callable&& callback,
			bool checked = false,
			bool add_eparator = false) {
			const auto action = new QAction(menu_name, nullptr);
			action->setCheckable(true);

			action->setChecked(checked);

			action_map_[action] = callback;
			submenu_->addAction(action);

			if (add_eparator)
				submenu_->addSeparator();

			submenu_->addAction(action);
			action_group_->addAction(action);

			return action;
		}
	private:
		QScopedPointer<QMenu> submenu_;
		QScopedPointer<QActionGroup> action_group_;
		MapType& action_map_;
	};

	explicit ActionMap(Type* object)
		: object_(object)
		, menu_(object) {
		qTheme.setMenuStyle(&menu_);
	}

	QAction* addAction(const QString& menu_name) {
		auto action = addAction(menu_name, []() {});
		action->setEnabled(false);
		return action;
	}

	template <typename Callable>
	void setCallback(QAction* action, Callable&& callback) {
		map_[action] = callback;
		action->setEnabled(true);
	}

	template <typename Callable>
	QAction* addAction(const QString& menu_name,
		Callable&& callback,
		bool add_eparator = false,
		bool checked = false) {
		auto* action = new QAction(menu_name, nullptr);

		map_[action] = callback;
		menu_.addAction(action);

        if (add_eparator) {
            menu_.addSeparator();
        }

        if (checked) {
            action->setCheckable(true);
            action->setChecked(checked);
        } else {
            action->setCheckable(false);
        }

		return action;
	}

	SubMenu* addSubMenu(const QString& menu_name) {
		return new SubMenu(menu_name, &menu_, map_);
	}

	void addSeparator() {
		menu_.addSeparator();
	}

	template <typename... Args>
	void exec(const QPoint& point, Args... args) {
		auto globalpos = object_->mapToGlobal(point);

		auto itr = map_.find(menu_.exec(globalpos));
		if (itr != map_.end()) {
			(*itr).second(std::forward<Args>(args)...);
		}
	}

private:	
	Type* object_;
	XMenu menu_;
	MapType map_;
};
