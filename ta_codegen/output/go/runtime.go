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

/* Hand-written: ta_codegen never writes this file. */

package talib

import "math"

// Every product below is converted to float64: Go may otherwise fuse it with
// an addition into an FMA the C library does not perform.

func b2i(b bool) int32 {
	if b {
		return 1
	}
	return 0
}

// f2i is C's (int) of a double with Java's answer where C has none: NaN is
// 0 and a value out of range saturates.
func f2i(x float64) int32 {
	switch {
	case x != x:
		return 0
	case x >= math.MaxInt32:
		return math.MaxInt32
	case x <= math.MinInt32:
		return math.MinInt32
	}
	return int32(x)
}

// The C max/min macros, (a > b) ? a : b: NaN and signed zeros resolve as C
// resolves them, which Go's builtin min/max and math.Max/Min do not.

func maxF(a, b float64) float64 {
	if a > b {
		return a
	}
	return b
}

func minF(a, b float64) float64 {
	if a < b {
		return a
	}
	return b
}

func maxI(a, b int32) int32 {
	if a > b {
		return a
	}
	return b
}

func minI(a, b int32) int32 {
	if a < b {
		return a
	}
	return b
}

func isZero(x float64) bool {
	return -0.00000000000001 < x && x < 0.00000000000001
}

func isZeroScaled(x, scale float64) bool {
	return math.Abs(x) <= float64(0.00000000000001*scale)
}

func isZeroOrNeg(x float64) bool {
	return x < 0.00000000000001
}

func isFinite(x float64) bool {
	return !math.IsNaN(x) && !math.IsInf(x, 0)
}

// candleRange and candleAverage are ta_utility.h's TA_CANDLERANGE_OF and
// TA_CANDLEAVERAGE_OF, term for term.
func candleRange(rangeType int32, open, high, low, close float64) float64 {
	switch rangeType {
	case 0:
		return math.Abs(close - open)
	case 1:
		return high - low
	case 2:
		upper, lower := open, close
		if close >= open {
			upper, lower = close, open
		}
		return (high - upper) + (lower - low)
	}
	return 0.0
}

func candleAverage(rangeType, avgPeriod int32, factor, sum, open, high, low, close float64) float64 {
	var avg float64
	if avgPeriod != 0 {
		avg = sum / float64(avgPeriod)
	} else {
		avg = candleRange(rangeType, open, high, low, close)
	}
	divisor := 1.0
	if rangeType == 2 {
		divisor = 2.0
	}
	return float64(factor*avg) / divisor
}

// sameStart is C's pointer equality: both slices start at one element.
func sameStart[T any](a, b []T) bool {
	return len(a) > 0 && len(b) > 0 && &a[0] == &b[0]
}

// overlaps reports whether two slices share an element. Slices of one array
// end at the same element once extended to their capacity, so it is blind
// only to a slice whose capacity a three-index expression cut.
func overlaps[T any](a, b []T) bool {
	if len(a) == 0 || len(b) == 0 || &a[:cap(a)][cap(a)-1] != &b[:cap(b)][cap(b)-1] {
		return false
	}
	aAfterB := cap(b) - cap(a)
	return aAfterB < len(b) && -aAfterB < len(a)
}

// overlapsPartly reports an output sharing memory with an input other than
// the input itself: the same start and length is computing in place.
func overlapsPartly(out, in []float64) bool {
	return overlaps(out, in) && (!sameStart(out, in) || len(out) != len(in))
}

func fitsInt32(v int) bool {
	return v >= math.MinInt32 && v <= math.MaxInt32
}

// requiredOut is how many values a call over [startIdx, endIdx] writes.
func requiredOut(startIdx, endIdx, lookback int) int {
	first := startIdx
	if lookback > first {
		first = lookback
	}
	if first > endIdx {
		return 0
	}
	return endIdx - first + 1
}
