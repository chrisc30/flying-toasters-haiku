# Flying Toasters

Classic [After Dark](https://en.wikipedia.org/wiki/After_Dark_(software)) screensaver recreation.

![image](https://user-images.githubusercontent.com/1062217/231195791-1b5be6d7-5461-4243-8199-2a7dc88458d4.png)

Runs on **Wayland**, **X11**, and **Raspberry Pi** using SDL2.

## Building

### Dependencies

- **Raspberry Pi / Debian / Ubuntu:**
  ```bash
  sudo apt install build-essential pkg-config libsdl2-dev
  # For xscreensaver support (draws directly on its window):
  sudo apt install libx11-dev libxpm-dev
  ```
- **macOS:**
  ```bash
  brew install sdl2
  ```

### Build

```bash
make build
```

The binary will be in `bin/flying-toasters`. Run `make run` to preview in windowed mode, or `./bin/flying-toasters` for fullscreen.

## Raspberry Pi & Wayland

On Raspberry Pi OS (64-bit, Bookworm) with Wayland, SDL2 uses Wayland automatically. No extra setup needed.

To force Wayland if both X11 and Wayland are available:
```bash
SDL_VIDEODRIVER=wayland ./bin/flying-toasters
```

**Controls:** Press Escape or close the window to exit.

## Using as a Screensaver

- **Wayland:** Use with a Wayland screensaver/inhibit daemon. Some options:
  - [swayidle](https://github.com/swaywm/swayidle) + custom script
  - [wayland-idle-inhibit](https://github.com/nwg-piotr/nwg-shell)
- **X11 / XScreensaver:** Add to `~/.xscreensaver`:
  ```
  /usr/local/bin/flying-toasters
  ```
  Requires `libx11-dev` and `libxpm-dev`. When launched by xscreensaver, draws directly on its window (no flickering). If you see "DISPLAY is not set", ensure xscreensaver is started with your session's DISPLAY (e.g. `export DISPLAY=:0` in your autostart).

## Docker

Cross-compile for Linux (e.g. from macOS):
```bash
docker build -t flying-toasters .
docker create --name ft flying-toasters
docker cp ft:/flying-toasters ./
docker rm ft
chmod +x flying-toasters
```

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
`~/config/non-packaged/add-ons/Screen Savers/`. Select it from the
**Screen Saver** tab in the Screen preferences app.

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
