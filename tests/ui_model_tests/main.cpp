#include <cassert>
#include <iostream>

#include "legacy_dialog_registry.h"

using qtlogin::ui_model::DialogKind;
using qtlogin::ui_model::findLegacyDialog;
using qtlogin::ui_model::legacyDialogs;

int main()
{
    const auto& dialogs = legacyDialogs();
    assert(dialogs.size() >= 40);

    assert(findLegacyDialog(DialogKind::MessageBox) != nullptr);
    assert(findLegacyDialog(DialogKind::Captcha) != nullptr);
    assert(findLegacyDialog(DialogKind::RealNameInfo) != nullptr);
    assert(findLegacyDialog(DialogKind::GuardianInfo) != nullptr);
    assert(findLegacyDialog(DialogKind::Protocol) != nullptr);
    assert(findLegacyDialog(DialogKind::Payment) != nullptr);
    assert(findLegacyDialog(DialogKind::PushMessageLogin) != nullptr);
    assert(findLegacyDialog(DialogKind::QuickAccountLogin) != nullptr);
    assert(findLegacyDialog(DialogKind::GameUpdate) != nullptr);

    std::cout << "ui_model_tests passed\n";
    return 0;
}
