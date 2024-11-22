import os
import re
import sys

def _read_version_info(version_file_path: str, version_pattern: str) -> dict:
    """
    Read a version string from a file.

    version_pattern must be a regular expression with a first group
    that includes MAJOR, MINOR, BUILD tokens. In other words, the
    keys must be present on the line being selected by the pattern.

    The second regex group must be the value of the token.

    Returns a dictionary with the keys 'MAJOR', 'MINOR', 'BUILD'.
    """
    version_info = {'MAJOR': None, 'MINOR': None, 'BUILD': None}

    with open(version_file_path, 'r') as file:
        for line in file:
            match = version_pattern.search(line)
            if match:
                key = match.group(1)
                value = match.group(2).strip('"')
                version_info[key] = value

    if None in version_info.values():
        print(f"Error: MAJOR, MINOR, and BUILD must be defined in {version_file_path}")
        sys.exit(1)

    return version_info

def _version_info_to_string(version_info: dict) -> str:
    """
    Convert a version information dictionary to a version string.

    The dictionary must have the keys 'MAJOR', 'MINOR', 'BUILD'.
    """
    # Verify that all keys are defined.
    if None in version_info.values():
        print(f"Error: MAJOR, MINOR, and BUILD must be defined in {version_info}")
        sys.exit(1)

    return f"{version_info.get('MAJOR', '0')}.{version_info.get('MINOR', '0')}.{version_info.get('BUILD', '0')}"

def _split_version_string(version: str) -> dict:
    """
    Split a version string into its components.

    The version string must be in the format "0.0.0"

    Returns a dictionary with the keys 'MAJOR', 'MINOR', 'BUILD'
    """
    version_info = {'MAJOR': None, 'MINOR': None, 'BUILD': None}

    dot_parts = version.split('.')
    if len(dot_parts) != 3:
        print(f"Error: version must be in format '0.0.0' got instead [{version}]")
        sys.exit(1)

    version_info['MAJOR'], version_info['MINOR'], version_info['BUILD'] = dot_parts

    return version_info

def get_version_string(root_dir: str) -> str:
    """
    Parse the file src/ta_common/ta_version.c to build a version string.

    The file contains the following C definitions:
        #define MAJOR "0"
        #define MINOR "6"
        #define BUILD "0"

    These become the string "0.6.0".
    """

    version_file_path = os.path.join(root_dir, "src", "ta_common", "ta_version.c")
    version_pattern = re.compile(r'#define\s+(MAJOR|MINOR|BUILD)\s+"(.*?)"')

    version_info = _read_version_info(version_file_path, version_pattern)

    return _version_info_to_string(version_info)

def get_version_string_cmake(root_dir: str) -> str:
    """
    Similar to get_version_string() but parse CMakeLists.txt instead of ta_version.c

    Excerpt of the file:
        SET(TA_LIB_VERSION_MAJOR 0)
        SET(TA_LIB_VERSION_MINOR 6)
        SET(TA_LIB_VERSION_BUILD 0)
    """
    version_file_path = os.path.join(root_dir, "CMakeLists.txt")
    version_pattern = re.compile(r'set\s*\(\s*TA_LIB_VERSION_(MAJOR|MINOR|BUILD)\s+(\d+)\s*\)', re.IGNORECASE)

    version_info = _read_version_info(version_file_path, version_pattern)

    return _version_info_to_string(version_info)


def set_version_string(root_dir: str, new_version:str):
    """
    Counterpart to get_version_string() that updates the src/ta_common/ta_version.c
    """
    version_file_path = os.path.join(root_dir, "src", "ta_common", "ta_version.c")

    current_version = get_version_string(root_dir)

    if current_version == new_version:
        return # No changes needed. The version is already up to date.

    version_info = _split_version_string(new_version)

    # Read the ta_version.c file
    with open(version_file_path, 'r') as version_file:
        lines = version_file.readlines()

    # Update the version information in the lines
    version_pattern = re.compile(r'#define\s+(MAJOR|MINOR|BUILD)\s+"(.*?)"')
    found_keys = set()
    for i, line in enumerate(lines):
        match = version_pattern.search(line)
        if match:
            key = match.group(1)
            if key in version_info:
                value = version_info[key]
                lines[i] = f'#define {key} "{value}"\n'
                found_keys.add(key)

    # Check if all required keys were found
    if not all(k in found_keys for k in ['MAJOR', 'MINOR', 'BUILD']):
        print(f"Error: MAJOR, MINOR, and BUILD must be defined in {version_file_path}")
        sys.exit(1)

    # Write the updated lines back to the ta_version.c file
    with open(version_file_path, 'w') as version_file:
        version_file.writelines(lines)

def set_version_string_cmake(root_dir: str, new_version:str):
    """
    Counterpart to get_version_string_cmake() that updates the
    file with the provided new_version string.

    The SET(TA_LIB_VERSION_XXXXXX, VALUE) pattern must already be present in the file,
    so only the VALUE portion need to be modified.

    If a given TA_LIB_VERSION_XXXXXX is not found in the file, the function will
    fail with a sys.exit(1).

    Excerpt of the file:
        SET(TA_LIB_VERSION_MAJOR 0)
        SET(TA_LIB_VERSION_MINOR 6)
        SET(TA_LIB_VERSION_BUILD 0)

    Example of new_version: "0.12.2"
    """

    version_file_path = os.path.join(root_dir, "CMakeLists.txt")

    current_version = get_version_string_cmake(root_dir)

    if current_version == new_version:
        return # No changes needed. The version is already up to date.

    version_info = _split_version_string(new_version)

    # Read the CMakeLists.txt file
    with open(version_file_path, 'r') as cmake_file:
        lines = cmake_file.readlines()

    # Update the version information in the lines
    version_pattern = re.compile(r'set\s*\(\s*TA_LIB_VERSION_(MAJOR|MINOR|BUILD)\s+.*\)', re.IGNORECASE)
    found_keys = set()
    for i, line in enumerate(lines):
        match = version_pattern.search(line)
        if match:
            key = match.group(1)
            if key in version_info:
                value = version_info[key]
                lines[i] = f'set(TA_LIB_VERSION_{key} {value})\n'
                found_keys.add(key)

    # Check if all required keys were found
    if not all(k in found_keys for k in ['MAJOR', 'MINOR', 'BUILD']):
        print(f"Error: MAJOR, MINOR, and BUILD must be defined in {version_file_path}")
        sys.exit(1)

    # Write the updated lines back to the CMakeLists.txt file
    with open(version_file_path, 'w') as cmake_file:
        cmake_file.writelines(lines)


def compare_version(version1: str, version2: str) -> int:
    """
    Compare two version strings.

    The format is 0.0.0

    Returns:
        -1 if version1 < version2
         0 if version1 == version2
         1 if version1 > version2
    """

    # Compare the parts as integer values
    v1_parts = list(map(int, version1.split('.')))
    v2_parts = list(map(int, version2.split('.')))
    for v1_part, v2_part in zip(v1_parts, v2_parts):
        if v1_part > v2_part:
            return 1
        elif v1_part < v2_part:
            return -1

    return 0 # Identical versions


def sync_versions(root_dir: str) -> str:
    """
    Synchronize the version between ta_version.c and CMakeLists.txt.

    The versions are first read from both. The highest version is selected.

    If the versions are the same, the function will touch nothing.

    If one file has a lower version, it is updated with the highest version.
    """
    version_c = get_version_string(root_dir)
    version_cmake = get_version_string_cmake(root_dir)

    compare_result: int = compare_version(version_c, version_cmake)
    if compare_result > 0:
        print(f"Updating CMakeLists.txt to ta_version.c [{version_c}]")
        set_version_string_cmake(root_dir, version_c)
    elif compare_result < 0:
        print(f"Updating ta_version.c to CMakeLists.txt [{version_cmake}]")
        set_version_string(root_dir, version_cmake)

    return version_c
