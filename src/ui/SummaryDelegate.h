#pragma once

#include <QStyledItemDelegate>

namespace gitscope::ui {

// Paints the commit message column with colored ref chips (branches, tags,
// HEAD) in front of the elided summary text.
class SummaryDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
};

} // namespace gitscope::ui
