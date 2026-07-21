#pragma once

#include <optional>
#include <QString>

namespace PointerFinder2::DataModels {

    // Holds configurable limits, boundaries, and performance options for an individual scan session.
    struct AppSettings {
        QString lastTargetAddress = "0";
        int maxOffset = 4095;
        int maxLevel = 7;
        QString staticAddressStart = "";
        QString staticAddressEnd = "";
        bool stopOnFirstPathFound = false;
        bool findAllPathLevels = false;
        int candidatesPerLevel = 10;
        int maxCandidates = 10000000;
        bool fastScanMode = true;
        bool printPartialPaths = false;
        std::optional<int> lastOffsetHint = std::nullopt;
        bool useLastOffsetHint = false;
        bool dynamicStaticDetection = false;
    };

}
