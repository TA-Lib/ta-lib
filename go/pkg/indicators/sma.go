package indicators

// SmaLookback returns the number of preceding bars consumed before first output.
func SmaLookback(period int) int {
	if period < 1 {
		return -1
	}
	return period - 1
}

// SmaBatch calculates Simple Moving Average over historical data for backtesting.
// Reuses the out slice to achieve zero heap allocations.
func SmaBatch(in []float64, period int, out []float64) OutRange {
	lookback := SmaLookback(period)
	n := len(in)
	if lookback < 0 || n <= lookback {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	var periodTotal float64
	for i := 0; i < lookback; i++ {
		periodTotal += in[i]
	}

	invPeriod := 1.0 / float64(period)
	trailingIdx := 0
	outIdx := 0

	for i := lookback; i < n; i++ {
		periodTotal += in[i]
		out[outIdx] = periodTotal * invPeriod
		periodTotal -= in[trailingIdx]
		trailingIdx++
		outIdx++
	}

	return OutRange{BegIdx: lookback, NbElement: outIdx}
}

// SmaStream maintains a circular buffer for O(1) ~2ns live market tick updates.
type SmaStream struct {
	ring        []float64
	cursor      int
	periodTotal float64
	invPeriod   float64
	period      int
	count       int
}

// NewSmaStream creates a streaming SMA instance.
func NewSmaStream(period int) *SmaStream {
	if period < 1 {
		period = 1
	}
	return &SmaStream{
		ring:      make([]float64, period),
		invPeriod: 1.0 / float64(period),
		period:    period,
	}
}

// Seed primes the stream with historical data.
func (s *SmaStream) Seed(history []float64) {
	start := 0
	if len(history) > s.period {
		start = len(history) - s.period
	}
	for i := start; i < len(history); i++ {
		s.Update(history[i])
	}
}

// Update is called when a bar closes. Mutates internal state in O(1) time.
func (s *SmaStream) Update(closedPrice float64) (float64, bool) {
	oldest := s.ring[s.cursor]
	s.ring[s.cursor] = closedPrice
	s.periodTotal += closedPrice - oldest

	s.cursor++
	if s.cursor >= s.period {
		s.cursor = 0
	}

	s.count++
	if s.count < s.period {
		return 0, false
	}
	return s.periodTotal * s.invPeriod, true
}

// Peek computes the forming indicator value for live intra-bar ticks without mutating state.
func (s *SmaStream) Peek(formingPrice float64) (float64, bool) {
	if s.count < s.period-1 {
		return 0, false
	}
	oldest := s.ring[s.cursor]
	tempSum := s.periodTotal - oldest + formingPrice
	return tempSum * s.invPeriod, true
}

