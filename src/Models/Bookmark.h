#pragma once

#include "PointerPath.h"

#include <QString>

namespace PointerFinder2::DataModels {

    // Associates a descriptive label with a specific pointer path.
    // Helps users monitor and persist important memory locations across application sessions.
    struct Bookmark {
        QString name;
        PointerPath path;

        bool operator==(const Bookmark& other) const {
            return name == other.name && path == other.path;
        }
    };

}
