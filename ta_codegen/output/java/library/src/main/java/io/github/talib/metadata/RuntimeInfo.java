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

import io.github.talib.RetCode;
import io.github.talib.TALibArgumentException;

/**
 * Run-time state of this library, by key.
 *
 * <p>The Java analog of C's {@code TA_GetRuntimeInfo()}. The keys:
 * <a href="https://ta-lib.org/spec/abstract/#runtime-info">https://ta-lib.org/spec/abstract/#runtime-info</a>.
 */
public final class RuntimeInfo {
    private RuntimeInfo() {
    }

    /**
     * The value of one key.
     *
     * @param key the key, spelled as in C
     * @return the value
     * @throws TALibArgumentException for an unknown or null key, carrying
     *         {@code RetCode.BAD_PARAM}
     */
    public static int get(String key) {
        if (key != null) {
            switch (key) {
                case "vmath.transcendental", "count.initialize", "count.shutdown":
                    return 0;
                default:
                    break;
            }
        }
        throw new TALibArgumentException("unknown runtime info key: " + key, RetCode.BAD_PARAM);
    }
}
