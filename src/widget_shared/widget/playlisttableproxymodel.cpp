#include <widget/playlisttableproxymodel.h>

PlayListTableFilterProxyModel::PlayListTableFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
}

void PlayListTableFilterProxyModel::addFilterByColumn(int32_t column) {
    filters_.push_back(column);
}

bool PlayListTableFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    return std::any_of(filters_.begin(), filters_.end(), [source_row, source_parent, this](auto filter) {
            auto index = sourceModel()->index(source_row, filter, source_parent);
            if (index.isValid()) {
                auto text = sourceModel()->data(index).toString();
                if (text.contains(filterRegularExpression())) {
                    return true;
                }
            }
            return false;
        });
}
