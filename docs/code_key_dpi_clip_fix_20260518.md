# Code Key Login DPI Clip Fix

Date: 2026-05-18

## Symptom

At 150% DPI, the QR/code-key login page showed a clipped QR area and clipped right-side text. The visible image looked smaller or cut by the frame.

## Root Cause

`CodeKeyLoginPanel` used `setFixedSize(442, 190)`.

The parent `LoginWindow` applies legacy DPI scaling after building the widget tree. Child widgets inside the QR panel were scaled, but the QR panel itself could remain constrained by the old fixed size. That caused the enlarged QR controls and background images to be clipped.

## Change

`CodeKeyLoginPanel` now uses `resize(442, 190)` as the base layout size instead of a fixed size.

This allows `LoginWindow` to scale the panel geometry together with the rest of the legacy skin layout.

## Demo Callback

`DXLoginDemo` already registered a login callback through `ShowLoginDialog`. The callback now also writes login success or error information to the DX demo window title, so successful callbacks are visible without relying only on debugger output.

## Verification

- `sdologin` Debug Win32 builds.
- `DXLoginDemo` Debug Win32 builds.
- `sdologin_tests` passes.
