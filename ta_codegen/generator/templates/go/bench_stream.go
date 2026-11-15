// --bench-stream: the Go counterpart of ta_bench_stream, by the same method so
// the two tools' rows can be joined on the function name. Keep in step with
// bench_gen.rs's C tool: corpus, defaults, the batch@last history, the 32-call
// peek blocks between untimed updates, and the best pass of each column.

// The bar feed: Update, Peek and batch@last read bar it&(feed-1), and every
// pass restarts at bar 0. The default 4096 bars is short enough for a branch
// predictor to learn, which makes a data-dependent branch read as free and a
// branch-free pick as a loss; --feed-bars walks a longer one.
const (
	benchFeedDefault = 4096
	benchFeedMax     = 131072
)

var (
	benchSinkI int
	benchSinkF float64
)

// A parameter reaches every call through one of these, so no call times a
// body specialised on a constant.
//
//go:noinline
func benchInt(v int) int {
	benchSinkI = v
	return benchSinkI
}

//go:noinline
func benchReal(v float64) float64 {
	benchSinkF = v
	return benchSinkF
}

type benchRun struct {
	core   *talib.Core
	iters  int
	period int
	passes int
	n      int
	in     [7][]float64
	feed   [7][benchFeedMax]float64
	mask   int
	rt     [7][]float64
	outR   [][]float64
	outI   [][]int32
	tb     []time.Duration
	tu     []time.Duration
	tp     []time.Duration
	blk    int
	nblk   int
	npk    int
	heap   uint64
	sink   float64
	jsonl  *os.File
	header string

	minRatio                         float64
	rows, slow, below, reject, short int
	worstRatio                       float64
	worstName                        string
}

func (r *benchRun) periodOr(def int) int {
	if r.period > 0 {
		return r.period
	}
	return def
}

// reserve grows the batch@last history to need bars, filling the new tail
// from the corpus as bench_rt_reserve does.
func (r *benchRun) reserve(need int) {
	if need <= len(r.rt[0]) {
		return
	}
	for k := range r.rt {
		old := len(r.rt[k])
		p := make([]float64, need)
		copy(p, r.rt[k])
		for i := old; i < need; i++ {
			p[i] = r.in[k][i%r.n]
		}
		r.rt[k] = p
	}
}

// The retained size of a handle: the live heap after a collection, after Open
// against before it. One collection before is not enough: the heap it reports
// still shrinks by a few kilobytes at the next one.
func (r *benchRun) trackStart() {
	var m runtime.MemStats
	runtime.GC()
	runtime.GC()
	runtime.ReadMemStats(&m)
	r.heap = m.HeapAlloc
}

func (r *benchRun) trackEnd() int64 {
	var m runtime.MemStats
	runtime.GC()
	runtime.ReadMemStats(&m)
	if m.HeapAlloc < r.heap {
		return 0
	}
	return int64(m.HeapAlloc - r.heap)
}

func benchBest(d []time.Duration) (best, spread float64) {
	lo, hi := d[0], d[0]
	for _, x := range d[1:] {
		lo, hi = min(lo, x), max(hi, x)
	}
	if lo <= 0 {
		return float64(lo), 0
	}
	return float64(lo), 100 * float64(hi-lo) / float64(lo)
}

func (r *benchRun) record(name, status string, b, u, p float64, lb int, hb int64, ratio, bs, us, ps float64) {
	if r.jsonl == nil {
		return
	}
	fmt.Fprintf(r.jsonl, `{%s,"func":%q,"status":%q,"batch_last_ns":%.3f,"update_ns":%.3f,"peek_ns":%.3f,"lookback":%d,"handle_bytes":%d,"speedup":%.3f,"batch_spread_pct":%.1f,"update_spread_pct":%.1f,"peek_spread_pct":%.1f}`+"\n",
		r.header, name, status, b, u, p, lb, hb, ratio, bs, us, ps)
}

// row prints the C tool's columns, then the spread of each timed column over
// the passes, in percent of its best pass (-1 where nothing was timed).
func (r *benchRun) row(name string, openErr error, lb int, hb int64, sink float64) {
	r.sink += sink
	b, bs := benchBest(r.tb)
	b /= float64(r.iters)
	if lb < 0 {
		b, bs = -1, -1
	}
	if openErr != nil {
		if errors.Is(openErr, talib.InsufficientHistory) {
			fmt.Printf("%s %.3f -1 -1 %d 0 short %.1f -1 -1\n", name, b, lb, bs)
			r.record(name, "short", b, -1, -1, lb, 0, -1, bs, -1, -1)
			r.short++
			return
		}
		if lb < 0 {
			fmt.Printf("%s -1 -1 -1 %d 0 -1 -1 -1 -1\n", name, lb)
		} else {
			fmt.Printf("%s %.3f -1 -1 %d 0 -1 %.1f -1 -1\n", name, b, lb, bs)
		}
		r.record(name, "rejected", b, -1, -1, lb, 0, -1, bs, -1, -1)
		r.reject++
		return
	}
	u, us := benchBest(r.tu)
	u /= float64(r.iters)
	p, ps := benchBest(r.tp)
	p /= float64(r.npk)
	ratio := b / u
	fmt.Printf("%s %.3f %.3f %.3f %d %d %.3f %.1f %.1f %.1f\n", name, b, u, p, lb, hb, ratio, bs, us, ps)
	r.record(name, "timed", b, u, p, lb, hb, ratio, bs, us, ps)
	r.rows++
	if ratio < 1.0 {
		r.slow++
	}
	if r.minRatio > 0.0 && ratio < r.minRatio {
		r.below++
	}
	if r.worstRatio < 0.0 || ratio < r.worstRatio {
		r.worstRatio = ratio
		r.worstName = name
	}
}

func benchMatches(filter, name string) bool {
	if filter == "" {
		return true
	}
	if len(filter) > 511 {
		filter = filter[:511]
	}
	name = strings.ToLower(name)
	for _, tok := range strings.Split(filter, ",") {
		if tok != "" && strings.Contains(name, strings.ToLower(tok)) {
			return true
		}
	}
	return false
}

func benchIntArg(arg, prefix string, dst *int) bool {
	if !strings.HasPrefix(arg, prefix) {
		return false
	}
	v, err := strconv.Atoi(arg[len(prefix):])
	if err != nil {
		fmt.Fprintf(os.Stderr, "--bench-stream: %s needs an integer, got '%s'\n", prefix, arg[len(prefix):])
		os.Exit(2)
	}
	*dst = v
	return true
}

func benchRealArg(arg, prefix string, dst *float64) bool {
	if !strings.HasPrefix(arg, prefix) {
		return false
	}
	v, err := strconv.ParseFloat(arg[len(prefix):], 64)
	if err != nil {
		fmt.Fprintf(os.Stderr, "--bench-stream: %s needs a number, got '%s'\n", prefix, arg[len(prefix):])
		os.Exit(2)
	}
	*dst = v
	return true
}

// benchCorpusHash is FNV-1a 64 over the little-endian bits of the seven
// series, each in full, in benchCorpusGen's argument order.
func benchCorpusHash(in *[7][]float64) uint64 {
	h := uint64(14695981039346656037)
	for _, s := range in {
		for _, v := range s {
			bits := math.Float64bits(v)
			for k := 0; k < 8; k++ {
				h ^= (bits >> (8 * k)) & 0xff
				h *= 1099511628211
			}
		}
	}
	return h
}

func benchStream(args []string) int {
	points, iters, passes, feed := 100000, 500, 3, benchFeedDefault
	filter, jsonl, shapeName := "", "", "randwalk"
	corpusHash := false
	cfg := benchCorpusCfg{shape: 0, seed: benchCorpusSeed, refPeriod: 0, trendStrength: benchCorpusTrend}
	r := &benchRun{core: talib.NewCore(), worstRatio: -1}
	for _, a := range args {
		switch {
		case benchIntArg(a, "--points=", &points), benchIntArg(a, "--iters=", &iters),
			benchIntArg(a, "--passes=", &passes), benchIntArg(a, "--period=", &r.period),
			benchIntArg(a, "--feed-bars=", &feed),
			benchIntArg(a, "--seed=", &cfg.seed), benchIntArg(a, "--regime-period=", &cfg.refPeriod),
			benchRealArg(a, "--trend-strength=", &cfg.trendStrength), benchRealArg(a, "--min-ratio=", &r.minRatio):
		case strings.HasPrefix(a, "--function="):
			filter = a[len("--function="):]
		case strings.HasPrefix(a, "--jsonl="):
			jsonl = a[len("--jsonl="):]
		case strings.HasPrefix(a, "--shape="):
			shapeName = a[len("--shape="):]
			if cfg.shape = benchShapeID(shapeName); cfg.shape < 0 {
				fmt.Fprintf(os.Stderr, "--bench-stream: unknown --shape=%s; one of:", shapeName)
				for _, sh := range benchShapes {
					fmt.Fprintf(os.Stderr, " %s", sh.name)
				}
				fmt.Fprintln(os.Stderr)
				return 2
			}
		case a == "--corpus-hash":
			corpusHash = true
		default:
			fmt.Fprintf(os.Stderr, "--bench-stream: unknown option '%s'\n", a)
			return 2
		}
	}
	if feed < benchFeedDefault || feed > benchFeedMax || feed&(feed-1) != 0 {
		fmt.Fprintf(os.Stderr, "--bench-stream: --feed-bars=N needs a power of two from %d to %d, got %d\n", benchFeedDefault, benchFeedMax, feed)
		return 2
	}
	r.mask = feed - 1
	points = min(max(points, feed), maxArraySize)
	iters = max(iters, 1)
	passes = max(passes, 1)
	if cfg.refPeriod <= 0 {
		cfg.refPeriod = r.periodOr(benchCorpusPeriod)
	}
	r.iters, r.passes, r.n = iters, passes, points
	for k := range r.in {
		r.in[k] = make([]float64, points)
	}
	benchCorpusGen(cfg, points, r.in[0], r.in[1], r.in[2], r.in[3], r.in[4], r.in[5], r.in[6])
	if corpusHash {
		fmt.Printf("corpus shape=%s seed=%d points=%d regime_period=%d fnv1a64=%016x\n",
			benchShapes[cfg.shape].name, cfg.seed, points, cfg.refPeriod, benchCorpusHash(&r.in))
		return 0
	}
	for k := range r.in {
		copy(r.feed[k][:], r.in[k])
	}
	r.reserve(iters + 8192)
	r.outR = make([][]float64, benchOutReal)
	for k := range r.outR {
		r.outR[k] = make([]float64, maxArraySize)
	}
	r.outI = make([][]int32, benchOutInt)
	for k := range r.outI {
		r.outI[k] = make([]int32, maxArraySize)
	}
	r.tb, r.tu, r.tp = make([]time.Duration, passes), make([]time.Duration, passes), make([]time.Duration, passes)
	r.blk = 1
	if iters >= 64 {
		r.blk = 32
	}
	r.nblk = iters / r.blk
	r.npk = max(r.nblk*r.blk, 1)
	r.header = fmt.Sprintf(`"points":%d,"iters":%d,"passes":%d,"period":%d,"feed_bars":%d,"shape":%q,"seed":%d,"go":%q,"goarch":%q`,
		points, iters, passes, r.period, feed, benchShapes[cfg.shape].name, cfg.seed, runtime.Version(), runtime.GOARCH)
	if jsonl != "" {
		f, err := os.OpenFile(jsonl, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0o644)
		if err != nil {
			fmt.Fprintf(os.Stderr, "--bench-stream: %v\n", err)
			return 2
		}
		defer f.Close()
		r.jsonl = f
	}
	fmt.Printf("# ta_codegen_serve_go --bench-stream points=%d iters=%d passes=%d shape=%s seed=%d %s\n",
		points, iters, passes, benchShapes[cfg.shape].name, cfg.seed, runtime.Version())
	if r.period > 0 {
		fmt.Printf("# --period=%d: every optInTimePeriod; all other params at their defaults\n", r.period)
	}
	if feed != benchFeedDefault {
		fmt.Printf("# --feed-bars=%d: every pass walks that many distinct bars\n", feed)
	}
	fmt.Println("# func batch_last_ns update_ns peek_ns lookback handle_bytes speedup batch_spread_pct update_spread_pct peek_spread_pct")
	benchStreamAll(r, filter)
	worst := r.worstName
	if worst == "" {
		worst = "-"
	}
	fmt.Printf("# %d timed, %d rejected, %d short (lookback >= --points); %d slower than batch@last; worst %s %.2fx\n",
		r.rows, r.reject, r.short, r.slow, worst, r.worstRatio)
	benchSinkF = r.sink
	if r.minRatio > 0.0 {
		verdict := "PASS"
		if r.below > 0 {
			verdict = "FAIL"
		}
		fmt.Printf("# --min-ratio=%.2f: %d below -> %s\n", r.minRatio, r.below, verdict)
		if r.below > 0 {
			return 1
		}
	}
	return 0
}
