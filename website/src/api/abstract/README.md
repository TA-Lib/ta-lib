---
title: Abstract API
description: "Look up any TA-Lib function by name at run time, read its inputs, parameters, outputs and flags, and call it without naming it at compile time, in C, Rust, Java and C#."
toc: false
---

**TL;DR:** Every function describes itself at run time and can be called by name. Use it when the function or its parameters are not fixed in your code.

## When to use it

- Generating wrappers or glue code for another language.
- Picking up new functions after a TA-Lib upgrade with no code change.
- Building a charting app: the indicator menu and each settings dialog come from the metadata.
- Searching over functions and parameters (optimizers, strategy search).

If you only need a handful of specific functions, calling them directly is simpler.

## Describe a function

Look a function up by name, or enumerate them all. Each one reports its group, its inputs, its optional parameters (type, range, default) and its outputs, with flags. Every flag is listed in the specification: [Abstract API and Metadata](/spec/abstract/#flags).

::: code-tabs#lang

@tab C

```c
#include <ta_abstract.h>

const TA_FuncHandle *handle;
const TA_FuncInfo *info;
const TA_OptInputParameterInfo *opt;

TA_GetFuncHandle( "SMA", &handle );
TA_GetFuncInfo( handle, &info );          /* name, group, hint, flags, nbInput, nbOptInput, nbOutput */
TA_GetOptInputParameterInfo( handle, 0, &opt );   /* displayName, type, dataSet (range), defaultValue */

/* Every function: TA_ForEachFunc( callback, opaqueData ); */
```

@tab Rust

```rust
use ta_lib::abstract_api::{for_each_func, get_func_handle};

let id = get_func_handle("SMA").expect("unknown function");
let info = id.info();   // name, group, hint, flags, inputs, opt_inputs, outputs

for_each_func(|f| println!("{} ({:?})", f.name, f.group));
```

@tab Java

```java
import io.github.talib.metadata.FuncInfo;
import io.github.talib.metadata.Functions;

FuncInfo f = Functions.byName("SMA");   // name(), group(), hint(), flags(), inputs(), optInputs(), outputs()

Functions.all().forEach(fi -> System.out.println(fi.name() + " (" + fi.group() + ")"));
```

@tab C\#

```csharp
using TALib;
using TALib.Metadata;

FuncInfo f = Core.Functions["SMA"];   // Name, Group, Hint, Flags, Inputs, OptInputs, Outputs

foreach (var fi in Core.Functions)
{
    Console.WriteLine($"{fi.Name} ({fi.Group})");
}
```

:::

The name lookup ignores case. `Core.Functions` in C# is an alias for `FunctionCatalog.Default`.

## Call a function

Bind the inputs, the optional parameters and the outputs to a parameter holder, then call it. A parameter left unset takes its default.

::: code-tabs#lang

@tab C

```c
TA_ParamHolder *params;
int outBeg, outNbElement;

TA_ParamHolderAlloc( handle, &params );
TA_SetInputParamRealPtr( params, 0, close );
TA_SetOptInputParamInteger( params, 0, 30 );
TA_SetOutputParamRealPtr( params, 0, out );

TA_CallFunc( params, 0, size - 1, &outBeg, &outNbElement );
TA_ParamHolderFree( params );
```

@tab Rust

```rust
let core = Core::new();
let mut out = vec![0.0; close.len()];

let mut call = get_func_handle("SMA").expect("unknown function").new_call(&core);
call.set_input(0, &close)?;
call.set_opt_input(0, 30)?;
call.set_output(0, &mut out)?;

let range = call.call(0, close.len() - 1)?;
```

@tab Java

```java
OutRange r = Functions.byName("SMA").newCall()
    .setInput(0, close)
    .setOptInput(0, 30)
    .setOutput(0, out)
    .call(0, close.length - 1);
```

@tab C\#

```csharp
OutRange r = Core.Functions["SMA"].CreateCall()
    .SetInput(0, close)
    .SetOptInput(0, 30)
    .SetOutput(0, outReal)
    .Call(0, close.Length - 1);
```

:::

The call behaves like the typed one: same values, same range, same rejections. A parameter holder is not thread-safe: confine one to one thread, or build one per call. The exact rules: [Abstract API and Metadata](/spec/abstract/).

## Lookback and display shift

The holder answers both from the optional parameters alone, so neither needs the inputs or outputs bound. The lookback sizes the outputs; the display shift tells a chart where to draw one output.

::: code-tabs#lang

@tab C

```c
int lookback, shift;

TA_GetFuncHandle( "DPO", &handle );
TA_ParamHolderAlloc( handle, &params );
TA_SetOptInputParamInteger( params, 0, 20 );

TA_GetLookback( params, &lookback );
TA_GetDisplayShift( params, 0, &shift );   /* output 0: -11 */
TA_ParamHolderFree( params );
```

@tab Rust

```rust
let core = Core::new();
let mut call = get_func_handle("DPO").expect("unknown function").new_call(&core);
call.set_opt_input(0, 20)?;

let lookback = call.lookback()?;
let shift = call.display_shift(0)?;   // output 0: -11
```

@tab Java

```java
ParamHolder call = Functions.byName("DPO").newCall().setOptInput(0, 20);

int lookback = call.lookback();
int shift = call.displayShift(0);   // output 0: -11
```

@tab C\#

```csharp
ParamHolder call = Core.Functions["DPO"].CreateCall().SetOptInput(0, 20);

int lookback = call.Lookback();
int shift = call.DisplayShift(0);   // output 0: -11
```

:::

A function with at least one shifted output carries the display-shift flag, and so does each such output, so a generic client can skip the query for every other function. The exact rules: [Lookback and Shift](/spec/lookback/).
