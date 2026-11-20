// Port of src/tools/ta_bench/bench_corpus.h, so --bench-stream times the series
// ta_bench_stream times for the same shape, seed and points. Every product is
// written float64(a*b), as in fuzz.go. The walk, ramp, flat and peg shapes match
// C bit for bit; gbm, trend-chop and gap also go through Go's Log, Exp, Sin and Cos,
// which may round a last bit differently from the C library's.

const (
	benchGenWalk = iota
	benchGenGBM
	benchGenTrendChop
	benchGenRamp
	benchGenFlat
	benchGenPeg
	benchGenGap
)

const (
	benchCorpusSeed   = 42
	benchCorpusPeriod = 14
	benchCorpusTrend  = 0.5
)

type benchShape struct {
	name  string
	gen   int
	param float64
}

var benchShapes = [...]benchShape{
	{"randwalk", benchGenWalk, 1.00},
	{"randwalk-lo", benchGenWalk, 0.25},
	{"randwalk-hi", benchGenWalk, 4.00},
	{"gbm", benchGenGBM, 0.20},
	{"trend-chop-0.5p", benchGenTrendChop, 0.50},
	{"trend-chop-1p", benchGenTrendChop, 1.00},
	{"trend-chop-2p", benchGenTrendChop, 2.00},
	{"trend-chop-4p", benchGenTrendChop, 4.00},
	{"mono-up", benchGenRamp, 1.00},
	{"mono-down", benchGenRamp, -1.00},
	{"constant", benchGenFlat, 0.00},
	{"peg", benchGenPeg, 0.00},
	{"gap-daily", benchGenGap, 0.15},
	{"gap-intraday", benchGenGap, 0.02},
}

type benchCorpusCfg struct {
	shape, seed, refPeriod int
	trendStrength          float64
}

func benchShapeID(name string) int {
	if name == "" {
		return 0
	}
	for i := range benchShapes {
		if benchShapes[i].name == name {
			return i
		}
	}
	return -1
}

type benchRng struct {
	m        splitMix64
	spare    float64
	hasSpare bool
}

func (r *benchRng) normal() float64 {
	if r.hasSpare {
		r.hasSpare = false
		return r.spare
	}
	u1 := r.m.unit()
	for u1 <= 1.0e-300 {
		u1 = r.m.unit()
	}
	u2 := r.m.unit()
	mag := math.Sqrt(float64(-2.0 * math.Log(u1)))
	r.spare = float64(mag * math.Sin(float64(6.283185307179586476925286766559*u2)))
	r.hasSpare = true
	return float64(mag * math.Cos(float64(6.283185307179586476925286766559*u2)))
}

func benchFmax(a, b float64) float64 {
	if a > b {
		return a
	}
	return b
}

func benchFmin(a, b float64) float64 {
	if a < b {
		return a
	}
	return b
}

// benchCorpusGen fills the seven series in ta_bench's order: open, high, low,
// close, volume, open interest, MAVP's periods.
func benchCorpusGen(cfg benchCorpusCfg, n int, o, h, l, c, v, oi, periods []float64) {
	shape := cfg.shape
	if shape < 0 || shape >= len(benchShapes) {
		shape = 0
	}
	refPeriod := cfg.refPeriod
	if refPeriod < 2 {
		refPeriod = benchCorpusPeriod
	}
	sh := benchShapes[shape]
	param := sh.param
	rng := benchRng{m: splitMix64{0x243F6A8885A308D3 ^ uint64(uint32(int32(cfg.seed)))*0xD1B54A32D192ED03}}
	period := 16

	switch sh.gen {
	case benchGenWalk:
		lcg := uint32(int32(cfg.seed))
		price := 100.0
		amp, hw := float64(2.0*param), float64(0.5*param)
		for i := 0; i < n; i++ {
			lcg = lcg*1103515245 + 12345
			r := float64(lcg>>16)/32768.0 - 1.0
			oo, cc := price, price+float64(r*amp)
			o[i], c[i] = oo, cc
			h[i] = benchFmax(oo, cc) + float64(math.Abs(r)*hw)
			l[i] = benchFmin(oo, cc) - float64(math.Abs(r)*hw)
			if l[i] < 1.0 {
				l[i] = 1.0
			}
			v[i] = 1000000.0 + float64(r*500000.0)
			oi[i] = 0.0
			price = cc
			if price < 1.0 {
				price = 1.0
			}
			period += int((lcg>>8)%7) - 3
			period = min(max(period, 2), 30)
			periods[i] = float64(period)
		}
		return
	case benchGenFlat:
		for i := 0; i < n; i++ {
			o[i], h[i], l[i], c[i] = 100.0, 100.0, 100.0, 100.0
			v[i], oi[i], periods[i] = 1000000.0, 0.0, 14.0
		}
		return
	case benchGenPeg:
		cents := int64(330)
		for i := 0; i < n; i++ {
			prev := cents
			if i%400 < 40 {
				cents += int64(rng.m.next()%5) - 2
				if cents < 100 {
					cents = 100
				}
			} else {
				cents = 330
			}
			o[i] = float64(prev) / 100.0
			c[i] = float64(cents) / 100.0
			h[i] = benchFmax(o[i], c[i])
			l[i] = benchFmin(o[i], c[i])
			v[i], oi[i], periods[i] = 1000000.0, 0.0, 14.0
		}
		return
	case benchGenRamp:
		span := 0.0
		if n > 1 {
			span = 900.0 / float64(n-1)
		}
		dir := -1.0
		if param > 0.0 {
			dir = 1.0
		}
		for i := 0; i < n; i++ {
			t := float64(n - 1 - i)
			if param > 0.0 {
				t = float64(i)
			}
			cc := 100.0 + float64(span*t)
			oo := cc - float64(float64(0.5*span)*dir)
			o[i], c[i] = oo, cc
			h[i] = benchFmax(oo, cc) + float64(0.25*span) + 1.0e-6
			l[i] = benchFmin(oo, cc) - float64(0.25*span) - 1.0e-6
			if l[i] < 1.0 {
				l[i] = 1.0
			}
			v[i], oi[i], periods[i] = 1000000.0, 0.0, 14.0
		}
		return
	case benchGenGap:
		const sigma = 0.01
		sj := float64(math.Sqrt(param) * sigma)
		s2 := float64(float64((1.0-param)*sigma) * sigma)
		sd := math.Sqrt(s2)
		logc := math.Log(100.0)
		for i := 0; i < n; i++ {
			logo := logc + float64(sj*rng.normal())
			d := float64(sd * rng.normal())
			u1 := 1.0 - rng.m.unit()
			u2 := 1.0 - rng.m.unit()
			up := float64(0.5 * (d + math.Sqrt(float64(d*d)-float64(float64(2.0*s2)*math.Log(u1)))))
			dn := float64(0.5 * (d - math.Sqrt(float64(d*d)-float64(float64(2.0*s2)*math.Log(u2)))))
			logc = logo + d
			o[i] = math.Exp(logo)
			c[i] = math.Exp(logc)
			h[i] = benchFmax(math.Exp(logo+up), benchFmax(o[i], c[i]))
			l[i] = benchFmin(math.Exp(logo+dn), benchFmin(o[i], c[i]))
			v[i] = 1000000.0 + float64(500000.0*(rng.m.unit()-0.5))
			oi[i] = 0.0
			period += int(rng.m.next()%7) - 3
			period = min(max(period, 2), 30)
			periods[i] = float64(period)
		}
		return
	}

	sigma, mu := 0.01, 0.0
	if sh.gen == benchGenGBM {
		sigma = param / math.Sqrt(252.0)
		mu = float64(0.05)/float64(252.0) - float64(float64(0.5*sigma)*sigma)
	}
	trendDrift := float64(cfg.trendStrength * sigma)
	phi := 0.10
	regimeF := 0.0
	if sh.gen == benchGenTrendChop {
		regimeF = float64(param*float64(refPeriod)) + 0.5
	}
	if top := float64(max(n, 2)); regimeF > top {
		regimeF = top
	}
	if regimeF < 0.0 {
		regimeF = 0.0
	}
	regime := int(regimeF)
	logp := math.Log(100.0)
	anchor := logp
	leg, legBar := 0, 0
	if sh.gen == benchGenTrendChop && regime < 2 {
		regime = 2
	}
	for i := 0; i < n; i++ {
		z := rng.normal()
		if sh.gen == benchGenTrendChop {
			if legBar >= regime {
				legBar = 0
				leg = (leg + 1) & 3
				anchor = logp
			}
			switch leg {
			case 0:
				logp += trendDrift + float64(sigma*z)
			case 2:
				logp += -trendDrift + float64(sigma*z)
			default:
				logp += float64(-phi*(logp-anchor)) + float64(sigma*z)
			}
			legBar++
		} else {
			logp += mu + float64(sigma*z)
		}
		cc := math.Exp(logp)
		oo := float64(cc * (1.0 + float64(float64(sigma*0.5)*rng.m.unit()) - float64(sigma*0.25)))
		hi, lo := benchFmax(oo, cc), benchFmin(oo, cc)
		w := float64(float64(sigma*0.5) * rng.m.unit())
		hi = float64(hi * (1.0 + w))
		w = float64(float64(sigma*0.5) * rng.m.unit())
		lo = float64(lo * (1.0 - w))
		o[i], c[i], h[i], l[i] = oo, cc, hi, lo
		if l[i] < 1.0e-6 {
			l[i] = 1.0e-6
		}
		v[i] = 1000000.0 + float64(500000.0*(rng.m.unit()-0.5))
		oi[i] = 0.0
		period += int(rng.m.next()%7) - 3
		period = min(max(period, 2), 30)
		periods[i] = float64(period)
	}
}
