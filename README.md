# Flying Toasters

Classic [After Dark](https://en.wikipedia.org/wiki/After_Dark_(software)) screensaver recreation.

![image](https://user-images.githubusercontent.com/1062217/231195791-1b5be6d7-5461-4243-8199-2a7dc88458d4.png)

Runs on **Haiku** using SDL2.

## Building

### Dependencies

sdl2_devel

**Controls:** Press Escape or close the window to exit.

## Using as a Screensaver

Select it from the
**Screen Saver** tab in the Screen preferences app.

## Haiku

This fork includes a native Haiku screen saver add-on, built as a
`BScreenSaver` shared object rather than a standalone executable, since
Haiku's screen saver system loads add-ons directly rather than spawning
separate windowed processes.

### Building

```bash
pkgman install sdl2_devel
make
make install
```

This builds `src/FlyingToasters` and installs it to
`~/config/non-packaged/add-ons/Screen Savers/`. 

### Config panel

Toaster count, toast count, and animation speed are adjustable from the
screen saver's Settings panel. After changing a setting, close and reopen
the preview to apply it — settings are only read when the screen saver
starts, not live while one is already running.

### Notes

If a rebuilt add-on doesn't seem to take effect, fully quit Screen
preferences and any running `screen_blanker` process before relaunching —
Haiku can keep an add-on resident in memory rather than reloading it from
disk on every preview.

## Credits

This project builds on prior work:

- **[torunar/flying-toasters-xscreensaver](https://github.com/torunar/flying-toasters-xscreensaver)** by Mikhail Shchekotov — the original Flying Toasters recreation for XScreensaver.
- **[marcusgreenwood/flying-toasters-wayland](https://github.com/marcusgreenwood/flying-toasters-wayland)** — Wayland/X11/Raspberry Pi port via SDL2, which this repo is forked from.

Licensed under the MIT License (see `LICENSE`), same as the upstream project.
