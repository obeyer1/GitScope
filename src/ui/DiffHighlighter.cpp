#include "ui/DiffHighlighter.h"

#include <QGuiApplication>
#include <QPalette>

namespace gitscope::ui {

DiffHighlighter::DiffHighlighter(QTextDocument* document) : QSyntaxHighlighter(document)
{
    const bool dark = QGuiApplication::palette().color(QPalette::Base).lightness() < 128;
    added_.setForeground(dark ? QColor(0x81c995) : QColor(0x1e7d32));
    removed_.setForeground(dark ? QColor(0xf28b82) : QColor(0xc5221f));
    hunk_.setForeground(dark ? QColor(0x8ab4f8) : QColor(0x1a73e8));
    hunk_.setFontWeight(QFont::Bold);
    meta_.setForeground(dark ? QColor(0x9aa0a6) : QColor(0x5f6368));
    meta_.setFontWeight(QFont::Bold);
}

void DiffHighlighter::highlightBlock(const QString& text)
{
    const int length = static_cast<int>(text.size());
    if (length == 0)
        return;

    const auto isMeta = text.startsWith(QLatin1String("diff "))
        || text.startsWith(QLatin1String("index ")) || text.startsWith(QLatin1String("+++ "))
        || text.startsWith(QLatin1String("--- ")) || text.startsWith(QLatin1String("new file"))
        || text.startsWith(QLatin1String("deleted file"))
        || text.startsWith(QLatin1String("rename "))
        || text.startsWith(QLatin1String("similarity "))
        || text.startsWith(QLatin1String("old mode")) || text.startsWith(QLatin1String("new mode"))
        || text.startsWith(QLatin1String("Binary files"));

    if (isMeta)
        setFormat(0, length, meta_);
    else if (text.startsWith(QLatin1String("@@")))
        setFormat(0, length, hunk_);
    else if (text.startsWith(QLatin1Char('+')))
        setFormat(0, length, added_);
    else if (text.startsWith(QLatin1Char('-')))
        setFormat(0, length, removed_);
}

} // namespace gitscope::ui
