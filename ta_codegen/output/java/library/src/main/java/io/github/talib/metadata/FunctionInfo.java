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

package io.github.talib.metadata;

import io.github.talib.Core;
import java.util.List;

/**
 * Everything the library knows about one indicator's guarded,
 * double-precision batch form.
 *
 * @param name          the canonical upper-case name, e.g. {@code SMA}
 * @param group         the functional group, e.g. {@code Overlap Studies}
 * @param hint          a one-line description ({@code ""} when none)
 * @param flags         see {@link FuncFlags}
 * @param inputs        inputs in call order
 * @param optInputs     optional parameters in call order
 * @param outputs       outputs in call order
 */
public record FunctionInfo(
       String name,
       String group,
       String hint,
       int flags,
       List<InputInfo> inputs,
       List<OptInputInfo> optInputs,
       List<OutputInfo> outputs) {

    /** Whether every bit in {@code mask} is set (see {@link FuncFlags}). */
    public boolean hasFlags(int mask) {
       return (flags & mask) == mask;
    }

    /**
     * Begins a call to this function with arguments bound at run time,
     * against {@link Core#DEFAULT}. See {@link ParamHolder}.
     */
    public ParamHolder newCall() {
       return new ParamHolder(this, Core.DEFAULT);
    }

    /**
     * Begins a call to this function with arguments bound at run time,
     * against a specific {@link Core}. See {@link ParamHolder}.
     */
    public ParamHolder newCall(Core core) {
       return new ParamHolder(this, core);
    }
}
