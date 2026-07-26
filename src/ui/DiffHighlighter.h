#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

namespace gitscope::ui {

// Colors unified-diff text: additions, deletions, hunk headers, and file
// metadata lines. Palette-aware (light/dark).
class DiffHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit DiffHighlighter(QTextDocument* document);

protected:
    void highlightBlock(const QString& text) override;

private:
    QTextCharFormat added_;
    QTextCharFormat removed_;
    QTextCharFormat hunk_;
    QTextCharFormat meta_;
};

} // namespace gitscope::ui
