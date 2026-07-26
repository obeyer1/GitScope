#include "ui/CommitTableModel.h"

#include <QDateTime>

#include <algorithm>

namespace gitscope::ui {

CommitTableModel::CommitTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void CommitTableModel::setCommits(std::vector<git::CommitInfo> commits,
                                  std::vector<git::GraphRow> rows,
                                  const git::DecorationMap& decorations)
{
    beginResetModel();
    commits_ = std::move(commits);
    rows_ = std::move(rows);

    maxLanes_ = 1;
    for (const git::GraphRow& row : rows_)
        maxLanes_ = std::max(maxLanes_, row.laneCount);

    refs_.clear();
    refs_.resize(commits_.size());
    for (std::size_t i = 0; i < commits_.size(); ++i) {
        const auto it = decorations.find(commits_[i].id);
        if (it == decorations.end())
            continue;
        QStringList labels;
        labels.reserve(static_cast<qsizetype>(it->second.size()));
        for (const git::RefDecoration& deco : it->second) {
            labels << QString(QLatin1Char(deco.type)) + QLatin1Char(':')
                          + QString::fromStdString(deco.label);
        }
        refs_[i] = labels;
    }
    endResetModel();
}

void CommitTableModel::clear()
{
    beginResetModel();
    commits_.clear();
    rows_.clear();
    refs_.clear();
    maxLanes_ = 1;
    endResetModel();
}

const git::CommitInfo* CommitTableModel::commitAt(int row) const
{
    if (row < 0 || static_cast<std::size_t>(row) >= commits_.size())
        return nullptr;
    return &commits_[static_cast<std::size_t>(row)];
}

int CommitTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(commits_.size());
}

int CommitTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant CommitTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};
    const auto row = static_cast<std::size_t>(index.row());
    if (row >= commits_.size())
        return {};
    const git::CommitInfo& commit = commits_[row];

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case GraphColumn:
            return {};
        case SummaryColumn:
            return QString::fromStdString(commit.summary);
        case AuthorColumn:
            return QString::fromStdString(commit.authorName);
        case DateColumn:
            return QDateTime::fromSecsSinceEpoch(commit.authorTime)
                .toString(QStringLiteral("yyyy-MM-dd HH:mm"));
        case HashColumn:
            return QString::fromStdString(commit.shortId);
        default:
            return {};
        }
    case Qt::ToolTipRole:
        if (index.column() == SummaryColumn)
            return QString::fromStdString(commit.summary);
        if (index.column() == HashColumn)
            return QString::fromStdString(commit.id);
        return {};
    case GraphRowRole:
        if (row < rows_.size())
            return QVariant::fromValue(rows_[row]);
        return {};
    case RefsRole:
        if (row < refs_.size())
            return refs_[row];
        return {};
    case CommitIdRole:
        return QString::fromStdString(commit.id);
    case AuthorRole:
        return QString::fromStdString(commit.authorName + " <" + commit.authorEmail + ">");
    default:
        return {};
    }
}

QVariant CommitTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case GraphColumn:
        return QString();
    case SummaryColumn:
        return tr("Message");
    case AuthorColumn:
        return tr("Author");
    case DateColumn:
        return tr("Date");
    case HashColumn:
        return tr("Commit");
    default:
        return {};
    }
}

void CommitFilterProxy::setNeedle(const QString& needle)
{
    needle_ = needle.trimmed();
    invalidate();
}

bool CommitFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (needle_.isEmpty())
        return true;
    const QAbstractItemModel* model = sourceModel();
    const QModelIndex summary = model->index(sourceRow, CommitTableModel::SummaryColumn, sourceParent);
    if (summary.data(Qt::DisplayRole).toString().contains(needle_, Qt::CaseInsensitive))
        return true;
    if (summary.data(CommitTableModel::AuthorRole).toString().contains(needle_, Qt::CaseInsensitive))
        return true;
    if (summary.data(CommitTableModel::CommitIdRole)
            .toString()
            .startsWith(needle_, Qt::CaseInsensitive))
        return true;
    const QStringList refs = summary.data(CommitTableModel::RefsRole).toStringList();
    for (const QString& ref : refs) {
        if (ref.mid(2).contains(needle_, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

} // namespace gitscope::ui
