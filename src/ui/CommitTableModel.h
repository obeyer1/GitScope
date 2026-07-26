#pragma once

#include "git/GraphBuilder.h"
#include "git/Types.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QStringList>

#include <vector>

namespace gitscope::ui {

class CommitTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { GraphColumn = 0, SummaryColumn, AuthorColumn, DateColumn, HashColumn, ColumnCount };
    enum Roles {
        GraphRowRole = Qt::UserRole + 1,
        RefsRole,     // QStringList; entries are "<type>:<label>"
        CommitIdRole, // full hex id
        AuthorRole,   // "name <email>", used for filtering
    };

    explicit CommitTableModel(QObject* parent = nullptr);

    void setCommits(std::vector<git::CommitInfo> commits, std::vector<git::GraphRow> rows,
                    const git::DecorationMap& decorations);
    void clear();

    int maxLaneCount() const { return maxLanes_; }
    const git::CommitInfo* commitAt(int row) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    std::vector<git::CommitInfo> commits_;
    std::vector<git::GraphRow> rows_;
    std::vector<QStringList> refs_; // per-row decoration labels, prebuilt
    int maxLanes_ = 1;
};

// Filters on commit message, author, and hash at once.
class CommitFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setNeedle(const QString& needle);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QString needle_;
};

} // namespace gitscope::ui

Q_DECLARE_METATYPE(gitscope::git::GraphRow)
