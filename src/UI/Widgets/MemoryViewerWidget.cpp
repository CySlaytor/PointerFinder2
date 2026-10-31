#include "MemoryViewerWidget.h"

#include <algorithm>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
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

        updateMetrics();
        updateScrollBars();
        viewport()->update();
    }

    void MemoryViewerWidget::clearMemory() {
        m_memoryData.clear();
        m_highlights.clear();
        m_hasSelection = false;
        updateScrollBars();
        viewport()->update();
    }

    void MemoryViewerWidget::setViewMode(ViewMode mode) {
        if (m_viewMode != mode) {
            m_viewMode = mode;
            updateMetrics();
            updateScrollBars();
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
    }

    void MemoryViewerWidget::setSelectedAddress(uint32_t address) {
        if (m_memoryData.empty()) return;

        uint32_t maxAddr = m_memoryBaseAddress + static_cast<uint32_t>(m_memoryData.size());
        m_selectedAddress = std::clamp(address, m_memoryBaseAddress, maxAddr > 1 ? maxAddr - 1 : m_memoryBaseAddress);
        m_hasSelection = true;

        goToAddress(m_selectedAddress);
        emit addressSelected(m_selectedAddress);
        viewport()->update();
    }

    void MemoryViewerWidget::goToAddress(uint32_t address) {
        if (m_memoryData.empty()) return;

        if (address < m_memoryBaseAddress) address = m_memoryBaseAddress;
        uint32_t offset = address - m_memoryBaseAddress;
        int targetRow = static_cast<int>(offset / 16);

        int visibleRows = (viewport()->height() - m_headerHeight) / m_lineHeight;
        int centeredRow = std::max(0, targetRow - (visibleRows / 2));

        verticalScrollBar()->setValue(centeredRow);
        m_selectedAddress = address;
        m_hasSelection = true;
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
            bool isBE = m_isBigEndian;
            for (int i = 0; i < 8; ++i) {
                int wx = m_hexColX + i * (wordWidth + wordSpacing);
                if (pos.x() >= wx - 2 && pos.x() < wx + wordWidth + 2) {
                    int charInWord = (pos.x() - wx) / (m_charWidth * 2);
                    charInWord = std::clamp(charInWord, 0, 1);
                    return isBE ? (rowAddress + (i * 2) + charInWord)
                                : (rowAddress + (i * 2) + (1 - charInWord));
                }
            }
        }
        else {
            int dwordWidth = m_charWidth * 8;
            int dwordSpacing = m_charWidth * 2;
            bool isBE = (m_viewMode == ViewMode::Mode32BitBE) || m_isBigEndian;
            for (int i = 0; i < 4; ++i) {
                int dx = m_hexColX + i * (dwordWidth + dwordSpacing);
                if (pos.x() >= dx - 3 && pos.x() < dx + dwordWidth + 3) {
                    int byteInDword = (pos.x() - dx) / (m_charWidth * 2);
                    byteInDword = std::clamp(byteInDword, 0, 3);
                    return isBE ? (rowAddress + (i * 4) + byteInDword)
                                : (rowAddress + (i * 4) + (3 - byteInDword));
                }
            }
        }

        if (pos.x() >= m_asciiColX) {
            int charIdx = (pos.x() - m_asciiColX) / m_charWidth;
            if (charIdx >= 0 && charIdx < 16) {
                return rowAddress + charIdx;
            }
        }

        return rowAddress;
    }

    void MemoryViewerWidget::mousePressEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton && !m_memoryData.empty()) {
            uint32_t addr = addressAtPosition(event->pos());
            if (addr >= m_memoryBaseAddress && addr < m_memoryBaseAddress + m_memoryData.size()) {
                m_selectedAddress = addr;
                m_hasSelection = true;
                emit addressSelected(m_selectedAddress);
                viewport()->update();
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

    void MemoryViewerWidget::mouseMoveEvent(QMouseEvent* event) {
        if (!m_memoryData.empty()) {
            uint32_t addr = addressAtPosition(event->pos());
            if (addr != 0) {
                auto it = m_highlights.find(addr);

                if (it != m_highlights.end()) {
                    uint32_t baseDisplayAddr = (it->second.startAddress != 0) ? it->second.startAddress : addr;
                    QString addrHex = "0x" + QString("%1").arg(baseDisplayAddr, 8, 16, QChar('0')).toUpper();

                    QString line1 = it->second.offset.isEmpty()
                        ? QString("<b>%1</b>").arg(addrHex)
                        : QString("<b>%1</b> (%2)").arg(addrHex, it->second.offset);

                    QString line2 = it->second.name.isEmpty() ? it->second.label : it->second.name;

                    QString line3 = it->second.isPointer
                        ? "<span style='color: #FF7B72;'>[Pointer Target]</span>"
                        : "<span style='color: #79C0FF;'>[Documented Variable]</span>";

                    QString tip = QString("<div style='margin:0; padding:0; line-height:115%;'>%1<br>%2<br>%3</div>")
                        .arg(line1)
                        .arg(line2)
                        .arg(line3);

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
        QToolTip::hideText();
        QAbstractScrollArea::leaveEvent(event);
    }

    void MemoryViewerWidget::keyPressEvent(QKeyEvent* event) {
        if (m_memoryData.empty()) {
            QAbstractScrollArea::keyPressEvent(event);
            return;
        }

        uint32_t maxAddr = m_memoryBaseAddress + static_cast<uint32_t>(m_memoryData.size());
        uint32_t step = (m_viewMode == ViewMode::Mode8Bit) ? 1 : ((m_viewMode == ViewMode::Mode16Bit) ? 2 : 4);

        if (event->key() == Qt::Key_Up) {
            if (m_selectedAddress >= m_memoryBaseAddress + 16) {
                setSelectedAddress(m_selectedAddress - 16);
            }
            return;
        }
        else if (event->key() == Qt::Key_Down) {
            if (m_selectedAddress + 16 < maxAddr) {
                setSelectedAddress(m_selectedAddress + 16);
            }
            return;
        }
        else if (event->key() == Qt::Key_Left) {
            if (m_selectedAddress >= m_memoryBaseAddress + step) {
                setSelectedAddress(m_selectedAddress - step);
            }
            return;
        }
        else if (event->key() == Qt::Key_Right) {
            if (m_selectedAddress + step < maxAddr) {
                setSelectedAddress(m_selectedAddress + step);
            }
            return;
        }
        else if (event->matches(QKeySequence::Copy)) {
            if (m_hasSelection) {
                QApplication::clipboard()->setText(QString("%1").arg(m_selectedAddress, 8, 16, QChar('0')).toUpper());
            }
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

        painter.drawText(m_asciiColX, m_headerHeight - 6, "Text");

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

                        if (it != m_highlights.end() && it->second.isChildOfActive) {
                            painter.fillRect(QRect(bx - 1, y, byteWidth + 2, m_lineHeight), activeContextBg);
                        }

                        if (m_hasSelection && currAddr == m_selectedAddress) {
                            QRect selRect(bx - 2, y, byteWidth + 4, m_lineHeight);
                            painter.fillRect(selRect, selectionBgColor);
                            painter.setPen(selectionBorderColor);
                            painter.drawRect(selRect);
                        }

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
