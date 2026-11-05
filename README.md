# Winyl 3.3.1 — fixed x86 build

This repository is a community fork of **Winyl 3.3.1** (the last public release of the
Winyl audio player by Alex Kras), with a number of long-standing bugs fixed. The
application is still a **32-bit (x86) Unicode** build and keeps the original Windows XP+
compatibility (`v141_xp` toolset).

Original project: <http://winyl-player.com>  
License: **GNU GPL v3** (see `LICENSE`).

## Fixed in this fork

### 1. Buttons (including Repeat) ignoring clicks

The original code could get a button permanently stuck in the "pressed" state if the
matching `WM_LBUTTONUP` was lost (mouse capture taken by another window, minimization,
a modal dialog, drag & drop, etc.). The stuck element then consumed the next click, which
is why **Repeat, Shuffle, Mute, Next/Prev and other controls sometimes did nothing**.

* `SkinDraw::ResetPress()` releases all stale pressed elements before a new press.
* `SkinDraw::MouseUp()` now performs an action only for the element that was actually
  pressed by the last `MouseDown`; all other stale elements are released silently.
* A **double click** on a button is delivered by Windows as `WM_LBUTTONDBLCLK` instead of
  the second `WM_LBUTTONDOWN`. The old handler dropped it, so every second quick click on
  a button was lost. Both `WinylWnd::OnLButtonDblClk` and `SkinAlpha::OnLButtonDblClk`
  now process it as a normal button press.

### 2. Playlist / Library background

New optional, backward-compatible attributes for the `<Background>` element of the
playlist (`Playlist.xml`) and library (`Library.xml`) skins:

```xml
<Background File="Background\image.png" Stretch="1" />  <!-- stretch to viewport -->
<Background File="Background\image.png" Cover="1" />    <!-- fill, keep aspect ratio -->
<Background File="Background\image.png" Contain="1" />  <!-- fit whole image -->
```

Priority: `Contain` → `Cover` → `Stretch` → legacy `Crop`. When one of the new modes is
used, scrolling repaints the viewport instead of blitting already painted pixels, so the
wallpaper no longer drifts while the list scrolls.

## Download

A ready-to-use 32-bit executable is provided in [`dist/Winyl_x86.exe`](dist/Winyl_x86.exe)
and as a [release asset](https://github.com/qexdy3/winyl-3.3.1-fixed/releases/tag/v3.3.1-fixed).

* File version: 3.3.1.0
* Size: 2 417 152 bytes
* SHA-256: `044FDECCA7F1A302BF1D1B799D76FE2A9EFF913CE86F46DA9197ABBA29C5BB60`

The player needs the BASS audio libraries (`bass.dll`, `bassmix.dll`, `bassasio.dll`,
`basswasapi.dll`, `bass_fx.dll`) next to `Winyl.exe`. They are not included in this
repository; use the DLLs from your existing Winyl installation (or the official
Winyl/BASS distributions).

## Build

Requirements: Visual Studio 2017 with C++ Windows XP support, platform toolset
`v141_xp`, 32-bit (`Win32`). See [`BUILD.md`](BUILD.md) for the third-party libraries
(SQLite, TagLib, zlib, pugixml, BASS headers).

```bat
msbuild Winyl.sln /p:Configuration=Release /p:Platform=x86
```

Output: `Release\Winyl.exe`.

## Patches

The complete diff against upstream Winyl 3.3.1 is in
[`Winyl_3.3.1_fixes.patch`](Winyl_3.3.1_fixes.patch) (8 source files).

---

# Winyl 3.3.1 — исправленная x86-сборка

Этот репозиторий — форк **Winyl 3.3.1** с исправлением ряда давних ошибок. Программа
по-прежнему собирается как **32-битное (x86)** приложение с совместимостью с Windows XP+.

## Что исправлено

1. **Кнопки (включая Repeat) иногда не срабатывали.** Потерянный `WM_LBUTTONUP`
   оставлял кнопку «зажатой», и следующий клик поглощался. Кроме того, второй быстрый
   клик приходил как `WM_LBUTTONDBLCLK` и полностью игнорировался. Теперь залипшие
   элементы сбрасываются, действие выполняет только реально нажатая кнопка, а двойной
   клик обрабатывается как новое нажатие.
2. **Фон плейлиста и библиотеки.** Добавлены необязательные атрибуты `Stretch`, `Cover`
   и `Contain` у `<Background>`; при их использовании фон больше не «уезжает» при
   прокрутке. Старые скины работают как раньше.

Готовая 32-битная сборка: [`dist/Winyl_x86.exe`](dist/Winyl_x86.exe)
(SHA-256 `044FDECCA7F1A302BF1D1B799D76FE2A9EFF913CE86F46DA9197ABBA29C5BB60`).
