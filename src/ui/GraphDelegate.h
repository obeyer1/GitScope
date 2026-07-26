#pragma once

#include <QStyledItemDelegate>

namespace gitscope::ui {

// Paints the commit-graph column: lane lines, merge/branch curves, and the
// commit node, colored per lane.
class GraphDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    static int laneWidth() { return 14; }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

} // namespace gitscope::ui
