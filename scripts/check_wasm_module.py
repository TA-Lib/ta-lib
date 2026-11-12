#!/usr/bin/env python3
"""Fails unless a WebAssembly module imports nothing, contains at least one
f64x2.relaxed_madd and no other relaxed-SIMD instruction.

Decodes every instruction of every function body rather than searching for
byte patterns, which immediates and constants would match. An opcode it does
not know fails the check instead of being skipped.

Usage: check_wasm_module.py <module.wasm>
"""
import sys

RELAXED = {
    0x100: 'i8x16.relaxed_swizzle',
    0x101: 'i32x4.relaxed_trunc_f32x4_s',
    0x102: 'i32x4.relaxed_trunc_f32x4_u',
    0x103: 'i32x4.relaxed_trunc_f64x2_s_zero',
    0x104: 'i32x4.relaxed_trunc_f64x2_u_zero',
    0x105: 'f32x4.relaxed_madd',
    0x106: 'f32x4.relaxed_nmadd',
    0x107: 'f64x2.relaxed_madd',
    0x108: 'f64x2.relaxed_nmadd',
    0x109: 'i8x16.relaxed_laneselect',
    0x10a: 'i16x8.relaxed_laneselect',
    0x10b: 'i32x4.relaxed_laneselect',
    0x10c: 'i64x2.relaxed_laneselect',
    0x10d: 'f32x4.relaxed_min',
    0x10e: 'f32x4.relaxed_max',
    0x10f: 'f64x2.relaxed_min',
    0x110: 'f64x2.relaxed_max',
    0x111: 'i16x8.relaxed_q15mulr_s',
    0x112: 'i16x8.relaxed_dot_i8x16_i7x16_s',
    0x113: 'i32x4.relaxed_dot_i8x16_i7x16_add_s',
}
ALLOWED = 0x107
SIMD_UNASSIGNED = {0x9a, 0xa2, 0xa5, 0xa6, 0xaf, 0xb0, 0xb2, 0xb3, 0xb4, 0xbb,
                   0xc2, 0xc5, 0xc6, 0xcf, 0xd0, 0xd2, 0xd3, 0xd4, 0xe2, 0xee}


class Reader:
    def __init__(self, data, pos=0, end=None):
        self.data, self.pos = data, pos
        self.end = len(data) if end is None else end

    def byte(self):
        if self.pos >= self.end:
            raise ValueError(f'read past the end at offset {self.pos}')
        b = self.data[self.pos]
        self.pos += 1
        return b

    def u(self):
        result = shift = 0
        while True:
            b = self.byte()
            result |= (b & 0x7f) << shift
            shift += 7
            if not b & 0x80:
                return result

    def s(self):
        result = shift = 0
        while True:
            b = self.byte()
            result |= (b & 0x7f) << shift
            shift += 7
            if not b & 0x80:
                return result - (1 << shift) if b & 0x40 else result

    def skip(self, n):
        if self.pos + n > self.end:
            raise ValueError(f'read past the end at offset {self.pos}')
        self.pos += n

    def memarg(self):
        align = self.u()
        if align & 0x40:
            self.u()
        self.u()


def instruction(r, relaxed):
    op = r.byte()
    if op in (0x02, 0x03, 0x04, 0x06):
        r.s()
    elif op in (0x07, 0x08, 0x09, 0x0c, 0x0d, 0x10, 0x12, 0x18,
                0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0xd2):
        r.u()
    elif op == 0x0e:
        for _ in range(r.u() + 1):
            r.u()
    elif op in (0x11, 0x13):
        r.u()
        r.u()
    elif op == 0x1c:
        r.skip(r.u())
    elif 0x28 <= op <= 0x3e:
        r.memarg()
    elif op in (0x3f, 0x40):
        r.u()
    elif op == 0x41 or op == 0x42:
        r.s()
    elif op == 0x43:
        r.skip(4)
    elif op == 0x44:
        r.skip(8)
    elif op == 0xd0:
        r.byte()
    elif op == 0xfc:
        sub = r.u()
        if sub in (8, 12, 14):
            r.u()
            r.u()
        elif sub in (9, 10, 11, 13, 15, 16, 17):
            r.u()
            if sub == 10:
                r.u()
        elif sub > 17:
            raise ValueError(f'unknown opcode 0xfc {sub} at offset {r.pos}')
    elif op == 0xfd:
        sub = r.u()
        if sub <= 11 or sub in (0x5c, 0x5d):
            r.memarg()
        elif sub in (12, 13):
            r.skip(16)
        elif 21 <= sub <= 34:
            r.byte()
        elif 0x54 <= sub <= 0x5b:
            r.memarg()
            r.byte()
        elif sub in RELAXED:
            relaxed[sub] = relaxed.get(sub, 0) + 1
        elif sub > 0xff or sub in SIMD_UNASSIGNED:
            raise ValueError(f'unknown opcode 0xfd {sub:#x} at offset {r.pos}')
    elif op == 0xfe:
        sub = r.u()
        if sub == 0x03:
            r.byte()
        elif sub <= 0x02 or 0x10 <= sub <= 0x4e:
            r.memarg()
        else:
            raise ValueError(f'unknown opcode 0xfe {sub:#x} at offset {r.pos}')
    elif op in (0x00, 0x01, 0x05, 0x0b, 0x0f, 0x19, 0x1a, 0x1b, 0xd1) or 0x45 <= op <= 0xc4:
        pass
    else:
        raise ValueError(f'unknown opcode {op:#x} at offset {r.pos - 1}')


def scan(data):
    """(import count, {relaxed opcode: count}, function body count)."""
    if data[:8] != b'\0asm\x01\0\0\0':
        raise ValueError('not a WebAssembly 1.0 binary module')
    r = Reader(data, 8)
    imports, relaxed, bodies = 0, {}, 0
    while r.pos < r.end:
        sid = r.byte()
        size = r.u()
        start = r.pos
        if sid == 2:
            imports = Reader(data, start, start + size).u()
        elif sid == 10:
            sec = Reader(data, start, start + size)
            for _ in range(sec.u()):
                body_size = sec.u()
                body = Reader(data, sec.pos, sec.pos + body_size)
                for _ in range(body.u()):
                    body.u()
                    body.byte()
                while body.pos < body.end:
                    instruction(body, relaxed)
                sec.pos = body.end
                bodies += 1
        r.pos = start + size
    return imports, relaxed, bodies


def main():
    if len(sys.argv) != 2:
        print(__doc__.strip().splitlines()[-1])
        return 2
    path = sys.argv[1]
    with open(path, 'rb') as f:
        data = f.read()
    try:
        imports, relaxed, bodies = scan(data)
    except ValueError as e:
        print(f'Error: {path}: cannot decode: {e}')
        return 1
    errors = []
    if imports:
        errors.append(f'{imports} import(s); the module must import nothing')
    for op, n in sorted(relaxed.items()):
        if op != ALLOWED:
            errors.append(f'{n} x {RELAXED[op]}; only {RELAXED[ALLOWED]} may appear')
    if not relaxed.get(ALLOWED):
        errors.append(f'no {RELAXED[ALLOWED]}: the fused sites are not using it')
    if errors:
        for e in errors:
            print(f'Error: {path}: {e}')
        return 1
    print(f'{path}: {bodies} function bodies, no imports, '
          f'{relaxed[ALLOWED]} x {RELAXED[ALLOWED]} and no other relaxed-SIMD instruction. OK.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
