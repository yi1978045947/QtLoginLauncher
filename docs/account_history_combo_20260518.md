# Account History Combo

Date: 2026-05-18

Last updated: 2026-05-19

## Legacy Reference

Old DuiLib flow:

- `trunk/src/Login/CommonWindow2.cpp`
  - `_InitCombox` reads `CConfigManager::GetAllUserName` and `GetAllUserType`.
  - Duplicate accounts are removed case-insensitively during initialization.
  - `GetMaxAccountCount` controls the maximum local account count.
  - `_RecordLastUserInfo2` trims the successful account, removes an existing entry, inserts it at index 0, selects it, then truncates the UI list.
- `trunk/src/Login/PushMessageLoginDlg.cpp`
  - One-click login success calls `_RecordLastUserInfo(m_strCurrentAccount, m_pCombxAccount)`.
- `trunk/src/Login/AccountListContainerElementUI.h`
  - Dropdown item supports a remove button and sends `itemremove`.

Legacy storage format:

```xml
<UserInfo>
  <Accounts>
    <Account name="account" type="0" />
  </Accounts>
</UserInfo>
```

## Qt Rewrite

Implemented in:

- `src/config/config_manager.h/.cpp`
  - `maxAccountCount(fallback)`
  - `userAccounts(maxCount)`
  - `recordUserAccount(account, type, maxCount)`
  - `removeUserAccount(account)`
- `src/sdologin/account_history_combo.h/.cpp`
  - Keeps the old `combobox` skin instead of using native `QComboBox`.
  - Right-side arrow area opens a dropdown.
  - Dropdown background uses `combobox/bg.png`.
  - Rows use `combobox/itemselect.png` for full-row hover/selection, falling back to `combobox/select_item.png`.
  - Delete button uses `login/icon_delete_n.png`, `icon_delete_o.png`, `icon_delete_c.png`, falling back to the old `combobox/delete_*` images.
  - The dropdown is custom-painted by one popup widget instead of being rebuilt from child `QPushButton` rows. This avoids deleting or hiding button children while a delete-click event is still being dispatched.
  - Delete first closes the popup and updates the in-memory row list, then queues the outer `userinfo.xml` write to the next Qt event tick.
  - The popup is owned logically by `AccountHistoryCombo` and deleted in the combo destructor, even though it is parented to the top login window for overlay positioning.
- `tests/account_history_combo_tests`
  - Creates a real Qt Widgets `AccountHistoryCombo`.
  - Opens the skinned dropdown through the arrow button.
  - Sends a Qt mouse click to the delete-icon area.
  - Verifies the delete callback can refresh the account list without crashing or re-entering the popup event handler.
- `src/sdologin/account_history_model.h/.cpp`
  - `preferredAccountHistoryText` selects the newest local account as the default input text.
  - `removeAccountHistoryEntry` removes matching entries case-insensitively for both the popup and shared account refresh.
  - `accountHistoryPopupFontPixelSize` keeps dropdown text scaled with the legacy skin DPI factor; the base DuiLib dropdown font is treated as 16 px.
- `src/sdologin/login_window.cpp`
  - Password login uses `AccountHistoryCombo`.
  - Password login records the submitted account after successful server login.
  - Password login and one-click login share one account input value while the login window is open.
  - One-click login passes account history and records the submitted account after successful mobile confirmation.
  - Shared account edits are stored as guarded `QPointer<QLineEdit>` values and pruned on destroy, so delete/refresh paths do not touch stale line-edit pointers.
- `src/sdologin/push_message_login_panel.cpp`
  - One-click account field now uses the same account history combo.

Default maximum count is configured in:

```xml
<MaxAccountCount value="10" />
```

under `assets/default_config/config/config.xml`.

## Behavior

- Successful password login records the account typed in the password login account box.
- Successful one-click login records the account typed in the one-click account box.
- When the login window opens, both password login and one-click login account boxes are filled with the newest saved account.
- Editing or selecting an account in either password login or one-click login immediately syncs the other account box.
- Repeated account names are de-duplicated case-insensitively.
- Latest successful account is moved to the top.
- The saved list is truncated to `MaxAccountCount`; default is 10.
- Deleting an item from the dropdown writes back to `userinfo.xml`.
- The deleted row is removed from the next dropdown open without synchronously rebuilding child controls or writing config during the remove click handler.
- At 150% DPI, dropdown account text uses a 24 px font so it visually tracks the scaled login window instead of Qt's unscaled default font.
- Passwords are never stored by this feature.

## Verification

Primary test:

```bat
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target config_tests
.\qtlogin_rewrite\build_qt5_32\bin\Debug\config_tests.exe
```

Build check:

```bat
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin -- /m
```

2026-05-19 crash fix verification:

```bat
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin config_tests sdologin_tests account_history_combo_tests DXLoginDemo -- /m
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe
.\qtlogin_rewrite\build_qt5_32\bin\Debug\config_tests.exe
.\qtlogin_rewrite\build_qt5_32\bin\Debug\account_history_combo_tests.exe
```
