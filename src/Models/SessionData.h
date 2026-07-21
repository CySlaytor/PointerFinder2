#pragma once

#include "ArrayGroup.h"
#include "Bookmark.h"
#include "PointerPath.h"
#include "ScanParameters.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace PointerFinder2::DataModels {

    // Combines all bookmarks, array groups, discovered results, parameters, and layout choices
    // into a single, unified serializable workspace session.
    struct SessionData {
        QString emulatorTargetName;
        int32_t processId = -1;
        ScanParameters lastScanParameters;
        std::vector<PointerPath> results;
        std::vector<Bookmark> bookmarks;
        std::vector<ArrayGroup> arrayGroups;
        QString sortedColumnName = "";
        int sortDirection = 0; // Sort direction: 0 = None, 1 = Ascending, 2 = Descending

        QJsonObject toJson() const {
            QJsonObject root;
            root["EmulatorTargetName"] = emulatorTargetName;
            root["ProcessId"] = processId;
            root["SortedColumnName"] = sortedColumnName;
            root["SortDirection"] = sortDirection;

            QJsonObject params;
            params["TargetAddress"] = static_cast<double>(lastScanParameters.targetAddress);
            params["MaxLevel"] = lastScanParameters.maxLevel;
            params["MaxOffset"] = lastScanParameters.maxOffset;
            params["StaticBaseStart"] = static_cast<double>(lastScanParameters.staticBaseStart);
            params["StaticBaseEnd"] = static_cast<double>(lastScanParameters.staticBaseEnd);
            params["LimitCpuUsage"] = lastScanParameters.limitCpuUsage;
            params["StopOnFirstPathFound"] = lastScanParameters.stopOnFirstPathFound;
            params["FindAllPathLevels"] = lastScanParameters.findAllPathLevels;
            params["CandidatesPerLevel"] = lastScanParameters.candidatesPerLevel;
            params["MaxCandidates"] = lastScanParameters.maxCandidates;
            params["FinalAddressTarget"] = static_cast<double>(lastScanParameters.finalAddressTarget);
            params["FastScanMode"] = lastScanParameters.fastScanMode;
            params["PrintPartialPaths"] = lastScanParameters.printPartialPaths;
            params["DynamicStaticDetection"] = lastScanParameters.dynamicStaticDetection;
            params["DetectArrays"] = lastScanParameters.detectArrays;
            params["ArraySearchRange"] = static_cast<double>(lastScanParameters.arraySearchRange);

            if (lastScanParameters.lastOffsetHint.has_value()) {
                params["LastOffsetHint"] = lastScanParameters.lastOffsetHint.value();
            }
            root["LastScanParameters"] = params;

            QJsonArray resultsArray;
            for (const auto& path : results) {
                QJsonObject pathObj;
                pathObj["BaseAddress"] = static_cast<double>(path.baseAddress);
                pathObj["FinalAddress"] = static_cast<double>(path.finalAddress);
                pathObj["IsPartial"] = path.isPartial;

                QJsonArray offsetsArr;
                for (int32_t offset : path.offsets) {
                    offsetsArr.append(offset);
                }
                pathObj["Offsets"] = offsetsArr;

                QJsonArray brokenArr;
                for (const auto& [stateIdx, brokeAddr] : path.brokenStateAddresses) {
                    QJsonObject brokenItem;
                    brokenItem["State"] = stateIdx;
                    brokenItem["Address"] = static_cast<double>(brokeAddr);
                    brokenArr.append(brokenItem);
                }
                pathObj["BrokenStateAddresses"] = brokenArr;

                resultsArray.append(pathObj);
            }
            root["Results"] = resultsArray;

            QJsonArray bookmarksArray;
            for (const auto& bm : bookmarks) {
                QJsonObject bmObj;
                bmObj["Name"] = bm.name;
                bmObj["BaseAddress"] = static_cast<double>(bm.path.baseAddress);
                bmObj["FinalAddress"] = static_cast<double>(bm.path.finalAddress);
                bmObj["IsPartial"] = bm.path.isPartial;

                QJsonArray offsetsArr;
                for (int32_t offset : bm.path.offsets) {
                    offsetsArr.append(offset);
                }
                bmObj["Offsets"] = offsetsArr;
                bookmarksArray.append(bmObj);
            }
            root["Bookmarks"] = bookmarksArray;

            QJsonArray arrayGroupsArr;
            for (const auto& group : arrayGroups) {
                QJsonObject gObj;
                gObj["BaseAddress"] = static_cast<double>(group.baseAddress);
                gObj["ElementCount"] = static_cast<double>(group.elementCount);

                QJsonArray elementsArr;
                for (uint32_t elem : group.elementAddresses) {
                    elementsArr.append(static_cast<double>(elem));
                }
                gObj["Elements"] = elementsArr;

                QJsonArray targetsArr;
                for (uint32_t target : group.targets) {
                    targetsArr.append(static_cast<double>(target));
                }
                gObj["Targets"] = targetsArr;
                arrayGroupsArr.append(gObj);
            }
            root["ArrayGroups"] = arrayGroupsArr;

            return root;
        }

        static SessionData fromJson(const QJsonObject& root) {
            SessionData data;
            data.emulatorTargetName = root["EmulatorTargetName"].toString();
            data.processId = root["ProcessId"].toInt();
            data.sortedColumnName = root["SortedColumnName"].toString();
            data.sortDirection = root["SortDirection"].toInt();

            if (root.contains("LastScanParameters")) {
                QJsonObject params = root["LastScanParameters"].toObject();
                data.lastScanParameters.targetAddress = static_cast<uint32_t>(params["TargetAddress"].toDouble());
                data.lastScanParameters.maxLevel = params["MaxLevel"].toInt();
                data.lastScanParameters.maxOffset = params["MaxOffset"].toInt();
                data.lastScanParameters.staticBaseStart = static_cast<uint32_t>(params["StaticBaseStart"].toDouble());
                data.lastScanParameters.staticBaseEnd = static_cast<uint32_t>(params["StaticBaseEnd"].toDouble());
                data.lastScanParameters.limitCpuUsage = params["LimitCpuUsage"].toBool();
                data.lastScanParameters.stopOnFirstPathFound = params["StopOnFirstPathFound"].toBool();
                data.lastScanParameters.findAllPathLevels = params["FindAllPathLevels"].toBool();
                data.lastScanParameters.candidatesPerLevel = params["CandidatesPerLevel"].toInt();
                data.lastScanParameters.maxCandidates = params["MaxCandidates"].toInt();
                data.lastScanParameters.finalAddressTarget = static_cast<uint32_t>(params["FinalAddressTarget"].toDouble());
                data.lastScanParameters.fastScanMode = params["FastScanMode"].toBool();
                data.lastScanParameters.printPartialPaths = params["PrintPartialPaths"].toBool();

                if (params.contains("DynamicStaticDetection")) {
                    data.lastScanParameters.dynamicStaticDetection = params["DynamicStaticDetection"].toBool();
                }

                if (params.contains("DetectArrays")) {
                    data.lastScanParameters.detectArrays = params["DetectArrays"].toBool();
                }
                if (params.contains("ArraySearchRange")) {
                    data.lastScanParameters.arraySearchRange = static_cast<uint32_t>(params["ArraySearchRange"].toDouble());
                }

                if (params.contains("LastOffsetHint")) {
                    data.lastScanParameters.lastOffsetHint = params["LastOffsetHint"].toInt();
                }
            }

            if (root.contains("Results")) {
                QJsonArray resultsArray = root["Results"].toArray();
                data.results.reserve(resultsArray.size());
                for (const auto& val : resultsArray) {
                    QJsonObject pathObj = val.toObject();
                    PointerPath path;
                    path.baseAddress = static_cast<uint32_t>(pathObj["BaseAddress"].toDouble());
                    path.finalAddress = static_cast<uint32_t>(pathObj["FinalAddress"].toDouble());
                    path.isPartial = pathObj["IsPartial"].toBool();

                    QJsonArray offsetsArr = pathObj["Offsets"].toArray();
                    path.offsets.reserve(offsetsArr.size());
                    for (const auto& offsetVal : offsetsArr) {
                        path.offsets.push_back(offsetVal.toInt());
                    }

                    if (pathObj.contains("BrokenStateAddresses")) {
                        QJsonArray brokenArr = pathObj["BrokenStateAddresses"].toArray();
                        for (const auto& brokenVal : brokenArr) {
                            QJsonObject brokenItem = brokenVal.toObject();
                            int stateIdx = brokenItem["State"].toInt();
                            uint32_t brokeAddr = static_cast<uint32_t>(brokenItem["Address"].toDouble());
                            path.brokenStateAddresses.push_back({ stateIdx, brokeAddr });
                        }
                    }

                    data.results.push_back(path);
                }
            }

            if (root.contains("Bookmarks")) {
                QJsonArray bookmarksArray = root["Bookmarks"].toArray();
                data.bookmarks.reserve(bookmarksArray.size());
                for (const auto& val : bookmarksArray) {
                    QJsonObject bmObj = val.toObject();
                    Bookmark bm;
                    bm.name = bmObj["Name"].toString();
                    bm.path.baseAddress = static_cast<uint32_t>(bmObj["BaseAddress"].toDouble());
                    bm.path.finalAddress = static_cast<uint32_t>(bmObj["FinalAddress"].toDouble());
                    bm.path.isPartial = bmObj["IsPartial"].toBool();

                    QJsonArray offsetsArr = bmObj["Offsets"].toArray();
                    bm.path.offsets.reserve(offsetsArr.size());
                    for (const auto& offsetVal : offsetsArr) {
                        bm.path.offsets.push_back(offsetVal.toInt());
                    }
                    data.bookmarks.push_back(bm);
                }
            }

            if (root.contains("ArrayGroups")) {
                QJsonArray arrayGroupsArr = root["ArrayGroups"].toArray();
                data.arrayGroups.reserve(arrayGroupsArr.size());
                for (const auto& val : arrayGroupsArr) {
                    QJsonObject gObj = val.toObject();
                    ArrayGroup group;
                    group.baseAddress = static_cast<uint32_t>(gObj["BaseAddress"].toDouble());
                    group.elementCount = static_cast<uint32_t>(gObj["ElementCount"].toDouble());

                    QJsonArray elementsArr = gObj["Elements"].toArray();
                    group.elementAddresses.reserve(elementsArr.size());
                    for (const auto& elemVal : elementsArr) {
                        group.elementAddresses.push_back(static_cast<uint32_t>(elemVal.toDouble()));
                    }

                    QJsonArray targetsArr = gObj["Targets"].toArray();
                    group.targets.reserve(targetsArr.size());
                    for (const auto& targetVal : targetsArr) {
                        group.targets.push_back(static_cast<uint32_t>(targetVal.toDouble()));
                    }
                    data.arrayGroups.push_back(group);
                }
            }

            return data;
        }
    };

}
