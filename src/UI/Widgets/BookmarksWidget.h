#pragma once

#include "../../Emulators/IEmulatorManager.h"
#include "../../Models/Bookmark.h"

#include <QPushButton>
#include <QTreeWidget>
#include <QWidget>
#include <vector>

namespace PointerFinder2::UI::Controls {

    // This panel acts as a 'Watchlist' for bookmarked pointers. It helps users monitor
    // multiple saved paths and inspect their resolved values in real-time.
    class BookmarksWidget : public QWidget {
        Q_OBJECT
    public:
        explicit BookmarksWidget(QWidget* parent = nullptr);
        ~BookmarksWidget() override;

        void setEmulatorManager(Emulators::IEmulatorManager* manager);
        void addBookmark(const DataModels::Bookmark& bookmark);
        void addBookmarks(const std::vector<DataModels::Bookmark>& bookmarks);
        void setBookmarks(const std::vector<DataModels::Bookmark>& bookmarks);

        // Defined inline to resolve the unresolved external linker symbol (LNK2019)
        std::vector<DataModels::Bookmark> getBookmarks() const { return m_bookmarksList; }

        void clearBookmarks();
        void refreshBookmarksUI();
        void updateThemeIcons();

    signals:
        void statusMessageRequested(const QString& message);

    private slots:
        void onRefreshBookmarks();
        void onDeleteSelectedBookmarks();
        void showBookmarkContextMenu(const QPoint& pos);
        void onBookmarkItemChanged(QTreeWidgetItem* item, int column);

    private:
        QTreeWidget* m_bookmarkTreeWidget = nullptr;
        QPushButton* m_refreshBookmarksBtn = nullptr;
        QPushButton* m_deleteBookmarkBtn = nullptr;

        std::vector<DataModels::Bookmark> m_bookmarksList;
        Emulators::IEmulatorManager* m_currentManager = nullptr;

        void setupLayout();
        QIcon getThemeIcon(const QString& resourcePath) const;
    };

}
