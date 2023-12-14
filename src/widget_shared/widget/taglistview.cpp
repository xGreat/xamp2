#include <widget/taglistview.h>

#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <thememanager.h>

#include <QHBoxLayout>
#include <QLabel>

TagWidgetItem::TagWidgetItem(const QString& tag, QColor color, QLabel* label, QListWidget* parent)
	: QListWidgetItem(tag, parent)
	, color_(color)
	, tag_(tag)
	, label_(label) {
	setFlags(Qt::NoItemFlags);
}

TagWidgetItem::~TagWidgetItem() = default;

QString TagWidgetItem::getTag() const {
	return tag_;
}

bool TagWidgetItem::isEnable() const {
	return enabled_;
}

void TagWidgetItem::setEnable(bool enable) {
	enabled_ = enable;
	if (enabled_) {
		switch (qTheme.themeColor()) {
		case ThemeColor::DARK_THEME:
			listWidget()->itemWidget(this)->setStyleSheet(
				qSTR("border-radius: 6px; border: 1px solid %1; background-color: #171818;").arg(color_.name())
			);
			break;
		case ThemeColor::LIGHT_THEME:
			listWidget()->itemWidget(this)->setStyleSheet(
				qSTR("border-radius: 6px; border: 1px solid %1; background-color: #e6e6e6;").arg(color_.name())
			);
		}		
		label_->setStyleSheet(
			qSTR("color: %1;").arg(color_.name())
		);
	}
	else {
		QColor color;
		switch (qTheme.themeColor()) {
		case ThemeColor::DARK_THEME:
			color = QColor(qTEXT("#4d4d4d"));
			listWidget()->itemWidget(this)->setStyleSheet(
				qSTR("border-radius: 6px; border: 1px solid %1; background-color: #424548;").arg(color.name())
			);
			label_->setStyleSheet(
				qSTR("color: white;")
			);
			break;
		case ThemeColor::LIGHT_THEME:
			color = Qt::lightGray;
			listWidget()->itemWidget(this)->setStyleSheet(
				qSTR("border-radius: 6px; border: 1px solid transparent; background-color: #e6e6e6;")
			);
			label_->setStyleSheet(
				qSTR("color: #2e2f31;")
			);
			break;
		}				
	}
	listWidget()->update();
}

void TagWidgetItem::enable() {
	setEnable(!isEnable());
}

TagListView::TagListView(QWidget* parent)
	: QFrame(parent) {
	taglist_ = new QListWidget();
	taglist_->setDragEnabled(false);
	taglist_->setUniformItemSizes(false);
	taglist_->setFlow(QListView::LeftToRight);
	taglist_->setResizeMode(QListView::Adjust);
	taglist_->setFrameStyle(QFrame::StyledPanel);
	taglist_->setViewMode(QListView::IconMode);
	taglist_->setFixedHeight(120);
	taglist_->setSpacing(5);
	taglist_->setSortingEnabled(true);

	(void)QObject::connect(taglist_, &QListWidget::itemClicked, [this](auto* item) {
		if (!item) {
			return;
		}

		auto* tag_item = dynamic_cast<TagWidgetItem*>(item);
		if (!tag_item) {
			return;
		}
		tag_item->enable();
		if (!tag_item->isEnable()) {
			tags_.remove(tag_item->getTag());
		}
		else {
			tags_.insert(tag_item->getTag());
		}
		if (!tags_.isEmpty()) {
			emit tagChanged(tags_);
		}
		else {
			emit tagClear();
		}
		});

	auto* tag_layout = new QVBoxLayout();
	tag_layout->addWidget(taglist_);
	setLayout(tag_layout);
}

void TagListView::setListViewFixedHeight(int32_t height) {
	taglist_->setFixedHeight(height);
}

void TagListView::onCurrentThemeChanged(ThemeColor theme_color) {
	switch (theme_color) {
	case ThemeColor::DARK_THEME:
		taglist_->setStyleSheet(qTEXT(
		"QListWidget {"
		"  border: none;"
		"}"
		"QListWidget::item {"
		"  border: 1px solid transparent;"
		"  border-radius: 8px;"
		"  background-color: #2f3233;"
		"}"
		"QListWidget::item:selected {"
		"  background-color: transparent;"
		"}"
		));	
		break;
	case ThemeColor::LIGHT_THEME:
		taglist_->setStyleSheet(qTEXT(
			"QListWidget {"
			"  border: none;"
			"}"
			"QListWidget::item {"
			"  border: 1px solid transparent;"
			"  border-radius: 8px;"
			"  background-color: #e6e6e6;"
			"}"
			"QListWidget::item:selected {"
			"  background-color: transparent;"
			"}"
		));
		break;
	}
}

void TagListView::onThemeColorChanged(QColor background_color, QColor color) {

}

void TagListView::sort() {
	taglist_->sortItems();
}

void TagListView::disableAllTag(const QString& skip_tag) {
	for (auto i = 0; i < taglist_->count(); ++i) {
		auto* item = dynamic_cast<TagWidgetItem*>(taglist_->item(i));
		if (!item) {
			continue;
		}
		if (item->getTag() != skip_tag) {
			item->setEnable(false);
		}
	}
}

void TagListView::enableTag(const QString& tag) {
	auto items = taglist_->findItems(tag, Qt::MatchContains);
	if (items.isEmpty()) {
		return;
	}
	auto* item = dynamic_cast<TagWidgetItem*>(items.first());
	emit taglist_->itemClicked(item);
}

void TagListView::addTag(const QString& tag, bool uniform_item_sizes) {
	const auto items = taglist_->findItems(tag, Qt::MatchContains);
	if (!items.isEmpty()) {
		return;
	}

	const auto color = qTheme.highlightColor();

	auto f = font();
	auto* layout = new QHBoxLayout();
	auto* tag_label = new QLabel(tag);	
	tag_label->setAlignment(Qt::AlignCenter);	

	auto* item = new TagWidgetItem(tag, color, tag_label, taglist_);
	f.setBold(true);
	f.setPointSize(qTheme.fontSize(8));
	tag_label->setFont(f);

	if (!uniform_item_sizes) {
		const QFontMetrics metrics(f);
		const auto width = metrics.horizontalAdvance(tag) * 1.5;
		item->setSizeHint(QSize(width, 30));
	}
	else {
		item->setSizeHint(QSize(80, 30));
	}

	layout->addWidget(tag_label);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	auto* widget = new QWidget();
	widget->setLayout(layout);
	widget->setStyleSheet(
		qSTR("border-radius: 6px; border: 1px solid transparent;")
	);
	taglist_->setItemWidget(item, widget);
	item->enable();	
}

void TagListView::clearTag() {
	taglist_->clear();
}