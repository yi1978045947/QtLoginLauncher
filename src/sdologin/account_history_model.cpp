#include "account_history_model.h"

#include <algorithm>
#include <cmath>
#include <cwctype>

namespace qtlogin::sdologin {
namespace {

bool equalsIgnoreCase(std::wstring left, std::wstring right)
{
    std::transform(left.begin(), left.end(), left.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    std::transform(right.begin(), right.end(), right.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return left == right;
}

}

std::wstring preferredAccountHistoryText(
    const std::vector<qtlogin::config::ConfigManager::UserAccountEntry>& accounts)
{
    return accounts.empty() ? std::wstring() : accounts.front().name;
}

std::vector<qtlogin::config::ConfigManager::UserAccountEntry> removeAccountHistoryEntry(
    const std::vector<qtlogin::config::ConfigManager::UserAccountEntry>& accounts,
    const std::wstring& account)
{
    std::vector<qtlogin::config::ConfigManager::UserAccountEntry> filtered = accounts;
    filtered.erase(std::remove_if(filtered.begin(), filtered.end(), [&account](const qtlogin::config::ConfigManager::UserAccountEntry& item) {
        return equalsIgnoreCase(item.name, account);
    }), filtered.end());
    return filtered;
}

bool shouldRefreshAccountHistoryUiAfterRecord(bool loginWindowCompleting)
{
    return !loginWindowCompleting;
}

bool shouldQueueAccountHistoryRemoveFromPopupClick()
{
    return true;
}

int accountHistoryPopupFontPixelSize(double legacyDpiScaleFactor)
{
    const double scale = legacyDpiScaleFactor > 0.0 ? legacyDpiScaleFactor : 1.0;
    return static_cast<int>(std::lround(16.0 * scale));
}

}
