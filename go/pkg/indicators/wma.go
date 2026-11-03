package indicators

// WmaLookback returns the lookback for WMA.
func WmaLookback(period int) int {
	if period < 1 {
		return -1
	}
	return period - 1
}

// WmaBatch calculates Weighted Moving Average for backtesting.
func WmaBatch(in []float64, period int, out []float64) OutRange {
	lookback := WmaLookback(period)
	n := len(in)
	if lookback < 0 || n <= lookback {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	divider := float64(period * (period + 1) / 2)
	invDivider := 1.0 / divider

	var weightedSum, sum float64
	for i := 0; i < period; i++ {
		val := in[i]
		sum += val
		weightedSum += val * float64(i+1)
	}

	out[0] = weightedSum * invDivider
	outIdx := 1
	trailingIdx := 0

	for i := period; i < n; i++ {
		val := in[i]
		oldest := in[trailingIdx]
		trailingIdx++

		weightedSum = weightedSum - sum + val*float64(period)
		sum = sum - oldest + val
		out[outIdx] = weightedSum * invDivider
		outIdx++
	}

	return OutRange{BegIdx: lookback, NbElement: outIdx}
}

// WmaStream provides O(1) streaming updates for WMA.
type WmaStream struct {
	ring        []float64
	cursor      int
	period      int
	count       int
	sum         float64
	weightedSum float64
	invDivider  float64
}

// NewWmaStream creates a streaming WMA instance.
func NewWmaStream(period int) *WmaStream {
	if period < 1 {
		period = 1
	}
	divider := float64(period * (period + 1) / 2)
	return &WmaStream{
		ring:       make([]float64, period),
		period:     period,
		invDivider: 1.0 / divider,
	}
}

// Update calculates new WMA on bar close in O(1) time.
func (s *WmaStream) Update(closedPrice float64) (float64, bool) {
	s.count++
	if s.count <= s.period {
		s.ring[s.cursor] = closedPrice
		s.cursor = (s.cursor + 1) % s.period
		s.sum += closedPrice
		s.weightedSum += closedPrice * float64(s.count)
		if s.count == s.period {
			return s.weightedSum * s.invDivider, true
		}
		return 0, false
	}

	oldest := s.ring[s.cursor]
	s.ring[s.cursor] = closedPrice
	s.cursor = (s.cursor + 1) % s.period

	s.weightedSum = s.weightedSum - s.sum + closedPrice*float64(s.period)
	s.sum = s.sum - oldest + closedPrice

	return s.weightedSum * s.invDivider, true
}

// Peek returns the WMA for an intra-bar forming tick.
func (s *WmaStream) Peek(formingPrice float64) (float64, bool) {
	if s.count < s.period {
		return 0, false
	}
	oldest := s.ring[s.cursor]
	tempWeighted := s.weightedSum - s.sum + formingPrice*float64(s.period)
	_ = oldest
	return tempWeighted * s.invDivider, true
}

