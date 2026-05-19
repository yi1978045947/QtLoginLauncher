#pragma once

#include <objbase.h>
#include <windows.h>

namespace qtlogin::sdk {

bool queryDirectXCompatModule(REFIID riid, void** intf);
void releaseDirectXCompatHooks();

}
