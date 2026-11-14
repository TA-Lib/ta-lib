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

import "strconv"

// RetCode is the status of a call, numbered as C's TA_RetCode. A failed call
// returns it as its error; a successful one returns a nil error.
type RetCode int32

// The RetCode values, C's TA_RetCode set.
const (
	Success                RetCode = 0
	LibNotInitialize       RetCode = 1
	BadParam               RetCode = 2
	AllocErr               RetCode = 3
	GroupNotFound          RetCode = 4
	FuncNotFound           RetCode = 5
	InvalidHandle          RetCode = 6
	InvalidParamHolder     RetCode = 7
	InvalidParamHolderType RetCode = 8
	InvalidParamFunction   RetCode = 9
	InputNotAllInitialize  RetCode = 10
	OutputNotAllInitialize RetCode = 11
	OutOfRangeStartIndex   RetCode = 12
	OutOfRangeEndIndex     RetCode = 13
	InvalidListType        RetCode = 14
	BadObject              RetCode = 15
	NotSupported           RetCode = 16
	InsufficientHistory    RetCode = 17
	InternalError          RetCode = 5000
	UnknownErr             RetCode = 0xFFFF
)

var retCodeNames = map[RetCode]string{
	Success:                "TA_SUCCESS",
	LibNotInitialize:       "TA_LIB_NOT_INITIALIZE",
	BadParam:               "TA_BAD_PARAM",
	AllocErr:               "TA_ALLOC_ERR",
	GroupNotFound:          "TA_GROUP_NOT_FOUND",
	FuncNotFound:           "TA_FUNC_NOT_FOUND",
	InvalidHandle:          "TA_INVALID_HANDLE",
	InvalidParamHolder:     "TA_INVALID_PARAM_HOLDER",
	InvalidParamHolderType: "TA_INVALID_PARAM_HOLDER_TYPE",
	InvalidParamFunction:   "TA_INVALID_PARAM_FUNCTION",
	InputNotAllInitialize:  "TA_INPUT_NOT_ALL_INITIALIZE",
	OutputNotAllInitialize: "TA_OUTPUT_NOT_ALL_INITIALIZE",
	OutOfRangeStartIndex:   "TA_OUT_OF_RANGE_START_INDEX",
	OutOfRangeEndIndex:     "TA_OUT_OF_RANGE_END_INDEX",
	InvalidListType:        "TA_INVALID_LIST_TYPE",
	BadObject:              "TA_BAD_OBJECT",
	NotSupported:           "TA_NOT_SUPPORTED",
	InsufficientHistory:    "TA_INSUFFICIENT_HISTORY",
	InternalError:          "TA_INTERNAL_ERROR",
	UnknownErr:             "TA_UNKNOWN_ERR",
}

// Error returns C's enumerator spelling of the code.
func (r RetCode) Error() string {
	if s, ok := retCodeNames[r]; ok {
		return s
	}
	return "TA_RETCODE(" + strconv.Itoa(int(r)) + ")"
}

// OutRange is the range a batch call wrote: BegIdx is the first bar with a
// value, NBElement how many values were written.
type OutRange struct {
	BegIdx    int
	NBElement int
}
