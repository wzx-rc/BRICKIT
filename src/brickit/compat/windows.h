// Non-Windows builds only (iPad, Linux test build): <windows.h> of the shared code and of the engine helpers it
// uses resolves here, to the few portable Win32 calls of WinCompat.h. The PC build never sees this folder.
#pragma once
#include "../WinCompat.h"
