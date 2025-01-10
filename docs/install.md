---
hide:
    - toc
---
Instructions are for installing the C/C++ shared, static libraries and headers on your system.

Latest release is [0.6.3 on Github](https://github.com/ta-lib/ta-lib/releases/latest)

For python users, see instead [ta-lib-python](https://github.com/TA-Lib/ta-lib-python)

- [Windows](#windows)
    - [Executable Installer (recommended)](#executable-installer-recommended)
    - [Binaries](#windows-binaries)
    - [Build from source](#windows-build-from-source)

- [macOS](#macos)
    - [Homebrew (recommended)](#macos-homebrew-recommended)
    - [Build from source](#macos-build-from-source)

- [Linux](#linux)
    - [Debian packages](#linux-debian-packages)
    - [Build from source](#linux-build-from-source)


## Windows

### Executable Installer (recommended)

1. **Download** latest [ta-lib-0.6.3-windows-x86_64.msi](https://github.com/ta-lib/ta-lib/releases/download/v0.6.3/ta-lib-0.6.3-windows-x86_64.msi)

2. **Run the Installer**:
    - Double-click the downloaded `.msi` file.
    - Follow the on-screen instructions.


    To update, just repeat the installation (older version is automatically uninstalled).

    If you choose to uninstall, us the [Add/Remove Apps](https://support.microsoft.com/en-us/windows/uninstall-or-remove-apps-and-programs-in-windows-4b55f974-2cc6-2d2b-d092-5905080eaf98) in windows settings.

 If you prefer a non-interactive installation, you can use msiexec [from the command line](https://learn.microsoft.com/en-us/windows/win32/msi/standard-installer-command-line-options).


### Windows Binaries

Use the .zip packages when you prefer to get the libraries without installing (e.g. to embed the TA-Lib binaries in your own installer).

| Platform | Download |
|------------------------|--|
| Intel/AMD 64-bits| [ta-lib-0.6.3-windows-x86_64.zip](https://github.com/ta-lib/ta-lib/releases/download/v0.6.3/ta-lib-0.6.3-windows-x86_64.zip) |
| Intel/AMD 32-bits| [ta-lib-0.6.3-windows-x86_32.zip](https://github.com/ta-lib/ta-lib/releases/download/v0.6.3/ta-lib-0.6.3-windows-x86_32.zip) |
| ARM64 | Not yet available. |

### Windows Build from Source

Install VSCode 2022 community and do:
```cmd
C:\ta-lib> "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
C:\ta-lib> mkdir build
C:\ta-lib> cd build
C:\ta-lib\build> cmake ..
C:\ta-lib\build> cmake --build .
```
You might need to adjust the `vcvarsall.bat` command depending on your VSCode installation and platform.


## macOS

### macOS Homebrew (recommended)

```bash
brew install ta-lib
```

See the [homebrew formula](https://formulae.brew.sh/formula/ta-lib) for the latest supported release and platforms.

### macOS Build from Source

Ensure you have the required dependencies: `brew install automake && brew install libtool`

1. **Download** latest [ta-lib-0.6.3-src.tar.gz](https://github.com/ta-lib/ta-lib/releases/download/v0.6.3/ta-lib-0.6.3-src.tar.gz) (or, alternatively, clone down [https://github.com/ta-lib/ta-lib/](https://github.com/ta-lib/ta-lib/) and checkout the main branch)

2. **Extract the Tarball** if you downloaded the source manually:
   ```bash
   tar -xzf ta-lib-0.6.3-src.tar.gz
   cd ta-lib-0.6.3
   ```

3. **Build and Install**:
   ```bash
   chmod +x autogen.sh  # ensure the permissions are set to generate the configure file
   ./autogen.sh         # generate the configure file
   ./configure
   make
   sudo make install
   ```

    Follow the same procedure for an update (the older version is overwritten, no need to uninstall).

    If you choose to uninstall do:
    ```bash
    sudo make uninstall
    ```

## Linux

### Linux Debian Packages

Recommended for all debian-based distributions (e.g. Ubuntu, Mint...)

1. **Download** the `.deb` package matching your platform:

    | Platform | Download |
    |------------------------|--|
    | Intel/AMD 64-bits | [ta-lib_0.6.3_amd64.deb](https://github.com/ta-lib/ta-lib/releases/download/v0.6.3/ta-lib_0.6.3_amd64.deb) |
    | ARM64 (e.g. Raspberry Pi)| [ta-lib_0.6.3_arm64.deb](https://github.com/ta-lib/ta-lib/releases/download/v0.6.3/ta-lib_0.6.3_arm64.deb) |
    | Intel/AMD 32-bits| [ta-lib_0.6.3_i386.deb](https://github.com/ta-lib/ta-lib/releases/download/v0.6.3/ta-lib_0.6.3_i386.deb) |

2. **Install or Update**:
   ```bash
   # For Intel/AMD (64 bits)
   sudo dpkg -i ta-lib_0.6.3_amd64.deb
   # or
   sudo dpkg -i ta-lib_0.6.3_arm64.deb
   # or
   sudo dpkg -i ta-lib_0.6.3_i386.deb
   ```
   If you choose to uninstall do:
    ```bash
    sudo dpkg -r ta-lib
    ```

### Linux Build from Source

1. **Download** latest [ta-lib-0.6.3-src.tar.gz](https://github.com/ta-lib/ta-lib/releases/download/v0.6.3/ta-lib-0.6.3-src.tar.gz) (or, alternatively, clone down [https://github.com/ta-lib/ta-lib/](https://github.com/ta-lib/ta-lib/) and checkout the main branch)

2. **Extract the Tarball** if you downloaded the source manually:
   ```bash
   tar -xzf ta-lib-0.6.3-src.tar.gz
   cd ta-lib-0.6.3
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

