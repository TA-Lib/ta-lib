#!/usr/bin/env python3
"""Fails unless a WebAssembly module imports nothing, uses f64x2.relaxed_madd
in the indicator code and no other relaxed-SIMD instruction anywhere, carries
no custom section and no path of the machine that built it.

Decodes every instruction of every function body rather than searching for
byte patterns, which immediates and constants would match. An opcode it does
not know fails the check instead of being skipped. The fused instruction is
counted outside the body of the `fma_probe` export, which holds one whatever
the indicator code compiles to.

Usage: check_wasm_module.py <module.wasm>
"""
import os
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
PROBE_EXPORT = 'fma_probe'
# Fewer bodies than this using the fused instruction means fmadd! no longer
# compiles to it; the shipped module has several times as many.
MIN_FUSED_BODIES = 20
# Custom sections the shipped module may carry. Its release profile strips
# them all, so a non-empty list is a deliberate change.
ALLOWED_CUSTOM = ()
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# What `build.py wasm` remaps ROOT to; a clone at this very path has nothing to remap.
REMAPPED_ROOT = '/ta-lib'
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

    def name(self):
        n = self.u()
        start = self.pos
        self.skip(n)
        return self.data[start:self.pos].decode('utf-8', 'replace')

    def limits(self):
        flags = self.byte()
        self.u()
        if flags & 1:
            self.u()

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
    """{'imports', 'func_imports', 'exports' {name: (kind, index)}, 'custom'
    [names], 'bodies' [{relaxed opcode: count}] in code-section order}."""
    if data[:8] != b'\0asm\x01\0\0\0':
        raise ValueError('not a WebAssembly 1.0 binary module')
    r = Reader(data, 8)
    found = {'imports': 0, 'func_imports': 0, 'exports': {}, 'custom': [], 'bodies': []}
    while r.pos < r.end:
        sid = r.byte()
        size = r.u()
        start = r.pos
        sec = Reader(data, start, start + size)
        if sid == 0:
            found['custom'].append(sec.name())
        elif sid == 2:
            found['imports'] = sec.u()
            for _ in range(found['imports']):
                sec.name()
                sec.name()
                kind = sec.byte()
                if kind == 0:
                    found['func_imports'] += 1
                    sec.u()
                elif kind == 1:
                    sec.byte()
                    sec.limits()
                elif kind == 2:
                    sec.limits()
                elif kind == 3:
                    sec.byte()
                    sec.byte()
                elif kind == 4:
                    sec.byte()
                    sec.u()
                else:
                    raise ValueError(f'unknown import kind {kind:#x} at offset {sec.pos}')
        elif sid == 7:
            for _ in range(sec.u()):
                name = sec.name()
                kind = sec.byte()
                found['exports'][name] = (kind, sec.u())
        elif sid == 10:
            for _ in range(sec.u()):
                body_size = sec.u()
                body = Reader(data, sec.pos, sec.pos + body_size)
                for _ in range(body.u()):
                    body.u()
                    body.byte()
                relaxed = {}
                while body.pos < body.end:
                    instruction(body, relaxed)
                sec.pos = body.end
                found['bodies'].append(relaxed)
        r.pos = start + size
    return found


def build_paths():
    """The build machine's paths a remapped, stripped module must not contain.

    Each is a root plus a directory under it: a bare root can be a suffix of a
    path the module rightly holds, the remap target or the standard library's.
    """
    paths = set()
    for root in (ROOT, os.path.realpath(ROOT)):
        if root.rstrip(os.sep) != REMAPPED_ROOT:
            paths.add(os.path.join(root, 'ta_codegen') + os.sep)
    home = os.path.expanduser('~')
    if os.path.isabs(home):
        paths.update(os.path.join(home, d) + os.sep for d in ('.cargo', '.rustup'))
    return sorted(paths)


def main():
    if len(sys.argv) != 2:
        print(__doc__.strip().splitlines()[-1])
        return 2
    path = sys.argv[1]
    with open(path, 'rb') as f:
        data = f.read()
    try:
        found = scan(data)
    except ValueError as e:
        print(f'Error: {path}: cannot decode: {e}')
        return 1
    errors = []
    if found['imports']:
        errors.append(f"{found['imports']} import(s); the module must import nothing")
    relaxed = {}
    for body in found['bodies']:
        for op, n in body.items():
            relaxed[op] = relaxed.get(op, 0) + n
    for op, n in sorted(relaxed.items()):
        if op != ALLOWED:
            errors.append(f'{n} x {RELAXED[op]}; only {RELAXED[ALLOWED]} may appear')
    probe = found['exports'].get(PROBE_EXPORT)
    probe_body = None if probe is None or probe[0] != 0 else probe[1] - found['func_imports']
    if probe_body is None or not 0 <= probe_body < len(found['bodies']):
        errors.append(f'no function export {PROBE_EXPORT}')
    fused = [i for i, body in enumerate(found['bodies']) if body.get(ALLOWED) and i != probe_body]
    if len(fused) < MIN_FUSED_BODIES:
        errors.append(f'{len(fused)} function bodies besides {PROBE_EXPORT} use {RELAXED[ALLOWED]}, '
                      f'fewer than {MIN_FUSED_BODIES}: the fused sites are not using it')
    for name in found['custom']:
        if name not in ALLOWED_CUSTOM:
            errors.append(f"custom section '{name}': the release profile must strip it")
    for p in build_paths():
        if p.encode() in data:
            errors.append(f'contains the build path {p}: the source paths are not remapped')
    if errors:
        for e in errors:
            print(f'Error: {path}: {e}')
        return 1
    print(f"{path}: {len(found['bodies'])} function bodies, no imports, no custom section, "
          f'{relaxed[ALLOWED]} x {RELAXED[ALLOWED]} in {len(fused)} bodies besides {PROBE_EXPORT}, '
          f'no other relaxed-SIMD instruction. OK.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
