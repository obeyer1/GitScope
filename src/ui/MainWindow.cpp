#include "ui/MainWindow.h"

#include "git/GitException.h"
#include "git/GraphBuilder.h"
#include "ui/BranchPanel.h"
#include "ui/CommitTableModel.h"
#include "ui/DiffViewer.h"
#include "ui/GraphDelegate.h"
#include "ui/SummaryDelegate.h"

#include <QAction>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>
#include <QUrl>

#include <algorithm>

namespace gitscope::ui {

namespace {
// Hard cap so pathologically large repositories stay responsive.
constexpr std::size_t kMaxCommits = 20000;
} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("GitScope"));
    resize(1280, 800);
    setAcceptDrops(true);

    // --- Actions -----------------------------------------------------------
    auto* openAction = new QAction(tr("&Open Repository…"), this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::chooseRepository);

    refreshAction_ = new QAction(tr("&Refresh"), this);
    refreshAction_->setShortcut(QKeySequence::Refresh);
    refreshAction_->setEnabled(false);
    connect(refreshAction_, &QAction::triggered, this, &MainWindow::refresh);

    closeAction_ = new QAction(tr("&Close Repository"), this);
    closeAction_->setEnabled(false);
    connect(closeAction_, &QAction::triggered, this, &MainWindow::closeRepository);

    auto* quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::closeAllWindows);

    auto* aboutAction = new QAction(tr("&About GitScope"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(refreshAction_);
    fileMenu->addAction(closeAction_);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);

    // --- Toolbar -----------------------------------------------------------
    QToolBar* toolbar = addToolBar(tr("Main"));
    toolbar->setMovable(false);
    toolbar->addAction(openAction);
    toolbar->addAction(refreshAction_);
    auto* spacer = new QWidget(toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    filterEdit_ = new QLineEdit(toolbar);
    filterEdit_->setPlaceholderText(tr("Filter commits (message, author, hash)…"));
    filterEdit_->setClearButtonEnabled(true);
    filterEdit_->setMaximumWidth(320);
    filterEdit_->setEnabled(false);
    toolbar->addWidget(filterEdit_);

    // --- Commit table ------------------------------------------------------
    model_ = new CommitTableModel(this);
    proxy_ = new CommitFilterProxy(this);
    proxy_->setSourceModel(model_);

    commitTable_ = new QTableView(this);
    commitTable_->setModel(proxy_);
    commitTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    commitTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    commitTable_->setShowGrid(false);
    commitTable_->setWordWrap(false);
    commitTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    commitTable_->verticalHeader()->setVisible(false);
    commitTable_->verticalHeader()->setDefaultSectionSize(24);
    commitTable_->horizontalHeader()->setStretchLastSection(false);
    commitTable_->horizontalHeader()->setSectionResizeMode(CommitTableModel::SummaryColumn,
                                                           QHeaderView::Stretch);
    commitTable_->setItemDelegateForColumn(CommitTableModel::GraphColumn,
                                           new GraphDelegate(commitTable_));
    commitTable_->setItemDelegateForColumn(CommitTableModel::SummaryColumn,
                                           new SummaryDelegate(commitTable_));

    connect(commitTable_->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex& current, const QModelIndex&) { onCommitSelected(current); });
    connect(filterEdit_, &QLineEdit::textChanged, this,
            [this](const QString& text) { proxy_->setNeedle(text); });

    // --- Panels ------------------------------------------------------------
    branchPanel_ = new BranchPanel(this);
    connect(branchPanel_, &BranchPanel::refSelected, this, &MainWindow::onRefSelected);

    diffViewer_ = new DiffViewer(this);

    auto* rightSplitter = new QSplitter(Qt::Vertical, this);
    rightSplitter->addWidget(commitTable_);
    rightSplitter->addWidget(diffViewer_);
    rightSplitter->setStretchFactor(0, 3);
    rightSplitter->setStretchFactor(1, 2);
    rightSplitter->setChildrenCollapsible(false);

    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(branchPanel_);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setSizes({220, 1060});
    mainSplitter->setChildrenCollapsible(false);

    // --- Welcome page ------------------------------------------------------
    auto* welcome = new QLabel(
        tr("<div style='text-align:center'>"
           "<h2>GitScope</h2>"
           "<p>Local-only, read-only Git repository browser.</p>"
           "<p>Open a repository with <b>File → Open Repository…</b> (%1),<br>"
           "drop a folder onto this window, or pass a path on the command line.</p>"
           "</div>")
            .arg(QKeySequence(QKeySequence::Open).toString(QKeySequence::NativeText)));
    welcome->setAlignment(Qt::AlignCenter);

    stack_ = new QStackedWidget(this);
    stack_->addWidget(welcome);      // index 0
    stack_->addWidget(mainSplitter); // index 1
    setCentralWidget(stack_);

    statusBar()->showMessage(tr("No repository open"));
}

void MainWindow::openRepository(const QString& path)
{
    try {
        auto repo = std::make_unique<git::Repository>(git::Repository::open(path.toStdString()));
        repo_ = std::move(repo);
    } catch (const git::GitException& e) {
        QMessageBox::warning(this, QStringLiteral("GitScope"),
                             tr("Could not open a Git repository at\n%1\n\n%2")
                                 .arg(path, QString::fromUtf8(e.what())));
        return;
    }

    currentRef_.clear();
    filterEdit_->clear();
    loadRefs();
    loadLog();

    const git::RepoSummary summary = repo_->summary();
    const QString title = QString::fromStdString(
        !summary.workDir.empty() ? summary.workDir : summary.gitDir);
    setWindowTitle(QStringLiteral("GitScope — %1").arg(title));
    refreshAction_->setEnabled(true);
    closeAction_->setEnabled(true);
    filterEdit_->setEnabled(true);
    stack_->setCurrentIndex(1);
}

void MainWindow::chooseRepository()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Open Git Repository"), QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly);
    if (!dir.isEmpty())
        openRepository(dir);
}

void MainWindow::refresh()
{
    if (!repo_)
        return;
    loadRefs();
    loadLog();
}

void MainWindow::closeRepository()
{
    repo_.reset();
    currentRef_.clear();
    model_->clear();
    branchPanel_->clear();
    diffViewer_->clear();
    filterEdit_->clear();
    filterEdit_->setEnabled(false);
    refreshAction_->setEnabled(false);
    closeAction_->setEnabled(false);
    setWindowTitle(QStringLiteral("GitScope"));
    statusBar()->showMessage(tr("No repository open"));
    stack_->setCurrentIndex(0);
}

void MainWindow::onRefSelected(const QString& refName)
{
    if (!repo_ || currentRef_ == refName)
        return;
    currentRef_ = refName;
    loadLog();
}

void MainWindow::onCommitSelected(const QModelIndex& current)
{
    if (!repo_ || !current.isValid()) {
        diffViewer_->clear();
        return;
    }
    const QModelIndex source = proxy_->mapToSource(current);
    const git::CommitInfo* info = model_->commitAt(source.row());
    if (info == nullptr) {
        diffViewer_->clear();
        return;
    }
    try {
        diffViewer_->setDetails(repo_->details(info->id));
    } catch (const git::GitException& e) {
        diffViewer_->clear();
        statusBar()->showMessage(tr("Failed to load commit: %1").arg(QString::fromUtf8(e.what())));
    }
}

void MainWindow::loadRefs()
{
    if (!repo_)
        return;
    try {
        branchPanel_->setRefs(repo_->branches(), repo_->tags());
    } catch (const git::GitException& e) {
        QMessageBox::warning(this, QStringLiteral("GitScope"),
                             tr("Failed to read references:\n%1").arg(QString::fromUtf8(e.what())));
    }
}

void MainWindow::loadLog()
{
    if (!repo_)
        return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    try {
        auto commits = repo_->log(currentRef_.toStdString(), kMaxCommits);
        auto rows = git::GraphBuilder::build(commits);
        const std::size_t count = commits.size();
        model_->setCommits(std::move(commits), std::move(rows), repo_->decorations());

        const int lanes = std::min(model_->maxLaneCount(), GraphDelegate::maxDisplayLanes());
        commitTable_->setColumnWidth(CommitTableModel::GraphColumn,
                                     GraphDelegate::laneWidth() * (lanes + 1) + 8);
        commitTable_->resizeColumnToContents(CommitTableModel::AuthorColumn);
        commitTable_->resizeColumnToContents(CommitTableModel::DateColumn);
        commitTable_->resizeColumnToContents(CommitTableModel::HashColumn);

        const QString scope = currentRef_.isEmpty() ? tr("all refs") : currentRef_;
        QString message = tr("%1 commits — %2").arg(count).arg(scope);
        if (count >= kMaxCommits)
            message += tr(" (showing the first %1)").arg(kMaxCommits);
        statusBar()->showMessage(message);

        if (proxy_->rowCount() > 0) {
            commitTable_->scrollToTop();
            commitTable_->setCurrentIndex(
                proxy_->index(0, CommitTableModel::SummaryColumn));
        } else {
            diffViewer_->clear();
        }
    } catch (const git::GitException& e) {
        model_->clear();
        diffViewer_->clear();
        QMessageBox::warning(this, QStringLiteral("GitScope"),
                             tr("Failed to load history:\n%1").arg(QString::fromUtf8(e.what())));
    }
    QApplication::restoreOverrideCursor();
}

void MainWindow::showAbout()
{
    QMessageBox::about(
        this, tr("About GitScope"),
        tr("<h3>GitScope %1</h3>"
           "<p>Local-only, read-only Git repository visualization client.</p>"
           "<p>GitScope never modifies repositories, never accesses the network, "
           "and never runs subprocesses or Git hooks.</p>"
           "<p>Qt %2 — libgit2 %3</p>"
           "<p><a href='https://github.com/obeyer1/GitScope'>github.com/obeyer1/GitScope</a> — "
           "MIT license</p>")
            .arg(QStringLiteral(GITSCOPE_VERSION), QString::fromUtf8(qVersion()),
                 QString::fromUtf8(git::LibGit2::versionString())));
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty()
        && event->mimeData()->urls().first().isLocalFile())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;
    QString path = urls.first().toLocalFile();
    const QFileInfo info(path);
    if (info.isFile())
        path = info.absolutePath();
    openRepository(path);
}

} // namespace gitscope::ui
