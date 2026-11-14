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

package talib

import (
	"errors"
	"go/ast"
	"go/doc"
	"go/parser"
	"go/token"
	"math"
	"path/filepath"
	"strconv"
	"strings"
	"testing"
)

func series(n int) []float64 {
	s := make([]float64, n)
	for i := range s {
		s[i] = 100 + float64(i%17) - float64(i%5)*0.25
	}
	return s
}

func TestPublicTier(t *testing.T) {
	const n = 100
	in := series(n)
	c := NewCore()
	out := make([]float64, n)
	check := func(what string, err, want error) {
		t.Helper()
		if want == nil && err != nil || want != nil && !errors.Is(err, want) {
			t.Errorf("%s: %v, want %v", what, err, want)
		}
	}
	_, err := c.SMA(0, n-1, in, 10, out)
	check("success", err, nil)
	_, err = c.SMA(0, n-1, in, 0, out)
	check("period 0", err, BadParam)
	if strconv.IntSize == 64 {
		_, err = c.SMA(0, n-1, in, math.MaxInt32+1, out)
		check("period beyond int32", err, BadParam)
	}
	_, err = c.SMA(0, n-1, in, 10, out[:10])
	check("short output", err, BadParam)
	_, err = c.SMA(0, n-1, in, 10, nil)
	check("absent output", err, BadParam)
	_, err = c.SMA(0, n-1, in[:n-1], 10, out)
	check("short input", err, BadParam)
	_, err = c.SMA(-1, n-1, in, 10, out)
	check("negative start", err, OutOfRangeStartIndex)
	_, err = c.MA(0, n-1, in, 10, MAType(99), out)
	check("MA type out of range", err, BadParam)
	_, err = c.BBANDS(0, n-1, in, 10, 2, 2, MATypeSMA, out, out, make([]float64, n))
	check("two outputs at one address", err, BadParam)
	buf := append([]float64(nil), in...)
	_, err = c.SMA(0, n-1, buf, 10, buf[1:])
	check("output partly over its input", err, BadParam)
	_, err = c.SMA(0, n-2, buf[1:], 10, buf[:n-1])
	check("output partly before its input", err, BadParam)
	halves := append([]float64(nil), in...)
	_, err = c.SMA(0, 9, halves[:10], 10, halves[50:])
	check("disjoint halves of one array", err, nil)
	want, _ := c.SMA(0, n-1, in, 10, out)
	got, err := c.SMA(0, n-1, buf, 10, buf)
	check("in place", err, nil)
	if got != want {
		t.Errorf("in place: %+v, want %+v", got, want)
	}
	for i := 0; i < want.NBElement; i++ {
		if math.Float64bits(buf[i]) != math.Float64bits(out[i]) {
			t.Fatalf("in place [%d]: %v, want %v", i, buf[i], out[i])
		}
	}
}

func TestLookback(t *testing.T) {
	c := NewCore()
	if lb, err := c.SMALookback(30); lb != 29 || err != nil {
		t.Errorf("SMALookback(30): %d, %v", lb, err)
	}
	if lb, err := c.SMALookback(IntegerDefault); lb != 29 || err != nil {
		t.Errorf("SMALookback(IntegerDefault): %d, %v", lb, err)
	}
	if lb, err := c.SMALookback(0); lb != 0 || !errors.Is(err, BadParam) {
		t.Errorf("SMALookback(0): %d, %v", lb, err)
	}
	if strconv.IntSize == 64 {
		if lb, err := c.SMALookback(math.MaxInt32 + 1); lb != 0 || !errors.Is(err, BadParam) {
			t.Errorf("SMALookback(MaxInt32+1): %d, %v", lb, err)
		}
	}
}

func TestNoUnsafeOrReflectImport(t *testing.T) {
	files, err := filepath.Glob("*.go")
	if err != nil || len(files) < 100 {
		t.Fatalf("found %d files: %v", len(files), err)
	}
	for _, name := range files {
		if strings.HasSuffix(name, "_test.go") {
			continue
		}
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

func TestStreamShapes(t *testing.T) {
	c := NewCore()
	in := series(120)
	out := make([]float64, 120)
	r, _ := c.SMA(0, 119, in, 10, out)
	s, last, err := c.SMAOpen(in[:100], 10)
	if err != nil || math.Float64bits(last) != math.Float64bits(out[99-r.BegIdx]) {
		t.Fatalf("SMAOpen: %v, %v", last, err)
	}
	p, err := s.Peek(in[100])
	if err != nil {
		t.Fatal(err)
	}
	v, err := s.Update(in[100])
	if err != nil || math.Float64bits(v) != math.Float64bits(out[100-r.BegIdx]) || math.Float64bits(p) != math.Float64bits(v) {
		t.Errorf("SMA Update %v Peek %v, want %v (%v)", v, p, out[100-r.BegIdx], err)
	}
	for _, bad := range []float64{math.NaN(), math.Inf(1), math.Inf(-1)} {
		before := s.OutRange()
		if _, err := s.Update(bad); !errors.Is(err, BadParam) {
			t.Errorf("Update(%v): %v", bad, err)
		}
		if _, err := s.Peek(bad); !errors.Is(err, BadParam) {
			t.Errorf("Peek(%v): %v", bad, err)
		}
		if s.OutRange() != before {
			t.Errorf("a rejected bar moved the range")
		}
	}

	mc, ms, mh := make([]float64, 120), make([]float64, 120), make([]float64, 120)
	r, _ = c.MACD(0, 119, in, 12, 26, 9, mc, ms, mh)
	h, v0, v1, v2, err := c.MACDOpen(in[:100], 12, 26, 9)
	if err != nil || math.Float64bits(v0) != math.Float64bits(mc[99-r.BegIdx]) || math.Float64bits(v2) != math.Float64bits(mh[99-r.BegIdx]) {
		t.Fatalf("MACDOpen: %v %v %v, %v", v0, v1, v2, err)
	}
	u0, u1, u2, err := h.Update(in[100])
	if err != nil || math.Float64bits(u1) != math.Float64bits(ms[100-r.BegIdx]) {
		t.Errorf("MACD Update: %v %v %v, %v", u0, u1, u2, err)
	}
	if _, _, _, err := h.Peek(math.NaN()); !errors.Is(err, BadParam) {
		t.Errorf("MACD Peek(NaN): %v", err)
	}
}

// A handle that peeked, and one whose clone was fed other bars, must answer
// what an untouched twin answers: a ring (SMA) and sub-handles (STOCH).
func TestPeekAndCloneLeaveTheHandle(t *testing.T) {
	c := NewCore()
	cl := series(160)
	hi, lo := make([]float64, 160), make([]float64, 160)
	for i := range cl {
		hi[i], lo[i] = cl[i]+2, cl[i]-2
	}

	ref, _, _ := c.SMAOpen(cl[:60], 10)
	peeked, _, _ := c.SMAOpen(cl[:60], 10)
	for k := 0; k < 5; k++ {
		_, _ = peeked.Peek(1e6 + float64(k))
	}
	src, _, _ := c.SMAOpen(cl[:60], 10)
	fork := src.Clone()
	for bar := 60; bar < 80; bar++ {
		_, _ = fork.Update(2 * cl[bar])
	}
	for bar := 60; bar < 160; bar++ {
		want, _ := ref.Update(cl[bar])
		for i, h := range []*SMAStream{peeked, src} {
			if got, _ := h.Update(cl[bar]); math.Float64bits(got) != math.Float64bits(want) {
				t.Fatalf("SMA bar %d: handle %d answers %v, want %v", bar, i, got, want)
			}
		}
	}

	open := func() *STOCHStream {
		s, _, _, err := c.STOCHOpen(hi[:60], lo[:60], cl[:60], 5, 3, MATypeEMA, 3, MATypeSMA)
		if err != nil {
			t.Fatal(err)
		}
		return s
	}
	sref, speeked, ssrc := open(), open(), open()
	for k := 0; k < 5; k++ {
		_, _, _ = speeked.Peek(1e6, -1e6, float64(k))
	}
	sfork := ssrc.Clone()
	for bar := 60; bar < 80; bar++ {
		_, _, _ = sfork.Update(hi[bar]+50, lo[bar]-50, cl[bar])
	}
	for bar := 60; bar < 160; bar++ {
		k0, d0, _ := sref.Update(hi[bar], lo[bar], cl[bar])
		for i, h := range []*STOCHStream{speeked, ssrc} {
			k, d, _ := h.Update(hi[bar], lo[bar], cl[bar])
			if math.Float64bits(k) != math.Float64bits(k0) || math.Float64bits(d) != math.Float64bits(d0) {
				t.Fatalf("STOCH bar %d: handle %d answers %v %v, want %v %v", bar, i, k, d, k0, d0)
			}
		}
	}
}

func TestOpenAndFillRejectsAliasing(t *testing.T) {
	c := NewCore()
	in := series(100)
	buf := append([]float64(nil), in...)
	if _, _, err := c.SMAOpenAndFill(buf, 10, buf); !errors.Is(err, BadParam) {
		t.Errorf("output is the input: %v", err)
	}
	if _, _, err := c.SMAOpenAndFill(buf[:91], 10, buf[9:]); !errors.Is(err, BadParam) {
		t.Errorf("output partly over the input: %v", err)
	}
	out := make([]float64, 400)
	if _, _, err := c.MACDOpenAndFill(in, 12, 26, 9, out[:100], out[:100], out[200:300]); !errors.Is(err, BadParam) {
		t.Errorf("two outputs at one address: %v", err)
	}
	if _, _, err := c.MACDOpenAndFill(in, 12, 26, 9, out[:100], out[50:150], out[200:300]); !errors.Is(err, BadParam) {
		t.Errorf("two outputs overlapping: %v", err)
	}
	if _, _, err := c.MACDOpenAndFill(in, 12, 26, 9, out[:100], out[100:200], out[200:300]); err != nil {
		t.Errorf("disjoint outputs: %v", err)
	}
}

// pkg.go.dev and go doc show what this sees: every exported identifier's doc
// comment, starting with its name.
func TestEveryExportedIdentifierIsDocumented(t *testing.T) {
	names, err := filepath.Glob("*.go")
	if err != nil {
		t.Fatal(err)
	}
	fset := token.NewFileSet()
	var files []*ast.File
	for _, name := range names {
		if strings.HasSuffix(name, "_test.go") {
			continue
		}
		f, err := parser.ParseFile(fset, name, nil, parser.ParseComments)
		if err != nil {
			t.Fatal(err)
		}
		files = append(files, f)
	}
	p, err := doc.NewFromFiles(fset, files, "ta-lib.github.io/go")
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
			if !strings.HasPrefix(m.Doc, m.Name+" ") {
				t.Errorf("%s.%s: doc does not start with its name", typ.Name, m.Name)
			}
			checked++
		}
	}
	if checked < 1000 {
		t.Fatalf("checked only %d identifiers", checked)
	}
	t.Logf("%d exported identifiers and groups documented", checked)
}
