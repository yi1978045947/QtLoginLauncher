# DX Embedded Alpha Cutoff

Date: 2026-05-18

## Problem

When `sdologin` is embedded into the DX demo/caller window, Qt is running as a child window over a DirectX-rendered host. Semi-transparent pixels in the old Duilib skin can blend against Qt/Win32 backing pixels instead of the DX scene, which creates dark or colored halos around the wooden board.

The old Duilib path did not expose this in the same way because its drawing path and host integration were different.

## Current Strategy

Only in embedded-window mode:

- if the loaded background image has an alpha channel, convert it before painting;
- pixels with alpha >= `250` become fully opaque;
- pixels with alpha < `250` become fully transparent;
- the window mask is still built from the converted image;
- embedded painting disables smooth pixmap interpolation to avoid generating new semi-transparent edge pixels.

Top-level `sdologin.exe` windows still keep the normal alpha/smooth rendering path.

## Files

- `src/sdologin/window_embed_style.h`
- `src/sdologin/window_embed_style.cpp`
- `src/sdologin/login_window.cpp`
- `tests/sdologin_tests/main.cpp`

## Verification

`sdologin_tests` checks that embedded alpha removal is enabled only for embedded windows with alpha images, and that the cutoff remains `250`.
