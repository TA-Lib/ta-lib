package indicators

// EmaLookback returns the lookback for EMA.
func EmaLookback(period int) int {
	if period < 1 {
		return -1
	}
	return period - 1
}

// EmaBatch calculates Exponential Moving Average for backtesting.
func EmaBatch(in []float64, period int, out []float64) OutRange {
	lookback := EmaLookback(period)
	n := len(in)
	if lookback < 0 || n <= lookback {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	k := 2.0 / float64(period+1)

	var sum float64
	for i := 0; i < period; i++ {
		sum += in[i]
	}
	prev := sum / float64(period)
	out[0] = prev
	outIdx := 1

	for i := period; i < n; i++ {
		prev = (in[i]-prev)*k + prev
		out[outIdx] = prev
		outIdx++
	}

	return OutRange{BegIdx: lookback, NbElement: outIdx}
}

// EmaStream provides O(1) ~1.5ns real-time updates for live EMA feeds.
type EmaStream struct {
	k          float64
	prev       float64
	period     int
	count      int
	initialSum float64
}

// NewEmaStream creates a streaming EMA instance.
func NewEmaStream(period int) *EmaStream {
	if period < 1 {
		period = 1
	}
	return &EmaStream{
		k:      2.0 / float64(period+1),
		period: period,
	}
}

// Seed warms up the EMA state with historical bars.
func (s *EmaStream) Seed(history []float64) {
	for _, p := range history {
		s.Update(p)
	}
}

// Update processes a closed bar in O(1) time.
func (s *EmaStream) Update(closedPrice float64) (float64, bool) {
	s.count++
	if s.count < s.period {
		s.initialSum += closedPrice
		return 0, false
	}
	if s.count == s.period {
		s.initialSum += closedPrice
		s.prev = s.initialSum / float64(s.period)
		return s.prev, true
	}
	s.prev = (closedPrice-s.prev)*s.k + s.prev
	return s.prev, true
}

// Peek returns the forming EMA value for intra-bar price ticks.
func (s *EmaStream) Peek(formingPrice float64) (float64, bool) {
	if s.count < s.period {
		return 0, false
	}
	return (formingPrice-s.prev)*s.k + s.prev, true
}

