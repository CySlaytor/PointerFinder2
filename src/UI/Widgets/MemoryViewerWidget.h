#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <QAbstractScrollArea>
#include <QFont>
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

        explicit MemoryViewerWidget(QWidget* parent = nullptr);
        ~MemoryViewerWidget() override;

        void setMemoryData(std::vector<uint8_t> memData, uint32_t memoryBaseAddress, bool isBigEndian = false);
        void clearMemory();

        ViewMode getViewMode() const { return m_viewMode; }
        void setViewMode(ViewMode mode);

        void setHighlightedAddresses(const std::unordered_map<uint32_t, HighlightInfo>& highlights);
        void clearHighlights();

        uint32_t getSelectedAddress() const { return m_selectedAddress; }
        void setSelectedAddress(uint32_t address);
        void goToAddress(uint32_t address);

        uint32_t readDwordAt(uint32_t address) const;

        bool hasData() const { return !m_memoryData.empty(); }
        uint32_t getMemoryBase() const { return m_memoryBaseAddress; }
        size_t getMemorySize() const { return m_memoryData.size(); }

    signals:
        void addressSelected(uint32_t address);
        void addressDoubleClicked(uint32_t address);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        std::vector<uint8_t> m_memoryData;
        uint32_t m_memoryBaseAddress = 0;
        bool m_isBigEndian = false;
        ViewMode m_viewMode = ViewMode::Mode32Bit;

        std::unordered_map<uint32_t, HighlightInfo> m_highlights;

        uint32_t m_selectedAddress = 0;
        bool m_hasSelection = false;

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
        int64_t totalRows() const;
        uint32_t addressAtPosition(const QPoint& pos) const;
    };

}
