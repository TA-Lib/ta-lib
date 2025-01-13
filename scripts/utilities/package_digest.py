from dataclasses import dataclass
import hashlib
import json
import os
import sys

from utilities.files import path_join

def _calculate_md5(filepath: str) -> str:
    # Calculate md5 of a binary file (do not use for text file because
    # of portability issue for line endings)
    hash_md5 = hashlib.md5()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def _digests_path(root_dir: str) -> str:
    return path_join(root_dir, "dist", "digests")

def _asset_file_name_to_filepath(root_dir: str, asset_file_name: str) -> str:
    return path_join(_digests_path(root_dir), f"{asset_file_name}.digest")

@dataclass
class PackageDigest:
    root_dir: str = ""          # Mandatory at construction
    asset_file_name: str = ""        # Mandatory at construction
    sources_digest: str = ""    # Mandatory at construction
    builder_id: str = ""        # Mandatory at construction

    built_success: str = "False"    # "True" or "False"

    package_md5: str = "Disabled"     # "Disabled", "Unknown" or "hash_of_package"
    gen_code_pass: str = "Disabled"   # "Disabled", "Unknown", "True" or "False"
    ta_regtest_pass: str = "Disabled" # "Disabled", "Unknown", "True" or "False"
    dist_test_pass: str = "Disabled"  # "Disabled", ""Unknown", True" or "False"

    @staticmethod
    def default(root_dir: str, asset_file_name: str, sources_digest: str, builder_id: str) -> 'PackageDigest':
        pdigest = PackageDigest(
            root_dir=root_dir,
            asset_file_name=asset_file_name,
            sources_digest=sources_digest,
            builder_id=builder_id,
        )

        pdigest._update_features()

        return pdigest

    def _update_features(self):
        # Use the asset_file_name to figure out which features are enabled.
        #
        # When name is "github-*", then it is tracking the state of a repo branch, else
        # assume it is tracking a package in dist/
        if self.asset_file_name.startswith("github-"):
            # gen_code is enabled only when doing repos branch processing.
            self._disable_package_md5()
            self._disable_dist_test()
            self._enable_gen_code()
            self._enable_ta_regtest()
        else:
            self._enable_package_md5()
            self._enable_dist_test()
            self._disable_gen_code()
            # ta_regtest must work for src.tar.gz (needed for homebrew maintainer).
            if self.asset_file_name.endswith("-src.tar.gz"):
                self._enable_ta_regtest()
            else:
                self._disable_ta_regtest()

    @staticmethod
    def from_json(root_dir: str, data: dict) -> 'PackageDigest':
        # Note: root_dir purposely not saved in JSON.

        # Exit the process on missing mandatory fields.
        # For non-mandatory fields, just initialize as if the action
        # was failed or not done.
        pdigest = PackageDigest(
            root_dir=root_dir,
            asset_file_name=data.get("asset_file_name", ""),
            sources_digest=data.get("sources_digest", ""),
            builder_id=data.get("builder_id", ""),
            built_success=data.get("built_success", ""),
            package_md5=data.get("package_md5", ""),
            gen_code_pass=data.get("gen_code_pass", ""),
            ta_regtest_pass=data.get("ta_regtest_pass", ""),
            dist_test_pass=data.get("dist_test_pass", ""),
        )

        if pdigest.asset_file_name == "" or pdigest.builder_id == "" or pdigest.sources_digest == "":
            print(f"Error: Missing mandatory field(s) in JSON: {data}")
            sys.exit(1)

        # Verify supported values.
        if pdigest.built_success not in ["True", "False"]:
            print(f"Error: Invalid value for built_success: {pdigest.built_success}")
            sys.exit(1)

        if pdigest.gen_code_pass not in ["Disabled", "Unknown", "True", "False"]:
            print(f"Error: Invalid value for gen_code_pass: {pdigest.gen_code_pass}")
            sys.exit(1)

        if pdigest.ta_regtest_pass not in ["Disabled", "Unknown", "True", "False"]:
            print(f"Error: Invalid value for ta_regtest_pass: {pdigest.ta_regtest_pass}")
            sys.exit(1)

        if pdigest.dist_test_pass not in ["Disabled", "Unknown", "True", "False"]:
            print(f"Error: Invalid value for dist_test_pass: {pdigest.dist_test_pass}")
            sys.exit(1)

        # Refresh the enabling/disabling of features.
        pdigest._update_features()

        return pdigest

    def to_json(self):
        # Note: root_dir purposely not saved in JSON.
        return {
            "asset_file_name": self.asset_file_name,
            "sources_digest": self.sources_digest,
            "builder_id": self.builder_id,
            "built_success": self.built_success,
            "package_md5": self.package_md5,
            "gen_code_pass": self.gen_code_pass,
            "ta_regtest_pass": self.ta_regtest_pass,
            "dist_test_pass": self.dist_test_pass,
        }

    def calculate_md5(self) -> str:
        if self.package_md5 == "Disabled":
            return "Disabled"
        package_file_path = path_join(self.root_dir, "dist", self.asset_file_name)
        return _calculate_md5(package_file_path)

    def clear_tests(self):
        if self.gen_code_pass != "Disabled":
            self.gen_code_pass = "Unknown"

        if self.ta_regtest_pass != "Disabled":
            self.ta_regtest_pass = "Unknown"

        if self.dist_test_pass != "Disabled":
            self.dist_test_pass = "Unknown"

    def are_all_tests_passed(self) -> bool:
        if self.gen_code_pass != "Disabled" and self.gen_code_pass != "True":
            return False
        if self.ta_regtest_pass != "Disabled" and self.ta_regtest_pass != "True":
            return False
        if self.dist_test_pass != "Disabled" and self.dist_test_pass != "True":
            return False
        return True

    def _enable_package_md5(self):
        if self.package_md5 == "Disabled":
            self.package_md5 = "Unknown"

    def _disable_package_md5(self):
        self.package_md5 = "Disabled"

    def _enable_gen_code(self):
        if self.gen_code_pass == "Disabled":
            self.gen_code_pass = "Unknown"

    def _disable_gen_code(self):
        self.gen_code_pass = "Disabled"

    def _enable_ta_regtest(self):
        if self.ta_regtest_pass == "Disabled":
            self.ta_regtest_pass = "Unknown"

    def _disable_ta_regtest(self):
        self.ta_regtest_pass = "Disabled"

    def _enable_dist_test(self):
        if self.dist_test_pass == "Disabled":
            self.dist_test_pass = "Unknown"

    def _disable_dist_test(self):
        self.dist_test_pass = "Disabled"

    def write(self):
        digests_dir = _digests_path(self.root_dir)
        os.makedirs(digests_dir, exist_ok=True)

        filepath = _asset_file_name_to_filepath(self.root_dir, self.asset_file_name)

        with open(filepath, 'w') as file:
            json.dump(self.to_json(), file, indent=4)

        # Test that the file can be read back correctly
        try:
            read_back = PackageDigest.read(self.root_dir, self.asset_file_name)
            if read_back != self:
                raise ValueError(f"Error reading back {filepath}: {read_back} != {self}")
        except Exception as e:
            raise ValueError(f"Error writing {filepath}: {e}")

    @staticmethod
    def read_or_create(root_dir: str, asset_file_name: str, sources_digest: str, builder_id: str ) -> 'PackageDigest':
        filepath = _asset_file_name_to_filepath(root_dir, asset_file_name)
        create = False

        if not os.path.exists(filepath):
            create = True
        else:
            try:
                with open(filepath, 'r') as file:
                    data = json.load(file)
                return PackageDigest.from_json(root_dir,data)
            except Exception as e:
                create = True

        if create:
            pd = PackageDigest.default(root_dir, asset_file_name, sources_digest, builder_id)
            try:
                pd.write()
                return pd
            except Exception as e:
                print(f"Error creating {filepath}: {e}")
                sys.exit(1)

    @staticmethod
    def read(root_dir: str, asset_file_name: str) -> 'PackageDigest':
        # Throws an exception if reading fails.
        filepath = _asset_file_name_to_filepath(root_dir, asset_file_name)
        create = False

        if not os.path.exists(filepath):
            raise FileNotFoundError(f"File not found: {filepath}")
        else:
            try:
                with open(filepath, 'r') as file:
                    data = json.load(file)
                return PackageDigest.from_json(root_dir,data)
            except Exception as e:
                raise ValueError(f"Error reading {filepath}: {e}")

