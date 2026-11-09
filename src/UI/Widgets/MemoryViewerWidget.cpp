#include "MemoryViewerWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QToolTip>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace PointerFinder2::UI::Widgets {

MemoryViewerWidget::MemoryViewerWidget(QWidget* parent) : QAbstractScrollArea(parent), m_monoFont("Consolas", 10) {
    m_monoFont.setStyleHint(QFont::Monospace);
    setFont(m_monoFont);

    setFocusPolicy(Qt::StrongFocus);
    viewport()->setFocusProxy(this);
    viewport()->setMouseTracking(true);

    updateMetrics();
    updateScrollBars();

    // Setup floating action banner for array definitions
    m_rangeBannerWidget = new QWidget(viewport());
    m_rangeBannerWidget->setObjectName("rangeBannerWidget");

    auto* bannerLayout = new QHBoxLayout(m_rangeBannerWidget);
    bannerLayout->setContentsMargins(6, 2, 6, 2);
    bannerLayout->setSpacing(8);

    m_rangeBannerLabel = new QLabel(m_rangeBannerWidget);
    m_defineArrayBtn = new QPushButton("Define Array", m_rangeBannerWidget);
    m_defineArrayBtn->setObjectName("defineArrayBtn");
    m_closeBannerBtn = new QPushButton("✕", m_rangeBannerWidget);
    m_closeBannerBtn->setObjectName("closeBannerBtn");

    bannerLayout->addWidget(m_rangeBannerLabel);
    bannerLayout->addWidget(m_defineArrayBtn);
    bannerLayout->addWidget(m_closeBannerBtn);

    connect(m_defineArrayBtn, &QPushButton::clicked, this, [this]() {
        if (m_hasRangeSelection) {
            emit defineArrayRequested(m_rangeStart, m_rangeEnd, m_rangeSpan);
        }
    });
    connect(m_closeBannerBtn, &QPushButton::clicked, this, &MemoryViewerWidget::clearRangeSelection);

    m_rangeBannerWidget->hide();
}

MemoryViewerWidget::~MemoryViewerWidget() = default;

int MemoryViewerWidget::getContentWidth32Bit() const {
    QFontMetrics fm(m_monoFont);
    int charW = fm.horizontalAdvance('0');
    int addrW = charW * 8 + 10;
    int hexW = (4 * (charW * 8)) + (3 * (charW * 2)); // Exactly 4 dwords of 8 hex digits for 32-bit mode
    int asciiX = 10 + addrW + 10 + hexW + 15;
    // Exact pixel boundary: end of ASCII text + scrollbar width + outer frame border
    return asciiX + (16 * charW) + 12;
}

void MemoryViewerWidget::updateMetrics() {
    QFontMetrics fm(m_monoFont);
    m_charWidth = fm.horizontalAdvance('0');
    m_charHeight = fm.height();
    m_lineHeight = m_charHeight + 3;
    m_headerHeight = m_lineHeight + 4;

    m_addrColX = 10;
    m_addrColWidth = m_charWidth * 8 + 10;
    m_hexColX = m_addrColX + m_addrColWidth + 10;

    int hexBlockWidth = 0;
    switch (m_viewMode) {
    case ViewMode::Mode8Bit:
        hexBlockWidth = (16 * (m_charWidth * 2)) + (15 * m_charWidth);
        break;
    case ViewMode::Mode16Bit:
        hexBlockWidth = (8 * (m_charWidth * 4)) + (7 * m_charWidth);
        break;
    case ViewMode::Mode32Bit:
    case ViewMode::Mode32BitBE:
        hexBlockWidth = (4 * (m_charWidth * 8)) + (3 * (m_charWidth * 2));
        break;
    case ViewMode::ModeFloat:
    case ViewMode::ModeFloatBE:
        hexBlockWidth = (4 * (m_charWidth * 9)) + (3 * (m_charWidth * 2));
        break;
    }

    m_asciiColX = m_hexColX + hexBlockWidth + 15;
}

void MemoryViewerWidget::setMemoryData(std::vector<uint8_t> memData, uint32_t memoryBaseAddress, bool isBigEndian) {
    std::vector<MemoryRegionDescriptor> singleDesc = {{memoryBaseAddress, memData.size()}};
    setMemoryRegions(std::move(memData), singleDesc, isBigEndian);
}

void MemoryViewerWidget::setMemoryRegions(std::vector<uint8_t> memData, const std::vector<MemoryRegionDescriptor>& regions,
                                          bool isBigEndian) {
    m_memoryData = std::move(memData);
    m_isBigEndian = isBigEndian;
    m_regions.clear();

    if (regions.empty()) {
        m_memoryBaseAddress = 0;
    } else {
        m_memoryBaseAddress = regions.front().startAddress;
    }

    int64_t accumulatedRows = 0;
    size_t accumulatedOffset = 0;

    for (const auto& r : regions) {
        StackedRegion reg;
        reg.startAddress = r.startAddress;
        reg.size = r.size;
        reg.bufferOffset = accumulatedOffset;
        reg.startRow = accumulatedRows;
        reg.rowCount = static_cast<int64_t>((r.size + 15) / 16);

        accumulatedRows += reg.rowCount;
        accumulatedOffset += r.size;
        m_regions.push_back(reg);
    }

    if (m_isBigEndian) {
        if (m_viewMode == ViewMode::Mode32Bit) {
            m_viewMode = ViewMode::Mode32BitBE;
        } else if (m_viewMode == ViewMode::ModeFloat) {
            m_viewMode = ViewMode::ModeFloatBE;
        }
    }

    if (!isValidAddress(m_selectedAddress)) {
        m_selectedAddress = m_memoryBaseAddress;
    }

    clearRangeSelection();
    m_navHistory.clear();
    m_navHistory.push_back({m_selectedAddress, 0, true});
    m_navHistoryIndex = 0;
    emit navigationHistoryChanged(false, false, 1, 1);

    updateMetrics();
    updateScrollBars();
    viewport()->update();
}

void MemoryViewerWidget::clearMemory() {
    m_memoryData.clear();
    m_regions.clear();
    m_highlights.clear();
    m_definedArrays.clear();
    m_hasSelection = false;
    clearRangeSelection();
    m_navHistory.clear();
    m_navHistoryIndex = -1;
    emit navigationHistoryChanged(false, false, 0, 0);
    updateScrollBars();
    viewport()->update();
}

void MemoryViewerWidget::setViewMode(ViewMode mode) {
    if (m_viewMode != mode) {
        m_viewMode = mode;
        updateMetrics();
        updateScrollBars();
        updateRangeBanner();
        viewport()->update();
    }
}

void MemoryViewerWidget::setFloatDisplayMode(FloatDisplayMode mode) {
    if (m_floatDisplayMode != mode) {
        m_floatDisplayMode = mode;
        viewport()->update();
    }
}

void MemoryViewerWidget::setHighlightedAddresses(const std::unordered_map<uint32_t, HighlightInfo>& highlights) {
    m_highlights = highlights;
    viewport()->update();
}

void MemoryViewerWidget::clearHighlights() {
    m_highlights.clear();
    viewport()->update();
}

void MemoryViewerWidget::setDefinedArrays(const std::vector<DefinedArrayInfo>& arrays) {
    m_definedArrays = arrays;
    viewport()->update();
}

void MemoryViewerWidget::clearDefinedArrays() {
    m_definedArrays.clear();
    viewport()->update();
}

bool MemoryViewerWidget::isAddressDocumented(uint32_t address) const { return m_highlights.find(address) != m_highlights.end(); }

bool MemoryViewerWidget::isValidAddress(uint32_t address) const { return rowForAddress(address) >= 0; }

const MemoryViewerWidget::StackedRegion* MemoryViewerWidget::findRegionForRow(int64_t row) const {
    for (const auto& reg : m_regions) {
        if (row >= reg.startRow && row < reg.startRow + reg.rowCount) {
            return &reg;
        }
    }
    return nullptr;
}

const MemoryViewerWidget::StackedRegion* MemoryViewerWidget::findRegionForAddress(uint32_t address) const {
    for (const auto& reg : m_regions) {
        if (address >= reg.startAddress && address < reg.startAddress + reg.size) {
            return &reg;
        }
    }
    return nullptr;
}

int64_t MemoryViewerWidget::rowForAddress(uint32_t address) const {
    for (const auto& reg : m_regions) {
        if (address >= reg.startAddress && address < reg.startAddress + reg.size) {
            uint32_t offsetInReg = address - reg.startAddress;
            return reg.startRow + static_cast<int64_t>(offsetInReg / 16);
        }
    }
    return -1;
}

int64_t MemoryViewerWidget::bufferOffsetForAddress(uint32_t address) const {
    for (const auto& reg : m_regions) {
        if (address >= reg.startAddress && address < reg.startAddress + reg.size) {
            return static_cast<int64_t>(reg.bufferOffset + (address - reg.startAddress));
        }
    }
    return -1;
}

void MemoryViewerWidget::clearRangeSelection() {
    if (m_hasRangeSelection) {
        m_hasRangeSelection = false;
        m_rangeStart = 0;
        m_rangeEnd = 0;
        m_rangeSpan = 0;
        if (m_rangeBannerWidget) {
            m_rangeBannerWidget->hide();
        }
        viewport()->update();
    }
}

void MemoryViewerWidget::updateRangeBanner() {
    if (!m_hasRangeSelection || m_rangeSpan <= 4) {
        if (m_rangeBannerWidget)
            m_rangeBannerWidget->hide();
        return;
    }

    bool isDark = (palette().color(QPalette::Base).lightness() < 128);
    if (isDark) {
        m_rangeBannerWidget->setStyleSheet("#rangeBannerWidget {"
                                           "    background-color: #21162E;"
                                           "    border: 1px solid #A371F7;"
                                           "    border-radius: 5px;"
                                           "    padding: 3px 6px;"
                                           "}"
                                           "QLabel {"
                                           "    color: #E2D9F3;"
                                           "    font-weight: bold;"
                                           "    font-size: 11px;"
                                           "}"
                                           "QPushButton#defineArrayBtn {"
                                           "    background-color: #7928CA;"
                                           "    color: #ffffff;"
                                           "    border: 1px solid #A371F7;"
                                           "    border-radius: 3px;"
                                           "    font-weight: bold;"
                                           "    font-size: 11px;"
                                           "    padding: 2px 10px;"
                                           "}"
                                           "QPushButton#defineArrayBtn:hover {"
                                           "    background-color: #8A3FFC;"
                                           "}"
                                           "QPushButton#closeBannerBtn {"
                                           "    background: transparent;"
                                           "    color: #b0a0cc;"
                                           "    border: none;"
                                           "    font-weight: bold;"
                                           "    font-size: 12px;"
                                           "    padding: 0 4px;"
                                           "}"
                                           "QPushButton#closeBannerBtn:hover {"
                                           "    color: #ffffff;"
                                           "}");
    } else {
        m_rangeBannerWidget->setStyleSheet("#rangeBannerWidget {"
                                           "    background-color: #F3EBFF;"
                                           "    border: 1px solid #A371F7;"
                                           "    border-radius: 5px;"
                                           "    padding: 3px 6px;"
                                           "}"
                                           "QLabel {"
                                           "    color: #4C1D95;"
                                           "    font-weight: bold;"
                                           "    font-size: 11px;"
                                           "}"
                                           "QPushButton#defineArrayBtn {"
                                           "    background-color: #7928CA;"
                                           "    color: #ffffff;"
                                           "    border: 1px solid #6B21A8;"
                                           "    border-radius: 3px;"
                                           "    font-weight: bold;"
                                           "    font-size: 11px;"
                                           "    padding: 2px 10px;"
                                           "}"
                                           "QPushButton#defineArrayBtn:hover {"
                                           "    background-color: #6B21A8;"
                                           "}"
                                           "QPushButton#closeBannerBtn {"
                                           "    background: transparent;"
                                           "    color: #7C3AED;"
                                           "    border: none;"
                                           "    font-weight: bold;"
                                           "    font-size: 12px;"
                                           "    padding: 0 4px;"
                                           "}"
                                           "QPushButton#closeBannerBtn:hover {"
                                           "    color: #4C1D95;"
                                           "}");
    }

    QString text = QString("Selected: 0x%1 - 0x%2 (%3 bytes / 0x%4)")
                       .arg(QString("%1").arg(m_rangeStart, 8, 16, QChar('0')).toUpper())
                       .arg(QString("%1").arg(m_rangeEnd, 8, 16, QChar('0')).toUpper())
                       .arg(m_rangeSpan)
                       .arg(QString::number(m_rangeSpan, 16).toUpper());

    m_rangeBannerLabel->setText(text);
    m_rangeBannerWidget->adjustSize();

    int w = m_rangeBannerWidget->width();
    int h = m_rangeBannerWidget->height();
    int x = std::max(10, (viewport()->width() - w) / 2);
    int y = viewport()->height() - h - 10;
    m_rangeBannerWidget->setGeometry(x, y, w, h);
    m_rangeBannerWidget->show();
    m_rangeBannerWidget->raise();
}

bool MemoryViewerWidget::canNavigateBack() const { return m_navHistoryIndex > 0; }

bool MemoryViewerWidget::canNavigateForward() const {
    return m_navHistoryIndex >= 0 && (m_navHistoryIndex + 1 < static_cast<int>(m_navHistory.size()));
}

void MemoryViewerWidget::navigateBack() {
    if (canNavigateBack()) {
        m_isNavigatingHistory = true;

        // Preserve current view and scroll position in the current slot before moving back
        if (m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
            m_navHistory[m_navHistoryIndex] = {m_selectedAddress, verticalScrollBar()->value(), m_hasSelection};
        }

        --m_navHistoryIndex;
        const auto& state = m_navHistory[m_navHistoryIndex];

        m_selectedAddress = state.address;
        m_hasSelection = state.hasSelection;
        verticalScrollBar()->setValue(state.scrollValue);

        m_isNavigatingHistory = false;
        emit addressSelected(m_selectedAddress);
        emit navigationHistoryChanged(canNavigateBack(), canNavigateForward(), m_navHistoryIndex + 1,
                                      static_cast<int>(m_navHistory.size()));
        viewport()->update();
    }
}

void MemoryViewerWidget::navigateForward() {
    if (canNavigateForward()) {
        m_isNavigatingHistory = true;

        // Preserve current view and scroll position in the current slot before moving forward
        if (m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
            m_navHistory[m_navHistoryIndex] = {m_selectedAddress, verticalScrollBar()->value(), m_hasSelection};
        }

        ++m_navHistoryIndex;
        const auto& state = m_navHistory[m_navHistoryIndex];

        m_selectedAddress = state.address;
        m_hasSelection = state.hasSelection;
        verticalScrollBar()->setValue(state.scrollValue);

        m_isNavigatingHistory = false;
        emit addressSelected(m_selectedAddress);
        emit navigationHistoryChanged(canNavigateBack(), canNavigateForward(), m_navHistoryIndex + 1,
                                      static_cast<int>(m_navHistory.size()));
        viewport()->update();
    }
}

uint32_t MemoryViewerWidget::readDwordAt(uint32_t address) const {
    int64_t offset = bufferOffsetForAddress(address);
    if (offset < 0 || static_cast<size_t>(offset) + 3 >= m_memoryData.size())
        return 0;

    bool isBE = (m_viewMode == ViewMode::Mode32BitBE) || (m_viewMode == ViewMode::ModeFloatBE) || m_isBigEndian;
    if (isBE) {
        return (static_cast<uint32_t>(m_memoryData[offset]) << 24) | (static_cast<uint32_t>(m_memoryData[offset + 1]) << 16) |
               (static_cast<uint32_t>(m_memoryData[offset + 2]) << 8) | (static_cast<uint32_t>(m_memoryData[offset + 3]));
    } else {
        return (static_cast<uint32_t>(m_memoryData[offset + 3]) << 24) | (static_cast<uint32_t>(m_memoryData[offset + 2]) << 16) |
               (static_cast<uint32_t>(m_memoryData[offset + 1]) << 8) | (static_cast<uint32_t>(m_memoryData[offset]));
    }
}

int64_t MemoryViewerWidget::totalRows() const {
    if (m_regions.empty())
        return 0;
    int64_t total = 0;
    for (const auto& reg : m_regions) {
        total += reg.rowCount;
    }
    return total;
}

void MemoryViewerWidget::updateScrollBars() {
    int visibleRows = (viewport()->height() - m_headerHeight) / m_lineHeight;
    int64_t maxRow = std::max<int64_t>(0, totalRows() - visibleRows);

    verticalScrollBar()->setRange(0, static_cast<int>(std::min<int64_t>(maxRow, std::numeric_limits<int>::max())));
    verticalScrollBar()->setPageStep(visibleRows);
}

void MemoryViewerWidget::resizeEvent(QResizeEvent* event) {
    QAbstractScrollArea::resizeEvent(event);
    updateScrollBars();
    updateRangeBanner();
}

void MemoryViewerWidget::setSelectedAddress(uint32_t address, bool recordHistory, bool centerTarget) {
    if (m_memoryData.empty() || m_regions.empty())
        return;

    // Verify or find appropriate region for target address
    const auto* reg = findRegionForAddress(address);
    if (!reg) {
        reg = &m_regions.front();
        address = reg->startAddress;
    }

    // Record history and navigate to destination
    goToAddress(address, recordHistory, centerTarget);
    emit addressSelected(m_selectedAddress);
    viewport()->update();
}

void MemoryViewerWidget::goToAddress(uint32_t address, bool recordHistory, bool centerTarget) {
    if (m_memoryData.empty() || m_regions.empty())
        return;

    int64_t targetRow = rowForAddress(address);
    if (targetRow < 0) {
        targetRow = 0;
        address = m_regions.front().startAddress;
    }

    int visibleRows = (viewport()->height() - m_headerHeight) / m_lineHeight;
    int currentRow = verticalScrollBar()->value();
    int maxRow = verticalScrollBar()->maximum();

    // 1. If this is a new navigation step, record the PREVIOUS state before scrolling
    if (recordHistory && !m_isNavigatingHistory && m_selectedAddress != 0) {
        if (m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
            m_navHistory[m_navHistoryIndex] = {m_selectedAddress, verticalScrollBar()->value(), m_hasSelection};
        }
    }

    // 2. Perform the view positioning
    if (centerTarget) {
        // Explicit pointer exploration or jump: Center the target address row in view
        int centeredRow = std::max(0, static_cast<int>(targetRow - (visibleRows / 2)));
        verticalScrollBar()->setValue(std::min(centeredRow, maxRow));
    } else {
        // Three-section viewport deadzone approach:
        // The visible rows are divided into three equal sections.
        // When moving with arrows, the cursor moves freely inside the middle third section.
        // When reaching either edge, the view scrolls row-by-row underneath the cursor, pinning it to the edge.
        if (visibleRows >= 6) {
            int topMargin = visibleRows / 3;
            int bottomMargin = (2 * visibleRows) / 3;

            if (targetRow < currentRow + topMargin) {
                int newScroll = std::max(0, static_cast<int>(targetRow - topMargin));
                verticalScrollBar()->setValue(newScroll);
            } else if (targetRow > currentRow + bottomMargin) {
                int newScroll = std::min(maxRow, static_cast<int>(targetRow - bottomMargin));
                verticalScrollBar()->setValue(newScroll);
            }
        } else if (visibleRows > 0) {
            if (targetRow < currentRow) {
                verticalScrollBar()->setValue(static_cast<int>(targetRow));
            } else if (targetRow >= currentRow + visibleRows) {
                verticalScrollBar()->setValue(std::min(maxRow, static_cast<int>(targetRow - visibleRows + 1)));
            }
        }
    }

    m_selectedAddress = address;
    m_hasSelection = true;

    // 3. Record or update the navigation state
    if (!m_isNavigatingHistory && address != 0) {
        int newScroll = verticalScrollBar()->value();

        if (recordHistory) {
            // If branching from an earlier point in history, discard forward history
            if (m_navHistoryIndex >= 0 && m_navHistoryIndex + 1 < static_cast<int>(m_navHistory.size())) {
                m_navHistory.erase(m_navHistory.begin() + (m_navHistoryIndex + 1), m_navHistory.end());
            }

            m_navHistory.push_back({address, newScroll, true});
            if (m_navHistory.size() > MAX_NAV_HISTORY) {
                m_navHistory.erase(m_navHistory.begin());
            }
            m_navHistoryIndex = static_cast<int>(m_navHistory.size()) - 1;

            emit navigationHistoryChanged(canNavigateBack(), canNavigateForward(), m_navHistoryIndex + 1,
                                          static_cast<int>(m_navHistory.size()));
        } else {
            // Track current cursor and scroll position in-place without creating a new step
            if (m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
                m_navHistory[m_navHistoryIndex] = {m_selectedAddress, newScroll, true};
            }
        }
    }

    viewport()->update();
}

uint32_t MemoryViewerWidget::addressAtPosition(const QPoint& pos) const {
    if (m_memoryData.empty() || m_regions.empty() || pos.y() < m_headerHeight)
        return 0;

    int rowOffset = (pos.y() - m_headerHeight) / m_lineHeight;
    int64_t row = verticalScrollBar()->value() + rowOffset;
    if (row < 0 || row >= totalRows())
        return 0;

    const auto* reg = findRegionForRow(row);
    if (!reg)
        return 0;

    int64_t relRow = row - reg->startRow;
    uint32_t rowAddress = reg->startAddress + static_cast<uint32_t>(relRow * 16);

    if (m_viewMode == ViewMode::Mode8Bit) {
        int byteWidth = m_charWidth * 2;
        int byteSpacing = m_charWidth;
        for (int i = 0; i < 16; ++i) {
            int bx = m_hexColX + i * (byteWidth + byteSpacing);
            if (pos.x() >= bx - 2 && pos.x() < bx + byteWidth + 2) {
                return rowAddress + i;
            }
        }
    } else if (m_viewMode == ViewMode::Mode16Bit) {
        int wordWidth = m_charWidth * 4;
        int wordSpacing = m_charWidth;
        for (int i = 0; i < 8; ++i) {
            int wx = m_hexColX + i * (wordWidth + wordSpacing);
            if (pos.x() >= wx - 2 && pos.x() < wx + wordWidth + 2) {
                // Snap click to 16-bit word alignment
                return rowAddress + (i * 2);
            }
        }
    } else {
        int dwordWidth = (m_viewMode == ViewMode::ModeFloat || m_viewMode == ViewMode::ModeFloatBE) ? m_charWidth * 9 : m_charWidth * 8;
        int dwordSpacing = m_charWidth * 2;
        for (int i = 0; i < 4; ++i) {
            int dx = m_hexColX + i * (dwordWidth + dwordSpacing);
            if (pos.x() >= dx - 3 && pos.x() < dx + dwordWidth + 3) {
                // Snap click to 32-bit DWORD boundary to prevent unaligned selections
                return rowAddress + (i * 4);
            }
        }
    }

    if (pos.x() >= m_asciiColX) {
        int charIdx = (pos.x() - m_asciiColX) / m_charWidth;
        if (charIdx >= 0 && charIdx < 16) {
            uint32_t raw = rowAddress + charIdx;
            if (m_viewMode == ViewMode::Mode32Bit || m_viewMode == ViewMode::Mode32BitBE || m_viewMode == ViewMode::ModeFloat ||
                m_viewMode == ViewMode::ModeFloatBE) {
                return raw & ~3U;
            }
            if (m_viewMode == ViewMode::Mode16Bit) {
                return raw & ~1U;
            }
            return raw;
        }
    }

    return rowAddress;
}

void MemoryViewerWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !m_memoryData.empty()) {
        uint32_t addr = addressAtPosition(event->pos());
        if (isValidAddress(addr)) {
            if (event->modifiers() & Qt::ShiftModifier) {
                // Handle Shift+Click contiguous range selection
                if (m_rangeAnchor == 0) {
                    m_rangeAnchor = (m_selectedAddress != 0) ? m_selectedAddress : addr;
                }

                uint32_t cellBytes = (m_viewMode == ViewMode::Mode8Bit) ? 1 : ((m_viewMode == ViewMode::Mode16Bit) ? 2 : 4);
                uint32_t low = std::min(m_rangeAnchor, addr);
                uint32_t high = std::max(m_rangeAnchor, addr) + (cellBytes - 1);

                m_rangeStart = low;
                m_rangeEnd = high;
                m_rangeSpan = (high - low) + 1;
                m_hasRangeSelection = (m_rangeSpan > cellBytes);

                m_selectedAddress = addr;
                m_hasSelection = true;
                updateRangeBanner();
                emit addressSelected(m_selectedAddress);
                emit rangeSelected(m_rangeStart, m_rangeEnd, m_rangeSpan);
                viewport()->update();
            } else {
                clearRangeSelection();
                m_rangeAnchor = addr;
                m_selectedAddress = addr;
                m_hasSelection = true;

                // Update current history slot position when clicking to select an address
                if (!m_isNavigatingHistory && m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
                    m_navHistory[m_navHistoryIndex] = {m_selectedAddress, verticalScrollBar()->value(), true};
                }

                emit addressSelected(m_selectedAddress);
                viewport()->update();
            }
        }
    }
    QAbstractScrollArea::mousePressEvent(event);
}

void MemoryViewerWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !m_memoryData.empty()) {
        uint32_t addr = addressAtPosition(event->pos());
        if (isValidAddress(addr)) {
            m_selectedAddress = addr;
            m_hasSelection = true;
            emit addressDoubleClicked(m_selectedAddress);
        }
    }
    QAbstractScrollArea::mouseDoubleClickEvent(event);
}

void MemoryViewerWidget::contextMenuEvent(QContextMenuEvent* event) {
    if (!m_memoryData.empty()) {
        uint32_t addr = addressAtPosition(event->pos());
        if (isValidAddress(addr)) {
            m_selectedAddress = addr;
            m_hasSelection = true;

            // Update current history slot before opening context menu
            if (!m_isNavigatingHistory && m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
                m_navHistory[m_navHistoryIndex] = {m_selectedAddress, verticalScrollBar()->value(), true};
            }

            emit addressSelected(m_selectedAddress);
            viewport()->update();
            emit contextMenuRequested(addr, event->globalPos());
            return;
        }
    }
    QAbstractScrollArea::contextMenuEvent(event);
}

void MemoryViewerWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_memoryData.empty()) {
        uint32_t addr = addressAtPosition(event->pos());
        m_hoverAddress = addr;
        if (addr != 0) {
            auto it = m_highlights.find(addr);

            // Check defined array membership for multi-tier tooltip context
            const DefinedArrayInfo* activeArray = nullptr;
            for (const auto& arr : m_definedArrays) {
                if (addr >= arr.startAddress && addr <= arr.endAddress) {
                    activeArray = &arr;
                    break;
                }
            }

            if (it != m_highlights.end() || activeArray != nullptr) {
                bool isDark = (palette().color(QPalette::Base).lightness() < 128);
                uint32_t baseDisplayAddr = (it != m_highlights.end() && it->second.startAddress != 0) ? it->second.startAddress : addr;
                QString addrHex = "0x" + QString("%1").arg(baseDisplayAddr, 8, 16, QChar('0')).toUpper();

                QString line1 = (it != m_highlights.end() && !it->second.offset.isEmpty())
                                    ? QString("<b>%1</b> (%2)").arg(addrHex, it->second.offset)
                                    : QString("<b>%1</b>").arg(addrHex);

                QString line2 = (it != m_highlights.end()) ? (it->second.name.isEmpty() ? it->second.label : it->second.name) : "";

                QString line3 = "";
                if (it != m_highlights.end()) {
                    line3 = it->second.isPointer ? (isDark ? "<span style='color: #FF7B72;'>[Pointer Target]</span>"
                                                           : "<span style='color: #B91C1C;'>[Pointer Target]</span>")
                                                 : (isDark ? "<span style='color: #79C0FF;'>[Documented Variable]</span>"
                                                           : "<span style='color: #0369A1;'>[Documented Variable]</span>");
                }

                QString arrayTag = "";
                if (activeArray != nullptr) {
                    uint32_t offsetInArray = addr - activeArray->startAddress;

                    // Clean redundant duplicate size tags from array name (e.g. "[80 bytes] Pointer to Array")
                    QString cleanArrName = activeArray->name;
                    cleanArrName.remove(QRegularExpression(R"(\[\d+\s*bytes\]\s*)", QRegularExpression::CaseInsensitiveOption));
                    cleanArrName = cleanArrName.trimmed();

                    QString tagColor = isDark ? "#D2A8FF" : "#7C3AED";

                    if (cleanArrName.isEmpty() || cleanArrName.compare("Array", Qt::CaseInsensitive) == 0 ||
                        cleanArrName.compare("Pointer to Array", Qt::CaseInsensitive) == 0) {
                        arrayTag = QString("<span style='color: %1;'>[Array: (%2 bytes), Offset: +0x%3]</span>")
                                       .arg(tagColor)
                                       .arg(activeArray->size)
                                       .arg(QString::number(offsetInArray, 16).toUpper());
                    } else {
                        arrayTag = QString("<span style='color: %1;'>[Array: %2 (%3 bytes), Offset: +0x%4]</span>")
                                       .arg(tagColor)
                                       .arg(cleanArrName)
                                       .arg(activeArray->size)
                                       .arg(QString::number(offsetInArray, 16).toUpper());
                    }
                }

                QString tip = QString("<div style='margin:0; padding:0; line-height:120%;'>%1%2%3%4</div>")
                                  .arg(line1)
                                  .arg(line2.isEmpty() ? "" : "<br>" + line2)
                                  .arg(line3.isEmpty() ? "" : "<br>" + line3)
                                  .arg(arrayTag.isEmpty() ? "" : "<br>" + arrayTag);

                QToolTip::showText(event->globalPosition().toPoint(), tip, viewport());
            } else {
                QToolTip::hideText();
            }
        } else {
            QToolTip::hideText();
        }
    }
    QAbstractScrollArea::mouseMoveEvent(event);
}

void MemoryViewerWidget::leaveEvent(QEvent* event) {
    m_hoverAddress = 0;
    QToolTip::hideText();
    QAbstractScrollArea::leaveEvent(event);
}

void MemoryViewerWidget::keyPressEvent(QKeyEvent* event) {
    if (m_memoryData.empty() || m_regions.empty()) {
        QAbstractScrollArea::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Escape && m_hasRangeSelection) {
        clearRangeSelection();
        event->accept();
        return;
    }

    // Handle scoped shortcuts strictly within the Memory Viewer boundary
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_F) {
        event->accept();
        emit followPointerTriggered();
        return;
    }
    if (event->key() == Qt::Key_V) {
        event->accept();
        emit addVariableTriggered();
        return;
    }
    if (event->key() == Qt::Key_P) {
        event->accept();
        emit addPointerTriggered();
        return;
    }
    if (event->key() == Qt::Key_X || event->key() == Qt::Key_Delete) {
        uint32_t targetAddr = (m_hoverAddress != 0) ? m_hoverAddress : m_selectedAddress;
        if (targetAddr != 0) {
            event->accept();
            emit removeDocumentationTriggered(targetAddr);
            return;
        }
    }
    if (event->key() == Qt::Key_Backspace || (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_Left)) {
        event->accept();
        navigateBack();
        return;
    }
    if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_Right) {
        event->accept();
        navigateForward();
        return;
    }

    // Standardized clipboard copy shortcuts within Memory Viewer
    if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && event->key() == Qt::Key_C) {
        event->accept();
        emit copyRaTriggered();
        return;
    }
    if (event->modifiers() == (Qt::ControlModifier | Qt::AltModifier) && event->key() == Qt::Key_C) {
        event->accept();
        emit copyValueTriggered();
        return;
    }
    if (event->matches(QKeySequence::Copy)) {
        event->accept();
        emit copyAddressTriggered();
        return;
    }

    uint32_t step = (m_viewMode == ViewMode::Mode8Bit) ? 1 : ((m_viewMode == ViewMode::Mode16Bit) ? 2 : 4);
    const auto* curReg = findRegionForAddress(m_selectedAddress);
    if (!curReg)
        curReg = &m_regions.front();

    if (event->key() == Qt::Key_Up) {
        if (m_selectedAddress >= curReg->startAddress + 16) {
            setSelectedAddress(m_selectedAddress - 16, false, false);
        } else {
            // Cross boundary into previous memory segment if available
            int regIdx = static_cast<int>(curReg - &m_regions.front());
            if (regIdx > 0) {
                const auto& prevReg = m_regions[regIdx - 1];
                uint32_t targetEnd = prevReg.startAddress + static_cast<uint32_t>(prevReg.size) - 16 + (m_selectedAddress % 16);
                setSelectedAddress(targetEnd, false, false);
            }
        }
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Down) {
        if (m_selectedAddress + 16 < curReg->startAddress + curReg->size) {
            setSelectedAddress(m_selectedAddress + 16, false, false);
        } else {
            // Cross boundary into next memory segment if available
            int regIdx = static_cast<int>(curReg - &m_regions.front());
            if (regIdx + 1 < static_cast<int>(m_regions.size())) {
                const auto& nextReg = m_regions[regIdx + 1];
                uint32_t targetStart = nextReg.startAddress + (m_selectedAddress % 16);
                setSelectedAddress(targetStart, false, false);
            }
        }
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Left) {
        if (m_selectedAddress >= curReg->startAddress + step) {
            setSelectedAddress(m_selectedAddress - step, false, false);
        } else {
            int regIdx = static_cast<int>(curReg - &m_regions.front());
            if (regIdx > 0) {
                const auto& prevReg = m_regions[regIdx - 1];
                uint32_t targetEnd = prevReg.startAddress + static_cast<uint32_t>(prevReg.size) - step;
                setSelectedAddress(targetEnd, false, false);
            }
        }
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Right) {
        if (m_selectedAddress + step < curReg->startAddress + curReg->size) {
            setSelectedAddress(m_selectedAddress + step, false, false);
        } else {
            int regIdx = static_cast<int>(curReg - &m_regions.front());
            if (regIdx + 1 < static_cast<int>(m_regions.size())) {
                const auto& nextReg = m_regions[regIdx + 1];
                setSelectedAddress(nextReg.startAddress, false, false);
            }
        }
        event->accept();
        return;
    }

    QAbstractScrollArea::keyPressEvent(event);
}

QString MemoryViewerWidget::formatFloatString(float fVal, uint32_t rawVal) const {
    if (m_floatDisplayMode == FloatDisplayMode::Raw) {
        if (rawVal == 0) {
            return "0.0";
        } else if (std::isnan(fVal)) {
            return "NaN";
        } else if (std::isinf(fVal)) {
            return (fVal > 0) ? "+Inf" : "-Inf";
        } else if (std::abs(fVal) >= 1e7f || (std::abs(fVal) < 1e-4f && std::abs(fVal) > 0.0f)) {
            return QString::number(fVal, 'e', 2);
        } else {
            QString floatStr = QString::number(fVal, 'f', (std::abs(fVal) >= 100.0f) ? 2 : 3);
            if (floatStr.contains('.')) {
                while (floatStr.endsWith('0') && !floatStr.endsWith(".0")) {
                    floatStr.chop(1);
                }
            }
            return floatStr;
        }
    }

    // Float timer representation mode
    if (rawVal == 0) {
        return "00:00.00";
    }
    if (std::isnan(fVal)) {
        return "NaN";
    }
    if (std::isinf(fVal)) {
        return (fVal > 0) ? "+Inf" : "-Inf";
    }

    double totalSeconds = 0.0;
    switch (m_floatDisplayMode) {
    case FloatDisplayMode::TimeSeconds:
        totalSeconds = static_cast<double>(fVal);
        break;
    case FloatDisplayMode::TimeFrames60:
        totalSeconds = static_cast<double>(fVal) / 60.0;
        break;
    case FloatDisplayMode::TimeFrames50:
        totalSeconds = static_cast<double>(fVal) / 50.0;
        break;
    case FloatDisplayMode::TimeCentiseconds:
        totalSeconds = static_cast<double>(fVal) / 100.0;
        break;
    case FloatDisplayMode::TimeMilliseconds:
        totalSeconds = static_cast<double>(fVal) / 1000.0;
        break;
    default:
        totalSeconds = static_cast<double>(fVal);
        break;
    }

    // Fall back to compact scientific notation if the float is unreasonably large (e.g. pointer/data misinterpreted as float)
    if (std::abs(totalSeconds) >= 360000.0 || (std::abs(totalSeconds) < 1e-4 && std::abs(totalSeconds) > 0.0)) {
        if (std::abs(totalSeconds) < 1e-4)
            return "00:00.00";
        return QString::number(fVal, 'e', 2);
    }

    bool isNeg = (totalSeconds < 0.0);
    double absSec = std::abs(totalSeconds);

    int hours = static_cast<int>(absSec / 3600.0);
    double rem = absSec - (hours * 3600.0);
    int minutes = static_cast<int>(rem / 60.0);
    double secs = rem - (minutes * 60.0);
    int isecs = static_cast<int>(secs);
    int centis = static_cast<int>((secs - isecs) * 100.0 + 0.5);

    if (centis >= 100) {
        centis = 0;
        isecs++;
        if (isecs >= 60) {
            isecs = 0;
            minutes++;
            if (minutes >= 60) {
                minutes = 0;
                hours++;
            }
        }
    }

    if (hours > 0) {
        return QString("%1%2:%3:%4").arg(isNeg ? "-" : "").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(isecs, 2, 10, QChar('0'));
    } else {
        return QString("%1%2:%3.%4")
            .arg(isNeg ? "-" : "")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(isecs, 2, 10, QChar('0'))
            .arg(centis, 2, 10, QChar('0'));
    }
}

void MemoryViewerWidget::paintEvent(QPaintEvent*) {
    QPainter painter(viewport());
    painter.setFont(m_monoFont);

    if (m_memoryData.empty() || m_regions.empty()) {
        painter.setPen(palette().color(QPalette::Disabled, QPalette::Text));
        painter.drawText(viewport()->rect(), Qt::AlignCenter,
                         "No memory dump loaded.\nClick 'Re-dump / Refresh RAM' to capture active memory.");
        return;
    }

    bool isDark = (palette().color(QPalette::Base).lightness() < 128);

    QRect headerRect(0, 0, viewport()->width(), m_headerHeight);
    QColor headerBg = isDark ? QColor(24, 27, 31) : QColor(232, 232, 232);
    QColor headerTextColor = isDark ? QColor(139, 148, 158) : QColor(74, 74, 74);
    QColor headerDividerColor = isDark ? QColor(48, 54, 61) : QColor(208, 208, 208);

    painter.fillRect(headerRect, headerBg);
    painter.setPen(headerTextColor);

    painter.drawText(m_addrColX, m_headerHeight - 6, "Address");

    if (m_viewMode == ViewMode::Mode8Bit) {
        int byteWidth = m_charWidth * 2;
        int byteSpacing = m_charWidth;
        const char hexDigits[] = "0123456789ABCDEF";
        for (int i = 0; i < 16; ++i) {
            int bx = m_hexColX + i * (byteWidth + byteSpacing);
            painter.drawText(bx, m_headerHeight - 6, QString(hexDigits[i]));
        }
    } else if (m_viewMode == ViewMode::Mode16Bit) {
        int wordWidth = m_charWidth * 4;
        int wordSpacing = m_charWidth;
        const char* wordHeaders[] = {"0", "2", "4", "6", "8", "A", "C", "E"};
        for (int i = 0; i < 8; ++i) {
            int wx = m_hexColX + i * (wordWidth + wordSpacing);
            painter.drawText(wx, m_headerHeight - 6, wordHeaders[i]);
        }
    } else if (m_viewMode == ViewMode::ModeFloat || m_viewMode == ViewMode::ModeFloatBE) {
        int dwordWidth = m_charWidth * 9;
        int dwordSpacing = m_charWidth * 2;
        const char* dwordHeaders[] = {"0", "4", "8", "C"};
        for (int i = 0; i < 4; ++i) {
            int dx = m_hexColX + i * (dwordWidth + dwordSpacing);
            painter.drawText(dx, m_headerHeight - 6, dwordHeaders[i]);
        }
    } else {
        int dwordWidth = m_charWidth * 8;
        int dwordSpacing = m_charWidth * 2;
        const char* dwordHeaders[] = {"0", "4", "8", "C"};
        for (int i = 0; i < 4; ++i) {
            int dx = m_hexColX + i * (dwordWidth + dwordSpacing);
            painter.drawText(dx, m_headerHeight - 6, dwordHeaders[i]);
        }
    }

    painter.drawText(m_asciiColX, m_headerHeight - 6, "ASCII Text");

    painter.setPen(headerDividerColor);
    painter.drawLine(0, m_headerHeight - 1, viewport()->width(), m_headerHeight - 1);

    int visibleRows = ((viewport()->height() - m_headerHeight) / m_lineHeight) + 1;
    int64_t startRow = verticalScrollBar()->value();
    int64_t endRow = std::min(startRow + visibleRows, totalRows());

    const QColor addrColor = isDark ? QColor(100, 160, 220) : QColor(0, 102, 180);
    const QColor textColor = palette().color(QPalette::Text);
    const QColor asciiColor = isDark ? QColor(160, 160, 160) : QColor(90, 90, 90);
    const QColor zeroColor = isDark ? QColor(100, 100, 100) : QColor(150, 150, 150);

    const QColor selectionBorderColor = isDark ? QColor(245, 130, 32) : QColor(230, 100, 10);
    const QColor selectionBgColor = isDark ? QColor(245, 130, 32, 60) : QColor(245, 130, 32, 45);

    const QColor pointerColor = isDark ? QColor(255, 123, 114) : QColor(190, 20, 20);  // Crimson in light mode
    const QColor variableColor = isDark ? QColor(121, 192, 255) : QColor(9, 105, 218); // Cobalt blue in light mode
    const QColor activeContextBg = isDark ? QColor(22, 48, 72, 160) : QColor(210, 232, 255, 180);

    // Array styling colors
    const QColor arrayBgColor = isDark ? QColor(121, 40, 202, 75) : QColor(138, 43, 226, 35);
    const QColor arrayBorderColor = isDark ? QColor(163, 113, 247, 180) : QColor(138, 43, 226, 120);
    const QColor rangeSelectBgColor = isDark ? QColor(138, 75, 175, 120) : QColor(160, 100, 220, 70);

    static const char hexCharsUpper[] = "0123456789ABCDEF";

    for (int64_t r = startRow; r < endRow; ++r) {
        int y = m_headerHeight + static_cast<int>((r - startRow) * m_lineHeight);

        // Locate specific memory segment containing virtual row
        const StackedRegion* reg = findRegionForRow(r);
        if (!reg)
            continue;

        int64_t relRow = r - reg->startRow;
        uint32_t rowAddress = reg->startAddress + static_cast<uint32_t>(relRow * 16);
        size_t dataOffset = reg->bufferOffset + static_cast<size_t>(relRow * 16);

        painter.setPen(addrColor);
        QString addrStr = QString("%1").arg(rowAddress, 8, 16, QChar('0')).toUpper();
        painter.drawText(m_addrColX, y + m_charHeight, addrStr);

        if (m_viewMode == ViewMode::Mode8Bit) {
            int byteWidth = m_charWidth * 2;
            int byteSpacing = m_charWidth;

            for (int i = 0; i < 16; ++i) {
                size_t bo = dataOffset + i;
                uint32_t currAddr = rowAddress + i;
                int bx = m_hexColX + i * (byteWidth + byteSpacing);

                if (bo < m_memoryData.size()) {
                    uint8_t byteVal = m_memoryData[bo];
                    auto it = m_highlights.find(currAddr);

                    // Tier 1: Array range background
                    for (const auto& arr : m_definedArrays) {
                        if (currAddr >= arr.startAddress && currAddr <= arr.endAddress) {
                            painter.fillRect(QRect(bx - 1, y, byteWidth + 2, m_lineHeight), arrayBgColor);
                            break;
                        }
                    }

                    // Active Shift+Click Range Selection highlight
                    if (m_hasRangeSelection && currAddr >= m_rangeStart && currAddr <= m_rangeEnd) {
                        painter.fillRect(QRect(bx - 1, y, byteWidth + 2, m_lineHeight), rangeSelectBgColor);
                    }

                    // Tier 2: Active struct / child context background
                    if (it != m_highlights.end() && it->second.isChildOfActive) {
                        painter.fillRect(QRect(bx - 1, y, byteWidth + 2, m_lineHeight), activeContextBg);
                    }

                    // Tier 4: Cursor focus box
                    if (m_hasSelection && currAddr == m_selectedAddress) {
                        QRect selRect(bx - 2, y, byteWidth + 4, m_lineHeight);
                        painter.fillRect(selRect, selectionBgColor);
                        painter.setPen(selectionBorderColor);
                        painter.drawRect(selRect);
                    }

                    // Tier 3: Foreground text with nibble-level pointer mask support
                    if (it != m_highlights.end()) {
                        if (it->second.isPointer && it->second.pointerMask != 0xFFFFFFFF) {
                            uint32_t byteOffsetInPtr = (currAddr >= it->second.startAddress) ? (currAddr - it->second.startAddress) : 0;
                            int shiftByte = (m_isBigEndian ? (3 - (byteOffsetInPtr % 4)) : (byteOffsetInPtr % 4)) * 8;
                            uint8_t hnVal = (byteVal >> 4) & 0xF;
                            uint8_t lnVal = byteVal & 0xF;
                            uint32_t hnMask = (it->second.pointerMask >> (shiftByte + 4)) & 0xF;
                            uint32_t lnMask = (it->second.pointerMask >> shiftByte) & 0xF;

                            bool hnIsPtr = (hnMask == 0xF) ? true : ((hnMask == 0) ? false : ((hnVal & ~hnMask) == 0));
                            bool lnIsPtr = (lnMask == 0xF) ? true : ((lnMask == 0) ? false : ((lnVal & ~lnMask) == 0));

                            painter.setPen(hnIsPtr ? pointerColor : (hnVal == 0 ? zeroColor : textColor));
                            painter.drawText(bx, y + m_charHeight, QString(hexCharsUpper[hnVal]));

                            painter.setPen(lnIsPtr ? pointerColor : (lnVal == 0 ? zeroColor : textColor));
                            painter.drawText(bx + m_charWidth, y + m_charHeight, QString(hexCharsUpper[lnVal]));
                        } else {
                            painter.setPen(it->second.isPointer ? pointerColor : variableColor);
                            painter.drawText(bx, y + m_charHeight, QString("%1").arg(byteVal, 2, 16, QChar('0')).toUpper());
                        }
                    } else {
                        painter.setPen(byteVal == 0 ? zeroColor : textColor);
                        painter.drawText(bx, y + m_charHeight, QString("%1").arg(byteVal, 2, 16, QChar('0')).toUpper());
                    }
                }
            }
        } else if (m_viewMode == ViewMode::Mode16Bit) {
            int wordWidth = m_charWidth * 4;
            int wordSpacing = m_charWidth;
            bool isBE = m_isBigEndian;

            for (int i = 0; i < 8; ++i) {
                uint32_t wordAddr = rowAddress + (i * 2);
                int wx = m_hexColX + i * (wordWidth + wordSpacing);

                if (dataOffset + (i * 2) + 1 < m_memoryData.size()) {
                    // Tier 1: Array background
                    for (const auto& arr : m_definedArrays) {
                        if ((wordAddr + 1 >= arr.startAddress) && (wordAddr <= arr.endAddress)) {
                            painter.fillRect(QRect(wx - 1, y, wordWidth + 2, m_lineHeight), arrayBgColor);
                            break;
                        }
                    }

                    // Active Shift+Click Range highlight
                    if (m_hasRangeSelection && (wordAddr + 1 >= m_rangeStart) && (wordAddr <= m_rangeEnd)) {
                        painter.fillRect(QRect(wx - 1, y, wordWidth + 2, m_lineHeight), rangeSelectBgColor);
                    }

                    auto it0 = m_highlights.find(wordAddr);
                    auto it1 = m_highlights.find(wordAddr + 1);
                    bool childActive = (it0 != m_highlights.end() && it0->second.isChildOfActive) ||
                                       (it1 != m_highlights.end() && it1->second.isChildOfActive);
                    if (childActive) {
                        painter.fillRect(QRect(wx - 1, y, wordWidth + 2, m_lineHeight), activeContextBg);
                    }

                    if (m_hasSelection && (m_selectedAddress & ~1U) == wordAddr) {
                        QRect selRect(wx - 2, y, wordWidth + 4, m_lineHeight);
                        painter.fillRect(selRect, selectionBgColor);
                        painter.setPen(selectionBorderColor);
                        painter.drawRect(selRect);
                    }

                    for (int b = 0; b < 2; ++b) {
                        uint32_t byteAddr = isBE ? (wordAddr + b) : (wordAddr + (1 - b));
                        size_t byteOffset = dataOffset + (byteAddr - rowAddress);
                        uint8_t byteVal = m_memoryData[byteOffset];
                        int bx = wx + b * (m_charWidth * 2);

                        auto it = m_highlights.find(byteAddr);
                        if (it != m_highlights.end()) {
                            if (it->second.isPointer && it->second.pointerMask != 0xFFFFFFFF) {
                                uint32_t byteOffsetInPtr = (byteAddr >= it->second.startAddress) ? (byteAddr - it->second.startAddress) : 0;
                                int shiftByte = (isBE ? (3 - (byteOffsetInPtr % 4)) : (byteOffsetInPtr % 4)) * 8;
                                uint8_t hnVal = (byteVal >> 4) & 0xF;
                                uint8_t lnVal = byteVal & 0xF;
                                uint32_t hnMask = (it->second.pointerMask >> (shiftByte + 4)) & 0xF;
                                uint32_t lnMask = (it->second.pointerMask >> shiftByte) & 0xF;

                                bool hnIsPtr = (hnMask == 0xF) ? true : ((hnMask == 0) ? false : ((hnVal & ~hnMask) == 0));
                                bool lnIsPtr = (lnMask == 0xF) ? true : ((lnMask == 0) ? false : ((lnVal & ~lnMask) == 0));

                                painter.setPen(hnIsPtr ? pointerColor : (hnVal == 0 ? zeroColor : textColor));
                                painter.drawText(bx, y + m_charHeight, QString(hexCharsUpper[hnVal]));

                                painter.setPen(lnIsPtr ? pointerColor : (lnVal == 0 ? zeroColor : textColor));
                                painter.drawText(bx + m_charWidth, y + m_charHeight, QString(hexCharsUpper[lnVal]));
                            } else {
                                painter.setPen(it->second.isPointer ? pointerColor : variableColor);
                                painter.drawText(bx, y + m_charHeight, QString("%1").arg(byteVal, 2, 16, QChar('0')).toUpper());
                            }
                        } else {
                            painter.setPen(byteVal == 0 ? zeroColor : textColor);
                            painter.drawText(bx, y + m_charHeight, QString("%1").arg(byteVal, 2, 16, QChar('0')).toUpper());
                        }
                    }
                }
            }
        } else if (m_viewMode == ViewMode::ModeFloat || m_viewMode == ViewMode::ModeFloatBE) {
            int dwordWidth = m_charWidth * 9;
            int dwordSpacing = m_charWidth * 2;
            bool isBE = (m_viewMode == ViewMode::ModeFloatBE) || m_isBigEndian;

            for (int i = 0; i < 4; ++i) {
                uint32_t dwordAddr = rowAddress + (i * 4);
                int dx = m_hexColX + i * (dwordWidth + dwordSpacing);

                if (dataOffset + (i * 4) + 3 < m_memoryData.size()) {
                    // Tier 1: Array background
                    for (const auto& arr : m_definedArrays) {
                        if ((dwordAddr + 3 >= arr.startAddress) && (dwordAddr <= arr.endAddress)) {
                            painter.fillRect(QRect(dx - 1, y, dwordWidth + 2, m_lineHeight), arrayBgColor);
                            break;
                        }
                    }

                    // Active Shift+Click Range highlight
                    if (m_hasRangeSelection && (dwordAddr + 3 >= m_rangeStart) && (dwordAddr <= m_rangeEnd)) {
                        painter.fillRect(QRect(dx - 1, y, dwordWidth + 2, m_lineHeight), rangeSelectBgColor);
                    }

                    bool childActive = false;
                    for (uint32_t k = 0; k < 4; ++k) {
                        auto itK = m_highlights.find(dwordAddr + k);
                        if (itK != m_highlights.end() && itK->second.isChildOfActive) {
                            childActive = true;
                            break;
                        }
                    }
                    if (childActive) {
                        painter.fillRect(QRect(dx - 1, y, dwordWidth + 2, m_lineHeight), activeContextBg);
                    }

                    if (m_hasSelection && (m_selectedAddress & ~3U) == dwordAddr) {
                        QRect selRect(dx - 2, y, dwordWidth + 4, m_lineHeight);
                        painter.fillRect(selRect, selectionBgColor);
                        painter.setPen(selectionBorderColor);
                        painter.drawRect(selRect);
                    }

                    uint32_t rawVal = 0;
                    if (isBE) {
                        rawVal = (static_cast<uint32_t>(m_memoryData[dataOffset + (i * 4)]) << 24) |
                                 (static_cast<uint32_t>(m_memoryData[dataOffset + (i * 4) + 1]) << 16) |
                                 (static_cast<uint32_t>(m_memoryData[dataOffset + (i * 4) + 2]) << 8) |
                                 (static_cast<uint32_t>(m_memoryData[dataOffset + (i * 4) + 3]));
                    } else {
                        rawVal = (static_cast<uint32_t>(m_memoryData[dataOffset + (i * 4) + 3]) << 24) |
                                 (static_cast<uint32_t>(m_memoryData[dataOffset + (i * 4) + 2]) << 16) |
                                 (static_cast<uint32_t>(m_memoryData[dataOffset + (i * 4) + 1]) << 8) |
                                 (static_cast<uint32_t>(m_memoryData[dataOffset + (i * 4)]));
                    }

                    float fVal = 0.0f;
                    std::memcpy(&fVal, &rawVal, sizeof(float));

                    QString floatStr = formatFloatString(fVal, rawVal);

                    auto it = m_highlights.find(dwordAddr);
                    if (it != m_highlights.end()) {
                        painter.setPen(it->second.isPointer ? pointerColor : variableColor);
                    } else {
                        painter.setPen(rawVal == 0 ? zeroColor : textColor);
                    }

                    painter.drawText(dx, y + m_charHeight, floatStr);
                }
            }
        } else {
            int dwordWidth = m_charWidth * 8;
            int dwordSpacing = m_charWidth * 2;
            bool isBE = (m_viewMode == ViewMode::Mode32BitBE) || m_isBigEndian;

            for (int i = 0; i < 4; ++i) {
                uint32_t dwordAddr = rowAddress + (i * 4);
                int dx = m_hexColX + i * (dwordWidth + dwordSpacing);

                if (dataOffset + (i * 4) + 3 < m_memoryData.size()) {
                    // Tier 1: Array background
                    for (const auto& arr : m_definedArrays) {
                        if ((dwordAddr + 3 >= arr.startAddress) && (dwordAddr <= arr.endAddress)) {
                            painter.fillRect(QRect(dx - 1, y, dwordWidth + 2, m_lineHeight), arrayBgColor);
                            break;
                        }
                    }

                    // Active Shift+Click Range highlight
                    if (m_hasRangeSelection && (dwordAddr + 3 >= m_rangeStart) && (dwordAddr <= m_rangeEnd)) {
                        painter.fillRect(QRect(dx - 1, y, dwordWidth + 2, m_lineHeight), rangeSelectBgColor);
                    }

                    bool childActive = false;
                    for (uint32_t k = 0; k < 4; ++k) {
                        auto itK = m_highlights.find(dwordAddr + k);
                        if (itK != m_highlights.end() && itK->second.isChildOfActive) {
                            childActive = true;
                            break;
                        }
                    }
                    if (childActive) {
                        painter.fillRect(QRect(dx - 1, y, dwordWidth + 2, m_lineHeight), activeContextBg);
                    }

                    if (m_hasSelection && (m_selectedAddress & ~3U) == dwordAddr) {
                        QRect selRect(dx - 2, y, dwordWidth + 4, m_lineHeight);
                        painter.fillRect(selRect, selectionBgColor);
                        painter.setPen(selectionBorderColor);
                        painter.drawRect(selRect);
                    }

                    for (int b = 0; b < 4; ++b) {
                        uint32_t byteAddr = isBE ? (dwordAddr + b) : (dwordAddr + (3 - b));
                        size_t byteOffset = dataOffset + (byteAddr - rowAddress);
                        uint8_t byteVal = m_memoryData[byteOffset];
                        int bx = dx + b * (m_charWidth * 2);

                        auto it = m_highlights.find(byteAddr);
                        if (it != m_highlights.end()) {
                            // If this address is a pointer with a specific mask (not 0xFFFFFFFF), apply nibble-level masking
                            if (it->second.isPointer && it->second.pointerMask != 0xFFFFFFFF) {
                                uint8_t hnVal = (byteVal >> 4) & 0xF;
                                uint8_t lnVal = byteVal & 0xF;

                                int shiftH = (7 - (b * 2)) * 4;
                                int shiftL = (6 - (b * 2)) * 4;
                                uint32_t hMask = (it->second.pointerMask >> shiftH) & 0xF;
                                uint32_t lMask = (it->second.pointerMask >> shiftL) & 0xF;

                                bool hIsPtr = (hMask == 0xF) ? true : ((hMask == 0) ? false : ((hnVal & ~hMask) == 0));
                                bool lIsPtr = (lMask == 0xF) ? true : ((lMask == 0) ? false : ((lnVal & ~lMask) == 0));

                                painter.setPen(hIsPtr ? pointerColor : (hnVal == 0 ? zeroColor : textColor));
                                painter.drawText(bx, y + m_charHeight, QString(hexCharsUpper[hnVal]));

                                painter.setPen(lIsPtr ? pointerColor : (lnVal == 0 ? zeroColor : textColor));
                                painter.drawText(bx + m_charWidth, y + m_charHeight, QString(hexCharsUpper[lnVal]));
                            } else {
                                painter.setPen(it->second.isPointer ? pointerColor : variableColor);
                                painter.drawText(bx, y + m_charHeight, QString("%1").arg(byteVal, 2, 16, QChar('0')).toUpper());
                            }
                        } else {
                            painter.setPen(byteVal == 0 ? zeroColor : textColor);
                            painter.drawText(bx, y + m_charHeight, QString("%1").arg(byteVal, 2, 16, QChar('0')).toUpper());
                        }
                    }
                }
            }
        }

        // ASCII Preview Column
        for (int b = 0; b < 16; ++b) {
            size_t byteOffset = dataOffset + b;
            uint32_t currByteAddr = rowAddress + b;
            int ax = m_asciiColX + b * m_charWidth;

            if (byteOffset < m_memoryData.size()) {
                uint8_t byteVal = m_memoryData[byteOffset];
                QChar asciiCh = (byteVal >= 32 && byteVal <= 126) ? QChar(byteVal) : QChar('.');

                // Array background highlight for ASCII column
                for (const auto& arr : m_definedArrays) {
                    if (currByteAddr >= arr.startAddress && currByteAddr <= arr.endAddress) {
                        painter.fillRect(QRect(ax, y, m_charWidth, m_lineHeight), arrayBgColor);
                        break;
                    }
                }

                if (m_hasRangeSelection && currByteAddr >= m_rangeStart && currByteAddr <= m_rangeEnd) {
                    painter.fillRect(QRect(ax, y, m_charWidth, m_lineHeight), rangeSelectBgColor);
                }

                auto it = m_highlights.find(currByteAddr);
                if (it != m_highlights.end()) {
                    painter.setPen(it->second.isPointer ? pointerColor : variableColor);
                } else {
                    painter.setPen(asciiColor);
                }
                painter.drawText(ax, y + m_charHeight, QString(asciiCh));
            }
        }
    }
}

} // namespace PointerFinder2::UI::Widgets
