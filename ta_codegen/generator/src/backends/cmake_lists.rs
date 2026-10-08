use crate::ir::FuncDef;
use std::path::Path;

const BEGIN_MARKER: &str = "# [ta_codegen begin LIB_SOURCES]";
const END_MARKER: &str = "# [ta_codegen end LIB_SOURCES]";

/// Generate the `set(LIB_SOURCES ...)` section of `CMakeLists.txt`.
///
/// Uses begin/end markers to identify and replace only the generated section,
/// leaving the rest of CMakeLists.txt unchanged.
///
/// Sources included (sorted alphabetically):
/// - `src/ta_func/ta_utility.c` (hand-written helper, always first)
/// - `src/ta_func/ta_NAME.c` for each function in `ta_codegen/input/` (from `funcs`)
/// - `src/ta_abstract/frames/ta_frame.c`
/// - `src/ta_abstract/ta_func_api.c`
/// - `src/ta_abstract/ta_group_idx.c`
pub fn generate(funcs: &[FuncDef], cmake_path: &Path) {
    let names = super::sorted_source_stems(funcs);

    // Build the replacement block
    let mut block = String::new();
    block.push_str(BEGIN_MARKER);
    block.push('\n');
    block.push_str("set(LIB_SOURCES\n");

    // ta_utility.c is a hand-written helper — always first
    block.push_str("\t\"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_utility.c\"\n");

    for name in &names {
        block.push_str(&format!(
            "\t\"${{CMAKE_CURRENT_SOURCE_DIR}}/src/ta_func/ta_{name}.c\"\n"
        ));
    }

    // Fixed ta_abstract sources that CMake also needs
    block.push_str("\t\"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/frames/ta_frame.c\"\n");
    block.push_str("\t\"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_func_api.c\"\n");
    block.push_str("\t\"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_group_idx.c\"\n");
    block.push(')');
    block.push('\n');
    block.push_str(END_MARKER);

    // Read existing CMakeLists.txt
    let existing = std::fs::read_to_string(cmake_path)
        .unwrap_or_else(|_| panic!("Cannot read {}", cmake_path.display()));

    let begin_pos = existing.find(BEGIN_MARKER).unwrap_or_else(|| {
        panic!(
            "Cannot find '{}' in {}",
            BEGIN_MARKER,
            cmake_path.display()
        )
    });
    let end_pos = existing.find(END_MARKER).unwrap_or_else(|| {
        panic!(
            "Cannot find '{}' in {}",
            END_MARKER,
            cmake_path.display()
        )
    });
    let end_pos = end_pos + END_MARKER.len();

    let new_content = format!(
        "{}{}{}",
        &existing[..begin_pos],
        block,
        &existing[end_pos..]
    );

    super::write_if_changed(cmake_path, &new_content, "CMakeLists.txt LIB_SOURCES", names.len());
}
