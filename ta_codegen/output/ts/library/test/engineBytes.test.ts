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
 *  111226 MF,CC  First Version
 */

// The default entry from the source tree, where the build has not written the
// engine's bytes module: ready() finds nothing to import.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import * as defaultEntry from '../src/engine/index.ts';
import * as jsEntry from '../src/index.ts';
import { engineStatus, fallThroughCount, setEngineMode } from '../src/runtime/wasm.ts';

test('ready() resolves to the TypeScript engine when the bytes cannot be imported, and every call still answers', async () => {
  assert.deepEqual(await defaultEntry.ready(), { engine: 'js', reason: 'unavailable' });
  assert.deepEqual(engineStatus(), { engine: 'js', reason: 'unavailable' });
  const close = Float64Array.from({ length: 64 }, (_, i) => 10 + (i % 5));
  setEngineMode('force');
  try {
    assert.deepEqual(defaultEntry.sma(close, { timePeriod: 4 }), jsEntry.sma(close, { timePeriod: 4 }));
  } finally {
    setEngineMode('table');
  }
  assert.equal(fallThroughCount(), 1);
});

test('the default entry exports the names of the ta-lib/js barrel, plus ready', () => {
  const names = Object.keys(jsEntry).sort();
  assert.deepEqual(Object.keys(defaultEntry).sort(), [...names, 'ready'].sort());
});
