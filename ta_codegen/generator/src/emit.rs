//! Writing generated files.

use std::path::Path;

/// `std::fs::write`, except an identical file is left alone. Panics on a non-ASCII
/// header under `include/`.
///
/// Every consumer downstream of `generate` decides what to rebuild from
/// mtimes: cargo, CMake, `make`, and the `build.py servers` gcc step. Writing
/// unconditionally therefore told all of them that all 175 indicators changed
/// on every run, even for a regenerate that produced byte-identical output —
/// so a warm `target/` was worth nothing, and a job that generates twice built
/// twice. The arm64 nightly job did exactly that: `Building C server` at 124s
/// followed by `Building C server` at 129s over the same bytes, plus two
/// 64-second cargo passes, ~3.2 minutes of one 14.9-minute job.
///
/// Skipping the write is safe in the one direction that matters: the content
/// is compared, not the timestamp, so a file whose bytes differ is always
/// rewritten. `regen-check` still compares committed bytes and is unaffected.
pub fn write_if_changed<P: AsRef<Path>, C: AsRef<[u8]>>(path: P, contents: C) -> std::io::Result<()> {
    let path = path.as_ref();
    assert_installed_header_is_ascii(path, contents.as_ref());
    if let Ok(existing) = std::fs::read(path) {
        if existing == contents.as_ref() {
            return Ok(());
        }
    }
    std::fs::write(path, contents)
}

pub fn assert_installed_header_is_ascii(path: &Path, contents: &[u8]) {
    let installed = path.extension().is_some_and(|e| e == "h")
        && path.parent().and_then(Path::file_name).is_some_and(|d| d == "include");
    if !installed {
        return;
    }
    let mut lines = contents.split(|&b| b == b'\n').enumerate();
    if let Some((n, line)) = lines.find(|(_, l)| !l.is_ascii()) {
        panic!(
            "{}:{}: installed header must be ASCII (MSVC decodes a BOM-less header in the \
             consumer's code page); first non-ASCII line: {}",
            path.display(),
            n + 1,
            String::from_utf8_lossy(line)
        );
    }
}

/// `std::fs::copy`, except an identical destination is left alone.
///
/// Same reason as [`write_if_changed`]: the hand-written Rust template modules
/// are copied verbatim on every run, and a fresh mtime on three files inside
/// the crate is enough to make cargo rebuild the whole library.
pub fn copy_if_changed(src: &Path, dest: &Path) -> std::io::Result<()> {
    let new = std::fs::read(src)?;
    if let Ok(existing) = std::fs::read(dest) {
        if existing == new {
            return Ok(());
        }
    }
    std::fs::write(dest, new)
}
