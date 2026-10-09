//! `TA_GetRuntimeInfo`'s analog in Rust, Java and C#.
//!
//! A key C answers is answered in every language, so a caller written against
//! one probes the others without a rejection. None of the three has an optional
//! path a key reports, nor a `TA_Initialize`: each answers 0.

/// Every key, in C's spelling. `runtime_info_keys_are_c_s` holds it to the C source.
pub const KEYS: &[&str] = &["vmath.transcendental", "count.initialize", "count.shutdown"];

/// Where the keys are described. The one description: no emitted doc repeats it.
const SPEC: &str = "https://ta-lib.org/spec/abstract/#runtime-info";

fn quoted(separator: &str) -> String {
    KEYS.iter().map(|k| format!("\"{k}\"")).collect::<Vec<_>>().join(separator)
}

/// `abstract_api::get_runtime_info`.
pub fn rust_fn() -> String {
    format!(
        "\n/// Rust analog of C's `TA_GetRuntimeInfo()`: a run-time state of this library, by key.\n\
         /// It promises no speed and no value. The keys: <{SPEC}>.\n\
         ///\n\
         /// An unknown key is [`RetCode::BadParam`](crate::RetCode::BadParam).\n\
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
         \x20* <p>The Java analog of C's {{@code TA_GetRuntimeInfo()}}. It promises no speed and\n\
         \x20* no value. The keys: <a href=\"{SPEC}\">{SPEC}</a>.\n\
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
         /// The C# analog of C's <c>TA_GetRuntimeInfo()</c>. It promises no speed and no\n\
         /// value. The keys: <see href=\"{SPEC}\"/>.\n\
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
