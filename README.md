# MyView - Modern Image Viewer

MyView is a sleek, fast, and modern image viewer built for Linux using Qt6. It is designed to be unobtrusive, highly responsive, and feature-rich without the clutter.

![MyView Logo](src/logo.png)

## Features

- **Modern UX**: Dark theme, floating HUD controls, and smooth animations.
- **Smart Zoom**: Double-click to toggle between "Fit to Screen" and "Actual Size" (1:1).
- **Navigation**: Intuitive arrow key navigation and mouse wheel support.
- **Formats**: Supports all major formats (JPG, PNG, WEBP, GIF, SVG, BMP, TIFF).
- **Transparency**: Checkerboard background for transparent images.
- **Workflow**: 
    - **Tabbed Interface**: Open multiple images in tabs (Ctrl+N, Ctrl+T).
    - **Drag & Drop**: Drag images directly into the window.
    - **Singleton**: Smartly handles duplicates—focuses existing tabs instead of opening new ones.

## Installation

### Ubuntu / Debian
Download the latest `.deb` package from the [Releases](https://github.com/StartYourFork/myview/releases) page.

```bash
sudo apt install ./myview-1.0.0-Linux.deb
```
This will install `MyView` and register it as an image viewer. You can then right-click any image and select "Open With MyView".

### Portable AppImage
Download the `MyView-x86_64.AppImage`, make it executable, and run:

```bash
chmod +x MyView-x86_64.AppImage
./MyView-x86_64.AppImage
```

### Build from Source
See [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md) for detailed compilation steps.

## Shortcuts

| Action | Shortcut |
|--------|----------|
| **New Tab** (Welcome) | `Ctrl` + `N` |
| **Close Tab** | `Ctrl` + `W` |
| **Next Tab** | `Ctrl` + `Tab` |
| **Previous Tab** | `Ctrl` + `Shift` + `Tab` |
| **Next Image** | `Right Arrow` |
| **Previous Image** | `Left Arrow` |
| **Zoom In/Out** | `Ctrl` + `Wheel` |
| **Reset Zoom** | `Esc` |

## License
MIT License. See [LICENSE](LICENSE) for details.
