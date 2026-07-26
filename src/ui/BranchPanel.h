#pragma once

#include "git/Types.h"

#include <QWidget>

#include <vector>

class QTreeWidget;
class QTreeWidgetItem;

namespace gitscope::ui {

// Sidebar listing "All refs", local branches, remotes, and tags. Selecting
// an entry emits refSelected with the full ref name ("" for all refs).
class BranchPanel : public QWidget {
    Q_OBJECT
public:
    explicit BranchPanel(QWidget* parent = nullptr);

    void setRefs(const std::vector<git::BranchInfo>& branches,
                 const std::vector<git::TagInfo>& tags);
    void clear();

signals:
    void refSelected(const QString& refName);

private:
    QTreeWidgetItem* addGroup(const QString& title);

    QTreeWidget* tree_;
};

} // namespace gitscope::ui
