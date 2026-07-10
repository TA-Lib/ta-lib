// dump the If stmt containing memmove from rsi.c
fn main() {
    let root = std::path::Path::new("../input/rsi");
    let mut f = ta_codegen_lib::parser::yaml::parse_yaml(&root.join("rsi.yaml"));
    let p = ta_codegen_lib::parser::c_source::parse_c_source(&root.join("rsi.c"));
    ta_codegen_lib::parser::c_source::wire_parsed_source(&mut f, &p);
    for s in &f.body {
        let d = format!("{s:?}");
        if d.contains("emmove") || d.contains("ArrayCopy") || d.contains("ARRAY_COPY") {
            println!("{d}");
        }
    }
}
