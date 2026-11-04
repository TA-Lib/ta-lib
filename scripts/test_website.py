#!/usr/bin/env python3
"""Tests for the registry lookups in utilities/website.py, with the network stubbed.

    python3 scripts/test_website.py
"""

import json
import os
import sys
import unittest
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utilities import website  # noqa: E402


def packument(versions):
    return json.dumps({"name": "ta-lib", "versions": versions}).encode()


class LatestNpmVersion(unittest.TestCase):
    def lookup(self, answer):
        with mock.patch.object(website, "_get", return_value=answer):
            return website.latest_npm_version()

    def test_only_the_previous_holders_versions_is_not_published(self):
        body = packument({v: {"version": v} for v in ("0.9.0", "0.10.0", "0.11.0")})
        self.assertEqual(self.lookup((body, 200, None)), website.NOT_PUBLISHED)

    def test_unknown_name_is_not_published(self):
        self.assertEqual(self.lookup((None, 404, "Not Found")), website.NOT_PUBLISHED)

    def test_unreachable_registry_is_unknown(self):
        self.assertIsNone(self.lookup((None, None, "timed out")))

    def test_every_one_of_ours_deprecated_is_unknown(self):
        foreign = {v: {"version": v} for v in ("0.9.0", "0.10.0", "0.11.0")}
        body = packument({**foreign, "0.8.3": {"deprecated": "broken"}})
        self.assertIsNone(self.lookup((body, 200, None)))

    def test_newest_undeprecated_version_wins(self):
        body = packument({"0.11.0": {}, "0.8.4": {}, "0.8.10": {}, "0.9.1": {"deprecated": "broken"}})
        self.assertEqual(self.lookup((body, 200, None)), "0.8.10")


if __name__ == "__main__":
    unittest.main(verbosity=2)
