// Bit-exact port of src/tools/ta_regtest/fuzz_data.h. Every product is written
// float64(a*b): Go may fuse a product with an addition, across statements too,
// where C, built with -ffp-contract=off, rounds each.

const (
	fuzzRandwalk  = 0
	fuzzMonoUp    = 1
	fuzzMonoDown  = 2
	fuzzConstant  = 3
	fuzzTieHeavy  = 4
	fuzzExtreme   = 5
	fuzzWithZeros = 6
	fuzzCandle    = 7
	fuzzZerosum   = 8
	fuzzDomainMax = 9
)

const fuzzDomainBound = 3e37

type splitMix64 struct{ s uint64 }

func (m *splitMix64) next() uint64 {
	m.s += 0x9E3779B97F4A7C15
	z := m.s
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9
	z = (z ^ (z >> 27)) * 0x94D049BB133111EB
	return z ^ (z >> 31)
}

func (m *splitMix64) unit() float64 {
	return float64(float64(m.next()>>11) * (1.0 / 9007199254740992.0))
}

func (m *splitMix64) bit() float64 {
	return float64(m.next() & 1)
}

func fuzzSeed(seed int32, salt uint64) *splitMix64 {
	return &splitMix64{0x243F6A8885A308D3 ^ uint64(uint32(seed))*0xD1B54A32D192ED03 ^ salt}
}

type fuzzBars [6][]float64

func (b *fuzzBars) cdlBar(p, n int, o, h, l, cl float64) int {
	hi, lo := h, l
	if hi < o {
		hi = o
	}
	if hi < cl {
		hi = cl
	}
	if lo > o {
		lo = o
	}
	if lo > cl {
		lo = cl
	}
	if p < n {
		b[0][p], b[1][p], b[2][p], b[3][p], b[4][p], b[5][p] = o, hi, lo, cl, 1000.0, 100.0
		p++
	}
	return p
}

func (b *fuzzBars) cdlFlat(p, n, k int, base float64) int {
	for ; k > 0; k-- {
		p = b.cdlBar(p, n, base, base+1.0, base-1.0, base)
	}
	return p
}

func pick(c bool, a, b float64) float64 {
	if c {
		return a
	}
	return b
}

func (b *fuzzBars) cdlHikkake(p, n int, base, w float64, dir, brk, conf int) int {
	h1, l1 := base+w, base-w
	h2 := pick(brk == 1, base+w+0.5, base+float64(0.6*w))
	l2 := pick(brk == 2, base-w-0.5, base-float64(0.6*w))
	var h3, l3 float64
	if dir > 0 {
		h3 = pick(brk == 3, h2+float64(0.4*w), h2-float64(0.4*w))
		l3 = l2 - float64(0.4*w)
	} else {
		h3 = pick(brk == 4, h2-float64(0.4*w), h2+float64(0.4*w))
		l3 = l2 + float64(0.4*w)
	}
	p = b.cdlBar(p, n, base, h1, l1, base)
	p = b.cdlBar(p, n, base, h2, l2, base)
	p = b.cdlBar(p, n, base, h3, l3, base)
	if conf == 2 {
		p = b.cdlFlat(p, n, 3, base)
	}
	if conf != 0 {
		cc := pick(dir > 0, h2+w, l2-w)
		p = b.cdlBar(p, n, cc, cc+0.5, cc-0.5, cc)
	}
	return p
}

func (b *fuzzBars) cdlHikkakeMod(p, n int, base, w float64, dir, brk, conf int) int {
	h1, l1 := base+w, base-w
	h2 := pick(brk == 1, base+w+0.5, base+float64(0.7*w))
	l2 := pick(brk == 1, base-w-0.5, base-float64(0.7*w))
	h3 := pick(brk == 2, h2+float64(0.3*w), base+float64(0.45*w))
	l3 := pick(brk == 2, l2-float64(0.3*w), base-float64(0.45*w))
	var h4, l4 float64
	if dir > 0 {
		h4 = pick(brk == 3, h3+float64(0.3*w), h3-float64(0.3*w))
		l4 = l3 - float64(0.3*w)
	} else {
		h4 = pick(brk == 3, h3-float64(0.3*w), h3+float64(0.3*w))
		l4 = l3 + float64(0.3*w)
	}
	c2 := pick(brk == 4, base, pick(dir > 0, l2, h2))
	p = b.cdlBar(p, n, base, h1, l1, base)
	p = b.cdlBar(p, n, base, h2, l2, c2)
	p = b.cdlBar(p, n, base, h3, l3, base)
	p = b.cdlBar(p, n, base, h4, l4, base)
	if conf != 0 {
		cc := pick(dir > 0, h3+w, l3-w)
		p = b.cdlBar(p, n, cc, cc+0.5, cc-0.5, cc)
	}
	return p
}

func (b *fuzzBars) cdlPrimer(p, n, k int, base, bd, hr float64) int {
	for i := 0; i < k; i++ {
		o, c := base+bd, base
		if i&1 != 0 {
			o, c = base, base+bd
		}
		p = b.cdlBar(p, n, o, base+bd+hr, base-hr, c)
	}
	return p
}

// fuzz_cdl_<name>: a primer, then one row per bar of open, high, low and close
// as offsets from the base.
var cdlCatalog = []struct {
	hr   float64
	bars [][4]float64
}{
	{1.0, [][4]float64{{0, 12.5, -0.5, 12.0}, {18.0, 18.5, 13.5, 14.0}, {16.0, 16.5, 5.5, 6.0}}},
	{1.0, [][4]float64{{0, 11, -1, 10}, {8, 8, 0, 0}, {5, 5, -5, -5}, {2, 2, -8, -8}}},
	{1.0, [][4]float64{{0, 10.2, -0.2, 10.0}, {5.0, 15.2, 4.8, 15.0}, {10.0, 20.2, 9.8, 20.0}}},
	{1.0, [][4]float64{{8.0, 8.0, -12.0, 0}, {6.0, 6.0, -4.0, 2.0}, {1.0, 1.0, 0, 0}}},
	{1.0, [][4]float64{{0, 5.0, -1.0, 4.0}, {2.0, 7.0, 1.0, 6.0}, {4.0, 9.0, 3.0, 8.0}, {10.0, 11.0, -3.0, -2.0}}},
	{1.0, [][4]float64{{0, 0, -3.0, -3.0}, {-4.0, -4.0, -7.0, -7.0}, {-9.0, -6.0, -12.0, -11.0}, {-5.0, -4.0, -16.0, -15.0}}},
	{1.0, [][4]float64{{0, 21.0, -1.0, 20.0}, {31.0, 32.0, 29.0, 30.0}, {16.0, 17.0, 14.0, 15.0}, {14.0, 15.0, 12.0, 13.0}, {35.0, 41.0, 34.0, 40.0}}},
	{1.0, [][4]float64{{0, 12.5, -0.5, 12.0}, {10.0, 10.5, 8.5, 9.0}, {8.0, 8.5, 6.5, 7.0}, {6.0, 6.5, 4.5, 5.0}, {6.0, 20.5, 5.5, 20.0}}},
	{1.0, [][4]float64{{0, 6.0, 0, 6.0}, {5.0, 7.0, 5.0, 7.0}, {7.0, 8.0, 7.0, 8.0}}},
	{6.0, [][4]float64{{10.0, 12.0, -1.0, 0}, {-5.0, 1.0, -6.0, 0.35}}},
	{1.0, [][4]float64{{10.0, 11.0, -12.0, -10.0}, {5.0, 6.0, -15.0, -5.0}, {-4.0, -2.0, -5.0, -3.0}}},
	{1.0, [][4]float64{{20, 20, 0, 0}, {30, 50, 30, 50}}},
	{1.0, [][4]float64{{40, 40, 20, 20}, {45, 70, 45, 70}}},
	{1.0, [][4]float64{{0, 21, -1, 20}, {30, 31, 4, 5}}},
	{1.0, [][4]float64{{20, 21, -1, 0}, {-10, 16, -11, 15}}},
	{1.0, [][4]float64{{10, 11, -1, 0}, {-10, 4, -12, 2.5}}},
	{1.0, [][4]float64{{20, 21, -1, 0}, {7, 8, 4, 5}}},
	{1.0, [][4]float64{{0, 12.5, -0.5, 12}, {6, 7.5, 5.5, 7}, {8, 8.5, -2.5, -2}}},
	{1.0, [][4]float64{{0, 0, -3, -3}, {-3, -3, -6, -6}, {-6, -6, -9, -9}}},
	{1.0, [][4]float64{{0, 12, 0, 12}, {8, 25, 8, 25}, {25, 26, 25, 26}}},
	{1.0, [][4]float64{{0, 10.5, -0.5, 10}, {16, 16.5, 14.5, 15}, {18, 18.5, 11.5, 12}}},
	{1.0, [][4]float64{{20, 21, 7, 8}, {4, 5, -1, 0}, {-2, 0, -5, -4}, {-7, -6, -11, -9}, {5, 7, 4, 6}}},
	{1.0, [][4]float64{{20, 21, 14, 15}, {15, 16, 9, 10}, {10, 11, 4, 5}, {5, 12, -2, 0}, {6, 15, 6, 15}}},
	{1.0, [][4]float64{{0, 6, -1, 5}, {8, 14, 7, 13}, {10, 11, 1, 2}}},
}

func (b *fuzzBars) candle(seed int32, n int) {
	m := fuzzSeed(seed, 0)
	p := b.cdlFlat(0, n, 6, 100.0)
	for _, w := range cdlCatalog {
		p = b.cdlPrimer(p, n, 6, 100.0, 2.0, w.hr)
		for _, r := range w.bars {
			p = b.cdlBar(p, n, 100.0+r[0], 100.0+r[1], 100.0+r[2], 100.0+r[3])
		}
	}
	for p < n-16 {
		base := 100.0 + float64((m.unit()-0.5)*40.0)
		w := 8.0 + float64(m.unit()*20.0)
		variant := int(m.unit() * 10.0)
		dir := -1
		if m.bit() != 0 {
			dir = 1
		}
		switch variant {
		case 0:
			p = b.cdlHikkake(p, n, base, w, dir, 0, 1)
		case 1:
			p = b.cdlHikkake(p, n, base, w, dir, 0, 2)
		case 2:
			p = b.cdlHikkake(p, n, base, w, dir, 1, 0)
		case 3:
			p = b.cdlHikkake(p, n, base, w, dir, 2, 0)
		case 4:
			brk := 4
			if dir > 0 {
				brk = 3
			}
			p = b.cdlHikkake(p, n, base, w, dir, brk, 0)
		case 5:
			p = b.cdlHikkakeMod(p, n, base, w, dir, 0, 1)
		case 6:
			p = b.cdlHikkakeMod(p, n, base, w, dir, 0, 0)
		case 7:
			p = b.cdlHikkakeMod(p, n, base, w, dir, 1, 0)
		case 8:
			p = b.cdlHikkakeMod(p, n, base, w, dir, 3, 0)
		default:
			p = b.cdlHikkakeMod(p, n, base, w, dir, 4, 1)
		}
		p = b.cdlFlat(p, n, 6, base)
	}
	for p < n {
		p = b.cdlFlat(p, n, 1, 100.0)
	}
}

func (b *fuzzBars) zerosum(seed int32, n int) {
	m := fuzzSeed(seed, uint64(fuzzZerosum)<<32)
	for i := 0; i < n; i++ {
		r := m.unit()
		var open, hi, lo, close, t float64
		switch {
		case r < 0.34:
			a := 1.0 + float64(m.unit()*50.0)
			hi, lo = a, -a
		case r < 0.50:
		default:
			base := 90.0 + float64(m.unit()*20.0)
			w := 0.5 + float64(m.unit()*5.0)
			close = base
			t = m.unit() - 0.5
			open = base + t
			hi = pick(open > close, open, close) + w
			lo = pick(open < close, open, close) - w
		}
		b[0][i], b[1][i], b[2][i], b[3][i] = open, hi, lo, close
		t = float64(m.unit() * 1000.0)
		b[4][i] = 1000.0 + t
		t = float64(m.unit() * 100.0)
		b[5][i] = 100.0 + t
	}
}

func (b *fuzzBars) domainMax(seed int32, n int) {
	m := fuzzSeed(seed, uint64(fuzzDomainMax)<<32)
	for i := 0; i < n; i++ {
		r := m.unit()
		scale := pick(r < 0.90, fuzzDomainBound, 100.0)
		t := float64(m.unit() * 0.07)
		a := 0.93 + t
		hi := float64(a * scale)
		t = float64(m.unit() * 0.02)
		w := float64(hi * t)
		lo := hi - w
		span := hi - lo
		t = float64(m.unit() * span)
		close := lo + t
		t = float64(m.unit() * span)
		open := lo + t
		if r >= 0.45 && r < 0.90 {
			hi, lo = -lo, -hi
			open, close = -open, -close
		}
		b[0][i], b[1][i], b[2][i], b[3][i] = open, hi, lo, close
		t = float64(m.unit() * 0.99)
		a = 0.01 + t
		b[4][i] = float64(a * fuzzDomainBound)
		t = float64(m.unit() * 0.99)
		a = 0.01 + t
		b[5][i] = float64(a * fuzzDomainBound)
	}
}

// fuzzGen is fuzz_gen: the first n bars of each array from (shape, seed).
func fuzzGen(shape, seed int32, n int, o, h, l, c, v, oi []float64) {
	b := &fuzzBars{o, h, l, c, v, oi}
	switch shape {
	case fuzzCandle:
		b.candle(seed, n)
		return
	case fuzzZerosum:
		b.zerosum(seed, n)
		return
	case fuzzDomainMax:
		b.domainMax(seed, n)
		return
	}
	m := fuzzSeed(seed, uint64(uint32(shape))<<32)
	walk := 100.0
	for i := 0; i < n; i++ {
		var close, t float64
		switch shape {
		case fuzzConstant:
			o[i], h[i], l[i], c[i], v[i], oi[i] = 42.0, 42.0, 42.0, 42.0, 1000000.0, 10000.0
			continue
		case fuzzTieHeavy:
			close = float64(3 + int32(m.unit()*5.0))
			o[i], c[i] = close, close
			h[i] = close + m.bit()
			l[i] = close - m.bit()
			v[i] = float64(float64(1+int32(m.unit()*4.0)) * 1000.0)
			oi[i] = 1000.0
			continue
		case fuzzMonoUp:
			t = float64(float64(i) * 0.5)
			close = 10.0 + t
		case fuzzMonoDown:
			t = float64(float64(i) * 0.25)
			close = 500.0 - t
		case fuzzExtreme:
			t = m.unit()
			close = pick(m.bit() != 0, float64((1.0+t)*1.0e9), float64((1.0+t)*1.0e-7))
		case fuzzWithZeros:
			r := m.unit()
			switch {
			case r < 0.15:
				close = 0.0
			case r < 0.30:
				close = math.Copysign(0, -1)
			default:
				t = r - 0.5
				close = float64(t * 8.0)
			}
		default:
			t = m.unit() - 0.5
			t = float64(t * 0.04)
			walk = float64(walk * (1.0 + t))
			close = walk
		}
		mag := float64(math.Abs(close)*0.01) + 0.001
		t = m.unit() - 0.5
		t = float64(t * mag)
		open := close + t
		hi := pick(open > close, open, close)
		t = float64(m.unit() * mag)
		hi = hi + t
		lo := pick(open < close, open, close)
		t = float64(m.unit() * mag)
		lo = lo - t
		o[i], h[i], l[i], c[i] = open, hi, lo, close
		t = float64(m.unit() * 1.0e6)
		v[i] = 1000.0 + t
		t = float64(m.unit() * 1.0e4)
		oi[i] = 100.0 + t
	}
}

// fuzzHashInit is fuzz_hash_init(); never retype it: it is not the standard
// FNV basis.
func fuzzHashInit() uint64 {
	return 1469598103934665603
}

func fuzzHashWord(h uint64, w uint64, nbytes int) uint64 {
	for k := 0; k < nbytes; k++ {
		h ^= (w >> (8 * k)) & 0xff
		h *= 1099511628211
	}
	return h
}

func fuzzHashF64(h uint64, a []float64) uint64 {
	for _, x := range a {
		h = fuzzHashWord(h, math.Float64bits(x), 8)
	}
	return h
}

func fuzzHashI32(h uint64, a []int32) uint64 {
	for _, x := range a {
		h = fuzzHashWord(h, uint64(uint32(x)), 4)
	}
	return h
}

func fuzzHashFin(h uint64) uint64 {
	h ^= h >> 33
	h *= 0xFF51AFD7ED558CCD
	h ^= h >> 33
	h *= 0xC4CEB9FE1A85EC53
	h ^= h >> 33
	return h
}
