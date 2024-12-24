import hashlib
import os
import re
import sys
from typing import Tuple

from .common import expand_globs
from .files import path_join

def _read_version_info(version_file_path: str, version_pattern: str) -> dict:
    """
    Read a version string from a file.

    version_pattern must be a regular expression with a first group
    that includes MAJOR, MINOR, PATCH tokens. In other words, the
    keys must be present on the line being selected by the pattern.

    The second regex group must be the value of the token.

    Returns a dictionary with the keys 'MAJOR', 'MINOR', 'PATCH'.
    """
    version_info = {'MAJOR': None, 'MINOR': None, 'PATCH': None}

    with open(version_file_path, 'r') as file:
        for line in file:
            match = version_pattern.search(line)
            if match:
                key = match.group(1)
                value = match.group(2).strip('"')
                version_info[key] = value

    if None in version_info.values():
        print(f"Error: MAJOR, MINOR, and PATCH must be defined in {version_file_path}")
        sys.exit(1)

    return version_info

def _version_info_to_string(version_info: dict) -> str:
    """
    Convert a version information dictionary to a version string.

    The dictionary must have the keys 'MAJOR', 'MINOR', 'PATCH'.
    """
    # Verify that all keys are defined.
    if None in version_info.values():
        print(f"Error: MAJOR, MINOR, and PATCH must be defined in {version_info}")
        sys.exit(1)

    return f"{version_info.get('MAJOR', '0')}.{version_info.get('MINOR', '0')}.{version_info.get('PATCH', '0')}"

def _split_version_string(version: str) -> dict:
    """
    Split a version string into its components.

    The version string must be in the format "0.0.0"

    Returns a dictionary with the keys 'MAJOR', 'MINOR', 'PATCH'
    """
    version_info = {'MAJOR': None, 'MINOR': None, 'PATCH': None}

    dot_parts = version.split('.')
    if len(dot_parts) != 3:
        print(f"Error: version must be in format '0.0.0' got instead [{version}]")
        sys.exit(1)

    version_info['MAJOR'], version_info['MINOR'], version_info['PATCH'] = dot_parts

    return version_info

def get_version_string(root_dir: str) -> str:
    """
    Parse the file VERSION to get the version string.

    The file contains a single line with the version string "major.minor.patch"

    Exit on any error.
    """
    version_file_path = path_join(root_dir, "VERSION")

    try:
        with open(version_file_path, 'r') as version_file:
            version_string = version_file.readline().strip()
        # Validate the "0.0.0" format.
        if not re.match(r'^\d+\.\d+\.\d+$', version_string):
            print(f"Error: VERSION file must contain a valid version string in the format '0.0.0'. Got instead [{version_string}]")
            sys.exit(1)
    except Exception as e:
        print(f"Error reading version file: {e}")
        sys.exit(1)

    return version_string

def set_version_string(root_dir: str, new_version:str):
    """
    Counterpart to get_version_string() that updates the VERSION file with the
    provided new_version string.
    """
    version_file_path = path_join(root_dir, "VERSION")

    current_version = get_version_string(root_dir)

    if current_version == new_version:
        return # No changes needed. The version is already up to date.

    with open(version_file_path, 'w') as version_file:
        version_file.write(new_version + '\n')

def get_version_string_source_code(root_dir: str) -> str:
    """
    Parse the file src/ta_common/ta_version.c to build a version string.

    The file contains the following C definitions:
        #define MAJOR "0"
        #define MINOR "6"
        #define PATCH "0"

    These become the string "0.6.1".
    """

    version_file_path = path_join(root_dir, "src", "ta_common", "ta_version.c")
    version_pattern = re.compile(r'#define\s+(MAJOR|MINOR|PATCH)\s+"(.*?)"')

    version_info = _read_version_info(version_file_path, version_pattern)

    return _version_info_to_string(version_info)

def set_version_string_source_code(root_dir: str, new_version:str):
    """
    Counterpart to get_version_string_source_code() that updates the src/ta_common/ta_version.c
    """
    version_file_path = path_join(root_dir, "src", "ta_common", "ta_version.c")

    current_version = get_version_string_source_code(root_dir)

    if current_version == new_version:
        return # No changes needed. The version is already up to date.

    version_info = _split_version_string(new_version)

    # Read the ta_version.c file
    with open(version_file_path, 'r') as version_file:
        lines = version_file.readlines()

    # Update the version information in the lines
    version_pattern = re.compile(r'#define\s+(MAJOR|MINOR|PATCH)\s+"(.*?)"')
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
    if not all(k in found_keys for k in ['MAJOR', 'MINOR', 'PATCH']):
        print(f"Error: MAJOR, MINOR, and PATCH must be defined in {version_file_path}")
        sys.exit(1)

    # Write the updated lines back to the ta_version.c file
    with open(version_file_path, 'w') as version_file:
        version_file.writelines(lines)

def get_version_string_cmake(root_dir: str) -> str:
    """
    Similar to get_version_string_source_code() but parse CMakeLists.txt instead of ta_version.c

    Excerpt of the file:
        SET(TA_LIB_VERSION_MAJOR 0)
        SET(TA_LIB_VERSION_MINOR 6)
        SET(TA_LIB_VERSION_PATCH 0)
    """
    version_file_path = path_join(root_dir, "CMakeLists.txt")
    version_pattern = re.compile(r'set\s*\(\s*TA_LIB_VERSION_(MAJOR|MINOR|PATCH)\s+(\d+)\s*\)', re.IGNORECASE)

    version_info = _read_version_info(version_file_path, version_pattern)

    return _version_info_to_string(version_info)


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
        SET(TA_LIB_VERSION_PATCH 0)

    Example of new_version: "0.12.2"
    """

    version_file_path = path_join(root_dir, "CMakeLists.txt")

    current_version = get_version_string_cmake(root_dir)

    if current_version == new_version:
        return # No changes needed. The version is already up to date.

    version_info = _split_version_string(new_version)

    # Read the CMakeLists.txt file
    with open(version_file_path, 'r') as cmake_file:
        lines = cmake_file.readlines()

    # Update the version information in the lines
    version_pattern = re.compile(r'set\s*\(\s*TA_LIB_VERSION_(MAJOR|MINOR|PATCH)\s+.*\)', re.IGNORECASE)
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
    if not all(k in found_keys for k in ['MAJOR', 'MINOR', 'PATCH']):
        print(f"Error: MAJOR, MINOR, and PATCH must be defined in {version_file_path}")
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


def sync_versions(root_dir: str) -> Tuple[bool,str]:
    """
    Synchronize the version between ta_version.c, CMakeLists.txt and the root VERSION file.

    The versions are first read from all. The highest version is selected.

    If the versions are all the same, this function will touch nothing.

    When a file has a lower version, it is updated with the highest version.

    Return true if any file was updated.
    """
    version_file = get_version_string(root_dir)
    version_c = get_version_string_source_code(root_dir)
    version_cmake = get_version_string_cmake(root_dir)

    # Identify the highest version among all sources.
    # Put the highest in the variable highest_version
    highest_version = version_file
    if compare_version(highest_version, version_cmake) < 0:
        highest_version = version_cmake
    if compare_version(highest_version, version_c) < 0:
        highest_version = version_c

    # Update files with a lower version.
    is_updated = False
    compare_result: int = compare_version(highest_version, version_file)
    if compare_result > 0:
        print(f"Updating VERSION to [{highest_version}]")
        set_version_string(root_dir, highest_version)
        is_updated = True

    compare_result: int = compare_version(highest_version, version_c)
    if compare_result > 0:
        print(f"Updating ta_version.c to [{highest_version}]")
        set_version_string_source_code(root_dir, highest_version)
        is_updated = True

    compare_result: int = compare_version(highest_version, version_cmake)
    if compare_result > 0:
        print(f"Updating CMakeLists.txt to [{highest_version}]")
        set_version_string_cmake(root_dir, highest_version)
        is_updated = True

    return is_updated, version_c

def check_versions(root_dir: str) -> str:
    # Similar to sync_versions() but only checks if the versions are in sync, do not modify anything.
    version_file = get_version_string(root_dir)
    version_c = get_version_string_source_code(root_dir)
    version_cmake = get_version_string_cmake(root_dir)

    if version_file != version_c:
        print(f"Error: VERSION [{version_file}] does not match ta_version.c [{version_c}]")
        return None

    if version_file != version_cmake:
        print(f"Error: VERSION [{version_file}] does not match CMakeLists.txt [{version_cmake}]")
        return None

    return version_file

def write_sources_digest(root_dir: str, new_digest: str) -> bool:
    """Update the ta_common.h file with the new digest."""
    ta_common_h_path = path_join(root_dir, 'include', 'ta_common.h')
    updated = False
    try:
        with open(ta_common_h_path, 'r') as f:
            lines = f.readlines()

        with open(ta_common_h_path, 'w') as f:
            for line in lines:
                if line.startswith('#define TA_LIB_SOURCES_DIGEST'):
                    if line.strip() != f'#define TA_LIB_SOURCES_DIGEST {new_digest}':
                        f.write(f'#define TA_LIB_SOURCES_DIGEST {new_digest}\n')
                        updated = True
                    else:
                        f.write(line)
                else:
                    f.write(line)
            if not updated:
                print(f"Error: Missing TA_LIB_SOURCES_DIGEST in ta_common.h")
                sys.exit(1)

        # Read it back to confirm the change
        written_digest = read_sources_digest(root_dir)
        if written_digest != new_digest:
            print(f"Error: Failed to update TA_LIB_SOURCES_DIGEST in ta_common.h")
            sys.exit(1)

    except IOError as e:
        print(f"Error updating file {ta_common_h_path}: {e}")
        raise
    return updated

def read_sources_digest(root_dir: str) -> str:
    # Read the value of the TA_LIB_SOURCES_DIGEST. Return None if not found.
    ta_common_h_path = path_join(root_dir, 'include', 'ta_common.h')
    try:
        with open(ta_common_h_path, 'r') as f:
            for line in f:
                if line.startswith('#define TA_LIB_SOURCES_DIGEST'):
                    return line.split()[2].strip()
    except IOError as e:
        print(f"Error reading file {ta_common_h_path}: {e}")
        sys.exit(1)

    return None

def calculate_sources_digest(root_dir: str, silent: bool = False) -> str:
    # This is for a calculated digest of all source file contant relevant
    # to packaging. It helps to trig CI repackaging when a change
    # is detected.
    #
    # The digest is written as a "#define TA_LIB_SOURCE_DIGEST XXXXXXXX"
    # in ta_common.h
    #
    # Return true when ta_common.h was updated with a new digest.
    file_patterns = [
        "CMakeLists.txt",
        "configure.ac",
        "ta-lib.*.in",
        "cmake/*",
        "src/**/*.c",
        "src/**/*.h",
        "src/**/*.am",
        "*.am",
        "src/ta_abstract/templates/*",
        "ta_func_api.xml",
        "ta_func_list.txt",
        "java/src/**/*.java",
        "LICENSE",
        "VERSION",
    ]

    file_list = expand_globs(root_dir, file_patterns)

    # Remove potential duplicate entries
    file_list = list(set(file_list))

    # Normalize file paths by removing the root directory prefix (for portable sorting).
    normalized_file_list = [os.path.relpath(file_path, root_dir) for file_path in file_list]

    # Sort lower case to ensure consistent across platforms
    sorted_files = sorted(normalized_file_list, key=lambda x: x.lower())

    running_hash = hashlib.md5()
    n_lines = 0
    n_files = 0
    n_opens = 0
    n_chars = 0
    for file_path in sorted_files:
        try:
            n_files += 1
            full_file_path = path_join(root_dir, file_path)
            with open(full_file_path, 'r', encoding='utf-8') as f:
                n_opens += 1
                for line in f:
                    # Normalize line endings to Unix-style LF, remove leading/trailing whitespace
                    normalized_line = line.replace('\r\n', '\n').replace('\r', '\n').strip()
                    utf8_line = normalized_line.encode('utf-8')
                    n_chars += len(utf8_line)
                    running_hash.update(normalized_line.encode('utf-8'))
                    n_lines += 1
                #print(f" Hash: {running_hash.hexdigest()}, File: {file_path}, Lines: {n_lines}, Opens: {n_opens}, Chars: {n_chars}")
        except Exception as e:
            print(f"Error reading file while updating source digest [{file_path}]: {e}")
            sys.exit(1)

    if not silent:
        print(f"Digest input is n_files: {n_files}, n_lines: {n_lines}, n_opens: {n_opens}, n_chars: {n_chars}")
    return running_hash.hexdigest()

def sync_sources_digest(root_dir: str) -> Tuple[bool,str]:

    calculated_digest = calculate_sources_digest(root_dir, silent=True)
    # Update ta_common.h (touch only if different)
    current_digest = read_sources_digest(root_dir)
    if current_digest == calculated_digest:
        return False, calculated_digest

    print(f"Difference detected in source digest. Updating ta_common.h")
    print(f"Old source digest: {current_digest}")
    print(f"New source digest: {calculated_digest}")

    write_sources_digest(root_dir, calculated_digest)
    return True, calculated_digest


def check_sources_digest(root_dir: str) -> str:
    # Similar to sync_sources_digest() but only checks if the digests are in sync, do not modify anything.
    current_digest = read_sources_digest(root_dir)
    if current_digest is None:
        print(f"Error: TA_LIB_SOURCES_DIGEST not found in ta_common.h")
        return None

    calculated_digest = calculate_sources_digest(root_dir, silent=True)
    if current_digest != calculated_digest:
        print(f"Error: TA_LIB_SOURCES_DIGEST [{current_digest}] does not match calculated digest [{calculated_digest}]")
        return None

    return calculated_digest

