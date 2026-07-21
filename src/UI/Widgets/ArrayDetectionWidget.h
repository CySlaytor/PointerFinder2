#pragma once

#include "../../Emulators/IEmulatorManager.h"
#include "../../Models/ArrayGroup.h"
#include "../../Models/PointerPath.h"

#include <QTreeWidget>
#include <QWidget>
#include <vector>

namespace PointerFinder2::UI::Controls {

    struct PathArrayMatch {
        DataModels::PointerPath path;
        int stepIndex = -1;
        DataModels::ArrayGroup group;
    };

    struct ArrayDetectionResult {
        std::vector<DataModels::ArrayGroup> groups;
        std::vector<PathArrayMatch> matches;
    };

    // This panel displays lists of pointers positioned side-by-side in memory.
    // It is used to identify organized sets of variables (like arrays) in the game's RAM.
    class ArrayDetectionWidget : public QWidget {
        Q_OBJECT
    public:
        explicit ArrayDetectionWidget(QWidget* parent = nullptr);
        ~ArrayDetectionWidget() override;

        void setEmulatorManager(Emulators::IEmulatorManager* manager);
        void setResults(const std::vector<DataModels::ArrayGroup>& groups, const std::vector<PathArrayMatch>& matches);
        void clear();
        void refreshUI();
        void updateThemeIcons();

        std::vector<DataModels::ArrayGroup> getArrayGroups() const { return m_arrayGroupsList; }
        std::vector<PathArrayMatch> getArrayMatches() const { return m_arrayMatches; }

    signals:
        void statusMessageRequested(const QString& message);
        void bookmarkRequested(const DataModels::PointerPath& path);

    private slots:
        void showArrayContextMenu(const QPoint& pos);

    private:
        QTreeWidget* m_arrayTreeWidget = nullptr;
        std::vector<DataModels::ArrayGroup> m_arrayGroupsList;
        std::vector<PathArrayMatch> m_arrayMatches;
        Emulators::IEmulatorManager* m_currentManager = nullptr;

        void setupLayout();
        QIcon getThemeIcon(const QString& resourcePath) const;
    };

}
