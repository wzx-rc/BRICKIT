// BrickIt - crash log (Wzx 10/11/2026: « un système de crashlog pour savoir quand il crashe ce qu'il s'est passé »).
// A crash (an exception, abort, a pure call, terminate) writes <data>\crashlogs\crash_<date>.txt (the exception, the
// stack with the function names from BrickIt.pdb, the last actions of the app, its state) and a .dmp (minidump, for
// Visual Studio). A freeze (the frame loop still for 15 s) writes hang_<date>.txt with the stack of the main thread.
// Ctrl+Maj+F12 writes report_<date>.txt (a glitch that is not a crash). At the next start, a message says one exists.
// iPad (11/11/2026): the same reports from the signals, in Documents/Rapports (Fichiers); at the next start BrickIt
// offers to send it (share sheet); « Envoyer un rapport » in the scene menu sends the state + the last crash.
#pragma once
#include <functional>
#include <string>

namespace brickit {

void crashInit(const std::wstring& dir, const std::wstring& version);   // the handlers, the watchdog
void crumb(const char* fmt, ...);            // one line of the journal (the last 400 kept, with the time)
void crashHeartbeat();                        // each frame (the watchdog)
void crashSetActive(bool active);             // iPad: false in the background (the frame loop stops: not a freeze)
void crashSetState(std::function<std::string()> f);   // what the app adds to a report (its state)
std::wstring crashWriteReport(const char* kind);      // a report now (Ctrl+Maj+F12; iPad: the last crash joined): its path
std::wstring crashLastUnseen();               // a crash or freeze report not yet announced (empty: none)

}  // namespace brickit
