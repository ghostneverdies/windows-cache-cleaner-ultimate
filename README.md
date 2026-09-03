# Windows Cache Cleaner Ultimate

A Windows C++ utility for cleaning system, browser, development-tool, gaming, and application caches.

## Download

The easiest option is to download the prebuilt executable:

**[Download Windows Cache Cleaner Ultimate](https://github.com/ghostneverdies/windows-cache-cleaner-ultimate/releases/download/v1.1/cleaner.exe)**

After downloading:

1. Right-click `cleaner.exe`.
2. Select **Run as administrator**.
3. Allow the Windows UAC prompt if shown.
4. Let the cleaner finish.

Administrator privileges are recommended because some Windows system locations require elevated access.

## Build from Source

Clone the repository first:

```bash
git clone https://github.com/ghostneverdies/windows-cache-cleaner-ultimate.git
cd windows-cache-cleaner-ultimate
```

The repository contains the source code and the prebuilt executable.

### MSVC

Open **Developer Command Prompt for Visual Studio** and run:

```bat
cl /std:c++17 /EHsc /O2 cleaner.cpp /Fe:cleaner.exe
```

Then run:

```bat
cleaner.exe
```

For a release build:

```bat
cl /std:c++17 /EHsc /O2 /DNDEBUG cleaner.cpp /Fe:cleaner.exe
```

### GCC / MinGW-w64

From a MinGW-w64 terminal:

```bat
g++ -std=c++17 -O2 -o cleaner.exe cleaner.cpp
```

Then run:

```bat
cleaner.exe
```

Optional statically linked build:

```bat
g++ -std=c++17 -O2 -static -o cleaner.exe cleaner.cpp
```

## Requirements

- Windows 10 or newer
- C++17-compatible compiler when building from source
- Windows SDK when using MSVC
- MinGW-w64 when using GCC
- Administrator privileges recommended

This is a **Windows-only** program.

## What It Cleans

### Windows / System

- Windows Temp
- User Temp
- Windows Update download cache
- Thumbnail cache
- Windows Error Reporting (WER)
- Delivery Optimization cache
- Crash dumps / minidumps
- Direct3D cache
- Windows Prefetch
- Windows Installer patch cache

### Browsers

- Google Chrome cache
- Chrome Code Cache
- Chrome Service Worker cache
- Chrome GPU cache
- Microsoft Edge cache
- Edge Code Cache
- Edge Service Worker cache
- Edge GPU cache
- Firefox cache
- Firefox startup cache

### Development / Package Managers

- npm cache
- pip cache
- NuGet caches and packages

### Applications / Gaming

- NVIDIA shader caches
- NVIDIA download cache
- Discord caches
- Steam HTML/shader caches
- Roblox cache
- Nuitka build cache

### Windows Disk Cleanup

The program also runs Windows' built-in Disk Cleanup utility.

## Important

Close browsers, Discord, Steam, development tools, and other applications before running the cleaner.

Some files may be locked by running applications and cannot be removed.

The cleaner deletes cache files directly rather than moving them to the Recycle Bin.

Cache deletion may cause applications to rebuild their caches the next time they start.

## Repository

**[GitHub Repository](https://github.com/ghostneverdies/windows-cache-cleaner-ultimate)**

Source code:

```text
cleaner.cpp
```

Prebuilt executable:

```text
cleaner.exe
```

## License

This project is licensed under the MIT License. See `LICENSE` for details.
