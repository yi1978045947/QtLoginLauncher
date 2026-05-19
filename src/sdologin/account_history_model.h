#pragma once

#include <string>
#include <vector>

#include "config_manager.h"

namespace qtlogin::sdologin {

std::wstring preferredAccountHistoryText(
    const std::vector<qtlogin::config::ConfigManager::UserAccountEntry>& accounts);

std::vector<qtlogin::config::ConfigManager::UserAccountEntry> removeAccountHistoryEntry(
    const std::vector<qtlogin::config::ConfigManager::UserAccountEntry>& accounts,
    const std::wstring& account);

bool shouldRefreshAccountHistoryUiAfterRecord(bool loginWindowCompleting);

bool shouldQueueAccountHistoryRemoveFromPopupClick();

int accountHistoryPopupFontPixelSize(double legacyDpiScaleFactor);

}
