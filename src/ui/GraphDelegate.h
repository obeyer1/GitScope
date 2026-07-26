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

    // Upper bound on lanes given screen space. Busy repositories can need
    // hundreds of lanes; without a cap the graph column grows wider than the
    // window and pushes every other column out of view. Lanes beyond the cap
    // are clipped by the cell rect.
    static int maxDisplayLanes() { return 20; }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

} // namespace gitscope::ui
