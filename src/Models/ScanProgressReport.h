#pragma once

#include <QString>

#include <cstdint>

namespace PointerFinder2::DataModels {

// Relays search speed, completion status, match counts, and progress limits
// from core execution threads to the frontend interface.
struct ScanProgressReport {
    int64_t maxValue = 0;
    int64_t currentValue = 0;
    int foundCount = 0;
    int partialCount = 0;
    QString statusMessage;
};

} // namespace PointerFinder2::DataModels
