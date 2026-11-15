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

// Package talib is the official TA-Lib in pure Go: every technical analysis function, batch and streaming.
//
// # Batch
//
// Each function is a method of [Core] named as in TA-Lib. It takes the range
// of bars to compute, the inputs (oldest bar first), the parameters and the
// output slices, and returns the [OutRange] it wrote:
//
//	core := talib.NewCore()
//	out := make([]float64, len(closes))
//	r, err := core.SMA(0, len(closes)-1, closes, 30, out)
//
// out[k] is the value at bar r.BegIdx+k, for r.NBElement values. A parameter
// passed as [IntegerDefault] or [RealDefault] takes its default, and a
// Lookback method such as [Core.SMALookback] tells how many leading bars a
// call consumes; a DisplayShift method such as [Core.DPODisplayShift] tells
// how many bars ahead or behind a chart draws one output. Each method's documentation lists its inputs, parameters
// and outputs and links to the function's page on ta-lib.org.
//
// # Streaming
//
// For live data, open a stream on the history and feed it one closed bar at
// a time; each Update returns the value the batch call computes at that bar:
//
//	s, last, err := core.SMAOpen(closes, 30)
//	v, err := s.Update(nextClose)
//
// Peek previews a forming bar without committing it, and Clone forks a
// stream.
//
// # Settings
//
// A [Core] holds what every call reads: the unstable periods
// ([Core.SetUnstablePeriod]) and the candlestick settings
// ([Core.SetCandleSetting]). A zero Core holds the defaults, as the one
// [NewCore] returns does.
//
// # Errors
//
// A call that fails returns a [RetCode] as its error, such as [BadParam] for a
// parameter out of range; test for one with errors.Is.
//
// # Metadata
//
// The package [ta-lib.github.io/go/metadata] describes every function at run
// time and calls one chosen by name: list the functions, read each one's
// inputs, parameters, outputs and flags, then bind the arguments and call it.
// A program that does not import it links none of it. See
// https://ta-lib.org/api/abstract/.
//
// # Goroutines
//
// A Core is safe to share between goroutines once configured: call its setters
// only while no other goroutine uses it and no stream opened on it is in use. A
// stream belongs to one goroutine.
//
// Documentation: https://ta-lib.org/api/go/
package talib
