#include "DissectAddressBar.h"

#include "Models/GlobalSettings.h"
#include "UI/Common/ThemeManager.h"

#include <algorithm>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QTimer>

namespace PointerFinder2::UI {

    using namespace PointerFinder2::Core;
    using namespace PointerFinder2::DataModels;

    DissectAddressBar::DissectAddressBar(QWidget* parent)
        : QWidget(parent) {
        setupLayout();
    }

    DissectAddressBar::~DissectAddressBar() = default;

    QIcon DissectAddressBar::getThemeIcon(const QString& resourcePath) const {
        return ThemeManager::getIcon(resourcePath, GlobalSettings::activeTheme);
    }

    void DissectAddressBar::updateThemeIcons() {
        m_newAddressBtn->setIcon(getThemeIcon(":/icons/bookmark_add.svg"));
        m_deleteAddressBtn->setIcon(getThemeIcon(":/icons/delete.svg"));
        m_saveUserBtn->setIcon(getThemeIcon(":/icons/save.svg"));
        m_revertServerBtn->setIcon(getThemeIcon(":/icons/undo.svg"));
        m_resetBtn->setIcon(getThemeIcon(":/icons/refresh.svg"));
        m_toggleCodeNoteBtn->setIcon(getThemeIcon(":/icons/hierarchy_fixer.svg"));
    }

    void DissectAddressBar::setupLayout() {
        // Top Toolbar
        auto* topToolbar = new QHBoxLayout(this);
        topToolbar->setContentsMargins(0, 0, 0, 0);
        topToolbar->setSpacing(6);
        topToolbar->addWidget(new QLabel("Dissect Address:", this));

        m_addressSelectCombo = new QComboBox(this);
        m_addressSelectCombo->setMinimumWidth(280);
        m_addressSelectCombo->setFixedHeight(26);

        // Model and filter proxy setup for real-time dropdown shrinking
        m_addressSourceModel = new QStandardItemModel(this);
        m_addressProxyModel = new AddressFilterProxyModel(this);
        m_addressProxyModel->setSourceModel(m_addressSourceModel);
        m_addressSelectCombo->setModel(m_addressProxyModel);

        // Win32 classic compact dropdown appearance configuration
        m_addressSelectCombo->setMaxVisibleItems(14);
        auto* popupView = new QListView(m_addressSelectCombo);
        popupView->setUniformItemSizes(true);
        popupView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        popupView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        bool isDark = (GlobalSettings::activeTheme.compare("Light", Qt::CaseInsensitive) != 0);
        popupView->setStyleSheet(QString(
            "QListView {"
            "    background-color: %1;"
            "    color: %2;"
            "    border: 1px solid %3;"
            "    padding: 1px;"
            "    outline: 0;"
            "    font-family: 'Consolas', 'Segoe UI', monospace;"
            "    font-size: 11px;"
            "}"
            "QListView::item {"
            "    height: 19px;"
            "    min-height: 19px;"
            "    padding: 1px 4px;"
            "}"
            "QListView::item:hover {"
            "    background-color: %6;"
            "}"
            "QListView::item:selected {"
            "    background-color: %4;"
            "    color: %5;"
            "}"
        ).arg(isDark ? "#1a1a1a" : "#ffffff")
         .arg(isDark ? "#ffffff" : "#2b2b2b")
         .arg(isDark ? "#383838" : "#c0c0c0")
         .arg(isDark ? "#3b4252" : "#d2def5")
         .arg(isDark ? "#ffffff" : "#1e1e1e")
         .arg(isDark ? "#2c2c2c" : "#f0f0f0"));

        m_addressSelectCombo->setView(popupView);
        topToolbar->addWidget(m_addressSelectCombo);

        m_addressSearchEdit = new QLineEdit(this);
        m_addressSearchEdit->setPlaceholderText("Search note or address...");
        m_addressSearchEdit->setClearButtonEnabled(true);
        m_addressSearchEdit->setFixedWidth(170);
        m_addressSearchEdit->setFixedHeight(26);
        topToolbar->addWidget(m_addressSearchEdit);

        m_newAddressBtn = new QPushButton("New Address...", this);
        m_newAddressBtn->setIcon(getThemeIcon(":/icons/bookmark_add.svg"));
        m_newAddressBtn->setToolTip("Create a new structure or array at a manual address (Ctrl+N)");
        m_newAddressBtn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
        m_newAddressBtn->setFixedHeight(26);
        m_newAddressBtn->setAutoDefault(false);
        m_newAddressBtn->setDefault(false);
        topToolbar->addWidget(m_newAddressBtn);

        m_deleteAddressBtn = new QPushButton("Remove Address", this);
        m_deleteAddressBtn->setIcon(getThemeIcon(":/icons/delete.svg"));
        m_deleteAddressBtn->setToolTip("Delete this manually created address from the list and User.txt");
        m_deleteAddressBtn->setEnabled(false);
        m_deleteAddressBtn->setFixedHeight(26);
        m_deleteAddressBtn->setAutoDefault(false);
        m_deleteAddressBtn->setDefault(false);
        topToolbar->addWidget(m_deleteAddressBtn);

        // Right-align configuration and panel control buttons
        topToolbar->addStretch();

        m_saveUserBtn = new QPushButton("Save to User.txt", this);
        m_saveUserBtn->setIcon(getThemeIcon(":/icons/save.svg"));
        m_saveUserBtn->setShortcut(QKeySequence::Save);
        m_saveUserBtn->setToolTip("Save active note override to User.txt (Ctrl+S)");
        m_saveUserBtn->setFixedHeight(26);
        m_saveUserBtn->setAutoDefault(false);
        m_saveUserBtn->setDefault(false);
        topToolbar->addWidget(m_saveUserBtn);

        m_revertServerBtn = new QPushButton("Revert to Server", this);
        m_revertServerBtn->setIcon(getThemeIcon(":/icons/undo.svg"));
        m_revertServerBtn->setToolTip("Remove local override from User.txt and restore original server note");
        m_revertServerBtn->setEnabled(false);
        m_revertServerBtn->setFixedHeight(26);
        m_revertServerBtn->setAutoDefault(false);
        m_revertServerBtn->setDefault(false);
        topToolbar->addWidget(m_revertServerBtn);

        m_resetBtn = new QPushButton("Reset", this);
        m_resetBtn->setIcon(getThemeIcon(":/icons/refresh.svg"));
        m_resetBtn->setToolTip("Reload Notes.json and User.txt from disk if modified externally");
        m_resetBtn->setFixedHeight(26);
        m_resetBtn->setAutoDefault(false);
        m_resetBtn->setDefault(false);
        topToolbar->addWidget(m_resetBtn);

        m_toggleCodeNoteBtn = new QPushButton("Code Note Panel", this);
        m_toggleCodeNoteBtn->setCheckable(true);
        m_toggleCodeNoteBtn->setChecked(true);
        m_toggleCodeNoteBtn->setIcon(getThemeIcon(":/icons/hierarchy_fixer.svg"));
        m_toggleCodeNoteBtn->setFixedHeight(26);
        m_toggleCodeNoteBtn->setAutoDefault(false);
        m_toggleCodeNoteBtn->setDefault(false);
        topToolbar->addWidget(m_toggleCodeNoteBtn);

        connect(m_newAddressBtn, &QPushButton::clicked, this, &DissectAddressBar::newAddressClicked);
        connect(m_deleteAddressBtn, &QPushButton::clicked, this, &DissectAddressBar::deleteAddressClicked);
        connect(m_addressSelectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DissectAddressBar::onAddressComboChanged);
        connect(m_addressSearchEdit, &QLineEdit::textChanged, this, &DissectAddressBar::onAddressSearchTextChanged);
        connect(m_saveUserBtn, &QPushButton::clicked, this, &DissectAddressBar::saveUserTxtClicked);
        connect(m_revertServerBtn, &QPushButton::clicked, this, &DissectAddressBar::revertServerClicked);
        connect(m_resetBtn, &QPushButton::clicked, this, &DissectAddressBar::resetClicked);
        connect(m_toggleCodeNoteBtn, &QPushButton::clicked, this, &DissectAddressBar::toggleCodeNotePanelClicked);
    }

    void DissectAddressBar::populateAddressDropdown(const RaGameData& gameData, uint32_t currentBaseAddress, bool autoNavigate) {
        m_addressSelectCombo->blockSignals(true);
        m_addressSourceModel->clear();
        m_currentBaseAddress = currentBaseAddress;

        if (!gameData.isValid || gameData.notes.empty()) {
            auto* emptyItem = new QStandardItem("No active game code notes detected.");
            m_addressSourceModel->appendRow(emptyItem);
            m_addressSelectCombo->blockSignals(false);
            return;
        }

        auto sortedNotes = gameData.notes;
        // Sort by address
        std::sort(sortedNotes.begin(), sortedNotes.end(), [](const RaCodeNote& a, const RaCodeNote& b) {
            return a.address < b.address;
        });

        int selectSourceRow = 0;

        for (size_t i = 0; i < sortedNotes.size(); ++i) {
            const auto& n = sortedNotes[i];
            QString tag = n.isLocalOnly ? "(Local)" : (n.isModified ? "(Modified)" : "(Server)");
            QString itemText = QString("0x%1 - %2 %3")
                .arg(QString::number(n.address, 16).toUpper().rightJustified(8, '0'))
                .arg(n.title)
                .arg(tag);

            auto* item = new QStandardItem(itemText);
            item->setData(n.address, Qt::UserRole);
            m_addressSourceModel->appendRow(item);

            if (m_currentBaseAddress != 0 && n.address == m_currentBaseAddress) {
                selectSourceRow = static_cast<int>(i);
            }
        }

        m_addressProxyModel->setSearchFilter(m_addressSearchEdit ? m_addressSearchEdit->text() : "");

        QModelIndex srcIndex = m_addressSourceModel->index(selectSourceRow, 0);
        QModelIndex proxyIndex = m_addressProxyModel->mapFromSource(srcIndex);
        if (proxyIndex.isValid()) {
            m_addressSelectCombo->setCurrentIndex(proxyIndex.row());
        }
        else if (m_addressProxyModel->rowCount() > 0) {
            m_addressSelectCombo->setCurrentIndex(0);
        }

        m_addressSelectCombo->blockSignals(false);

        if (m_addressSelectCombo->count() > 0 && m_currentBaseAddress != 0) {
            emit addressSelected(m_currentBaseAddress, autoNavigate);
        }
        else if (m_addressSelectCombo->count() > 0) {
            QVariant comboVal = m_addressSelectCombo->currentData();
            if (comboVal.isValid()) {
                m_currentBaseAddress = comboVal.toUInt();
                emit addressSelected(m_currentBaseAddress, autoNavigate);
            }
        }
    }

    void DissectAddressBar::onAddressSearchTextChanged(const QString& text) {
        m_addressSelectCombo->blockSignals(true);
        m_addressProxyModel->setSearchFilter(text);
        m_addressSelectCombo->blockSignals(false);

        if (m_addressProxyModel->rowCount() > 0) {
            // Retain active address selection if it is present within filtered results
            bool currentStillVisible = false;
            for (int i = 0; i < m_addressProxyModel->rowCount(); ++i) {
                QModelIndex proxyIdx = m_addressProxyModel->index(i, 0);
                if (proxyIdx.data(Qt::UserRole).toUInt() == m_currentBaseAddress) {
                    m_addressSelectCombo->setCurrentIndex(i);
                    currentStillVisible = true;
                    break;
                }
            }
            if (!currentStillVisible) {
                m_addressSelectCombo->setCurrentIndex(0);
                QVariant matchedVal = m_addressSelectCombo->currentData();
                if (matchedVal.isValid()) {
                    m_currentBaseAddress = matchedVal.toUInt();
                    emit addressSelected(m_currentBaseAddress, true);
                }
            }
        }
    }

    void DissectAddressBar::onAddressComboChanged(int index) {
        if (index < 0 || index >= m_addressSelectCombo->count()) return;
        QVariant comboData = m_addressSelectCombo->itemData(index);
        if (!comboData.isValid()) return;

        m_currentBaseAddress = comboData.toUInt();
        emit addressSelected(m_currentBaseAddress, true);
    }

    void DissectAddressBar::selectAddressInUI(uint32_t address) {
        m_currentBaseAddress = address;

        m_addressSelectCombo->blockSignals(true);
        for (int i = 0; i < m_addressProxyModel->rowCount(); ++i) {
            QModelIndex proxyIdx = m_addressProxyModel->index(i, 0);
            if (proxyIdx.data(Qt::UserRole).toUInt() == address) {
                m_addressSelectCombo->setCurrentIndex(i);
                break;
            }
        }
        m_addressSelectCombo->blockSignals(false);
    }

    void DissectAddressBar::updateToolbarButtonStates(const RaGameData& gameData, uint32_t activeAddr) {
        bool canRevert = false;
        bool canDelete = false;

        if (activeAddr != 0 && gameData.isValid) {
            auto it = std::find_if(gameData.notes.begin(), gameData.notes.end(), [&](const RaCodeNote& n) {
                return n.address == activeAddr;
            });
            if (it != gameData.notes.end()) {
                if (it->isModified) {
                    canRevert = true;
                }
                if (it->isLocalOnly) {
                    canDelete = true;
                }
            }
        }
        m_revertServerBtn->setEnabled(canRevert);
        m_deleteAddressBtn->setEnabled(canDelete);
    }

    void DissectAddressBar::setCodeNotePanelButtonChecked(bool checked) {
        m_toggleCodeNoteBtn->blockSignals(true);
        m_toggleCodeNoteBtn->setChecked(checked);
        m_toggleCodeNoteBtn->blockSignals(false);
    }

    uint32_t DissectAddressBar::getSelectedAddress() const {
        return m_currentBaseAddress;
    }

    void DissectAddressBar::showSaveFeedback() {
        QString origText = m_saveUserBtn->text();
        m_saveUserBtn->setText("✓ Saved!");
        m_saveUserBtn->setEnabled(false);
        QTimer::singleShot(1500, this, [this, origText]() {
            m_saveUserBtn->setText(origText);
            m_saveUserBtn->setEnabled(true);
        });
    }

    void DissectAddressBar::showResetFeedback() {
        QString origText = m_resetBtn->text();
        m_resetBtn->setText("✓ Refreshed!");
        m_resetBtn->setEnabled(false);
        QTimer::singleShot(1500, this, [this, origText]() {
            m_resetBtn->setText(origText);
            m_resetBtn->setEnabled(true);
        });
    }

}
