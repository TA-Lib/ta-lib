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


/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  101926 MF,CC  First version.
 */

package io.github.talib;

/**
 * Rule rB9 past the guard: a body's {@code INTERNAL_ERROR} reaches the caller as
 * a {@link TALibStateException} carrying the code, from the batch and the
 * streaming tier. No shipped guard is reachable, so this calls the mappers both
 * tiers throw through.
 */
public final class InternalFailureTest {
    public static void main(String[] args) {
        RuntimeException[] thrown = {
            Core.failure("SMA", RetCode.INTERNAL_ERROR),
            Core.streamFailure("SMA", RetCode.INTERNAL_ERROR),
        };
        int failures = 0;
        for (RuntimeException e : thrown) {
            if (!(e instanceof TALibStateException s) || s.retCode() != RetCode.INTERNAL_ERROR) {
                System.out.println("  FAIL: an internal error is " + e.getClass().getName());
                failures++;
            }
        }
        if (failures > 0) {
            System.out.println("InternalFailureTest: " + failures + " FAILED");
            System.exit(1);
        }
        System.out.println("InternalFailureTest: OK");
    }
}
