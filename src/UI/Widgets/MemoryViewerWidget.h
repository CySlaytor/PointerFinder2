#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <QAbstractScrollArea>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace PointerFinder2::UI::Widgets {

    // A virtualized, read-only hex and ASCII memory viewer replicating the PCSX2 Debugger Memory View.
    // Supports 8-bit, 16-bit, 32-bit, and 32-bit BE modes with intelligent, context-aware structure highlighting.
    class MemoryViewerWidget : public QAbstractScrollArea {
        Q_OBJECT
    public:
        enum class ViewMode {
            Mode8Bit,
            Mode16Bit,
            Mode32Bit,
            Mode32BitBE
        };

        struct HighlightInfo {
            QString label;
            QString name;
            QString offset;
            uint32_t startAddress = 0;
            uint32_t size = 4;
            bool isPointer = false;
            bool isChildOfActive = false;
        };

        struct DefinedArrayInfo {
            QString name;
            uint32_t startAddress = 0;
            uint32_t endAddress = 0;
            uint32_t size = 0;
        };

        explicit MemoryViewerWidget(QWidget* parent = nullptr);
        ~MemoryViewerWidget() override;

        void setMemoryData(std::vector<uint8_t> memData, uint32_t memoryBaseAddress, bool isBigEndian = false);
        void clearMemory();

        ViewMode getViewMode() const { return m_viewMode; }
        void setViewMode(ViewMode mode);

        void setHighlightedAddresses(const std::unordered_map<uint32_t, HighlightInfo>& highlights);
        void clearHighlights();

        void setDefinedArrays(const std::vector<DefinedArrayInfo>& arrays);
        void clearDefinedArrays();

        bool hasRangeSelection() const { return m_hasRangeSelection; }
        uint32_t getRangeStart() const { return m_rangeStart; }
        uint32_t getRangeEnd() const { return m_rangeEnd; }
        uint32_t getRangeSpan() const { return m_rangeSpan; }
        void clearRangeSelection();

        // Pointer navigation history (up to 50 steps)
        bool canNavigateBack() const;
        bool canNavigateForward() const;
        void navigateBack();
        void navigateForward();

        uint32_t getSelectedAddress() const { return m_selectedAddress; }
        uint32_t getHoverAddress() const { return m_hoverAddress; }
        bool isAddressDocumented(uint32_t address) const;
        void setSelectedAddress(uint32_t address);
        void goToAddress(uint32_t address);

        uint32_t readDwordAt(uint32_t address) const;

        bool hasData() const { return !m_memoryData.empty(); }
        uint32_t getMemoryBase() const { return m_memoryBaseAddress; }
        size_t getMemorySize() const { return m_memoryData.size(); }

    signals:
        void addressSelected(uint32_t address);
        void addressDoubleClicked(uint32_t address);
        void contextMenuRequested(uint32_t address, const QPoint& globalPos);
        void rangeSelected(uint32_t startAddress, uint32_t endAddress, uint32_t byteCount);
        void defineArrayRequested(uint32_t startAddress, uint32_t endAddress, uint32_t byteCount);
        void navigationHistoryChanged(bool canBack, bool canForward);

        // Keyboard actions scoped to Memory Viewer
        void addVariableTriggered();
        void addPointerTriggered();
        void followPointerTriggered();
        void removeDocumentationTriggered(uint32_t address);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;

    private:
        std::vector<uint8_t> m_memoryData;
        uint32_t m_memoryBaseAddress = 0;
        bool m_isBigEndian = false;
        ViewMode m_viewMode = ViewMode::Mode32Bit;

        std::unordered_map<uint32_t, HighlightInfo> m_highlights;
        std::vector<DefinedArrayInfo> m_definedArrays;

        uint32_t m_selectedAddress = 0;
        uint32_t m_hoverAddress = 0;
        bool m_hasSelection = false;

        // Shift+Click range selection state
        bool m_hasRangeSelection = false;
        uint32_t m_rangeAnchor = 0;
        uint32_t m_rangeStart = 0;
        uint32_t m_rangeEnd = 0;
        uint32_t m_rangeSpan = 0;

        // Navigation history state
        static constexpr size_t MAX_NAV_HISTORY = 50;
        std::vector<uint32_t> m_navHistory;
        int m_navHistoryIndex = -1;
        bool m_isNavigatingHistory = false;

        // Floating action bar for array definition
        QWidget* m_rangeBannerWidget = nullptr;
        QLabel* m_rangeBannerLabel = nullptr;
        QPushButton* m_defineArrayBtn = nullptr;
        QPushButton* m_closeBannerBtn = nullptr;

        QFont m_monoFont;
        int m_charWidth = 8;
        int m_charHeight = 16;
        int m_lineHeight = 18;
        int m_headerHeight = 22;

        int m_addrColX = 10;
        int m_addrColWidth = 80;
        int m_hexColX = 100;
        int m_asciiColX = 450;

        void updateMetrics();
        void updateScrollBars();
        void updateRangeBanner();
        int64_t totalRows() const;
        uint32_t addressAtPosition(const QPoint& pos) const;
    };

}
