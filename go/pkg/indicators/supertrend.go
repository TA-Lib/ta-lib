package indicators

import "math"

// SupertrendBatch calculates Supertrend line and Direction (+1 for uptrend, -1 for downtrend).
func SupertrendBatch(high, low, close []float64, period int, multiplier float64,
	outTrend, outDir []float64) OutRange {

	n := len(close)
	if n <= period {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	atrBuf := make([]float64, n)
	atrRange := AtrBatch(high, low, close, period, atrBuf)
	if atrRange.NbElement == 0 {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	offset := atrRange.BegIdx
	outNb := atrRange.NbElement

	var prevUpper, prevLower, prevTrend float64
	prevDir := 1.0

	for i := 0; i < outNb; i++ {
		currIdx := i + offset
		mid := (high[currIdx] + low[currIdx]) * 0.5
		atr := atrBuf[i]

		basicUpper := mid + (multiplier * atr)
		basicLower := mid - (multiplier * atr)

		var finalUpper, finalLower float64
		if i == 0 {
			finalUpper = basicUpper
			finalLower = basicLower
			prevTrend = finalLower
		} else {
			if basicUpper < prevUpper || close[currIdx-1] > prevUpper {
				finalUpper = basicUpper
			} else {
				finalUpper = prevUpper
			}

			if basicLower > prevLower || close[currIdx-1] < prevLower {
				finalLower = basicLower
			} else {
				finalLower = prevLower
			}
		}

		var currTrend, currDir float64
		if prevTrend == prevUpper {
			if close[currIdx] > finalUpper {
				currTrend = finalLower
				currDir = 1.0
			} else {
				currTrend = finalUpper
				currDir = -1.0
			}
		} else {
			if close[currIdx] < finalLower {
				currTrend = finalUpper
				currDir = -1.0
			} else {
				currTrend = finalLower
				currDir = 1.0
			}
		}

		outTrend[i] = currTrend
		outDir[i] = currDir

		prevUpper = finalUpper
		prevLower = finalLower
		prevTrend = currTrend
		prevDir = currDir
	}
	_ = prevDir

	return OutRange{BegIdx: offset, NbElement: outNb}
}

// SupertrendStream provides O(1) streaming updates for live Supertrend.
type SupertrendStream struct {
	atrStream  *AtrStream
	multiplier float64
	prevUpper  float64
	prevLower  float64
	prevTrend  float64
	prevClose  float64
	isInit     bool
}

// NewSupertrendStream creates a streaming Supertrend instance.
func NewSupertrendStream(period int, multiplier float64) *SupertrendStream {
	return &SupertrendStream{
		atrStream:  NewAtrStream(period),
		multiplier: multiplier,
	}
}

// Update calculates Supertrend value and direction (+1 bull, -1 bear) on bar close.
func (s *SupertrendStream) Update(high, low, close float64) (trend, dir float64, ready bool) {
	atr, atrReady := s.atrStream.Update(high, low, close)
	if !atrReady {
		s.prevClose = close
		return 0, 0, false
	}

	mid := (high + low) * 0.5
	basicUpper := mid + (s.multiplier * atr)
	basicLower := mid - (s.multiplier * atr)

	var finalUpper, finalLower float64
	if !s.isInit {
		finalUpper = basicUpper
		finalLower = basicLower
		s.prevTrend = finalLower
		s.isInit = true
	} else {
		if basicUpper < s.prevUpper || s.prevClose > s.prevUpper {
			finalUpper = basicUpper
		} else {
			finalUpper = s.prevUpper
		}

		if basicLower > s.prevLower || s.prevClose < s.prevLower {
			finalLower = basicLower
		} else {
			finalLower = s.prevLower
		}
	}

	var currTrend, currDir float64
	if s.prevTrend == s.prevUpper {
		if close > finalUpper {
			currTrend = finalLower
			currDir = 1.0
		} else {
			currTrend = finalUpper
			currDir = -1.0
		}
	} else {
		if close < finalLower {
			currTrend = finalUpper
			currDir = -1.0
		} else {
			currTrend = finalLower
			currDir = 1.0
		}
	}

	s.prevUpper = finalUpper
	s.prevLower = finalLower
	s.prevTrend = currTrend
	s.prevClose = close

	return currTrend, currDir, true
}

func min(a, b float64) float64 {
	return math.Min(a, b)
}

func max(a, b float64) float64 {
	return math.Max(a, b)
}

