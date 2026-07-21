#include "SoundManager.h"

#include "../Models/GlobalSettings.h"

#include <QApplication>
#include <QByteArray> 
#include <QCoreApplication>
#include <QDir>
#include <QFile>

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace PointerFinder2::Core {

    // Resolves and plays audio files from disk or embedded application resources,
    // falling back to a native system beep if the underlying sound driver or file is unavailable.
    static void playNativeSound([[maybe_unused]] const QString& fileName, [[maybe_unused]] const QString& resourcePath, [[maybe_unused]] uint32_t fallbackBeepType) {
#ifdef Q_OS_WIN
        if (DataModels::GlobalSettings::useWindowsDefaultSound) {
            MessageBeep(fallbackBeepType);
            return;
        }

        QString externalPath = QDir(QCoreApplication::applicationDirPath()).filePath("Sounds/" + fileName);
        if (QFile::exists(externalPath)) {
            std::wstring wPath = externalPath.toStdWString();
            PlaySoundW(wPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
            return;
        }

        QFile file(resourcePath);
        if (file.open(QIODevice::ReadOnly)) {
            static QByteArray soundBuffer;
            soundBuffer = file.readAll();
            file.close();

            PlaySoundW(reinterpret_cast<LPCWSTR>(soundBuffer.constData()), NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
            return;
        }

        MessageBeep(fallbackBeepType);
#else
        QApplication::beep();
#endif
    }

    // Plays a positive sound alert indicating that the scan successfully found working pointers.
    void SoundManager::playSuccess() {
#ifdef Q_OS_WIN
        playNativeSound("sounds/success.wav", ":/sounds/success.wav", MB_OK);
#else
        playNativeSound("sounds/success.wav", ":/sounds/success.wav", 0);
#endif
    }

    // Plays an alert sound indicating that the search complete without finding any results.
    void SoundManager::playFail() {
#ifdef Q_OS_WIN
        playNativeSound("sounds/fail.wav", ":/sounds/fail.wav", MB_ICONHAND);
#else
        playNativeSound("sounds/fail.wav", ":/sounds/fail.wav", 0);
#endif
    }

    // Plays a neutral sound alert indicating that the scan was stopped or paused by the user.
    void SoundManager::playNotify() {
#ifdef Q_OS_WIN
        playNativeSound("sounds/notify.wav", ":/sounds/notify.wav", MB_ICONASTERISK);
#else
        playNativeSound("sounds/notify.wav", ":/sounds/notify.wav", 0);
#endif
    }

}