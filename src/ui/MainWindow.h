#pragma once

#include "git/Repository.h"

#include <QMainWindow>

#include <memory>

class QLabel;
class QLineEdit;
class QStackedWidget;
class QTableView;

namespace gitscope::ui {

class BranchPanel;
class CommitFilterProxy;
class CommitTableModel;
class DiffViewer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    void openRepository(const QString& path);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void chooseRepository();
    void refresh();
    void closeRepository();
    void onRefSelected(const QString& refName);
    void onCommitSelected(const QModelIndex& current);
    void showAbout();
    void loadRefs();
    void loadLog();

    std::unique_ptr<git::Repository> repo_;
    QString currentRef_; // empty = all refs

    QStackedWidget* stack_ = nullptr;
    BranchPanel* branchPanel_ = nullptr;
    QTableView* commitTable_ = nullptr;
    CommitTableModel* model_ = nullptr;
    CommitFilterProxy* proxy_ = nullptr;
    DiffViewer* diffViewer_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QAction* refreshAction_ = nullptr;
    QAction* closeAction_ = nullptr;
};

} // namespace gitscope::ui
