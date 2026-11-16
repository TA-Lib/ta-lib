/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Hand-written: ta_codegen never writes this file. Values are ta_regtest's,
 * through the JSON-RPC server; this holds what the server cannot reach. */

package abstract

import (
	"errors"
	"go/ast"
	"go/doc"
	"go/parser"
	"go/token"
	"math"
	"path/filepath"
	"sort"
	"strings"
	"testing"

	talib "ta-lib.github.io/go"
)

func series(n int, base float64) []float64 {
	s := make([]float64, n)
	for i := range s {
		s[i] = base + float64(i%17) - float64(i%5)*0.25
	}
	return s
}

func lookup(t *testing.T, name string) FuncInfo {
	t.Helper()
	f, ok := Lookup(name)
	if !ok {
		t.Fatalf("Lookup(%q) found nothing", name)
	}
	return f
}

func same(a, b []float64) bool {
	if len(a) != len(b) {
		return false
	}
	for i := range a {
		if math.Float64bits(a[i]) != math.Float64bits(b[i]) {
			return false
		}
	}
	return true
}

func TestRegistry(t *testing.T) {
	all := All()
	if !sort.SliceIsSorted(all, func(i, j int) bool { return all[i].Name() < all[j].Name() }) {
		t.Error("All is not in name order")
	}
	total := 0
	for _, g := range Groups() {
		total += len(InGroup(g))
		if strings.HasPrefix(g.String(), "Group(") {
			t.Errorf("group %d has no name", int(g))
		}
	}
	if total != len(all) {
		t.Errorf("the groups hold %d functions, All %d", total, len(all))
	}
	for _, f := range all {
		got, ok := Lookup(strings.ToLower(f.Name()))
		if !ok || got.Name() != f.Name() {
			t.Errorf("Lookup(%q) = %v, %v", strings.ToLower(f.Name()), got, ok)
		}
		if _, ok := f.UnstableID(); ok != f.HasFlags(FuncUnstablePeriod) {
			t.Errorf("%s: UnstableID %v against its flag", f, ok)
		}
	}
	for _, name := range []string{"", "NOPE", "SMA ", "ſma"} {
		f, ok := Lookup(name)
		if ok {
			t.Errorf("Lookup(%q) found a function", name)
		}
		if _, ok := f.UnstableID(); ok {
			t.Errorf("Lookup(%q): the zero FuncInfo has an unstable-period id", name)
		}
	}
}

func TestDescription(t *testing.T) {
	f := lookup(t, "BBANDS")
	if f.Group() != GroupOverlapStudies || f.Group().String() != "Overlap Studies" {
		t.Errorf("BBANDS group %v", f.Group())
	}
	in := f.Inputs()
	if len(in) != 1 || in[0].Kind != InputReal || in[0].ParamName != "inReal" {
		t.Errorf("BBANDS inputs %+v", in)
	}
	opts := f.OptInputs()
	if len(opts) != 4 || opts[0].ParamName != "optInTimePeriod" || opts[0].DefaultValue != 20 {
		t.Fatalf("BBANDS optional parameters %+v", opts)
	}
	if d, ok := opts[0].Domain.(IntegerRange); !ok || opts[0].Kind != OptIntegerRange || d.Default != 20 || len(d.Sweep()) == 0 {
		t.Errorf("BBANDS period domain %+v", opts[0].Domain)
	}
	if d, ok := opts[1].Domain.(RealRange); !ok || opts[1].Kind != OptRealRange || len(d.Sweep()) == 0 {
		t.Errorf("BBANDS deviation domain %+v", opts[1].Domain)
	}
	d, ok := opts[3].Domain.(IntegerList)
	if !ok || !strings.HasPrefix(d.ValueList(), "0=SMA;1=EMA;") {
		t.Errorf("BBANDS MA type domain %+v", opts[3].Domain)
	}
	if out := f.Outputs(); len(out) != 3 || out[0].Kind != OutputReal || out[0].Flags&OutUpperLimit == 0 {
		t.Errorf("BBANDS outputs %+v", out)
	}
	adx := lookup(t, "ADX").Inputs()
	if len(adx) != 1 || adx[0].Kind != InputPrice || !adx[0].Requires(PriceHigh|PriceLow|PriceClose) ||
		adx[0].Requires(PriceOpen) || len(adx[0].SignatureOrder) != 3 {
		t.Errorf("ADX inputs %+v", adx)
	}
	if lookup(t, "RSI").Attributes() == "{}" || lookup(t, "SMA").Attributes() != "{}" {
		t.Error("attributes")
	}
}

func TestAccessorsReturnCopies(t *testing.T) {
	f := lookup(t, "MA")
	opts := f.OptInputs()
	opts[1].Domain.(IntegerList).Values[0].Name = "changed"
	opts[0].ParamName = "changed"
	lookup(t, "ADX").Inputs()[0].SignatureOrder[0] = PriceOpen
	again := lookup(t, "MA").OptInputs()
	if again[0].ParamName != "optInTimePeriod" || again[1].Domain.(IntegerList).Values[0].Name != "SMA" {
		t.Errorf("a caller's change reached the registry: %+v", again)
	}
	if lookup(t, "ADX").Inputs()[0].SignatureOrder[0] != PriceHigh {
		t.Error("a caller's change reached the registry")
	}
}

func TestCallMatchesDirect(t *testing.T) {
	const n = 200
	core := talib.NewCore()
	in := series(n, 100)
	want := make([]float64, n)
	wr, err := core.SMA(0, n-1, in, 20, want)
	if err != nil {
		t.Fatal(err)
	}
	h := lookup(t, "SMA").NewCall(core)
	got := make([]float64, n)
	for _, err := range []error{h.SetInput(0, in), h.SetOptInputByName("optInTimePeriod", 20), h.SetOutput(0, got)} {
		if err != nil {
			t.Fatal(err)
		}
	}
	if lb, err := h.Lookback(); lb != 19 || err != nil {
		t.Errorf("Lookback: %d, %v", lb, err)
	}
	r, err := h.Call(0, n-1)
	if err != nil || r != wr || !same(got[:r.NBElement], want[:wr.NBElement]) {
		t.Errorf("SMA through the holder: %+v, %v", r, err)
	}

	hi, lo, cl := series(n, 102), series(n, 98), series(n, 100)
	wk, wd := make([]float64, n), make([]float64, n)
	wr, err = core.STOCH(0, n-1, hi, lo, cl, 5, 3, talib.MATypeEMA, 3, talib.MATypeSMA, wk, wd)
	if err != nil {
		t.Fatal(err)
	}
	h = lookup(t, "STOCH").NewCall(core)
	gk, gd := make([]float64, n), make([]float64, n)
	for _, err := range []error{
		h.SetPriceInput(0, PriceInput{Open: cl, High: hi, Low: lo, Close: cl}),
		h.SetOptInputInteger(0, 5), h.SetOptInputMAType(2, talib.MATypeEMA),
		h.SetOutput(0, gk), h.SetOutput(1, gd),
	} {
		if err != nil {
			t.Fatal(err)
		}
	}
	r, err = h.Call(0, n-1)
	if err != nil || r != wr || !same(gk[:r.NBElement], wk[:wr.NBElement]) || !same(gd[:r.NBElement], wd[:wr.NBElement]) {
		t.Errorf("STOCH through the holder: %+v, %v", r, err)
	}
}

// A rejected re-bind must leave the bundle that worked, not a mixture of the
// two: the holder validates every component before it writes one.
func TestPriceSetterValidatesBeforeWriting(t *testing.T) {
	const n = 120
	hi, lo, cl := series(n, 102), series(n, 98), series(n, 100)
	h := lookup(t, "ADX").NewCall(nil)
	before := make([]float64, n)
	if err := h.SetPriceInput(0, PriceInput{High: hi, Low: lo, Close: cl}); err != nil {
		t.Fatal(err)
	}
	if err := h.SetOutput(0, before); err != nil {
		t.Fatal(err)
	}
	if _, err := h.Call(0, n-1); err != nil {
		t.Fatal(err)
	}
	other := series(n, 500)
	if err := h.SetPriceInput(0, PriceInput{High: other, Low: nil, Close: other}); !errors.Is(err, talib.BadParam) {
		t.Fatalf("a missing component: %v", err)
	}
	after := make([]float64, n)
	if err := h.SetOutput(0, after); err != nil {
		t.Fatal(err)
	}
	if _, err := h.Call(0, n-1); err != nil || !same(before, after) {
		t.Errorf("the rejected bundle changed the call: %v", err)
	}
}

func TestRejections(t *testing.T) {
	const n = 50
	in := series(n, 100)
	h := lookup(t, "SMA").NewCall(nil)
	check := func(what string, err, want error) {
		t.Helper()
		if !errors.Is(err, want) {
			t.Errorf("%s: %v, want %v", what, err, want)
		}
	}
	_, err := h.Call(0, n-1)
	check("unbound input", err, talib.InputNotAllInitialize)
	check("bind input", h.SetInput(0, in), nil)
	_, err = h.Call(0, n-1)
	check("unbound output", err, talib.OutputNotAllInitialize)
	check("input index", h.SetInput(1, in), talib.BadParam)
	check("nil input", h.SetInput(0, nil), talib.BadParam)
	check("integer input slot", h.SetIntInput(0, []int32{1}), talib.InvalidParamHolderType)
	check("price input slot", h.SetPriceInput(0, PriceInput{}), talib.InvalidParamHolderType)
	check("real parameter slot", h.SetOptInputReal(0, 3), talib.InvalidParamHolderType)
	check("MA type on a period", h.SetOptInputMAType(0, talib.MATypeEMA), talib.InvalidParamHolderType)
	check("unknown name", h.SetOptInputByName("optInNothing", 3), talib.BadParam)
	check("fraction", h.SetOptInputByName("optInTimePeriod", 2.5), talib.BadParam)
	check("NaN", h.SetOptInputByName("optInTimePeriod", math.NaN()), talib.BadParam)
	check("beyond int32", h.SetOptInputByName("optInTimePeriod", 1e10), talib.BadParam)
	check("integer output slot", h.SetIntOutput(0, make([]int32, n)), talib.InvalidParamHolderType)
	check("short output accepted at bind", h.SetOutput(0, make([]float64, 5)), nil)
	_, err = h.Call(0, n-1)
	check("short output", err, talib.BadParam)
	check("bind output", h.SetOutput(0, make([]float64, n)), nil)
	check("short input accepted at bind", h.SetInput(0, in[:10]), nil)
	_, err = h.Call(0, n-1)
	check("short input", err, talib.BadParam)
	check("period out of range accepted at bind", h.SetOptInputInteger(0, 0), nil)
	_, err = h.Lookback()
	check("lookback, period 0", err, talib.BadParam)
	_, err = h.DisplayShift(0)
	check("display shift, period 0", err, talib.BadParam)
	check("default sentinel", h.SetOptInputInteger(0, talib.IntegerDefault), nil)
	if lb, err := h.Lookback(); lb != 29 || err != nil {
		t.Errorf("Lookback at the default: %d, %v", lb, err)
	}
	var zero ParamHolder
	_, err = zero.Call(0, 1)
	check("zero holder", err, talib.InvalidParamHolder)
	_, err = zero.Lookback()
	check("zero holder lookback", err, talib.InvalidParamHolder)
	_, err = zero.DisplayShift(0)
	check("zero holder display shift", err, talib.InvalidParamHolder)
}

func TestDisplayShift(t *testing.T) {
	core := talib.NewCore()
	h := lookup(t, "DPO").NewCall(core)
	if err := h.SetOptInputInteger(0, 20); err != nil {
		t.Fatal(err)
	}
	if ds, err := h.DisplayShift(0); ds != -11 || err != nil {
		t.Errorf("DPO(20) output 0: %d, %v", ds, err)
	}
	for _, idx := range []int{-1, 1} {
		if _, err := h.DisplayShift(idx); !errors.Is(err, talib.BadParam) {
			t.Errorf("DPO output %d: %v", idx, err)
		}
	}
	for _, f := range All() {
		h := f.NewCall(core)
		for o, out := range f.Outputs() {
			ds, err := h.DisplayShift(o)
			if err != nil || ds != 0 && out.Flags&OutDisplayShift == 0 {
				t.Errorf("%s output %d: %d, %v", f, o, ds, err)
			}
		}
	}
}

func TestRuntimeInfo(t *testing.T) {
	for _, key := range []string{"vmath.transcendental", "count.initialize", "count.shutdown"} {
		if v, err := RuntimeInfo(key); v != 0 || err != nil {
			t.Errorf("%s: %d, %v", key, v, err)
		}
	}
	if _, err := RuntimeInfo("wasm.engine"); !errors.Is(err, talib.BadParam) {
		t.Errorf("a key Go does not answer: %v", err)
	}
	if !strings.HasPrefix(FunctionDescriptionXML(), "<?xml") {
		t.Error("FunctionDescriptionXML")
	}
}

func goFiles(t *testing.T) []string {
	t.Helper()
	names, err := filepath.Glob("*.go")
	if err != nil {
		t.Fatal(err)
	}
	var out []string
	for _, name := range names {
		if !strings.HasSuffix(name, "_test.go") {
			out = append(out, name)
		}
	}
	if len(out) < 5 {
		t.Fatalf("found %d files", len(out))
	}
	return out
}

func TestNoUnsafeOrReflectImport(t *testing.T) {
	for _, name := range goFiles(t) {
		f, err := parser.ParseFile(token.NewFileSet(), name, nil, parser.ImportsOnly)
		if err != nil {
			t.Fatal(err)
		}
		for _, im := range f.Imports {
			if im.Path.Value == `"unsafe"` || im.Path.Value == `"reflect"` {
				t.Errorf("%s imports %s", name, im.Path.Value)
			}
		}
	}
}

// pkg.go.dev and go doc show what this sees: every exported identifier's doc
// comment, starting with its name, and every exported field's.
func TestEveryExportedIdentifierIsDocumented(t *testing.T) {
	fset := token.NewFileSet()
	var files []*ast.File
	for _, name := range goFiles(t) {
		f, err := parser.ParseFile(fset, name, nil, parser.ParseComments)
		if err != nil {
			t.Fatal(err)
		}
		files = append(files, f)
	}
	p, err := doc.NewFromFiles(fset, files, "ta-lib.github.io/go/abstract")
	if err != nil {
		t.Fatal(err)
	}
	checked := 0
	starts := func(name, text string) {
		checked++
		if !strings.HasPrefix(text, name+" ") {
			first, _, _ := strings.Cut(text, "\n")
			t.Errorf("%s: doc %q does not start with its name", name, first)
		}
	}
	values := func(vs []*doc.Value) {
		for _, v := range vs {
			checked++
			if strings.TrimSpace(v.Doc) == "" {
				t.Errorf("%v: no doc comment", v.Names)
			}
		}
	}
	starts("Package", p.Doc)
	values(p.Consts)
	values(p.Vars)
	for _, f := range p.Funcs {
		starts(f.Name, f.Doc)
	}
	for _, typ := range p.Types {
		starts(typ.Name, typ.Doc)
		values(typ.Consts)
		values(typ.Vars)
		for _, f := range typ.Funcs {
			starts(f.Name, f.Doc)
		}
		for _, m := range typ.Methods {
			starts(m.Name, m.Doc)
		}
		ts := typ.Decl.Specs[0].(*ast.TypeSpec)
		if st, ok := ts.Type.(*ast.StructType); ok {
			for _, field := range st.Fields.List {
				for _, n := range field.Names {
					if n.IsExported() {
						checked++
						if field.Doc == nil {
							t.Errorf("%s.%s: no doc comment", typ.Name, n.Name)
						}
					}
				}
			}
		}
	}
	if checked < 100 {
		t.Fatalf("checked only %d identifiers", checked)
	}
	t.Logf("%d exported identifiers, groups and fields documented", checked)
}
