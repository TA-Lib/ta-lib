// Every access names its byte order, so a big-endian host reads the same bits.
const view = new DataView(new ArrayBuffer(8));

/** @internal */
export function float64Bits(x: number): bigint {
  view.setFloat64(0, x, true);
  return view.getBigUint64(0, true);
}

/** @internal */
export function float64FromBits(bits: bigint): number {
  view.setBigUint64(0, BigInt.asUintN(64, bits), true);
  return view.getFloat64(0, true);
}

/** @internal */
export function float64Hi(x: number): number {
  view.setFloat64(0, x, true);
  return view.getUint32(4, true);
}

/** @internal */
export function float64Lo(x: number): number {
  view.setFloat64(0, x, true);
  return view.getUint32(0, true);
}

/** @internal */
export function float64FromWords(hi: number, lo: number): number {
  view.setUint32(4, hi, true);
  view.setUint32(0, lo, true);
  return view.getFloat64(0, true);
}

const HEX_BYTE: string[] = [];
for (let i = 0; i < 256; i++) HEX_BYTE.push(i.toString(16).padStart(2, '0'));
const HEX_ONLY = /^[0-9a-fA-F]*$/;

// The wire text of one value is C's `%016llx` of its bits: most significant
// digit first, lower-case. That is not the
// little-endian memory order the output hash reads.
/** @internal */
export function float64ArrayToHex(a: Float64Array, count: number = a.length): string {
  const parts: string[] = new Array<string>(count);
  for (let i = 0; i < count; i++) {
    view.setFloat64(0, a[i], false);
    let s = '';
    for (let k = 0; k < 8; k++) s += HEX_BYTE[view.getUint8(k)];
    parts[i] = s;
  }
  return parts.join('');
}

/** @internal */
export function float64ArrayFromHex(hex: string): Float64Array<ArrayBuffer> {
  checkHex(hex, 16);
  const n = hex.length / 16;
  const out = new Float64Array(n);
  for (let i = 0; i < n; i++) {
    const at = i * 16;
    view.setUint32(0, parseInt(hex.slice(at, at + 8), 16), false);
    view.setUint32(4, parseInt(hex.slice(at + 8, at + 16), 16), false);
    out[i] = view.getFloat64(0, false);
  }
  return out;
}

function checkHex(hex: string, group: number): void {
  if (hex.length % group !== 0 || !HEX_ONLY.test(hex)) {
    throw new RangeError(`not a string of ${group}-digit hex groups`);
  }
}
