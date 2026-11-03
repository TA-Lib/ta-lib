package indicators

import "math"

// AtrLookback returns the lookback for ATR.
func AtrLookback(period int) int {
	if period < 1 {
		return -1
	}
	return period
}

// TrueRange computes the True Range for a single bar.
func TrueRange(high, low, prevClose float64) float64 {
	tr := high - low
	diff1 := math.Abs(high - prevClose)
	diff2 := math.Abs(low - prevClose)

	if diff1 > tr {
		tr = diff1
	}
	if diff2 > tr {
		tr = diff2
	}
	return tr
}

// AtrBatch calculates Average True Range across historical price bars.
func AtrBatch(high, low, close []float64, period int, out []float64) OutRange {
	lookback := AtrLookback(period)
	n := len(close)
	if lookback < 0 || n <= lookback || len(high) != n || len(low) != n {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	var trTotal float64
	// First bar has no prevClose: TR = high[0] - low[0]
	trTotal += high[0] - low[0]

	for i := 1; i < period; i++ {
		trTotal += TrueRange(high[i], low[i], close[i-1])
	}

	prevAtr := trTotal / float64(period)
	out[0] = prevAtr
	outIdx := 1

	decay := float64(period - 1)
	invPeriod := 1.0 / float64(period)

	for i := period; i < n; i++ {
		tr := TrueRange(high[i], low[i], close[i-1])
		prevAtr = (prevAtr*decay + tr) * invPeriod
		out[outIdx] = prevAtr
		outIdx++
	}

	return OutRange{BegIdx: lookback, NbElement: outIdx}
}

// AtrStream provides O(1) streaming updates for live ATR.
type AtrStream struct {
	period    int
	count     int
	prevClose float64
	prevAtr   float64
	trSum     float64
	decay     float64
	invPeriod float64
}

// NewAtrStream creates a streaming ATR instance.
func NewAtrStream(period int) *AtrStream {
	if period < 1 {
		period = 1
	}
	return &AtrStream{
		period:    period,
		decay:     float64(period - 1),
		invPeriod: 1.0 / float64(period),
	}
}

// Update processes a closed bar (high, low, close) in O(1) time.
func (s *AtrStream) Update(high, low, close float64) (float64, bool) {
	s.count++
	var tr float64

	if s.count == 1 {
		tr = high - low
		s.prevClose = close
		s.trSum += tr
		return 0, false
	}

	tr = TrueRange(high, low, s.prevClose)
	s.prevClose = close

	if s.count <= s.period {
		s.trSum += tr
		if s.count == s.period {
			s.prevAtr = s.trSum * s.invPeriod
			return s.prevAtr, true
		}
		return 0, false
	}

	s.prevAtr = (s.prevAtr*s.decay + tr) * s.invPeriod
	return s.prevAtr, true
}

// Peek returns the forming ATR on an intra-bar tick.
func (s *AtrStream) Peek(formingHigh, formingLow float64) (float64, bool) {
	if s.count < s.period {
		return 0, false
	}
	tr := TrueRange(formingHigh, formingLow, s.prevClose)
	tempAtr := (s.prevAtr*s.decay + tr) * s.invPeriod
	return tempAtr, true
}

