#include "RaStorageService.h"

#include <windows.h>
#include <psapi.h>
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

namespace PointerFinder2::Core {

    QString RaStorageService::unescapeUserTxt(const QString& in) {
        QString out;
        out.reserve(in.length());
        for (int i = 0; i < in.length(); ++i) {
            if (in[i] == '\\' && i + 1 < in.length()) {
                QChar next = in[i + 1];
                if (next == 'r') { out.append('\r'); i++; }
                else if (next == 'n') { out.append('\n'); i++; }
                else if (next == 't') { out.append('\t'); i++; }
                else if (next == '"') { out.append('"'); i++; }
                else if (next == '\\') { out.append('\\'); i++; }
                else { out.append(next); i++; }
            }
            else {
                out.append(in[i]);
            }
        }
        return out;
    }

    QString RaStorageService::escapeUserTxt(const QString& in) {
        QString out;
        out.reserve(in.length() * 2);
        for (int i = 0; i < in.length(); ++i) {
            QChar c = in[i];
            if (c == '\r') { out.append("\\r"); }
            else if (c == '\n') { out.append("\\n"); }
            else if (c == '\t') { out.append("\\t"); }
            else if (c == '"') { out.append("\\\""); }
            else if (c == '\\') { out.append("\\\\"); }
            else { out.append(c); }
        }
        return out;
    }

    QString RaStorageService::extractTitleFromNote(const QString& noteText) {
        QString trimmed = noteText.trimmed();
        if (trimmed.isEmpty()) return "Untitled Note";
        QString firstLine = trimmed.split(QRegularExpression("[\r\n]+")).first().trimmed();
        if (firstLine.length() > 60) firstLine = firstLine.left(57) + "...";
        return firstLine;
    }

    QString RaStorageService::resolveRaDataDirectory(PlatformHandle processHandle) {
        if (!processHandle) return QString();

        HANDLE hProcess = static_cast<HANDLE>(processHandle);

        wchar_t exePathBuf[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, exePathBuf, &size) == 0) {
            if (GetModuleFileNameExW(hProcess, NULL, exePathBuf, MAX_PATH) == 0) {
                return QString();
            }
        }

        QFileInfo exeInfo(QString::fromWCharArray(exePathBuf));
        QDir exeDir = exeInfo.dir();

        // 1. Direct portable directory: <exeDir>/RACache/Data
        QDir candidate1(exeDir.filePath("RACache/Data"));
        if (candidate1.exists()) {
            return candidate1.absolutePath();
        }

        // 2. Parent directory: <exeDir>/../RACache/Data
        QDir candidate2(exeDir.filePath("../RACache/Data"));
        if (candidate2.exists()) {
            return candidate2.absolutePath();
        }

        // 3. Inspect module list for RA_Integration.dll
        HMODULE hMods[1024];
        DWORD cbNeeded = 0;
        if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
            int count = static_cast<int>(cbNeeded / sizeof(HMODULE));
            for (int i = 0; i < count; ++i) {
                wchar_t modNameBuf[MAX_PATH];
                if (GetModuleFileNameExW(hProcess, hMods[i], modNameBuf, MAX_PATH)) {
                    QString modPath = QString::fromWCharArray(modNameBuf);
                    if (modPath.contains("RA_Integration", Qt::CaseInsensitive)) {
                        QDir raDir = QFileInfo(modPath).dir();
                        QDir raCandidate(raDir.filePath("RACache/Data"));
                        if (raCandidate.exists()) return raCandidate.absolutePath();
                    }
                }
            }
        }

        // 4. Global AppData fallback: %APPDATA%/RetroAchievements/RACache/Data
        QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir globalCandidate(QDir(appData).filePath("../RetroAchievements/RACache/Data"));
        if (globalCandidate.exists()) {
            return globalCandidate.absolutePath();
        }

        return QString();
    }

    bool RaStorageService::loadActiveGameData(PlatformHandle processHandle, RaGameData& outData) {
        QString dataDirStr = resolveRaDataDirectory(processHandle);
        if (dataDirStr.isEmpty()) return false;

        QDir dataDir(dataDirStr);
        QFileInfoList notesFiles = dataDir.entryInfoList({ "*-Notes.json" }, QDir::Files, QDir::Time);
        if (notesFiles.isEmpty()) return false;

        QFileInfo bestNotesFile = notesFiles.first();
        QString baseName = bestNotesFile.fileName();
        QString gameId = baseName.left(baseName.indexOf('-'));

        QString userTxtFileName = gameId + "-User.txt";
        QFileInfo userTxtFile(dataDir.filePath(userTxtFileName));

        outData = RaGameData();
        outData.gameId = gameId;
        outData.notesJsonPath = bestNotesFile.absoluteFilePath();
        outData.userTxtPath = userTxtFile.absoluteFilePath();

        // 1. Read Notes.json (Read-Only Server baseline)
        QFile jsonFile(outData.notesJsonPath);
        if (jsonFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
            if (doc.isArray()) {
                QJsonArray arr = doc.array();
                for (const auto& itemVal : arr) {
                    if (itemVal.isObject()) {
                        QJsonObject obj = itemVal.toObject();
                        RaCodeNote note;
                        note.author = obj["User"].toString();
                        note.addressHex = obj["Address"].toString();
                        bool ok = false;
                        note.address = note.addressHex.toUInt(&ok, 16);
                        note.noteText = obj["Note"].toString();
                        note.title = extractTitleFromNote(note.noteText);
                        note.isModified = false;
                        note.isLocalOnly = false;
                        outData.notes.push_back(note);
                    }
                }
            }
            jsonFile.close();
        }

        // 2. Read User.txt (Local overlay)
        QFile txtFile(outData.userTxtPath);
        if (txtFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&txtFile);
            if (!stream.atEnd()) {
                outData.raVersionLine = stream.readLine(); // Line 1: version string (verbatim, uninterpreted)
            }
            if (!stream.atEnd()) {
                outData.gameTitle = stream.readLine();     // Line 2: game title
            }

            while (!stream.atEnd()) {
                QString line = stream.readLine();
                if (line.startsWith("N0:0x", Qt::CaseInsensitive)) {
                    int firstQuote = line.indexOf('"');
                    int lastQuote = line.lastIndexOf('"');
                    if (firstQuote != -1 && lastQuote > firstQuote) {
                        QString addrPart = line.mid(3, firstQuote - 3).trimmed();
                        if (addrPart.endsWith(':')) addrPart.chop(1);
                        bool ok = false;
                        uint32_t addr = addrPart.toUInt(&ok, 16);

                        QString rawNoteContent = line.mid(firstQuote + 1, lastQuote - firstQuote - 1);
                        QString unescapedNote = unescapeUserTxt(rawNoteContent);

                        auto it = std::find_if(outData.notes.begin(), outData.notes.end(), [&](const RaCodeNote& n) {
                            return n.address == addr;
                        });

                        if (it != outData.notes.end()) {
                            it->noteText = unescapedNote;
                            it->title = extractTitleFromNote(unescapedNote);
                            it->isModified = true;
                        }
                        else {
                            RaCodeNote newNote;
                            newNote.address = addr;
                            newNote.addressHex = QString("0x%1").arg(addr, 8, 16, QChar('0'));
                            newNote.noteText = unescapedNote;
                            newNote.title = extractTitleFromNote(unescapedNote);
                            newNote.isModified = false;
                            newNote.isLocalOnly = true;
                            newNote.author = "Local";
                            outData.notes.push_back(newNote);
                        }
                    }
                }
            }
            txtFile.close();
        }

        outData.isValid = true;
        return true;
    }

    bool RaStorageService::saveActiveNoteToUserTxt(RaGameData& gameData, uint32_t activeAddr, const QString& currentNote) {
        if (!gameData.isValid || gameData.userTxtPath.isEmpty() || activeAddr == 0) return false;

        // 1. Update in-memory model
        auto it = std::find_if(gameData.notes.begin(), gameData.notes.end(), [&](const RaCodeNote& n) {
            return n.address == activeAddr;
        });

        if (it != gameData.notes.end()) {
            it->noteText = currentNote;
            it->title = extractTitleFromNote(currentNote);
            it->isModified = !it->isLocalOnly;
        }
        else {
            RaCodeNote newNote;
            newNote.address = activeAddr;
            newNote.addressHex = QString("0x%1").arg(activeAddr, 8, 16, QChar('0'));
            newNote.noteText = currentNote;
            newNote.title = extractTitleFromNote(currentNote);
            newNote.isLocalOnly = true;
            newNote.isModified = false;
            newNote.author = "Local";
            gameData.notes.push_back(newNote);
        }

        // 2. Read existing User.txt to refresh passthrough lines
        QStringList existingLines;
        QFile readFile(gameData.userTxtPath);
        if (readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&readFile);
            while (!in.atEnd()) {
                existingLines.append(in.readLine());
            }
            readFile.close();
        }

        QString vLine = !gameData.raVersionLine.isEmpty() ? gameData.raVersionLine : "1.4.2.0";
        QString gTitle = !gameData.gameTitle.isEmpty() ? gameData.gameTitle : "Unknown Game";

        if (!existingLines.isEmpty()) {
            vLine = existingLines[0];
            if (existingLines.size() > 1) gTitle = existingLines[1];
        }

        // Reconstruct User.txt
        QStringList newFileLines;
        newFileLines.append(vLine);
        newFileLines.append(gTitle);

        QString activeAddrStr = QString("0x%1").arg(activeAddr, 8, 16, QChar('0')).toLower();
        bool wroteActiveNote = false;

        for (int i = 2; i < existingLines.size(); ++i) {
            const QString& line = existingLines[i];
            if (line.startsWith("N0:0x", Qt::CaseInsensitive)) {
                int firstQuote = line.indexOf('"');
                QString addrPart = line.mid(3, firstQuote - 3).trimmed();
                if (addrPart.endsWith(':')) addrPart.chop(1);
                bool parseOk = false;
                uint32_t lineAddr = addrPart.toUInt(&parseOk, 16);

                if (parseOk && lineAddr == activeAddr) {
                    QString escaped = escapeUserTxt(currentNote);
                    newFileLines.append(QString("N0:%1:\"%2\"").arg(activeAddrStr, escaped));
                    wroteActiveNote = true;
                    continue;
                }
            }
            newFileLines.append(line);
        }

        if (!wroteActiveNote) {
            QString escaped = escapeUserTxt(currentNote);
            newFileLines.append(QString("N0:%1:\"%2\"").arg(activeAddrStr, escaped));
        }

        // Write out atomically via temp file
        QString tempPath = gameData.userTxtPath + ".tmp";
        QFile writeFile(tempPath);
        if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream out(&writeFile);
        for (const QString& l : newFileLines) {
            out << l << "\n";
        }
        writeFile.close();

        QFile::remove(gameData.userTxtPath);
        return QFile::rename(tempPath, gameData.userTxtPath);
    }

    bool RaStorageService::revertServerNote(RaGameData& gameData, uint32_t activeAddr) {
        if (!gameData.isValid || gameData.userTxtPath.isEmpty() || activeAddr == 0) return false;

        QFile readFile(gameData.userTxtPath);
        if (!readFile.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

        QStringList lines;
        QTextStream in(&readFile);
        while (!in.atEnd()) {
            lines.append(in.readLine());
        }
        readFile.close();

        QStringList remainingLines;
        if (!lines.isEmpty()) remainingLines.append(lines[0]);
        if (lines.size() > 1) remainingLines.append(lines[1]);

        for (int i = 2; i < lines.size(); ++i) {
            const QString& line = lines[i];
            if (line.startsWith("N0:0x", Qt::CaseInsensitive)) {
                int firstQuote = line.indexOf('"');
                QString addrPart = line.mid(3, firstQuote - 3).trimmed();
                if (addrPart.endsWith(':')) addrPart.chop(1);
                bool parseOk = false;
                uint32_t lineAddr = addrPart.toUInt(&parseOk, 16);
                if (parseOk && lineAddr == activeAddr) {
                    continue; // Skip override line
                }
            }
            remainingLines.append(line);
        }

        QFile writeFile(gameData.userTxtPath);
        if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

        QTextStream out(&writeFile);
        for (const QString& l : remainingLines) {
            out << l << "\n";
        }
        writeFile.close();
        return true;
    }

}
