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
 *  110226 MF,CC  First Version
 */

// Test files are found by glob, so a lost file or test would just drop out of
// the run. The manifest must name exactly the hand-written files and how many
// tests each registers, so adding or losing one is a manifest edit.

import assert from 'node:assert/strict';
import { readdirSync, readFileSync } from 'node:fs';
import { test } from 'node:test';

import { FuncFlags, Functions } from '../src/metadata/index.ts';

const TESTS: Record<string, number> = {
  'batchArgs.test.ts': 10,
  'batchContract.test.ts': 10,
  'engineBytes.test.ts': 2,
  'engineContract.test.ts': 20,
  'engineLoader.test.ts': 5,
  'engineParity.test.ts': 2,
  'engineRouting.test.ts': 2,
  'engineTraps.test.ts': 3,
  'inputForms.test.ts': 1,
  'mavpBank.test.ts': 2,
  'metadata.test.ts': 12,
  'metadataSweep.test.ts': 6,
  'options.test.ts': 8,
  'phantomIo.test.ts': 5,
  'publicApi.test.ts': 6,
  'runtime.test.ts': 14,
  'stream.test.ts': 6,
  'streamContract.test.ts': 13,
  'suite.test.ts': 2,
};

const DIR = new URL('./', import.meta.url);
const GEN = new URL('gen/', DIR);
const read = (url: URL): string => readFileSync(url, 'utf8');
const testCalls = (src: string): string[] => src.match(/^\s*test\(\s*(['`]).*?\1/gm) ?? [];
const NOT_RUN = /\btest\.(skip|todo|only)\(|[{,]\s*(skip|todo|only)\s*:/;

test('every hand-written test file is in the manifest and registers exactly its count of tests', () => {
  const files = readdirSync(DIR).filter((f) => f.endsWith('.test.ts')).sort();
  assert.deepEqual(files, Object.keys(TESTS).sort());
  for (const f of files) {
    const src = read(new URL(f, DIR));
    const n = testCalls(src).length;
    assert.equal(n, TESTS[f], `${f}: ${n} tests, manifest ${TESTS[f]}`);
    assert.doesNotMatch(src, NOT_RUN, `${f}: a test that does not run`);
  }
});

test('the generated example tests run every registry function\'s documented examples', () => {
  const names = new Set<string>();
  for (const f of readdirSync(GEN).filter((g) => g.endsWith('.test.ts'))) {
    const src = read(new URL(f, GEN));
    assert.doesNotMatch(src, NOT_RUN, `gen/${f}: a test that does not run`);
    for (const call of testCalls(src)) names.add(call.replace(/^\s*test\(\s*['`]/, '').slice(0, -1));
  }
  let expected = 0;
  for (const f of Functions.all()) {
    const prefix = `${f.name}: `;
    const mine = [...names].filter((n) => n.startsWith(prefix));
    const want = f.hasFlags(FuncFlags.STREAMING) ? 3 : 1;
    assert.equal(mine.length, want, `${f.name}: ${mine.join(' | ')}`);
    expected += want;
  }
  assert.equal(names.size, expected);
});
