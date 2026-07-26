#pragma once

#include "git/Types.h"

#include <QWidget>

#include <vector>

class QLabel;
class QListWidget;
class QPlainTextEdit;

namespace gitscope::ui {

// Bottom pane: commit metadata header, changed-file list, and the unified
// diff for the selected file.
class DiffViewer : public QWidget {
    Q_OBJECT
public:
    explicit DiffViewer(QWidget* parent = nullptr);

    void setDetails(const git::CommitDetails& details);
    void clear();

private:
    void showFile(int row);

    QLabel* header_;
    QListWidget* fileList_;
    QPlainTextEdit* patchView_;
    std::vector<git::FileDiff> files_;
};

} // namespace gitscope::ui
