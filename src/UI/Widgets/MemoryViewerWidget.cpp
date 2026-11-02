#include "MemoryViewerWidget.h"

#include <algorithm>
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

namespace PointerFinder2::UI::Widgets {

    MemoryViewerWidget::MemoryViewerWidget(QWidget* parent)
        : QAbstractScrollArea(parent),
          m_monoFont("Consolas", 10) {
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
        m_rangeBannerWidget->setStyleSheet(
            "#rangeBannerWidget {"
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
            "}"
        );

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
        }

        m_asciiColX = m_hexColX + hexBlockWidth + 15;
    }

    void MemoryViewerWidget::setMemoryData(std::vector<uint8_t> memData, uint32_t memoryBaseAddress, bool isBigEndian) {
        m_memoryData = std::move(memData);
        m_memoryBaseAddress = memoryBaseAddress;
        m_isBigEndian = isBigEndian;

        if (m_isBigEndian && m_viewMode == ViewMode::Mode32Bit) {
            m_viewMode = ViewMode::Mode32BitBE;
        }

        if (m_selectedAddress < m_memoryBaseAddress || 
            m_selectedAddress >= m_memoryBaseAddress + m_memoryData.size()) {
            m_selectedAddress = m_memoryBaseAddress;
        }

        clearRangeSelection();
        m_navHistory.clear();
        m_navHistory.push_back({ m_selectedAddress, 0 });
        m_navHistoryIndex = 0;
        emit navigationHistoryChanged(false, false, 1, 1);

        updateMetrics();
        updateScrollBars();
        viewport()->update();
    }

    void MemoryViewerWidget::clearMemory() {
        m_memoryData.clear();
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

    bool MemoryViewerWidget::isAddressDocumented(uint32_t address) const {
        return m_highlights.find(address) != m_highlights.end();
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
            if (m_rangeBannerWidget) m_rangeBannerWidget->hide();
            return;
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

    bool MemoryViewerWidget::canNavigateBack() const {
        return m_navHistoryIndex > 0;
    }

    bool MemoryViewerWidget::canNavigateForward() const {
        return m_navHistoryIndex >= 0 && (m_navHistoryIndex + 1 < static_cast<int>(m_navHistory.size()));
    }

    void MemoryViewerWidget::navigateBack() {
        if (canNavigateBack()) {
            m_isNavigatingHistory = true;

            // Preserve current view and scroll position in the current slot before moving back
            if (m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
                m_navHistory[m_navHistoryIndex] = { m_selectedAddress, verticalScrollBar()->value() };
            }

            --m_navHistoryIndex;
            const auto& state = m_navHistory[m_navHistoryIndex];

            m_selectedAddress = state.address;
            m_hasSelection = true;
            verticalScrollBar()->setValue(state.scrollValue);

            m_isNavigatingHistory = false;
            emit addressSelected(m_selectedAddress);
            emit navigationHistoryChanged(canNavigateBack(), canNavigateForward(), m_navHistoryIndex + 1, static_cast<int>(m_navHistory.size()));
            viewport()->update();
        }
    }

    void MemoryViewerWidget::navigateForward() {
        if (canNavigateForward()) {
            m_isNavigatingHistory = true;

            // Preserve current view and scroll position in the current slot before moving forward
            if (m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
                m_navHistory[m_navHistoryIndex] = { m_selectedAddress, verticalScrollBar()->value() };
            }

            ++m_navHistoryIndex;
            const auto& state = m_navHistory[m_navHistoryIndex];

            m_selectedAddress = state.address;
            m_hasSelection = true;
            verticalScrollBar()->setValue(state.scrollValue);

            m_isNavigatingHistory = false;
            emit addressSelected(m_selectedAddress);
            emit navigationHistoryChanged(canNavigateBack(), canNavigateForward(), m_navHistoryIndex + 1, static_cast<int>(m_navHistory.size()));
            viewport()->update();
        }
    }

    uint32_t MemoryViewerWidget::readDwordAt(uint32_t address) const {
        if (address < m_memoryBaseAddress) return 0;
        size_t offset = address - m_memoryBaseAddress;
        if (offset + 3 >= m_memoryData.size()) return 0;

        bool isBE = (m_viewMode == ViewMode::Mode32BitBE) || m_isBigEndian;
        if (isBE) {
            return (static_cast<uint32_t>(m_memoryData[offset]) << 24) |
                   (static_cast<uint32_t>(m_memoryData[offset + 1]) << 16) |
                   (static_cast<uint32_t>(m_memoryData[offset + 2]) << 8) |
                   (static_cast<uint32_t>(m_memoryData[offset + 3]));
        }
        else {
            return (static_cast<uint32_t>(m_memoryData[offset + 3]) << 24) |
                   (static_cast<uint32_t>(m_memoryData[offset + 2]) << 16) |
                   (static_cast<uint32_t>(m_memoryData[offset + 1]) << 8) |
                   (static_cast<uint32_t>(m_memoryData[offset]));
        }
    }

    int64_t MemoryViewerWidget::totalRows() const {
        if (m_memoryData.empty()) return 0;
        return static_cast<int64_t>((m_memoryData.size() + 15) / 16);
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
        if (m_memoryData.empty()) return;

        uint32_t maxAddr = m_memoryBaseAddress + static_cast<uint32_t>(m_memoryData.size());
        m_selectedAddress = std::clamp(address, m_memoryBaseAddress, maxAddr > 1 ? maxAddr - 1 : m_memoryBaseAddress);
        m_hasSelection = true;

        goToAddress(m_selectedAddress, recordHistory, centerTarget);
        emit addressSelected(m_selectedAddress);
        viewport()->update();
    }

    void MemoryViewerWidget::goToAddress(uint32_t address, bool recordHistory, bool centerTarget) {
        if (m_memoryData.empty()) return;

        if (address < m_memoryBaseAddress) address = m_memoryBaseAddress;
        uint32_t offset = address - m_memoryBaseAddress;
        int targetRow = static_cast<int>(offset / 16);

        int visibleRows = (viewport()->height() - m_headerHeight) / m_lineHeight;
        int currentRow = verticalScrollBar()->value();
        int maxRow = verticalScrollBar()->maximum();

        // 1. If this is a new navigation step, record the PREVIOUS state before scrolling
        if (recordHistory && !m_isNavigatingHistory && m_selectedAddress != 0) {
            if (m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
                m_navHistory[m_navHistoryIndex] = { m_selectedAddress, currentRow };
            }
        }

        // 2. Perform the view positioning
        if (centerTarget) {
            // Explicit pointer exploration or jump: Center the target address row in view
            int centeredRow = std::max(0, targetRow - (visibleRows / 2));
            verticalScrollBar()->setValue(std::min(centeredRow, maxRow));
        }
        else {
            // Three-section viewport deadzone approach:
            // The visible rows are divided into three equal sections.
            // When moving with arrows, the cursor moves freely inside the middle third section.
            // When reaching either edge, the view scrolls row-by-row underneath the cursor, pinning it to the edge.
            if (visibleRows >= 6) {
                int topMargin = visibleRows / 3;
                int bottomMargin = (2 * visibleRows) / 3;

                if (targetRow < currentRow + topMargin) {
                    int newScroll = std::max(0, targetRow - topMargin);
                    verticalScrollBar()->setValue(newScroll);
                }
                else if (targetRow > currentRow + bottomMargin) {
                    int newScroll = std::min(maxRow, targetRow - bottomMargin);
                    verticalScrollBar()->setValue(newScroll);
                }
            }
            else if (visibleRows > 0) {
                if (targetRow < currentRow) {
                    verticalScrollBar()->setValue(targetRow);
                }
                else if (targetRow >= currentRow + visibleRows) {
                    verticalScrollBar()->setValue(std::min(maxRow, targetRow - visibleRows + 1));
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

                m_navHistory.push_back({ address, newScroll });
                if (m_navHistory.size() > MAX_NAV_HISTORY) {
                    m_navHistory.erase(m_navHistory.begin());
                }
                m_navHistoryIndex = static_cast<int>(m_navHistory.size()) - 1;

                emit navigationHistoryChanged(canNavigateBack(), canNavigateForward(), m_navHistoryIndex + 1, static_cast<int>(m_navHistory.size()));
            }
            else {
                // Track current cursor and scroll position in-place without creating a new step
                if (m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
                    m_navHistory[m_navHistoryIndex] = { m_selectedAddress, newScroll };
                }
            }
        }

        viewport()->update();
    }

    uint32_t MemoryViewerWidget::addressAtPosition(const QPoint& pos) const {
        if (m_memoryData.empty() || pos.y() < m_headerHeight) return 0;

        int rowOffset = (pos.y() - m_headerHeight) / m_lineHeight;
        int64_t row = verticalScrollBar()->value() + rowOffset;
        if (row < 0 || row >= totalRows()) return 0;

        uint32_t rowAddress = m_memoryBaseAddress + static_cast<uint32_t>(row * 16);

        if (m_viewMode == ViewMode::Mode8Bit) {
            int byteWidth = m_charWidth * 2;
            int byteSpacing = m_charWidth;
            for (int i = 0; i < 16; ++i) {
                int bx = m_hexColX + i * (byteWidth + byteSpacing);
                if (pos.x() >= bx - 2 && pos.x() < bx + byteWidth + 2) {
                    return rowAddress + i;
                }
            }
        }
        else if (m_viewMode == ViewMode::Mode16Bit) {
            int wordWidth = m_charWidth * 4;
            int wordSpacing = m_charWidth;
            for (int i = 0; i < 8; ++i) {
                int wx = m_hexColX + i * (wordWidth + wordSpacing);
                if (pos.x() >= wx - 2 && pos.x() < wx + wordWidth + 2) {
                    // Snap click to 16-bit word alignment
                    return rowAddress + (i * 2);
                }
            }
        }
        else {
            int dwordWidth = m_charWidth * 8;
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
                if (m_viewMode == ViewMode::Mode32Bit || m_viewMode == ViewMode::Mode32BitBE) {
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
            if (addr >= m_memoryBaseAddress && addr < m_memoryBaseAddress + m_memoryData.size()) {
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
                }
                else {
                    clearRangeSelection();
                    m_rangeAnchor = addr;
                    m_selectedAddress = addr;
                    m_hasSelection = true;

                    // Update current history slot position when clicking to select an address
                    if (!m_isNavigatingHistory && m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
                        m_navHistory[m_navHistoryIndex] = { m_selectedAddress, verticalScrollBar()->value() };
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
            if (addr >= m_memoryBaseAddress && addr < m_memoryBaseAddress + m_memoryData.size()) {
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
            if (addr >= m_memoryBaseAddress && addr < m_memoryBaseAddress + m_memoryData.size()) {
                m_selectedAddress = addr;
                m_hasSelection = true;

                // Update current history slot before opening context menu
                if (!m_isNavigatingHistory && m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
                    m_navHistory[m_navHistoryIndex] = { m_selectedAddress, verticalScrollBar()->value() };
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
                    uint32_t baseDisplayAddr = (it != m_highlights.end() && it->second.startAddress != 0) ? it->second.startAddress : addr;
                    QString addrHex = "0x" + QString("%1").arg(baseDisplayAddr, 8, 16, QChar('0')).toUpper();

                    QString line1 = (it != m_highlights.end() && !it->second.offset.isEmpty())
                        ? QString("<b>%1</b> (%2)").arg(addrHex, it->second.offset)
                        : QString("<b>%1</b>").arg(addrHex);

                    QString line2 = (it != m_highlights.end())
                        ? (it->second.name.isEmpty() ? it->second.label : it->second.name)
                        : "";

                    QString line3 = "";
                    if (it != m_highlights.end()) {
                        line3 = it->second.isPointer
                            ? "<span style='color: #FF7B72;'>[Pointer Target]</span>"
                            : "<span style='color: #79C0FF;'>[Documented Variable]</span>";
                    }

                    QString arrayTag = "";
                    if (activeArray != nullptr) {
                        uint32_t offsetInArray = addr - activeArray->startAddress;

                        // Clean redundant duplicate size tags from array name (e.g. "[80 bytes] Pointer to Array")
                        QString cleanArrName = activeArray->name;
                        cleanArrName.remove(QRegularExpression(R"(\[\d+\s*bytes\]\s*)", QRegularExpression::CaseInsensitiveOption));
                        cleanArrName = cleanArrName.trimmed();

                        if (cleanArrName.isEmpty() || cleanArrName.compare("Array", Qt::CaseInsensitive) == 0 ||
                            cleanArrName.compare("Pointer to Array", Qt::CaseInsensitive) == 0) {
                            arrayTag = QString("<span style='color: #D2A8FF;'>[Array: (%1 bytes), Offset: +0x%2]</span>")
                                .arg(activeArray->size)
                                .arg(QString::number(offsetInArray, 16).toUpper());
                        }
                        else {
                            arrayTag = QString("<span style='color: #D2A8FF;'>[Array: %1 (%2 bytes), Offset: +0x%3]</span>")
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
                }
                else {
                    QToolTip::hideText();
                }
            }
            else {
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
        if (m_memoryData.empty()) {
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

        uint32_t maxAddr = m_memoryBaseAddress + static_cast<uint32_t>(m_memoryData.size());
        uint32_t step = (m_viewMode == ViewMode::Mode8Bit) ? 1 : ((m_viewMode == ViewMode::Mode16Bit) ? 2 : 4);

        if (event->key() == Qt::Key_Up) {
            if (m_selectedAddress >= m_memoryBaseAddress + 16) {
                setSelectedAddress(m_selectedAddress - 16, false, false);
            }
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_Down) {
            if (m_selectedAddress + 16 < maxAddr) {
                setSelectedAddress(m_selectedAddress + 16, false, false);
            }
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_Left) {
            if (m_selectedAddress >= m_memoryBaseAddress + step) {
                setSelectedAddress(m_selectedAddress - step, false, false);
            }
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_Right) {
            if (m_selectedAddress + step < maxAddr) {
                setSelectedAddress(m_selectedAddress + step, false, false);
            }
            event->accept();
            return;
        }
        else if (event->matches(QKeySequence::Copy)) {
            if (m_hasSelection) {
                QApplication::clipboard()->setText(QString("%1").arg(m_selectedAddress, 8, 16, QChar('0')).toUpper());
            }
            event->accept();
            return;
        }

        QAbstractScrollArea::keyPressEvent(event);
    }

    void MemoryViewerWidget::paintEvent(QPaintEvent*) {
        QPainter painter(viewport());
        painter.setFont(m_monoFont);

        if (m_memoryData.empty()) {
            painter.setPen(palette().color(QPalette::Disabled, QPalette::Text));
            painter.drawText(viewport()->rect(), Qt::AlignCenter, "No memory dump loaded.\nClick 'Re-dump / Refresh RAM' to capture active memory.");
            return;
        }

        QRect headerRect(0, 0, viewport()->width(), m_headerHeight);
        painter.fillRect(headerRect, QColor(24, 27, 31));
        painter.setPen(QColor(139, 148, 158));

        painter.drawText(m_addrColX, m_headerHeight - 6, "Address");

        if (m_viewMode == ViewMode::Mode8Bit) {
            int byteWidth = m_charWidth * 2;
            int byteSpacing = m_charWidth;
            const char hexDigits[] = "0123456789ABCDEF";
            for (int i = 0; i < 16; ++i) {
                int bx = m_hexColX + i * (byteWidth + byteSpacing);
                painter.drawText(bx, m_headerHeight - 6, QString(hexDigits[i]));
            }
        }
        else if (m_viewMode == ViewMode::Mode16Bit) {
            int wordWidth = m_charWidth * 4;
            int wordSpacing = m_charWidth;
            const char* wordHeaders[] = { "0", "2", "4", "6", "8", "A", "C", "E" };
            for (int i = 0; i < 8; ++i) {
                int wx = m_hexColX + i * (wordWidth + wordSpacing);
                painter.drawText(wx, m_headerHeight - 6, wordHeaders[i]);
            }
        }
        else {
            int dwordWidth = m_charWidth * 8;
            int dwordSpacing = m_charWidth * 2;
            const char* dwordHeaders[] = { "0", "4", "8", "C" };
            for (int i = 0; i < 4; ++i) {
                int dx = m_hexColX + i * (dwordWidth + dwordSpacing);
                painter.drawText(dx, m_headerHeight - 6, dwordHeaders[i]);
            }
        }

        painter.drawText(m_asciiColX, m_headerHeight - 6, "ASCII Text");

        painter.setPen(QColor(48, 54, 61));
        painter.drawLine(0, m_headerHeight - 1, viewport()->width(), m_headerHeight - 1);

        int visibleRows = ((viewport()->height() - m_headerHeight) / m_lineHeight) + 1;
        int64_t startRow = verticalScrollBar()->value();
        int64_t endRow = std::min(startRow + visibleRows, totalRows());

        const QColor addrColor = QColor(100, 160, 220);
        const QColor textColor = palette().color(QPalette::Text);
        const QColor asciiColor = QColor(160, 160, 160);
        const QColor selectionBorderColor = QColor(245, 130, 32);
        const QColor selectionBgColor = QColor(245, 130, 32, 60);

        const QColor pointerColor = QColor(255, 123, 114);    // Coral/Red
        const QColor variableColor = QColor(121, 192, 255);   // Cyan/Blue
        const QColor activeContextBg = QColor(22, 48, 72, 160);

        // Array styling colors
        const QColor arrayBgColor = QColor(121, 40, 202, 75);       // Translucent purple background
        const QColor arrayBorderColor = QColor(163, 113, 247, 180);  // Lavender border
        const QColor rangeSelectBgColor = QColor(138, 75, 175, 120); // Active Shift+Click range highlight

        for (int64_t r = startRow; r < endRow; ++r) {
            int y = m_headerHeight + static_cast<int>((r - startRow) * m_lineHeight);
            uint32_t rowAddress = m_memoryBaseAddress + static_cast<uint32_t>(r * 16);
            size_t dataOffset = static_cast<size_t>(r * 16);

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

                        // Tier 3: Foreground text
                        if (it != m_highlights.end()) {
                            painter.setPen(it->second.isPointer ? pointerColor : variableColor);
                        }
                        else {
                            painter.setPen(byteVal == 0 ? QColor(100, 100, 100) : textColor);
                        }

                        painter.drawText(bx, y + m_charHeight, QString("%1").arg(byteVal, 2, 16, QChar('0')).toUpper());
                    }
                }
            }
            else if (m_viewMode == ViewMode::Mode16Bit) {
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
                                painter.setPen(it->second.isPointer ? pointerColor : variableColor);
                            }
                            else {
                                painter.setPen(byteVal == 0 ? QColor(100, 100, 100) : textColor);
                            }
                            painter.drawText(bx, y + m_charHeight, QString("%1").arg(byteVal, 2, 16, QChar('0')).toUpper());
                        }
                    }
                }
            }
            else {
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
                                painter.setPen(it->second.isPointer ? pointerColor : variableColor);
                            }
                            else {
                                painter.setPen(byteVal == 0 ? QColor(100, 100, 100) : textColor);
                            }
                            painter.drawText(bx, y + m_charHeight, QString("%1").arg(byteVal, 2, 16, QChar('0')).toUpper());
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
                    }
                    else {
                        painter.setPen(asciiColor);
                    }
                    painter.drawText(ax, y + m_charHeight, QString(asciiCh));
                }
            }
        }
    }

}
