#include "BookmarksWidget.h"

#include "../../Core/CodeNoteHelper.h"
#include "../../Models/CodeNoteSettings.h"
#include "../../Models/GlobalSettings.h"
#include "../Styles/ThemeManager.h"

#include <algorithm>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QVBoxLayout>

namespace PointerFinder2::UI::Controls {

    using namespace PointerFinder2::DataModels;
    using namespace PointerFinder2::Core;

    // Sets up the saved watchpoint list, including edit triggers,
    // search columns, and control buttons.
    BookmarksWidget::BookmarksWidget(QWidget* parent)
        : QWidget(parent) {
        setupLayout();
    }

    BookmarksWidget::~BookmarksWidget() = default;

    void BookmarksWidget::setupLayout() {
        auto* bookmarkLayout = new QVBoxLayout(this);
        bookmarkLayout->setContentsMargins(5, 5, 5, 5);

        m_bookmarkTreeWidget = new QTreeWidget(this);
        m_bookmarkTreeWidget->setColumnCount(3);
        m_bookmarkTreeWidget->setHeaderLabels({ "Description / Offsets", "Base Address", "Resolved Address" });
        m_bookmarkTreeWidget->setAlternatingRowColors(true);
        m_bookmarkTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        m_bookmarkTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_bookmarkTreeWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

        m_bookmarkTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Interactive);
        m_bookmarkTreeWidget->header()->setSectionResizeMode(1, QHeaderView::Interactive);
        m_bookmarkTreeWidget->header()->setSectionResizeMode(2, QHeaderView::Interactive);
        m_bookmarkTreeWidget->header()->setStretchLastSection(true);

        m_bookmarkTreeWidget->setColumnWidth(0, 250);
        m_bookmarkTreeWidget->setColumnWidth(1, 100);
        m_bookmarkTreeWidget->setColumnWidth(2, 120);

        QFont treeFont("Consolas", 10);
        treeFont.setStyleHint(QFont::Monospace);
        m_bookmarkTreeWidget->setFont(treeFont);

        bookmarkLayout->addWidget(m_bookmarkTreeWidget);

        auto* bookmarkButtons = new QHBoxLayout();
        m_refreshBookmarksBtn = new QPushButton("Refresh Watchlist", this);
        m_deleteBookmarkBtn = new QPushButton("Remove Selected", this);
        bookmarkButtons->addWidget(m_refreshBookmarksBtn);
        bookmarkButtons->addWidget(m_deleteBookmarkBtn);
        bookmarkButtons->addStretch();
        bookmarkLayout->addLayout(bookmarkButtons);

        connect(m_refreshBookmarksBtn, &QPushButton::clicked, this, &BookmarksWidget::onRefreshBookmarks);
        connect(m_deleteBookmarkBtn, &QPushButton::clicked, this, &BookmarksWidget::onDeleteSelectedBookmarks);
        connect(m_bookmarkTreeWidget, &QWidget::customContextMenuRequested, this, &BookmarksWidget::showBookmarkContextMenu);
        connect(m_bookmarkTreeWidget, &QTreeWidget::itemChanged, this, &BookmarksWidget::onBookmarkItemChanged);

        auto* bmDeleteAction = new QAction(this);
        bmDeleteAction->setShortcut(QKeySequence::Delete);
        connect(bmDeleteAction, &QAction::triggered, this, &BookmarksWidget::onDeleteSelectedBookmarks);
        m_bookmarkTreeWidget->addAction(bmDeleteAction);

        auto* bmCopyBaseAction = new QAction(this);
        bmCopyBaseAction->setShortcut(QKeySequence::Copy);
        connect(bmCopyBaseAction, &QAction::triggered, this, [this]() {
            QList<QTreeWidgetItem*> selected = m_bookmarkTreeWidget->selectedItems();
            if (selected.empty()) return;
            QTreeWidgetItem* rootItem = selected.first();
            while (rootItem->parent() != nullptr) {
                rootItem = rootItem->parent();
            }
            int index = m_bookmarkTreeWidget->indexOfTopLevelItem(rootItem);
            if (index >= 0 && index < static_cast<int>(m_bookmarksList.size())) {
                const auto& bm = m_bookmarksList[index];
                QString addrStr = m_currentManager ? m_currentManager->formatDisplayAddress(bm.path.baseAddress) : QString::number(bm.path.baseAddress, 16).toUpper();
                QApplication::clipboard()->setText(addrStr);
                emit statusMessageRequested("Base address copied to clipboard.");
            }
            });
        m_bookmarkTreeWidget->addAction(bmCopyBaseAction);

        auto* bmCopyRaAction = new QAction(this);
        bmCopyRaAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
        connect(bmCopyRaAction, &QAction::triggered, this, [this]() {
            QList<QTreeWidgetItem*> selected = m_bookmarkTreeWidget->selectedItems();
            if (selected.empty()) return;
            QTreeWidgetItem* rootItem = selected.first();
            while (rootItem->parent() != nullptr) {
                rootItem = rootItem->parent();
            }
            int index = m_bookmarkTreeWidget->indexOfTopLevelItem(rootItem);
            if (index >= 0 && index < static_cast<int>(m_bookmarksList.size())) {
                const auto& bm = m_bookmarksList[index];
                if (m_currentManager) {
                    QApplication::clipboard()->setText(bm.path.toRetroAchievementsString(m_currentManager));
                    emit statusMessageRequested("Bookmark path copied in RetroAchievements format.");
                }
            }
            });
        m_bookmarkTreeWidget->addAction(bmCopyRaAction);
    }

    QIcon BookmarksWidget::getThemeIcon(const QString& resourcePath) const {
        return ThemeManager::getIcon(resourcePath, GlobalSettings::activeTheme);
    }

    void BookmarksWidget::updateThemeIcons() {
    }

    void BookmarksWidget::setEmulatorManager(Emulators::IEmulatorManager* manager) {
        m_currentManager = manager;
    }

    void BookmarksWidget::addBookmark(const Bookmark& bookmark) {
        auto it = std::find_if(m_bookmarksList.begin(), m_bookmarksList.end(), [&](const Bookmark& item) {
            return item.path == bookmark.path;
            });

        if (it == m_bookmarksList.end()) {
            m_bookmarksList.push_back(bookmark);
            refreshBookmarksUI();
        }
    }

    void BookmarksWidget::addBookmarks(const std::vector<Bookmark>& bookmarks) {
        for (const auto& bm : bookmarks) {
            auto it = std::find_if(m_bookmarksList.begin(), m_bookmarksList.end(), [&](const Bookmark& item) {
                return item.path == bm.path;
                });
            if (it == m_bookmarksList.end()) {
                m_bookmarksList.push_back(bm);
            }
        }
        refreshBookmarksUI();
    }

    void BookmarksWidget::setBookmarks(const std::vector<Bookmark>& bookmarks) {
        m_bookmarksList = bookmarks;
        refreshBookmarksUI();
    }

    void BookmarksWidget::clearBookmarks() {
        m_bookmarksList.clear();
        m_bookmarkTreeWidget->clear();
    }

    // Reads active emulator memory and recalculates multi-level pointer paths
    // to resolve dynamic watchpoint targets in real-time.
    void BookmarksWidget::refreshBookmarksUI() {
        m_bookmarkTreeWidget->blockSignals(true);
        m_bookmarkTreeWidget->clear();

        for (size_t i = 0; i < m_bookmarksList.size(); ++i) {
            const auto& bm = m_bookmarksList[i];

            auto* rootItem = new QTreeWidgetItem(m_bookmarkTreeWidget);
            rootItem->setText(0, bm.name);
            rootItem->setFlags(rootItem->flags() | Qt::ItemIsEditable);

            QString baseStr = m_currentManager ? m_currentManager->formatDisplayAddress(bm.path.baseAddress) : QString::number(bm.path.baseAddress, 16).toUpper();
            rootItem->setText(1, baseStr);

            QString resolvedStr = "N/A";
            if (m_currentManager && m_currentManager->isAttached()) {
                auto resolved = m_currentManager->recalculateFinalAddress(bm.path, bm.path.finalAddress);
                if (resolved.has_value()) {
                    resolvedStr = m_currentManager->formatDisplayAddress(resolved.value());
                }
            }
            rootItem->setText(2, resolvedStr);

            for (size_t j = 0; j < bm.path.offsets.size(); ++j) {
                int32_t offset = bm.path.offsets[j];
                auto* childItem = new QTreeWidgetItem(rootItem);

                QString offsetStr;
                QString sign = (offset < 0) ? "-" : "+";
                QString offsetHex = QString::number(std::abs(offset), 16).toUpper();

                // Separating "0x" and the formatting arguments into separate fields prevents the Qt parser
                // from misinterpreting `%20` as parameter 20.
                offsetStr = QString("Offset %1: %2%3")
                    .arg(j + 1)
                    .arg(sign)
                    .arg("0x" + offsetHex);

                childItem->setText(0, offsetStr);
                childItem->setFlags(childItem->flags() & ~Qt::ItemIsEditable);
            }
        }
        m_bookmarkTreeWidget->blockSignals(false);
    }

    void BookmarksWidget::onRefreshBookmarks() {
        refreshBookmarksUI();
        emit statusMessageRequested("Bookmark resolved addresses refreshed.");
    }

    void BookmarksWidget::onDeleteSelectedBookmarks() {
        QList<QTreeWidgetItem*> selectedItems = m_bookmarkTreeWidget->selectedItems();
        if (selectedItems.empty()) return;

        std::vector<int> indicesToRemove;
        for (auto* item : selectedItems) {
            QTreeWidgetItem* rootItem = item;
            while (rootItem->parent() != nullptr) {
                rootItem = rootItem->parent();
            }
            int index = m_bookmarkTreeWidget->indexOfTopLevelItem(rootItem);
            if (index >= 0 && std::find(indicesToRemove.begin(), indicesToRemove.end(), index) == indicesToRemove.end()) {
                indicesToRemove.push_back(index);
            }
        }

        std::sort(indicesToRemove.rbegin(), indicesToRemove.rend());
        for (int idx : indicesToRemove) {
            m_bookmarksList.erase(m_bookmarksList.begin() + idx);
        }

        refreshBookmarksUI();
        emit statusMessageRequested(QString("Removed %1 bookmark(s).").arg(indicesToRemove.size()));
    }

    void BookmarksWidget::showBookmarkContextMenu(const QPoint& pos) {
        QTreeWidgetItem* item = m_bookmarkTreeWidget->itemAt(pos);
        if (!item) return;

        QTreeWidgetItem* rootItem = item;
        while (rootItem->parent() != nullptr) {
            rootItem = rootItem->parent();
        }

        int index = m_bookmarkTreeWidget->indexOfTopLevelItem(rootItem);
        if (index < 0 || index >= static_cast<int>(m_bookmarksList.size())) return;

        const Bookmark& bm = m_bookmarksList[index];

        QMenu menu(this);

        QAction* renameAct = menu.addAction(getThemeIcon(":/icons/rename.svg"), "Rename Bookmark");
        connect(renameAct, &QAction::triggered, this, [=]() {
            m_bookmarkTreeWidget->editItem(rootItem, 0);
            });

        QAction* copyBaseAct = menu.addAction(getThemeIcon(":/icons/copy.svg"), "Copy Base Address");
        connect(copyBaseAct, &QAction::triggered, this, [=]() {
            QString addrStr = m_currentManager ? m_currentManager->formatDisplayAddress(bm.path.baseAddress) : QString::number(bm.path.baseAddress, 16).toUpper();
            QApplication::clipboard()->setText(addrStr);
            emit statusMessageRequested("Base address copied to clipboard.");
            });

        QAction* copyRaAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy as RetroAchievements Format");
        connect(copyRaAct, &QAction::triggered, this, [=]() {
            if (m_currentManager) {
                QApplication::clipboard()->setText(bm.path.toRetroAchievementsString(m_currentManager));
                emit statusMessageRequested("Bookmark path copied in RetroAchievements format.");
            }
            });

        QAction* copyNoteAct = menu.addAction(getThemeIcon(":/icons/copy_note.svg"), "Copy as Code Note");
        connect(copyNoteAct, &QAction::triggered, this, [=]() {
            CodeNoteSettings settings = CodeNoteSettings::getFromGlobalSettings();
            QString note = CodeNoteHelper::buildCodeNote(bm.path.offsets, settings);
            QApplication::clipboard()->setText(note);
            emit statusMessageRequested("Bookmark path copied as custom code note.");
            });

        menu.addSeparator();
        QAction* deleteAct = menu.addAction(getThemeIcon(":/icons/delete.svg"), "Remove Bookmark(s)");
        connect(deleteAct, &QAction::triggered, this, &BookmarksWidget::onDeleteSelectedBookmarks);

        menu.exec(m_bookmarkTreeWidget->viewport()->mapToGlobal(pos));
    }

    void BookmarksWidget::onBookmarkItemChanged(QTreeWidgetItem* item, int column) {
        if (column == 0 && item->parent() == nullptr) {
            int index = m_bookmarkTreeWidget->indexOfTopLevelItem(item);
            if (index >= 0 && index < static_cast<int>(m_bookmarksList.size())) {
                m_bookmarksList[index].name = item->text(0);
                emit statusMessageRequested(QString("Bookmark renamed to '%1'.").arg(item->text(0)));
            }
        }
    }

}
