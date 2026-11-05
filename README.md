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

### 3. Visible toggle state and skin editor

Repeat, Shuffle and Mute show a bright underline when enabled, including during
hover animations. Skin authors can set the accent in the corresponding window
XML with `<Highlight Color="ff3d3d" />` under `<Window>`.

Open **Skin** (F4), select an unpacked skin and click **Edit...**. The editor
can replace background, button and other PNG/BMP/JPEG images and restore the
original images. It also edits the playlist and library background color and
Crop/Stretch/Cover/Contain mode. Image replacements and background settings
are saved immediately; original files are preserved alongside them as
`.winyl-original` backups. Apply the skin to see changes; editing the current
skin reloads it on closing the editor and stops playback. Packed `.wzp` skins
must first be extracted into a folder in `Skin` and the Skin dialog reopened.

### Ready-made Tokyo Ghoul theme

The anime / dead-inside theme is in
[`themes/Tokyo_Ghoul_No_Blue_Icons`](themes/Tokyo_Ghoul_No_Blue_Icons).
Copy that folder into the player's `Skin` directory and select it in the Skin
dialog. Artwork and names may be subject to their respective owners' rights.

## Download

A ready-to-use 32-bit executable is provided in [`dist/Winyl_x86.exe`](dist/Winyl_x86.exe)
and as a [release asset](https://github.com/qexdy3/winyl-3.3.1-fixed/releases/tag/v3.3.1-skin-editor).

* File version: 3.3.1.0
* Size: 2 435 584 bytes
* SHA-256: `0B900F56E2650D84806CABD3847863007EA3AFD312AB21446839C37C32861BCE`

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

[`Winyl_3.3.1_fixes.patch`](Winyl_3.3.1_fixes.patch) contains the earlier
button and background fixes. The skin editor and toggle indicator are in the
repository source, not in that historical patch.

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
3. **Индикатор режима.** Включённые Repeat, Shuffle и Mute выделены яркой полоской.
4. **Редактор скинов и тем.** Нажмите F4, выберите распакованный скин и нажмите
   **Edit...**. Можно заменить изображения фона, кнопок и других элементов, восстановить
   оригиналы и настроить цвет и режим фона плейлиста и библиотеки. Изменения сохраняются
   сразу, копии оригиналов получают расширение `.winyl-original`. При закрытии редактора
   текущий скин перезагружается, воспроизведение останавливается. Архив `.wzp` сначала
   нужно распаковать в отдельную папку внутри `Skin` и заново открыть окно выбора скина.

**Готовая аниме / дед-инсайд тема Tokyo Ghoul:**
[`themes/Tokyo_Ghoul_No_Blue_Icons`](themes/Tokyo_Ghoul_No_Blue_Icons).
Скопируйте папку темы в `Skin` установленного плеера и выберите её по F4.
Права на персонажей и изображения могут принадлежать их правообладателям.

Готовая 32-битная сборка: [`dist/Winyl_x86.exe`](dist/Winyl_x86.exe)
(SHA-256 `0B900F56E2650D84806CABD3847863007EA3AFD312AB21446839C37C32861BCE`).
