#pragma once

#include "Core/Storage/RaStorageService.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QWidget>

#include <cstdint>

namespace PointerFinder2::UI {

// Filter proxy model providing real-time multi-keyword and hex address filtering
class AddressFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    explicit AddressFilterProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

    void setSearchFilter(const QString& filter) {
        m_filterWords = filter.trimmed().split(' ', Qt::SkipEmptyParts);
        invalidateFilter();
    }

  protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        if (m_filterWords.isEmpty())
            return true;

        QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
        QString text = sourceModel()->data(idx, Qt::DisplayRole).toString();
        QVariant addrData = sourceModel()->data(idx, Qt::UserRole);
        uint32_t addr = addrData.isValid() ? addrData.toUInt() : 0;
        QString addrHexClean = QString::number(addr, 16).toUpper();

        for (const auto& word : m_filterWords) {
            QString cleanWord = word;
            if (cleanWord.startsWith("0x", Qt::CaseInsensitive)) {
                cleanWord = cleanWord.mid(2);
            }
            bool matchText = text.contains(word, Qt::CaseInsensitive);
            bool matchHex = (!cleanWord.isEmpty() && addrHexClean.contains(cleanWord, Qt::CaseInsensitive));

            if (!matchText && !matchHex) {
                return false;
            }
        }
        return true;
    }

  private:
    QStringList m_filterWords;
};

// Encapsulates the top address selection bar, real-time search filter,
// and RA note storage interaction buttons (Save, Revert, Reset, New, Remove).
class DissectAddressBar : public QWidget {
    Q_OBJECT
  public:
    explicit DissectAddressBar(QWidget* parent = nullptr);
    ~DissectAddressBar() override;

    void populateAddressDropdown(const Core::RaGameData& gameData, uint32_t currentBaseAddress, bool autoNavigate = true);
    void updateToolbarButtonStates(const Core::RaGameData& gameData, uint32_t activeAddr);
    void selectAddressInUI(uint32_t address);
    void setCodeNotePanelButtonChecked(bool checked);
    uint32_t getSelectedAddress() const;
    void showSaveFeedback();
    void showResetFeedback();
    void updateThemeIcons();

  signals:
    void addressSelected(uint32_t address, bool autoNavigate);
    void newAddressClicked();
    void cloneStrideClicked();
    void deleteAddressClicked();
    void saveUserTxtClicked();
    void revertServerClicked();
    void resetClicked();
    void toggleCodeNotePanelClicked();

  private slots:
    void onAddressComboChanged(int index);
    void onAddressSearchTextChanged(const QString& text);

  private:
    QComboBox* m_addressSelectCombo = nullptr;
    QStandardItemModel* m_addressSourceModel = nullptr;
    AddressFilterProxyModel* m_addressProxyModel = nullptr;
    QLineEdit* m_addressSearchEdit = nullptr;
    QPushButton* m_newAddressBtn = nullptr;
    QPushButton* m_cloneStrideBtn = nullptr;
    QPushButton* m_deleteAddressBtn = nullptr;
    QPushButton* m_saveUserBtn = nullptr;
    QPushButton* m_revertServerBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QPushButton* m_toggleCodeNoteBtn = nullptr;

    uint32_t m_currentBaseAddress = 0;

    void setupLayout();
    QIcon getThemeIcon(const QString& resourcePath) const;
};

} // namespace PointerFinder2::UI
