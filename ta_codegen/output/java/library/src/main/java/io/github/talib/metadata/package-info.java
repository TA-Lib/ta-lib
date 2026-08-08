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

/* GENERATED FILE — do not edit. Produced by ta_codegen
 * (generator/src/backends/java_metadata.rs) from ta_codegen/input/.
 * MF,CC
 */

/**
 * Runtime introspection: what the library's functions are, what they take,
 * and how to call one whose name is not known until run time.
 *
 * <p>The Java face of the C library's {@code ta_abstract} layer, and
 * generated from the same definitions as the indicators themselves, so a
 * row here cannot describe a method that does not exist.
 *
 * <p>{@link io.github.talib.metadata.Functions#all()} enumerates every function;
 * {@link io.github.talib.metadata.Functions#byName(java.lang.String)} looks one up.
 * Each {@link io.github.talib.metadata.FunctionInfo} carries its group, its flags and its
 * input/optional-input/output descriptors, and mints a
 * {@link io.github.talib.metadata.ParamHolder} to bind arguments into:
 *
 * <pre>{@code
 * FunctionInfo rsi = Functions.byName("RSI");
 * double[] out = new double[close.length];
 * OutRange r = rsi.newCall()
 *     .setInput(0, close)
 *     .setOptInput(0, 14)
 *     .setOutput(0, out)
 *     .call(0, close.length - 1);
 * }</pre>
 *
 * <p>Binding is checked, not reflective: the dispatch onto the typed method
 * is a generated {@code switch}, so the library stays AOT- and
 * jlink-friendly and a signature change is a compile error rather than a
 * run-time one. The typed API on {@link io.github.talib.Core} remains the
 * faster and safer choice whenever the function is known at compile time.
 */
package io.github.talib.metadata;

