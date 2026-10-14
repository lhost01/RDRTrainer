#pragma once

// RDR 修改器 (Red Dead Redemption PC trainer)
// Built on top of the RedHook script hook SDK (https://github.com/K3rhos/RedHookSDK, MIT).

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cmath>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <mutex>
#include <deque>
#include <functional>
#include <algorithm>
#include <type_traits>
#include <filesystem>

// ---- RedHook SDK -----------------------------------------------------------
#include "RedHook.h"
#include "NativesCaller.h"
#include "RhMath.h"
#include "Enums.h"
#include "Structs.h"
#include "Natives.h"

// ---- Project --------------------------------------------------------------
#include "Core/Utils.h"
#include "Core/Log.h"
#include "Core/Config.h"
#include "Core/Input.h"
#include "Core/Screen.h"
#include "Core/TextPool.h"
#include "Core/Font.h"
#include "Core/Game.h"
#include "UI/UiTheme.h"
#include "UI/Renderer.h"
#include "UI/Notifications.h"

#include "Features/GodMode.h"
#include "Features/InfiniteAmmo.h"
#include "Features/Aimbot.h"
#include "Features/Money.h"
#include "Features/Misc.h"

#include "UI/Esp.h"
#include "UI/Menu.h"

#include "Application.h"
