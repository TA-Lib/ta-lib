---
hide:
    - toc
---
Latest release is [0.6.1 on Github](https://github.com/ta-lib/ta-lib-temp/releases/latest)

- [Windows](#windows)
    - [Executable Installer (recommended)](#executable-installer-recommended)
    - [Binaries](#windows-binaries)
    - [Build from source](#windows-build-from-source)

- [macOS](#macOS)
    - [Homebrew (recommended)](#macOS-homebrew-recommended)
    - [Build from source](#macOS-build-from-source)

- [Linux](#linux)
    - [Debian packages](#linux-debian-packages)
    - [Build from source](#linux-build-from-source)

## Windows

Only x86 64-bits binaries are distributed. Other windows platforms need to build from source.

### Executable Installer (recommended)

1. **Download** latest [ta-lib-0.6.1-windows-x86_64.msi](https://github.com/ta-lib/ta-lib-temp/releases/download/v0.6.1/ta-lib-0.6.1-windows-x86_64.msi)

2. **Run the Installer**:
    - Double-click the downloaded `.msi` file.
    - Follow the on-screen instructions.


    To update, just repeat the installation (the older version is automatically uninstalled).

    If you choose to uninstall, follow the [standard windows procedure](https://support.microsoft.com/en-us/windows/uninstall-or-remove-apps-and-programs-in-windows-4b55f974-2cc6-2d2b-d092-5905080eaf98)

 If you prefer a non-interactive installation, you can use msiexec [from the command line](https://learn.microsoft.com/en-us/windows/win32/msi/standard-installer-command-line-options).


### Windows Binaries
This is a package with all the static/shared binaries and headers needed to bundle TA-Lib with your own application (eliminates having the user install TA-Lib separately).

Latest is [ta-lib-0.6.1-windows-x86_64.zip](https://github.com/ta-lib/ta-lib-temp/releases/download/v0.6.1/ta-lib-0.6.1-windows-x86_64.zip)

### Windows Build from Source

TODO

## macOS

### macOS Homebrew (recommended)

```bash
brew install ta-lib
```

See the [homebrew formula](https://formulae.brew.sh/formula/ta-lib) for the latest supported release and platforms.

### macOS Build from Source

TODO

## Linux

### Linux Debian Packages

Recommended for all debian-based distributions (e.g. Ubuntu, Mint...)

1. **Download** the `.deb` package matching your platform:

    | Platform | Download |
    |------------------------|--|
    | Intel/AMD 64-bits | [ta-lib_0.6.1_amd64.deb](https://github.com/ta-lib/ta-lib-temp/releases/download/v0.6.1/ta-lib_0.6.1_amd64.deb) |
    | ARM64 (e.g. Raspberry Pi)| [ta-lib_0.6.1_arm64.deb](https://github.com/ta-lib/ta-lib-temp/releases/download/v0.6.1/ta-lib_0.6.1_arm64.deb) |
    | Intel/AMD 32-bits| [ta-lib_0.6.1_x86.deb](https://github.com/ta-lib/ta-lib-temp/releases/download/v0.6.1/ta-lib_0.6.1_x86.deb) |

2. **Install or Update**:
   ```bash
   # For Intel/AMD (64 bits)
   sudo dpkg -i ta-lib_0.6.1_amd64.deb
   # or
   sudo dpkg -i ta-lib_0.6.1_arm64.deb
   # or
   sudo dpkg -i ta-lib_0.6.1_x86.deb
   ```
   If you choose to uninstall do:
    ```bash
    sudo dpkg -r ta-lib
    ```

### Linux Build from Source

1. **Download** latest [ta-lib-0.6.1-src.tar.gz](https://github.com/ta-lib/ta-lib-temp/releases/download/v0.6.1/ta-lib-0.6.1-src.tar.gz)

2. **Extract the Tarball**:
   ```bash
   tar -xzf ta-lib-0.6.1-src.tar.gz
   cd ta-lib
   ```

3. **Build and Install**:
   ```bash
   ./configure
   make
   sudo make install
   ```

    Follow the same procedure for an update (the older version is overwritten, no need to uninstall).

    If you choose to uninstall do:
    ```bash
    sudo make uninstall
    ```

