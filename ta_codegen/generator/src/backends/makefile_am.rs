use crate::ir::FuncDef;
use std::path::Path;

/// Generate `src/ta_func/Makefile.am` from function definitions.
///
/// Uses the shared [`super::sorted_source_stems`] list (every `ta_codegen/input/`
/// function plus the un-ported `src/ta_func/` stubs like NVI/PVI) so the autotools
/// source set matches the CMake `LIB_SOURCES`. Writes only if content has changed.
pub fn generate(funcs: &[FuncDef], out_path: &Path, root: &Path) {
    let (names, _extras) = super::sorted_source_stems(funcs, root);

    let mut content = String::new();

    // Header (matches the old Makefile.am.template)
    content.push_str(
        "\nnoinst_LTLIBRARIES = libta_func.la\n\
         AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/ta_common\n\
         \n\
         libta_func_la_SOURCES = ta_utility.c \\\n",
    );

    // Function source entries
    for (i, name) in names.iter().enumerate() {
        if i + 1 < names.len() {
            content.push_str(&format!("\tta_{name}.c \\\n"));
        } else {
            content.push_str(&format!("\tta_{name}.c\n"));
        }
    }

    super::write_if_changed(out_path, &content, "Makefile.am", names.len());
}
