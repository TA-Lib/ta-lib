package indicators

// MacdLookback returns the lookback for MACD.
func MacdLookback(fastPeriod, slowPeriod, signalPeriod int) int {
	if fastPeriod < 1 || slowPeriod < 1 || signalPeriod < 1 {
		return -1
	}
	if fastPeriod > slowPeriod {
		fastPeriod, slowPeriod = slowPeriod, fastPeriod
	}
	return (slowPeriod - 1) + (signalPeriod - 1)
}

// MacdBatch calculates Moving Average Convergence/Divergence for backtesting.
func MacdBatch(in []float64, fastPeriod, slowPeriod, signalPeriod int,
	outMACD, outSignal, outHist []float64) OutRange {

	if fastPeriod > slowPeriod {
		fastPeriod, slowPeriod = slowPeriod, fastPeriod
	}

	lookback := MacdLookback(fastPeriod, slowPeriod, signalPeriod)
	n := len(in)
	if lookback < 0 || n <= lookback {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	// Compute fast and slow EMAs
	fastBuf := make([]float64, n)
	slowBuf := make([]float64, n)

	fastRange := EmaBatch(in, fastPeriod, fastBuf)
	slowRange := EmaBatch(in, slowPeriod, slowBuf)

	// Align fast and slow series into MACD line
	macdLen := slowRange.NbElement
	tempMacd := make([]float64, macdLen)
	fastOffset := slowRange.BegIdx - fastRange.BegIdx

	for i := 0; i < macdLen; i++ {
		tempMacd[i] = fastBuf[i+fastOffset] - slowBuf[i]
	}

	// Compute Signal line (EMA of MACD line)
	signalBuf := make([]float64, macdLen)
	sigRange := EmaBatch(tempMacd, signalPeriod, signalBuf)

	outNb := sigRange.NbElement
	macdOffset := sigRange.BegIdx

	for i := 0; i < outNb; i++ {
		m := tempMacd[i+macdOffset]
		s := signalBuf[i]
		outMACD[i] = m
		outSignal[i] = s
		outHist[i] = m - s
	}

	return OutRange{BegIdx: lookback, NbElement: outNb}
}

// MacdStream provides O(1) streaming updates for live MACD feeds.
type MacdStream struct {
	fastEma   *EmaStream
	slowEma   *EmaStream
	signalEma *EmaStream
	lookback  int
	count     int
}

// NewMacdStream creates a streaming MACD instance.
func NewMacdStream(fastPeriod, slowPeriod, signalPeriod int) *MacdStream {
	if fastPeriod > slowPeriod {
		fastPeriod, slowPeriod = slowPeriod, fastPeriod
	}
	return &MacdStream{
		fastEma:   NewEmaStream(fastPeriod),
		slowEma:   NewEmaStream(slowPeriod),
		signalEma: NewEmaStream(signalPeriod),
		lookback:  MacdLookback(fastPeriod, slowPeriod, signalPeriod),
	}
}

// Update computes the new (MACD, Signal, Hist) on a closed candle in O(1) time.
func (s *MacdStream) Update(closedPrice float64) (macd, signal, hist float64, ready bool) {
	s.count++
	fastVal, fastReady := s.fastEma.Update(closedPrice)
	slowVal, slowReady := s.slowEma.Update(closedPrice)

	if !fastReady || !slowReady {
		return 0, 0, 0, false
	}

	macdLine := fastVal - slowVal
	sigVal, sigReady := s.signalEma.Update(macdLine)

	if !sigReady {
		return macdLine, 0, 0, false
	}

	return macdLine, sigVal, macdLine - sigVal, true
}

// Peek returns forming (MACD, Signal, Hist) on intra-bar ticks.
func (s *MacdStream) Peek(formingPrice float64) (macd, signal, hist float64, ready bool) {
	fastVal, fastReady := s.fastEma.Peek(formingPrice)
	slowVal, slowReady := s.slowEma.Peek(formingPrice)

	if !fastReady || !slowReady {
		return 0, 0, 0, false
	}

	macdLine := fastVal - slowVal
	sigVal, sigReady := s.signalEma.Peek(macdLine)

	if !sigReady {
		return macdLine, 0, 0, false
	}

	return macdLine, sigVal, macdLine - sigVal, true
}

