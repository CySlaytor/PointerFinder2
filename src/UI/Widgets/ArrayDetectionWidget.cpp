#include "ArrayDetectionWidget.h"

#include "../../Core/CodeNoteHelper.h"
#include "../../Models/CodeNoteSettings.h"
#include "../../Models/GlobalSettings.h"
#include "../Styles/ThemeManager.h"

#include <algorithm>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QMenu>
#include <QVBoxLayout>

namespace PointerFinder2::UI::Controls {

    using namespace PointerFinder2::DataModels;
    using namespace PointerFinder2::Core;

    // Initializes tree layouts, column widths, and copy shortcuts
    // required to display and interact with clustered array memory.
    ArrayDetectionWidget::ArrayDetectionWidget(QWidget* parent)
        : QWidget(parent) {
        setupLayout();
    }

    ArrayDetectionWidget::~ArrayDetectionWidget() = default;

    void ArrayDetectionWidget::setupLayout() {
        auto* arrayLayout = new QVBoxLayout(this);
        arrayLayout->setContentsMargins(5, 5, 5, 5);

        m_arrayTreeWidget = new QTreeWidget(this);
        m_arrayTreeWidget->setColumnCount(3);
        m_arrayTreeWidget->setHeaderLabels({ "Detected Array Groups / Indices", "Pointer Address (RAM)", "Pointed Target Address" });
        m_arrayTreeWidget->setAlternatingRowColors(true);
        m_arrayTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

        m_arrayTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Interactive);
        m_arrayTreeWidget->header()->setSectionResizeMode(1, QHeaderView::Interactive);
        m_arrayTreeWidget->header()->setSectionResizeMode(2, QHeaderView::Interactive);
        m_arrayTreeWidget->header()->setStretchLastSection(true);

        m_arrayTreeWidget->setColumnWidth(0, 250);
        m_arrayTreeWidget->setColumnWidth(1, 120);
        m_arrayTreeWidget->setColumnWidth(2, 120);

        QFont treeFont("Consolas", 10);
        treeFont.setStyleHint(QFont::Monospace);
        m_arrayTreeWidget->setFont(treeFont);

        arrayLayout->addWidget(m_arrayTreeWidget);

        connect(m_arrayTreeWidget, &QWidget::customContextMenuRequested, this, &ArrayDetectionWidget::showArrayContextMenu);

        auto* arrayCopyAction = new QAction(this);
        arrayCopyAction->setShortcut(QKeySequence::Copy);
        connect(arrayCopyAction, &QAction::triggered, this, [this]() {
            QList<QTreeWidgetItem*> selected = m_arrayTreeWidget->selectedItems();
            if (selected.empty()) return;
            auto* item = selected.first();

            if (!item->text(1).isEmpty() && !item->text(2).isEmpty()) {
                QApplication::clipboard()->setText(item->text(1));
                emit statusMessageRequested("Element RAM address copied to clipboard.");
            }
            else {
                auto* rootItem = item;
                while (rootItem->parent() != nullptr) {
                    rootItem = rootItem->parent();
                }
                int matchIdx = rootItem->data(0, Qt::UserRole).toInt();
                if (matchIdx >= 0 && matchIdx < static_cast<int>(m_arrayMatches.size())) {
                    const auto& match = m_arrayMatches[matchIdx];
                    QString addrStr = m_currentManager ? m_currentManager->formatDisplayAddress(match.path.baseAddress) : QString::number(match.path.baseAddress, 16).toUpper();
                    QApplication::clipboard()->setText(addrStr);
                    emit statusMessageRequested("Base address copied to clipboard.");
                }
            }
            });
        m_arrayTreeWidget->addAction(arrayCopyAction);

        auto* separatorAction = new QAction(this);
        separatorAction->setSeparator(true);
        m_arrayTreeWidget->addAction(separatorAction);

        auto* arrayCopyRaAction = new QAction(this);
        arrayCopyRaAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
        connect(arrayCopyRaAction, &QAction::triggered, this, [this]() {
            QList<QTreeWidgetItem*> selected = m_arrayTreeWidget->selectedItems();
            if (selected.empty()) return;
            auto* item = selected.first();

            auto* rootItem = item;
            while (rootItem->parent() != nullptr) {
                rootItem = rootItem->parent();
            }

            int matchIdx = rootItem->data(0, Qt::UserRole).toInt();
            if (matchIdx >= 0 && matchIdx < static_cast<int>(m_arrayMatches.size()) && m_currentManager) {
                const auto& match = m_arrayMatches[matchIdx];
                QApplication::clipboard()->setText(match.path.toRetroAchievementsString(m_currentManager));
                emit statusMessageRequested("Path copied in RetroAchievements format.");
            }
            });
        m_arrayTreeWidget->addAction(arrayCopyRaAction);
    }

    QIcon ArrayDetectionWidget::getThemeIcon(const QString& resourcePath) const {
        return ThemeManager::getIcon(resourcePath, GlobalSettings::activeTheme);
    }

    void ArrayDetectionWidget::updateThemeIcons() {
    }

    void ArrayDetectionWidget::setEmulatorManager(Emulators::IEmulatorManager* manager) {
        m_currentManager = manager;
    }

    void ArrayDetectionWidget::setResults(const std::vector<ArrayGroup>& groups, const std::vector<PathArrayMatch>& matches) {
        m_arrayGroupsList = groups;
        m_arrayMatches = matches;
        refreshUI();
    }

    void ArrayDetectionWidget::clear() {
        m_arrayGroupsList.clear();
        m_arrayMatches.clear();
        m_arrayTreeWidget->clear();
    }

    // Formats the structured tree display, mapping elements, offsets,
    // and targets to highlight verified array boundaries in color.
    void ArrayDetectionWidget::refreshUI() {
        m_arrayTreeWidget->blockSignals(true);
        m_arrayTreeWidget->clear();

        if (!m_currentManager || m_arrayMatches.empty()) {
            m_arrayTreeWidget->blockSignals(false);
            return;
        }

        for (size_t i = 0; i < m_arrayMatches.size(); ++i) {
            const auto& match = m_arrayMatches[i];
            const auto& path = match.path;

            auto* pathItem = new QTreeWidgetItem(m_arrayTreeWidget);
            QString baseStr = m_currentManager->formatDisplayAddress(path.baseAddress);
            QString finalStr = m_currentManager->formatDisplayAddress(path.finalAddress);

            pathItem->setText(0, QString("0x%1 [%2 Array Elements] (Final: 0x%3)")
                .arg(baseStr)
                .arg(match.group.elementCount)
                .arg(finalStr));
            pathItem->setData(0, Qt::UserRole, static_cast<int>(i));

            std::vector<QTreeWidgetItem*> stepItems;

            auto* step0Item = new QTreeWidgetItem(pathItem);
            QString step0AddrStr = m_currentManager->formatDisplayAddress(path.baseAddress);
            step0Item->setText(0, QString("Base Address: 0x%1").arg(step0AddrStr));
            stepItems.push_back(step0Item);

            for (size_t j = 0; j < path.offsets.size(); ++j) {
                int32_t offset = path.offsets[j];
                auto* stepItem = new QTreeWidgetItem(pathItem);

                QString sign = (offset < 0) ? "-" : "+";
                QString offsetHex = QString::number(std::abs(offset), 16).toUpper();

                // Separating "0x" and the formatting arguments into separate fields prevents the Qt parser
                // from misinterpreting `%20` as parameter 20.
                stepItem->setText(0, QString("Offset %1: %2%3").arg(j + 1).arg(sign).arg("0x" + offsetHex));
                stepItems.push_back(stepItem);
            }

            if (match.stepIndex >= 0 && match.stepIndex < static_cast<int>(stepItems.size())) {
                auto* targetHighlightItem = stepItems[match.stepIndex];

                QFont boldFont = targetHighlightItem->font(0);
                boldFont.setBold(true);
                targetHighlightItem->setFont(0, boldFont);
                targetHighlightItem->setForeground(0, QColor(58, 220, 58));
                targetHighlightItem->setText(0, targetHighlightItem->text(0) + " [Array Detected]");

                for (size_t k = 0; k < match.group.elementAddresses.size(); ++k) {
                    auto* elemItem = new QTreeWidgetItem(targetHighlightItem);

                    QString elemAddr = m_currentManager->formatDisplayAddress(match.group.elementAddresses[k]);
                    QString targetVal = m_currentManager->formatDisplayAddress(match.group.targets[k]);

                    elemItem->setText(0, QString("Element %1").arg(k + 1));
                    elemItem->setText(1, elemAddr);
                    elemItem->setText(2, targetVal);
                    elemItem->setFlags(elemItem->flags() & ~Qt::ItemIsEditable);
                }
            }
        }

        m_arrayTreeWidget->blockSignals(false);
    }

    void ArrayDetectionWidget::showArrayContextMenu(const QPoint& pos) {
        auto* item = m_arrayTreeWidget->itemAt(pos);
        if (!item) return;

        QMenu menu(this);

        if (!item->text(1).isEmpty() && !item->text(2).isEmpty()) {
            auto* copyElemAction = menu.addAction(getThemeIcon(":/icons/copy.svg"), "Copy Element Address");
            connect(copyElemAction, &QAction::triggered, this, [=]() {
                QApplication::clipboard()->setText(item->text(1));
                emit statusMessageRequested("Element RAM address copied to clipboard.");
                });

            auto* copyTargetAction = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy Target Value");
            connect(copyTargetAction, &QAction::triggered, this, [=]() {
                QApplication::clipboard()->setText(item->text(2));
                emit statusMessageRequested("Element pointed target address copied to clipboard.");
                });
        }
        else if (item->parent() == nullptr) {
            int matchIdx = item->data(0, Qt::UserRole).toInt();
            if (matchIdx >= 0 && matchIdx < static_cast<int>(m_arrayMatches.size())) {
                const auto& match = m_arrayMatches[matchIdx];

                auto* copyBaseAct = menu.addAction(getThemeIcon(":/icons/copy.svg"), "Copy Base Address");
                connect(copyBaseAct, &QAction::triggered, this, [=]() {
                    QString addrStr = m_currentManager ? m_currentManager->formatDisplayAddress(match.path.baseAddress) : QString::number(match.path.baseAddress, 16).toUpper();
                    QApplication::clipboard()->setText(addrStr);
                    emit statusMessageRequested("Base address copied to clipboard.");
                    });

                auto* copyRaAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy as RetroAchievements Format");
                connect(copyRaAct, &QAction::triggered, this, [=]() {
                    if (m_currentManager) {
                        QApplication::clipboard()->setText(match.path.toRetroAchievementsString(m_currentManager));
                        emit statusMessageRequested("Path copied in RetroAchievements format.");
                    }
                    });

                auto* copyNoteAct = menu.addAction(getThemeIcon(":/icons/copy_note.svg"), "Copy as Code Note");
                connect(copyNoteAct, &QAction::triggered, this, [=]() {
                    CodeNoteSettings settings = CodeNoteSettings::getFromGlobalSettings();
                    QString note = CodeNoteHelper::buildCodeNote(match.path.offsets, settings);
                    QApplication::clipboard()->setText(note);
                    emit statusMessageRequested("Path copied as custom code note.");
                    });

                menu.addSeparator();

                auto* addBookmarkAct = menu.addAction(getThemeIcon(":/icons/bookmark_add.svg"), "Add to Bookmarks");
                connect(addBookmarkAct, &QAction::triggered, this, [this, match]() {
                    emit bookmarkRequested(match.path);
                    });
            }
        }

        if (!menu.actions().isEmpty()) {
            menu.exec(m_arrayTreeWidget->viewport()->mapToGlobal(pos));
        }
    }

}
