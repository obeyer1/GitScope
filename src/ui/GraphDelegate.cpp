#include "ui/GraphDelegate.h"

#include "git/GraphBuilder.h"
#include "ui/CommitTableModel.h"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace gitscope::ui {

namespace {

// Tableau-style palette: distinguishable in both light and dark themes.
const QColor kLaneColors[] = {
    QColor(0x4e79a7), QColor(0xf28e2b), QColor(0x59a14f), QColor(0xe15759), QColor(0xb07aa1),
    QColor(0x76b7b2), QColor(0xedc948), QColor(0xff9da7), QColor(0x9c755f), QColor(0xbab0ac),
};
constexpr int kColorCount = static_cast<int>(sizeof(kLaneColors) / sizeof(kLaneColors[0]));

QColor laneColor(int index)
{
    return kLaneColors[index % kColorCount];
}

} // namespace

void GraphDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const
{
    // Base item paint draws the row background and selection highlight.
    QStyledItemDelegate::paint(painter, option, index);

    const QVariant value = index.data(CommitTableModel::GraphRowRole);
    if (!value.canConvert<git::GraphRow>())
        return;
    const git::GraphRow row = value.value<git::GraphRow>();

    painter->save();
    painter->setClipRect(option.rect);
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF rect(option.rect);
    const qreal lane = laneWidth();
    const qreal left = rect.left() + lane / 2.0;
    const qreal top = rect.top();
    const qreal bottom = rect.bottom() + 1.0;
    const qreal centerY = rect.center().y();
    const auto laneX = [&](int laneIndex) { return left + laneIndex * lane; };
    const qreal nodeX = laneX(row.commitLane);

    QPen pen;
    pen.setWidthF(1.6);
    pen.setCapStyle(Qt::FlatCap);

    const auto drawFlow = [&](qreal xa, qreal ya, qreal xb, qreal yb) {
        QPainterPath path;
        path.moveTo(xa, ya);
        if (qFuzzyCompare(xa, xb)) {
            path.lineTo(xb, yb);
        } else {
            const qreal midY = (ya + yb) / 2.0;
            path.cubicTo(xa, midY, xb, midY, xb, yb);
        }
        painter->drawPath(path);
    };

    for (const git::GraphSegment& seg : row.segments) {
        pen.setColor(laneColor(seg.colorIndex));
        painter->setPen(pen);
        if (seg.topLane == git::GraphBuilder::kNode)
            drawFlow(nodeX, centerY, laneX(seg.bottomLane), bottom);
        else if (seg.bottomLane == git::GraphBuilder::kNode)
            drawFlow(laneX(seg.topLane), top, nodeX, centerY);
        else
            drawFlow(laneX(seg.topLane), top, laneX(seg.bottomLane), bottom);
    }

    painter->setPen(QPen(option.palette.base().color(), 1.5));
    painter->setBrush(laneColor(row.colorIndex));
    painter->drawEllipse(QPointF(nodeX, centerY), 3.5, 3.5);
    painter->restore();
}

QSize GraphDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QSize base = QStyledItemDelegate::sizeHint(option, index);
    int lanes = 1;
    const QVariant value = index.data(CommitTableModel::GraphRowRole);
    if (value.canConvert<git::GraphRow>())
        lanes = std::min(value.value<git::GraphRow>().laneCount, maxDisplayLanes());
    return {laneWidth() * lanes + laneWidth(), std::max(base.height(), 22)};
}

} // namespace gitscope::ui
