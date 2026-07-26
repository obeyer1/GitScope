#include "ui/BranchPanel.h"

#include <QTreeWidget>
#include <QVBoxLayout>

namespace gitscope::ui {

namespace {
constexpr int kRefRole = Qt::UserRole;
}

BranchPanel::BranchPanel(QWidget* parent) : QWidget(parent)
{
    tree_ = new QTreeWidget(this);
    tree_->setHeaderHidden(true);
    tree_->setColumnCount(1);
    tree_->setRootIsDecorated(true);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tree_);

    connect(tree_, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
                if (current == nullptr)
                    return;
                const QVariant ref = current->data(0, kRefRole);
                if (ref.isValid())
                    emit refSelected(ref.toString());
            });
}

QTreeWidgetItem* BranchPanel::addGroup(const QString& title)
{
    auto* group = new QTreeWidgetItem(tree_, {title});
    group->setFlags(Qt::ItemIsEnabled); // group headers are not selectable
    QFont font = group->font(0);
    font.setBold(true);
    group->setFont(0, font);
    group->setExpanded(true);
    return group;
}

void BranchPanel::setRefs(const std::vector<git::BranchInfo>& branches,
                          const std::vector<git::TagInfo>& tags)
{
    tree_->blockSignals(true);
    tree_->clear();

    auto* allItem = new QTreeWidgetItem(tree_, {tr("All refs")});
    allItem->setData(0, kRefRole, QString());
    QFont allFont = allItem->font(0);
    allFont.setItalic(true);
    allItem->setFont(0, allFont);

    QTreeWidgetItem* localGroup = nullptr;
    QTreeWidgetItem* remoteGroup = nullptr;
    for (const git::BranchInfo& branch : branches) {
        QTreeWidgetItem* group = nullptr;
        if (branch.isRemote) {
            if (remoteGroup == nullptr)
                remoteGroup = addGroup(tr("Remotes"));
            group = remoteGroup;
        } else {
            if (localGroup == nullptr)
                localGroup = addGroup(tr("Branches"));
            group = localGroup;
        }
        auto* item = new QTreeWidgetItem(group);
        item->setText(0, QString::fromStdString(branch.shortName)
                             + (branch.isHead ? QStringLiteral("  ●") : QString()));
        item->setData(0, kRefRole, QString::fromStdString(branch.refName));
        if (branch.isHead) {
            QFont font = item->font(0);
            font.setBold(true);
            item->setFont(0, font);
        }
    }

    if (!tags.empty()) {
        QTreeWidgetItem* tagGroup = addGroup(tr("Tags"));
        for (const git::TagInfo& tag : tags) {
            auto* item = new QTreeWidgetItem(tagGroup);
            item->setText(0, QString::fromStdString(tag.shortName));
            item->setData(0, kRefRole, QString::fromStdString(tag.refName));
        }
    }

    tree_->setCurrentItem(allItem);
    tree_->blockSignals(false);
}

void BranchPanel::clear()
{
    tree_->blockSignals(true);
    tree_->clear();
    tree_->blockSignals(false);
}

} // namespace gitscope::ui
