---
title: Abstract API
description: "Every TA-Lib function describes itself at run time in C, Rust, Java, C#, TypeScript and Go: list them, read each one's inputs, parameters, outputs and flags, and call it by name, so settings dialogs, parameter searches and generated bindings need no code per function."
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

Look a function up by name, or enumerate them all. Each one reports its group, its inputs, its optional parameters (type, range, default) and its outputs, with flags, plus a JSON string of attributes no flag can carry, such as RSI's 30/70 reference levels. The specification lists every [flag](/spec/abstract/#flags) and [attribute](/spec/abstract/#attributes).

::: code-tabs#lang

@tab C

```c
#include <ta_abstract.h>

static void printIfTranscendental( const TA_FuncInfo *fi, void *opaqueData )
{
   if( fi->flags & TA_FUNC_FLG_USES_TRANSCENDENTAL )
      printf( "%s (%s)\n", fi->name, fi->group );
}

const TA_FuncHandle *handle;
const TA_FuncInfo *info;
const TA_OptInputParameterInfo *opt;
const TA_IntegerRange *range;
const char *json;

TA_GetFuncHandle( "SMA", &handle );
TA_GetFuncInfo( handle, &info );          /* name, group, hint, flags, nbInput, nbOptInput, nbOutput */
TA_GetFuncAttributes( handle, &json );            /* "{}"; RSI: {"outputs":{"outReal":{"levels":[30,70]}}} */

int overlay = (info->flags & TA_FUNC_FLG_OVERLAP) != 0;   /* 1: drawn over the price chart */

TA_GetOptInputParameterInfo( handle, 0, &opt );   /* paramName "optInTimePeriod" */
if( opt->type == TA_OptInput_IntegerRange )
{
   range = (const TA_IntegerRange *)opt->dataSet;
   printf( "%d %d %g\n", range->min, range->max, opt->defaultValue );   /* 1 100000 30 */
}

/* Every function that uses a transcendental math function. */
TA_ForEachFunc( printIfTranscendental, NULL );
```

@tab Rust

```rust
use ta_lib::abstract_api::{for_each_func, get_func_handle, FuncFlags, OptInputType};

let id = get_func_handle("SMA").expect("unknown function");
let info = id.info();   // name, group, hint, flags, inputs, opt_inputs, outputs, attributes

let overlay = info.flags.contains(FuncFlags::OVERLAP);   // true: drawn over the price chart

let opt = &info.opt_inputs[0];   // param_name "optInTimePeriod"
if let OptInputType::IntegerRange { min, max, default, .. } = opt.kind {
    println!("{min} {max} {default}");   // 1 100000 30
}

// Every function that uses a transcendental math function.
for_each_func(|f| {
    if f.flags.contains(FuncFlags::USES_TRANSCENDENTAL) {
        println!("{} ({:?})", f.name, f.group);
    }
});
```

@tab Java

```java
import io.github.talib.metadata.FuncFlags;
import io.github.talib.metadata.FuncInfo;
import io.github.talib.metadata.Functions;
import io.github.talib.metadata.OptInputInfo;
import io.github.talib.metadata.OptInputType;

FuncInfo f = Functions.byName("SMA");   // name(), group(), hint(), flags(), inputs(), optInputs(), outputs(), attributes()

boolean overlay = f.hasFlags(FuncFlags.OVERLAP_STUDY);   // true: drawn over the price chart

OptInputInfo opt = f.optInputs().get(0);   // paramName() "optInTimePeriod"
if (opt.type() == OptInputType.INTEGER_RANGE) {
    System.out.println(opt.intMin() + " " + opt.intMax() + " " + opt.defaultValue());   // 1 100000 30.0
}

// Every function that uses a transcendental math function.
for (FuncInfo fi : Functions.all()) {
    if (fi.hasFlags(FuncFlags.USES_TRANSCENDENTAL)) {
        System.out.println(fi.name() + " (" + fi.group() + ")");
    }
}
```

@tab C\#

```csharp
using TALib;
using TALib.Metadata;

FuncInfo f = Core.Functions["SMA"];   // Name, Group, Hint, Flags, Inputs, OptInputs, Outputs, Attributes

bool overlay = f.Flags.HasFlag(FuncFlags.Overlap);   // true: drawn over the price chart

OptInputInfo opt = f.OptInputs[0];   // ParamName "optInTimePeriod"
if (opt.Domain is OptInputDomain.IntegerRange d)
{
    Console.WriteLine($"{d.Min} {d.Max} {d.Default}");   // 1 100000 30
}

// Every function that uses a transcendental math function.
foreach (var fi in Core.Functions)
{
    if (fi.Flags.HasFlag(FuncFlags.UsesTranscendental))
    {
        Console.WriteLine($"{fi.Name} ({fi.Group})");
    }
}
```

@tab TypeScript

```ts
import { FuncFlags, Functions, OptInputType } from 'ta-lib/abstract';   // or 'ta-lib/js/abstract', which never loads WebAssembly

const f = Functions.byName('SMA')!;   // name, group, hint, flags, inputs, optInputs, outputs, attributes

const overlay = f.hasFlags(FuncFlags.OVERLAP_STUDY);   // true: drawn over the price chart

const opt = f.optInputs[0];   // paramName 'optInTimePeriod'
if (opt.type === OptInputType.INTEGER_RANGE) {
  console.log(opt.min, opt.max, opt.defaultValue);   // 1 100000 30
}

// Every function that uses a transcendental math function.
for (const fi of Functions.all()) {
  if (fi.hasFlags(FuncFlags.USES_TRANSCENDENTAL)) {
    console.log(`${fi.name} (${fi.group})`);
  }
}
```

@tab Go

```go
import "ta-lib.github.io/go/abstract"

f, ok := abstract.Lookup("SMA")   // Name(), Group(), Hint(), Flags(), Inputs(), OptInputs(), Outputs(), Attributes()

overlay := f.HasFlags(abstract.FuncOverlap)   // true: drawn over the price chart

opt := f.OptInputs()[0]   // ParamName "optInTimePeriod"
if d, isRange := opt.Domain.(abstract.IntegerRange); isRange {
	fmt.Println(d.Min, d.Max, d.Default)   // 1 100000 30
}

// Every function that uses a transcendental math function.
for _, fi := range abstract.All() {
	if fi.HasFlags(abstract.FuncUsesTranscendental) {
		fmt.Printf("%s (%s)\n", fi.Name(), fi.Group())
	}
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

@tab TypeScript

```ts
const r = Functions.byName('SMA')!.newCall()
  .setInput(0, close)
  .setOptInput('timePeriod', 30)   // a parameter by its options key, or by position: 0
  .setOutput(0, out)
  .call();                         // the whole series, as the typed call's defaults
```

@tab Go

```go
f, _ := abstract.Lookup("SMA")
call := f.NewCall(talib.NewCore())
if err := errors.Join(
	call.SetInput(0, closes),
	call.SetOptInputInteger(0, 30),
	call.SetOutput(0, out),
); err != nil {
	log.Fatal(err)
}

r, err := call.Call(0, len(closes)-1)
```

:::

The call behaves like the typed one: same values, same range, same rejections. In TypeScript every row also carries the typed call's `key` (`timePeriod`, `prices`, `outUpperBand`), and an optional parameter binds by that key or by position. A parameter holder is not thread-safe: confine one to one thread, or build one per call. In Go each setter returns its error, a `talib.RetCode`, so the calls do not chain. The exact rules: [Abstract API and Metadata](/spec/abstract/).

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

@tab TypeScript

```ts
const call = Functions.byName('DPO')!.newCall().setOptInput(0, 20);

const lookback = call.lookback();
const shift = call.displayShift(0);   // output 0: -11
```

@tab Go

```go
f, _ := abstract.Lookup("DPO")
call := f.NewCall(talib.NewCore())
if err := call.SetOptInputInteger(0, 20); err != nil {
	log.Fatal(err)
}

lookback, err := call.Lookback()
shift, err := call.DisplayShift(0)   // output 0: -11
```

:::

A function with at least one shifted output carries the display-shift flag, and so does each such output, so a generic client can skip the query for every other function. The exact rules: [Lookback and Shift](/spec/lookback/).
