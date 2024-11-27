#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Default installation prefix
INSTALL_PREFIX="/usr/local"

# Function to display help message
show_help() {
    echo "Script to install or update TA-Lib"
    echo ""
    echo "Usage: $0 [Options]"
    echo ""
    echo "Options:"
    echo "  -p, --prefix <installation_prefix>  Specify the installation prefix (default: /usr/local)"
    echo "  -h, --help                          Show this help message and exit"
}

# Parse command line arguments for custom installation prefix
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -p|--prefix) INSTALL_PREFIX="$2"; shift ;;
        -h|--help) show_help; exit 0 ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

if ! command -v cmake &> /dev/null; then
    echo "CMake is not installed. Please install CMake and try again."
    exit 1
fi

# Check if the script is run with sudo
if [[ "$EUID" -ne 0 ]]; then
    echo "This script must be run with sudo. Please rerun the script like this:"
    echo "sudo" "$0" "$@"
    exit 1
fi

# For security (and avoid permission issues on created directories), run the
# following commands as the original non-sudo user.
sudo -u "$SUDO_USER" bash <<EOF
# Exit immediately if a command exits with a non-zero status
set -e

# echo "Running as user: \$(whoami)"
# echo "Effective user ID: \$(id -u)"

if [[ "\$(id -u)" -eq 0 ]]; then
    echo "Internal error: This subshell must not run with sudo privileges."
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Create the makefiles and build.
cmake -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" ..
cmake --build .

# Run ta-lib/bin/ta_regtest for validation.
../bin/ta_regtest
EOF

# Install in /usr/local (by default).
# You can use "install.sh --prefix" to specify a custom location.
cd build
sudo cmake --install .
cd ..

echo "Installation completed successfully!"