/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * This file is part of the TA-LIB project.
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
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF,CC    Mario Fortier, Claude (Anthropic AI)
 */
package io.github.talib;

/**
 * {@code startIdx} or {@code endIdx} is outside {@code [0, }{@link
 * Core#MAX_INDEX}{@code ]}, or {@code endIdx} precedes {@code startIdx}.
 *
 * <p>An {@link IndexOutOfBoundsException}, which is what the API documents and
 * what a caller catches; {@link #retCode()} distinguishes
 * {@link RetCode#OutOfRangeStartIndex} from {@link RetCode#OutOfRangeEndIndex},
 * which the type alone cannot.
 */
public class TaLibIndexException extends IndexOutOfBoundsException implements TaLibFailure {
   private static final long serialVersionUID = 1L;

   private final RetCode retCode;

   TaLibIndexException(String message, RetCode retCode) {
      super(message);
      this.retCode = retCode;
   }

   @Override
   public RetCode retCode() {
      return retCode;
   }
}
