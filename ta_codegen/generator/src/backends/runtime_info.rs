//! `TA_GetRuntimeInfo`'s analog in Rust, Java, C# and TypeScript, and the key
//! table `ta_regtest` asks every server from.
//!
//! A common key is answered in every backend, so a caller written against one
//! probes the others without a rejection; no backend but C has an optional path
//! a common key reports, nor a `TA_Initialize`, so each answers 0. A key that
//! describes what only one backend has is answered there alone, and every other
//! backend rejects it as unknown.

use std::fmt::Write as _;
use std::path::Path;

/// Which backends answer a key.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Scope {
    /// Every backend, C included.
    Common,
    /// TypeScript alone. `answer` is the expression computing it, and `max` the
    /// largest value it takes: `ta_regtest` holds the server to `0..=max`.
    TypeScript { answer: &'static str, max: usize },
}

/// One key, in C's spelling.
#[derive(Clone, Copy, Debug)]
pub struct Key {
    pub name: &'static str,
    pub scope: Scope,
}

/// `wasm.reason`'s values from 1 on, in order: `runtime/wasm.ts`'s `EngineReason`.
/// The rendered table is a `Record<EngineReason, number>`, so `tsc` rejects a
/// list that misses or invents a reason.
pub const WASM_REASONS: &[&str] = &["not-loaded", "unavailable", "compile", "instantiate", "fma", "trap"];

/// Every key: the one declaration of which backends answer it.
pub const KEYS: &[Key] = &[
    Key { name: "vmath.transcendental", scope: Scope::Common },
    Key { name: "count.initialize", scope: Scope::Common },
    Key { name: "count.shutdown", scope: Scope::Common },
    Key {
        name: "wasm.engine",
        scope: Scope::TypeScript { answer: "engineStatus().engine === 'wasm' ? 1 : 0", max: 1 },
    },
    Key {
        name: "wasm.reason",
        scope: Scope::TypeScript { answer: "reasonCode(engineStatus().reason)", max: WASM_REASONS.len() },
    },
];

/// The keys every backend answers.
pub fn common() -> impl Iterator<Item = &'static str> {
    KEYS.iter().filter(|k| k.scope == Scope::Common).map(|k| k.name)
}

/// Where the keys are described. The one description: no emitted doc repeats it.
const SPEC: &str = "https://ta-lib.org/spec/abstract/#runtime-info";

fn quoted(separator: &str) -> String {
    common().map(|k| format!("\"{k}\"")).collect::<Vec<_>>().join(separator)
}

/// `abstract_api::get_runtime_info`.
pub fn rust_fn() -> String {
    format!(
        "\n/// Rust analog of C's `TA_GetRuntimeInfo()`: a run-time state of this library, by key.\n\
         /// The keys: <{SPEC}>.\n\
         ///\n\
         /// An unknown key is [`RetCode::BadParam`].\n\
         pub fn get_runtime_info(key: &str) -> Result<i32, crate::RetCode> {{\n\
         \x20   match key {{\n\
         \x20       {keys} => Ok(0),\n\
         \x20       _ => Err(crate::RetCode::BadParam),\n\
         \x20   }}\n\
         }}\n",
        keys = quoted(" | "),
    )
}

/// The body of `io.github.talib.metadata.RuntimeInfo`, after the file header.
pub fn java_class() -> String {
    format!(
        "import io.github.talib.RetCode;\n\
         import io.github.talib.TALibArgumentException;\n\n\
         /**\n\
         \x20* Run-time state of this library, by key.\n\
         \x20*\n\
         \x20* <p>The Java analog of C's {{@code TA_GetRuntimeInfo()}}. The keys:\n\
         \x20* <a href=\"{SPEC}\">{SPEC}</a>.\n\
         \x20*/\n\
         public final class RuntimeInfo {{\n\
         \x20   private RuntimeInfo() {{\n\
         \x20   }}\n\n\
         \x20   /**\n\
         \x20    * The value of one key.\n\
         \x20    *\n\
         \x20    * @param key the key, spelled as in C\n\
         \x20    * @return the value\n\
         \x20    * @throws TALibArgumentException for an unknown or null key, carrying\n\
         \x20    *         {{@code RetCode.BAD_PARAM}}\n\
         \x20    */\n\
         \x20   public static int get(String key) {{\n\
         \x20       if (key != null) {{\n\
         \x20           switch (key) {{\n\
         \x20               case {keys}:\n\
         \x20                   return 0;\n\
         \x20               default:\n\
         \x20                   break;\n\
         \x20           }}\n\
         \x20       }}\n\
         \x20       throw new TALibArgumentException(\"unknown runtime info key: \" + key, RetCode.BAD_PARAM);\n\
         \x20   }}\n\
         }}\n",
        keys = quoted(", "),
    )
}

/// The body of `TALib.Metadata.RuntimeInfo`, after the file header.
pub fn csharp_class(namespace: &str) -> String {
    format!(
        "namespace {namespace};\n\n\
         /// <summary>\n\
         /// Run-time state of this library, by key.\n\
         /// </summary>\n\
         /// <remarks>\n\
         /// The C# analog of C's <c>TA_GetRuntimeInfo()</c>. The keys:\n\
         /// <see href=\"{SPEC}\"/>.\n\
         /// </remarks>\n\
         public static class RuntimeInfo\n{{\n\
         \x20   /// <summary>The value of one key.</summary>\n\
         \x20   /// <param name=\"key\">The key, spelled as in C.</param>\n\
         \x20   /// <exception cref=\"TALibArgumentException\">An unknown or null key, carrying\n\
         \x20   /// <c>RetCode.BadParam</c>.</exception>\n\
         \x20   public static int Get(string key)\n\
         \x20   {{\n\
         \x20       if (TryGet(key, out int value)) return value;\n\
         \x20       throw new TALibArgumentException($\"unknown runtime info key: {{key}}\", nameof(key), RetCode.BadParam);\n\
         \x20   }}\n\n\
         \x20   /// <summary><see cref=\"Get\"/> without the exception.</summary>\n\
         \x20   /// <param name=\"key\">The key, spelled as in C.</param>\n\
         \x20   /// <param name=\"value\">The value, or 0 for an unknown or null key.</param>\n\
         \x20   /// <returns>Whether the key is known.</returns>\n\
         \x20   public static bool TryGet(string? key, out int value)\n\
         \x20   {{\n\
         \x20       value = 0;\n\
         \x20       return key is {keys};\n\
         \x20   }}\n\
         }}\n",
        keys = quoted(" or "),
    )
}

/// The Java server's handler: the shipped class, its rejection back as a code.
pub const JAVA_SERVER_HANDLER: &str =
        "    static String handleGetRuntimeInfo(String json) {\n\
        \x20       int rc = 0;\n\
        \x20       int value = 0;\n\
        \x20       try {\n\
        \x20           value = io.github.talib.metadata.RuntimeInfo.get(jsonString(json, \"key\"));\n\
        \x20       } catch (io.github.talib.TALibArgumentException e) {\n\
        \x20           rc = e.retCode().asCInt();\n\
        \x20       }\n\
        \x20       return \"{\\\"retCode\\\":\" + rc + \",\\\"value\\\":\" + value + \"}\";\n\
        \x20   }\n\n";

/// The TypeScript `getRuntimeInfo`. It needs `TALibError`, `RetCode`,
/// `engineStatus` and the type `EngineReason` in scope.
pub fn ts_fn() -> String {
    let mut cases = common().fold(String::new(), |mut acc, k| {
        let _ = writeln!(acc, "    case '{k}':");
        acc
    });
    cases.push_str("      return 0;\n");
    for k in KEYS {
        if let Scope::TypeScript { answer, .. } = k.scope {
            let _ = write!(cases, "    case '{}':\n      return {answer};\n", k.name);
        }
    }
    let reasons = WASM_REASONS.iter().zip(1..).fold(String::new(), |mut acc, (r, code)| {
        let _ = writeln!(acc, "  '{r}': {code},");
        acc
    });
    let union = KEYS.iter().map(|k| format!("'{}'", k.name)).collect::<Vec<_>>().join(" | ");
    format!(
        "const WASM_REASON: Readonly<Record<EngineReason, number>> = {{\n\
         {reasons}\
         }};\n\n\
         function reasonCode(reason: EngineReason | null): number {{\n\
         \x20 return reason === null ? 0 : WASM_REASON[reason];\n\
         }}\n\n\
         /** The keys {{@link getRuntimeInfo}} answers. */\n\
         export type RuntimeInfoKey = {union};\n\n\
         /**\n\
         \x20* Run-time state of this library, by key: the TypeScript analog of C's\n\
         \x20* `TA_GetRuntimeInfo()`. The keys: {SPEC}.\n\
         \x20*\n\
         \x20* @param key - One of the keys above; any other string is rejected at run time.\n\
         \x20* @throws `TALibError` with code `TA_BAD_PARAM` for an unknown key.\n\
         \x20*/\n\
         export function getRuntimeInfo(key: RuntimeInfoKey | (string & {{}})): number {{\n\
         \x20 switch (key) {{\n\
         {cases}\
         \x20   default:\n\
         \x20     throw new TALibError(RetCode.BAD_PARAM, `unknown runtime info key: ${{key}}`);\n\
         \x20 }}\n\
         }}\n"
    )
}

/// The TypeScript server's handler: the library function, its rejection back as a code.
pub const TS_SERVER_HANDLER: &str = "function handleGetRuntimeInfo(p: Params): string {\n\
    \x20 let rc = 0;\n\
    \x20 let value = 0;\n\
    \x20 try {\n\
    \x20   value = getRuntimeInfo(String(p['key']));\n\
    \x20 } catch (e) {\n\
    \x20   rc = failureCode(e);\n\
    \x20 }\n\
    \x20 return `{\"retCode\":${rc},\"value\":${value}}`;\n\
    }\n\n";

/// `src/tools/ta_regtest/ta_runtime_info_keys.h`: [`KEYS`] for `ta_regtest`.
pub fn regtest_header() -> String {
    let mut o = String::from(super::variant_frame::LICENSE);
    o.push_str(
        "/* AUTO-GENERATED by ta_codegen. DO NOT EDIT.\n\
         * Source of truth: ta_codegen/generator/src/backends/runtime_info.rs\n\
         * (regenerate: cd ta_codegen/generator && cargo run -- generate)\n\
         *\n\
         * backend is \"\" for a key every backend answers, else the --backend=\n\
         * name of the one backend that answers it; max bounds that key's value.\n\
         */\n\n\
         #ifndef TA_RUNTIME_INFO_KEYS_H\n\
         #define TA_RUNTIME_INFO_KEYS_H\n\n\
         typedef struct\n{\n   const char *key;\n   const char *backend;\n   int max;\n} RuntimeInfoKeyRow;\n\n\
         static const RuntimeInfoKeyRow TA_RUNTIME_INFO_KEYS[] = {\n",
    );
    for k in KEYS {
        let (backend, max) = match k.scope {
            Scope::Common => ("", 0),
            Scope::TypeScript { max, .. } => ("ts", max),
        };
        let _ = writeln!(o, "   {{ \"{}\", \"{backend}\", {max} }},", k.name);
    }
    o.push_str("};\n\n#endif /* TA_RUNTIME_INFO_KEYS_H */\n");
    o
}

/// Write [`regtest_header`].
pub fn generate_regtest_header(root: &Path) {
    let path = root.join("src/tools/ta_regtest/ta_runtime_info_keys.h");
    super::write_if_changed_silent(&path, &regtest_header());
}
