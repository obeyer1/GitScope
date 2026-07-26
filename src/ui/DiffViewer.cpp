#include "ui/DiffViewer.h"

#include "ui/DiffHighlighter.h"

#include <QDateTime>
#include <QFontDatabase>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QVBoxLayout>

namespace gitscope::ui {

namespace {

QString escaped(const std::string& text)
{
    return QString::fromStdString(text).toHtmlEscaped();
}

QColor statusColor(char status)
{
    switch (status) {
    case 'A':
        return QColor(0x2e7d32);
    case 'D':
        return QColor(0xc62828);
    case 'R':
    case 'C':
        return QColor(0x6a1b9a);
    default:
        return QColor(0x1565c0); // modified & friends
    }
}

QString formatTime(qint64 secs)
{
    return QDateTime::fromSecsSinceEpoch(secs).toString(QStringLiteral("yyyy-MM-dd HH:mm"));
}

} // namespace

DiffViewer::DiffViewer(QWidget* parent) : QWidget(parent)
{
    header_ = new QLabel(this);
    header_->setWordWrap(true);
    header_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    header_->setContentsMargins(8, 6, 8, 6);
    // An explicit minimum stops commit-message content (long unbreakable
    // lines) from dictating a minimum width that would force the whole
    // window to grow wider than the screen.
    header_->setMinimumWidth(1);

    fileList_ = new QListWidget(this);
    fileList_->setSelectionMode(QAbstractItemView::SingleSelection);
    fileList_->setUniformItemSizes(true);

    patchView_ = new QPlainTextEdit(this);
    patchView_->setReadOnly(true);
    patchView_->setLineWrapMode(QPlainTextEdit::NoWrap);
    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    patchView_->setFont(mono);
    patchView_->setTabStopDistance(4.0 * QFontMetricsF(mono).horizontalAdvance(QLatin1Char('x')));
    new DiffHighlighter(patchView_->document()); // parented by the document

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(fileList_);
    splitter->addWidget(patchView_);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setChildrenCollapsible(false);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(header_);
    layout->addWidget(splitter, 1);

    connect(fileList_, &QListWidget::currentRowChanged, this, &DiffViewer::showFile);
    clear();
}

void DiffViewer::setDetails(const git::CommitDetails& details)
{
    files_ = details.files;

    int totalAdds = 0;
    int totalDels = 0;
    for (const git::FileDiff& file : files_) {
        totalAdds += file.additions;
        totalDels += file.deletions;
    }

    QString html = QStringLiteral("<b>%1</b><br>").arg(escaped(details.info.summary));
    html += QStringLiteral("<span style='font-family:monospace'>%1</span> — %2 &lt;%3&gt; — %4")
                .arg(escaped(details.info.id), escaped(details.info.authorName),
                     escaped(details.info.authorEmail), formatTime(details.info.authorTime));
    if (details.committerName != details.info.authorName
        || details.commitTime != details.info.authorTime) {
        html += QStringLiteral("<br><small>committed by %1 — %2</small>")
                    .arg(escaped(details.committerName), formatTime(details.commitTime));
    }
    html += QStringLiteral("<br><small>%1 file(s) changed, +%2 −%3%4</small>")
                .arg(files_.size())
                .arg(totalAdds)
                .arg(totalDels)
                .arg(details.info.parentIds.size() > 1
                         ? QStringLiteral(" — merge commit, diff vs. first parent")
                         : QString());

    // Message body beyond the summary line, shown verbatim.
    const QString full = QString::fromStdString(details.fullMessage).trimmed();
    const QString summary = QString::fromStdString(details.info.summary);
    if (full.size() > summary.size()) {
        const QString body = full.mid(summary.size()).trimmed();
        if (!body.isEmpty()) {
            // pre-wrap (not <pre>) so long body lines wrap instead of
            // widening the label.
            html += QStringLiteral("<div style='white-space:pre-wrap; font-family:monospace; "
                                   "margin-top:6px'>%1</div>")
                        .arg(body.toHtmlEscaped());
        }
    }
    header_->setText(html);

    fileList_->blockSignals(true);
    fileList_->clear();
    for (const git::FileDiff& file : files_) {
        QString label = QStringLiteral("%1  %2").arg(QChar::fromLatin1(file.status),
                                                     QString::fromStdString(file.path));
        if ((file.status == 'R' || file.status == 'C') && file.oldPath != file.path)
            label += QStringLiteral("  ← %1").arg(QString::fromStdString(file.oldPath));
        label += QStringLiteral("   +%1 −%2").arg(file.additions).arg(file.deletions);
        auto* item = new QListWidgetItem(label, fileList_);
        item->setForeground(statusColor(file.status));
    }
    fileList_->blockSignals(false);

    if (!files_.empty())
        fileList_->setCurrentRow(0);
    else
        patchView_->setPlainText(tr("(no file changes in this commit)"));
}

void DiffViewer::clear()
{
    files_.clear();
    header_->setText(tr("<i>No commit selected</i>"));
    fileList_->blockSignals(true);
    fileList_->clear();
    fileList_->blockSignals(false);
    patchView_->clear();
}

void DiffViewer::showFile(int row)
{
    if (row < 0 || static_cast<std::size_t>(row) >= files_.size()) {
        patchView_->clear();
        return;
    }
    const git::FileDiff& file = files_[static_cast<std::size_t>(row)];
    if (file.binary)
        patchView_->setPlainText(tr("(binary file — no textual diff)"));
    else if (file.patch.empty())
        patchView_->setPlainText(tr("(no textual changes)"));
    else
        patchView_->setPlainText(QString::fromStdString(file.patch));
}

} // namespace gitscope::ui
