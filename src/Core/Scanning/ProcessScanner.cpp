#include "ProcessScanner.h"

#include "../Emulators/EmulatorProfileRegistry.h"

#include <windows.h>

#include <psapi.h>
#include <tlhelp32.h>

#include <QFileInfo>

#include <algorithm>

namespace {
struct ProcessWindowSearch {
    DWORD pid;
    HWND hwnd;
};

// Finds the main emulator window on your screen while ignoring background helper or overlay windows.
BOOL CALLBACK FindProcessWindowProc(HWND hwnd, LPARAM lParam) {
    auto* search = reinterpret_cast<ProcessWindowSearch*>(lParam);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == search->pid && IsWindowVisible(hwnd)) {
        int len = GetWindowTextLengthW(hwnd);
        if (len > 0) {
            wchar_t className[256];
            GetClassNameW(hwnd, className, 256);
            QString classStr = QString::fromWCharArray(className);

            wchar_t titleBuf[512];
            GetWindowTextW(hwnd, titleBuf, 512);
            QString titleStr = QString::fromWCharArray(titleBuf);

            if (classStr.contains("Overlay", Qt::CaseInsensitive) || titleStr.contains("Overlay", Qt::CaseInsensitive) ||
                classStr.contains("Helper", Qt::CaseInsensitive)) {
                return TRUE;
            }

            LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
            HWND owner = GetWindow(hwnd, GW_OWNER);

            bool hasCaption = (style & WS_CAPTION) == WS_CAPTION;
            bool hasNoOwner = (owner == nullptr);

            if (hasCaption && hasNoOwner) {
                search->hwnd = hwnd;
                return FALSE;
            }

            if (!search->hwnd) {
                search->hwnd = hwnd;
            }
        }
    }
    return TRUE;
}

QString getWindowTitleOfProcess(uint32_t processId) {
    ProcessWindowSearch search = {static_cast<DWORD>(processId), nullptr};
    EnumWindows(FindProcessWindowProc, reinterpret_cast<LPARAM>(&search));
    if (search.hwnd) {
        int len = GetWindowTextLengthW(search.hwnd);
        if (len > 0) {
            std::vector<wchar_t> buf(len + 1);
            GetWindowTextW(search.hwnd, buf.data(), len + 1);
            return QString::fromWCharArray(buf.data());
        }
    }
    return QString();
}

std::vector<QString> getLoadedModuleNames(DWORD pid) {
    std::vector<QString> modules;
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) {
        hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    }
    if (!hProc)
        return modules;

    HMODULE hMods[1024];
    DWORD cbNeeded = 0;
    if (EnumProcessModules(hProc, hMods, sizeof(hMods), &cbNeeded)) {
        int count = static_cast<int>(cbNeeded / sizeof(HMODULE));
        for (int i = 0; i < count; ++i) {
            wchar_t szModName[MAX_PATH];
            if (GetModuleFileNameExW(hProc, hMods[i], szModName, MAX_PATH)) {
                modules.push_back(QFileInfo(QString::fromWCharArray(szModName)).fileName().toLower());
            }
        }
    }
    CloseHandle(hProc);
    return modules;
}
} // namespace

namespace PointerFinder2::Core {

std::vector<DetectedEmulatorInstance> ProcessScanner::scanForRunningEmulators() {
    std::vector<DetectedEmulatorInstance> found;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return found;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    const auto& profiles = Emulators::EmulatorProfileRegistry::getProfiles();

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            QString exeName = QString::fromWCharArray(pe.szExeFile);
            for (const auto& prof : profiles) {
                for (const auto& pName : prof.processNames) {
                    if (exeName.compare(pName, Qt::CaseInsensitive) == 0 || exeName.compare(pName + ".exe", Qt::CaseInsensitive) == 0) {

                        DetectedEmulatorInstance inst;
                        inst.profile = prof;
                        inst.pid = pe.th32ProcessID;

                        HWND hwnd = GetTopWindow(nullptr);
                        while (hwnd) {
                            DWORD windowPid = 0;
                            GetWindowThreadProcessId(hwnd, &windowPid);
                            if (windowPid == inst.pid && IsWindowVisible(hwnd)) {
                                int len = GetWindowTextLengthW(hwnd);
                                if (len > 0) {
                                    std::vector<wchar_t> titleBuf(len + 1);
                                    GetWindowTextW(hwnd, titleBuf.data(), len + 1);
                                    inst.windowTitle = QString::fromWCharArray(titleBuf.data());
                                    break;
                                }
                            }
                            hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
                        }

                        inst.windowTitle = getWindowTitleOfProcess(inst.pid);

                        // Distinguish between GBA and NDS core instances under the same RALibretro process name
                        if (prof.processNames.contains("RALibretro", Qt::CaseInsensitive)) {
                            auto loadedModules = getLoadedModuleNames(inst.pid);
                            bool hasFlycast = false;
                            bool hasNds = false;
                            bool hasGba = false;

                            for (const auto& mod : loadedModules) {
                                if (mod.contains("flycast_libretro")) {
                                    hasFlycast = true;
                                } else if (mod.contains("melondsds_libretro") || mod.contains("melonds_libretro") ||
                                           mod.contains("desmume_libretro")) {
                                    hasNds = true;
                                } else if (mod.contains("mgba_libretro")) {
                                    hasGba = true;
                                }
                            }

                            // If loaded core DLLs are detected, use them as the primary source of truth
                            if (hasFlycast || hasNds || hasGba) {
                                if (hasFlycast && prof.target != PointerFinder2::DataModels::EmulatorTarget::RALibretroDC) {
                                    continue;
                                }
                                if (hasNds && prof.target != PointerFinder2::DataModels::EmulatorTarget::RALibretroNDS) {
                                    continue;
                                }
                                if (hasGba && prof.target != PointerFinder2::DataModels::EmulatorTarget::RALibretroGBA) {
                                    continue;
                                }
                            } else {
                                // Fallback to window caption heuristics if process module enumeration is unavailable
                                QString title = inst.windowTitle.toLower();
                                if (!title.isEmpty()) {
                                    bool isDc = title.contains("flycast") || title.contains("dreamcast");
                                    bool isGba = title.contains("mgba") || title.contains("gameboy");
                                    bool isNds = title.contains("desmume") || title.contains("melonds") || title.contains("nintendo ds");

                                    if (isDc && prof.target != PointerFinder2::DataModels::EmulatorTarget::RALibretroDC) {
                                        continue;
                                    }
                                    if (isGba && prof.target != PointerFinder2::DataModels::EmulatorTarget::RALibretroGBA) {
                                        continue;
                                    }
                                    if (isNds && prof.target != PointerFinder2::DataModels::EmulatorTarget::RALibretroNDS) {
                                        continue;
                                    }
                                }
                            }
                        }

                        found.push_back(inst);
                    }
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return found;
}

} // namespace PointerFinder2::Core
