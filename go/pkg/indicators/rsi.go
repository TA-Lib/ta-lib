package indicators

// RsiLookback returns the lookback for RSI.
func RsiLookback(period int) int {
	if period < 1 {
		return -1
	}
	return period
}

// RsiBatch calculates Relative Strength Index (Wilder) for backtesting.
func RsiBatch(in []float64, period int, out []float64) OutRange {
	lookback := RsiLookback(period)
	n := len(in)
	if lookback < 0 || n <= lookback {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	var prevGain, prevLoss float64
	for i := 1; i <= period; i++ {
		diff := in[i] - in[i-1]
		if diff > 0 {
			prevGain += diff
		} else {
			prevLoss -= diff
		}
	}
	prevGain /= float64(period)
	prevLoss /= float64(period)

	outIdx := 0
	if prevLoss == 0 {
		out[outIdx] = 100.0
	} else {
		out[outIdx] = 100.0 - (100.0 / (1.0 + prevGain/prevLoss))
	}
	outIdx++

	invPeriod := 1.0 / float64(period)
	decay := float64(period - 1)

	for i := period + 1; i < n; i++ {
		diff := in[i] - in[i-1]
		var gain, loss float64
		if diff > 0 {
			gain = diff
		} else {
			loss = -diff
		}
		prevGain = (prevGain*decay + gain) * invPeriod
		prevLoss = (prevLoss*decay + loss) * invPeriod

		if prevLoss == 0 {
			out[outIdx] = 100.0
		} else {
			out[outIdx] = 100.0 - (100.0 / (1.0 + prevGain/prevLoss))
		}
		outIdx++
	}

	return OutRange{BegIdx: lookback, NbElement: outIdx}
}

// RsiStream provides O(1) streaming Wilder RSI for live tick feeds.
type RsiStream struct {
	period    int
	count     int
	prevPrice float64
	prevGain  float64
	prevLoss  float64
	invPeriod float64
	decay     float64
}

// NewRsiStream creates a streaming RSI instance.
func NewRsiStream(period int) *RsiStream {
	if period < 1 {
		period = 1
	}
	return &RsiStream{
		period:    period,
		invPeriod: 1.0 / float64(period),
		decay:     float64(period - 1),
	}
}

// Seed warms up the RSI stream with historical bars.
func (s *RsiStream) Seed(history []float64) {
	for _, p := range history {
		s.Update(p)
	}
}

// Update computes the new RSI on a closed candle in O(1) time.
func (s *RsiStream) Update(closedPrice float64) (float64, bool) {
	s.count++
	if s.count == 1 {
		s.prevPrice = closedPrice
		return 0, false
	}
	diff := closedPrice - s.prevPrice
	s.prevPrice = closedPrice

	var gain, loss float64
	if diff > 0 {
		gain = diff
	} else {
		loss = -diff
	}

	if s.count <= s.period {
		s.prevGain += gain
		s.prevLoss += loss
		return 0, false
	}
	if s.count == s.period+1 {
		s.prevGain = (s.prevGain + gain) * s.invPeriod
		s.prevLoss = (s.prevLoss + loss) * s.invPeriod
	} else {
		s.prevGain = (s.prevGain*s.decay + gain) * s.invPeriod
		s.prevLoss = (s.prevLoss*s.decay + loss) * s.invPeriod
	}

	if s.prevLoss == 0 {
		return 100.0, true
	}
	rs := s.prevGain / s.prevLoss
	return 100.0 - (100.0 / (1.0 + rs)), true
}

// Peek calculates the forming RSI on intra-bar ticks without mutating state.
func (s *RsiStream) Peek(formingPrice float64) (float64, bool) {
	if s.count < s.period+1 {
		return 0, false
	}
	diff := formingPrice - s.prevPrice
	var gain, loss float64
	if diff > 0 {
		gain = diff
	} else {
		loss = -diff
	}
	tempGain := (s.prevGain*s.decay + gain) * s.invPeriod
	tempLoss := (s.prevLoss*s.decay + loss) * s.invPeriod

	if tempLoss == 0 {
		return 100.0, true
	}
	return 100.0 - (100.0 / (1.0 + tempGain/tempLoss)), true
}

