fn main() {
    let root = std::path::Path::new("../input/imi");
    let mut f = ta_codegen_lib::parser::yaml::parse_yaml(&root.join("imi.yaml"));
    let p = ta_codegen_lib::parser::c_source::parse_c_source(&root.join("imi.c"));
    ta_codegen_lib::parser::c_source::wire_parsed_source(&mut f, &p);
    for s in &f.body {
        let d = format!("{s:?}");
        if d.contains("ForC") {
            println!("{}", &d[..d.len().min(700)]);
        }
    }
}
