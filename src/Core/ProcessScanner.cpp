#include "ProcessScanner.h"

#include "../Emulators/EmulatorProfileRegistry.h"

#include <windows.h>
#include <tlhelp32.h>
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

                if (classStr.contains("Overlay", Qt::CaseInsensitive) ||
                    titleStr.contains("Overlay", Qt::CaseInsensitive) ||
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
        ProcessWindowSearch search = { static_cast<DWORD>(processId), nullptr };
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
}

namespace PointerFinder2::Core {

    std::vector<DetectedEmulatorInstance> ProcessScanner::scanForRunningEmulators() {
        std::vector<DetectedEmulatorInstance> found;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return found;

        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);

        const auto& profiles = Emulators::EmulatorProfileRegistry::getProfiles();

        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                QString exeName = QString::fromWCharArray(pe.szExeFile);
                for (const auto& prof : profiles) {
                    for (const auto& pName : prof.processNames) {
                        if (exeName.compare(pName, Qt::CaseInsensitive) == 0 ||
                            exeName.compare(pName + ".exe", Qt::CaseInsensitive) == 0) {

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
                                QString title = inst.windowTitle.toLower();
                                if (!title.isEmpty()) {
                                    bool isGba = title.contains("mgba") || title.contains("gameboy") || title.contains("advance");
                                    bool isNds = title.contains("desmume") || title.contains("melonds") || title.contains("nintendo ds");

                                    if (isGba) {
                                        if (prof.target == PointerFinder2::DataModels::EmulatorTarget::RALibretroNDS) {
                                            continue; // Skip NDS since GBA is explicitly active
                                        }
                                    }
                                    else if (isNds) {
                                        if (prof.target == PointerFinder2::DataModels::EmulatorTarget::RALibretroGBA) {
                                            continue; // Skip GBA since NDS is explicitly active
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

}