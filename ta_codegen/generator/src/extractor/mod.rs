pub mod func_extractor;
pub mod table_parser;

/// A fully resolved TA function definition extracted from the C abstract tables.
#[derive(Debug, Clone, PartialEq)]
pub struct TableFuncDef {
    pub name: String,
    pub group: String,
    pub hint: String,
    pub flags: Vec<String>,
    pub inputs: Vec<InputDef>,
    pub optional_inputs: Vec<OptInputDef>,
    pub outputs: Vec<OutputDef>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct InputDef {
    pub name: String,
    pub param_type: String,
    pub price_flags: Vec<String>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct OptInputDef {
    pub name: String,
    pub param_type: String,
    pub display_name: String,
    pub hint: String,
    pub range: Option<(f64, f64)>,
    pub default: Option<f64>,
    pub suggested: Option<(f64, f64, f64)>,
    pub flags: Vec<String>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct OutputDef {
    pub name: String,
    pub param_type: String,
    pub flags: Vec<String>,
}
