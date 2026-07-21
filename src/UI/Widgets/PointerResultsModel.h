#pragma once

#include <memory>
#include <QAbstractTableModel>
#include <QHash>
#include <QString>
#include <vector>

namespace PointerFinder2::Core {
    class ResultsManager;
}
namespace PointerFinder2::Emulators {
    class IEmulatorManager;
}

namespace PointerFinder2::UI::Controls {

    // This class acts as a data bridge, taking raw lists of found pointer paths
    // and converting them into a format that the table view can easily read and display.
    class PointerResultsModel : public QAbstractTableModel {
        Q_OBJECT
    public:
        explicit PointerResultsModel(Core::ResultsManager* resultsManager, QObject* parent = nullptr);
        ~PointerResultsModel() override;

        void setEmulatorManager(Emulators::IEmulatorManager* emuManager);

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    public slots:
        void refreshModel(bool isNewDataSet);

    private:
        Core::ResultsManager* m_resultsManager = nullptr;
        Emulators::IEmulatorManager* m_emuManager = nullptr;
        int m_maxOffsetsCount = 0;

        mutable QHash<int32_t, QString> m_offsetStringCache;
        mutable QHash<uint32_t, QString> m_addressStringCache;

        void updateMaxOffsetsCount();
    };

}
