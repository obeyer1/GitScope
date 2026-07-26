#include "ui/SummaryDelegate.h"

#include "ui/CommitTableModel.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QPainter>

#include <algorithm>

namespace gitscope::ui {

namespace {

QColor chipColor(QChar type)
{
    switch (type.toLatin1()) {
    case 'H':
        return QColor(0x2e7d32); // checked-out branch: green
    case 'L':
        return QColor(0x1565c0); // local branch: blue
    case 'R':
        return QColor(0x6d4c41); // remote branch: brown
    case 'T':
        return QColor(0xb28704); // tag: dark amber
    case 'D':
        return QColor(0xc62828); // detached HEAD: red
    default:
        return QColor(0x616161);
    }
}

} // namespace

void SummaryDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                            const QModelIndex& index) const
{
    const QStringList refs = index.data(CommitTableModel::RefsRole).toStringList();
    if (refs.isEmpty()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Background and selection without the text; chips and text drawn manually.
    QStyleOptionViewItem background = opt;
    background.text.clear();
    QStyle* style = opt.widget != nullptr ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &background, painter, opt.widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QFont chipFont = opt.font;
    chipFont.setPointSizeF(std::max(7.5, opt.font.pointSizeF() - 1.5));
    chipFont.setBold(true);
    const QFontMetricsF chipMetrics(chipFont);

    qreal x = opt.rect.left() + 4.0;
    const qreal chipHeight = opt.rect.height() - 6.0;
    for (const QString& entry : refs) {
        if (entry.size() < 3)
            continue;
        const QChar type = entry.at(0);
        const QString label = entry.mid(2);
        const qreal width = chipMetrics.horizontalAdvance(label) + 10.0;
        if (x + width > opt.rect.right() - 60.0)
            break; // keep room for the message itself
        const QRectF chip(x, opt.rect.top() + 3.0, width, chipHeight);
        painter->setPen(Qt::NoPen);
        painter->setBrush(chipColor(type));
        painter->drawRoundedRect(chip, 4.0, 4.0);
        painter->setPen(Qt::white);
        painter->setFont(chipFont);
        painter->drawText(chip, Qt::AlignCenter, label);
        x += width + 4.0;
    }

    painter->setFont(opt.font);
    painter->setPen(opt.state.testFlag(QStyle::State_Selected)
                        ? opt.palette.highlightedText().color()
                        : opt.palette.text().color());
    const QRectF textRect(x + 2.0, opt.rect.top(), opt.rect.right() - x - 6.0, opt.rect.height());
    const QString elided =
        opt.fontMetrics.elidedText(opt.text, Qt::ElideRight, static_cast<int>(textRect.width()));
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);
    painter->restore();
}

} // namespace gitscope::ui
