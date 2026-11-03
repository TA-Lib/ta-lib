package indicators

import "math"

// BbandsLookback returns the lookback for Bollinger Bands.
func BbandsLookback(period int) int {
	return SmaLookback(period)
}

// BbandsBatch calculates Bollinger Bands (Upper, Middle, Lower) for backtesting.
func BbandsBatch(in []float64, period int, nbDevUp, nbDevDn float64,
	outUpper, outMiddle, outLower []float64) OutRange {

	lookback := BbandsLookback(period)
	n := len(in)
	if lookback < 0 || n <= lookback {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	smaRange := SmaBatch(in, period, outMiddle)
	outNb := smaRange.NbElement

	// Calculate standard deviation over the sliding window
	trailingIdx := 0
	invPeriod := 1.0 / float64(period)

	for i := 0; i < outNb; i++ {
		mean := outMiddle[i]
		var varianceSum float64
		for j := 0; j < period; j++ {
			diff := in[trailingIdx+j] - mean
			varianceSum += diff * diff
		}
		stdDev := math.Sqrt(varianceSum * invPeriod)
		outUpper[i] = mean + (stdDev * nbDevUp)
		outLower[i] = mean - (stdDev * nbDevDn)
		trailingIdx++
	}

	return OutRange{BegIdx: lookback, NbElement: outNb}
}

// BbandsStream provides O(1) streaming updates for Bollinger Bands.
type BbandsStream struct {
	smaStream *SmaStream
	ring      []float64
	cursor    int
	period    int
	nbDevUp   float64
	nbDevDn   float64
	invPeriod float64
}

// NewBbandsStream creates a streaming Bollinger Bands instance.
func NewBbandsStream(period int, nbDevUp, nbDevDn float64) *BbandsStream {
	if period < 1 {
		period = 1
	}
	return &BbandsStream{
		smaStream: NewSmaStream(period),
		ring:      make([]float64, period),
		period:    period,
		nbDevUp:   nbDevUp,
		nbDevDn:   nbDevDn,
		invPeriod: 1.0 / float64(period),
	}
}

// Update calculates (Upper, Middle, Lower) on candle close in O(1) time.
func (s *BbandsStream) Update(closedPrice float64) (upper, middle, lower float64, ready bool) {
	s.ring[s.cursor] = closedPrice
	s.cursor = (s.cursor + 1) % s.period

	mid, smaReady := s.smaStream.Update(closedPrice)
	if !smaReady {
		return 0, 0, 0, false
	}

	var varSum float64
	for i := 0; i < s.period; i++ {
		diff := s.ring[i] - mid
		varSum += diff * diff
	}
	stdDev := math.Sqrt(varSum * s.invPeriod)

	return mid + (stdDev * s.nbDevUp), mid, mid - (stdDev * s.nbDevDn), true
}

// Peek calculates forming (Upper, Middle, Lower) on intra-bar ticks.
func (s *BbandsStream) Peek(formingPrice float64) (upper, middle, lower float64, ready bool) {
	mid, smaReady := s.smaStream.Peek(formingPrice)
	if !smaReady {
		return 0, 0, 0, false
	}

	// Substitute oldest value in ring with formingPrice
	oldest := s.ring[s.cursor]
	_ = oldest
	var varSum float64
	for i := 0; i < s.period; i++ {
		val := s.ring[i]
		if i == s.cursor {
			val = formingPrice
		}
		diff := val - mid
		varSum += diff * diff
	}
	stdDev := math.Sqrt(varSum * s.invPeriod)

	return mid + (stdDev * s.nbDevUp), mid, mid - (stdDev * s.nbDevDn), true
}

