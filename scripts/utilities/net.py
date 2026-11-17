"""Retry a network fetch across the failures a second attempt can clear."""

import http.client
import socket
import sys
import time
import urllib.error


def _transient(error: BaseException) -> bool:
    if isinstance(error, urllib.error.HTTPError):
        return error.code == 429 or 500 <= error.code < 600
    return isinstance(error, (urllib.error.URLError, TimeoutError, socket.timeout,
                              ConnectionError, http.client.IncompleteRead,
                              http.client.RemoteDisconnected))


def fetch_retrying(fetch, what: str, attempts: int = 3, delays=(5, 15)):
    """Return fetch(), calling it again after a transient failure.

    fetch must redo the whole operation from scratch, since a retry follows a
    partial read. The last failure, or any non-transient one, is re-raised as is.
    """
    for attempt in range(1, attempts + 1):
        try:
            return fetch()
        except Exception as error:  # noqa: BLE001 - classified, then re-raised
            if not _transient(error):
                raise
            if attempt == attempts:
                print(f"{what}: attempt {attempt}/{attempts} failed ({error})", file=sys.stderr)
                raise
            delay = delays[min(attempt, len(delays)) - 1]
            print(f"{what}: attempt {attempt}/{attempts} failed ({error}); "
                  f"retrying in {delay}s", file=sys.stderr)
            time.sleep(delay)
