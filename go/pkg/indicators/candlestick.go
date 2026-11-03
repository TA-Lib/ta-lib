package indicators

import "math"

// Candlestick Pattern Signals:
// +100 = Bullish pattern
// -100 = Bearish pattern
//    0 = No pattern

// CdlDojiBatch identifies Doji candles across historical series.
func CdlDojiBatch(open, high, low, close []float64, out []int) OutRange {
	n := len(close)
	if n == 0 {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	for i := 0; i < n; i++ {
		body := math.Abs(close[i] - open[i])
		candleRange := high[i] - low[i]

		// Doji: body is less than 10% of total candle range
		if candleRange > 0 && body <= candleRange*0.1 {
			out[i] = 100
		} else {
			out[i] = 0
		}
	}
	return OutRange{BegIdx: 0, NbElement: n}
}

// CdlHammerBatch identifies Bullish Hammer patterns.
func CdlHammerBatch(open, high, low, close []float64, out []int) OutRange {
	n := len(close)
	if n == 0 {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	for i := 0; i < n; i++ {
		body := math.Abs(close[i] - open[i])
		candleRange := high[i] - low[i]
		lowerShadow := math.Min(open[i], close[i]) - low[i]
		upperShadow := high[i] - math.Max(open[i], close[i])

		// Hammer: lower shadow at least 2x body, very small upper shadow
		if candleRange > 0 && lowerShadow >= body*2.0 && upperShadow <= body*0.2 {
			out[i] = 100
		} else {
			out[i] = 0
		}
	}
	return OutRange{BegIdx: 0, NbElement: n}
}

// CdlEngulfingBatch identifies Bullish (+100) or Bearish (-100) Engulfing patterns.
func CdlEngulfingBatch(open, high, low, close []float64, out []int) OutRange {
	n := len(close)
	if n < 2 {
		return OutRange{BegIdx: 0, NbElement: 0}
	}

	out[0] = 0
	for i := 1; i < n; i++ {
		prevBull := close[i-1] > open[i-1]
		currBull := close[i] > open[i]

		// Bullish Engulfing: previous bear, current bull, current body completely engulfs previous body
		if !prevBull && currBull && open[i] <= close[i-1] && close[i] >= open[i-1] {
			out[i] = 100
		} else if prevBull && !currBull && open[i] >= close[i-1] && close[i] <= open[i-1] {
			// Bearish Engulfing
			out[i] = -100
		} else {
			out[i] = 0
		}
	}
	return OutRange{BegIdx: 1, NbElement: n - 1}
}

// CdlStreamer provides live single-candle pattern detection in O(1) time.
type CdlStreamer struct {
	prevOpen  float64
	prevHigh  float64
	prevLow   float64
	prevClose float64
	hasPrev   bool
}

func NewCdlStreamer() *CdlStreamer {
	return &CdlStreamer{}
}

// EvaluateBar evaluates candle patterns on bar close.
func (c *CdlStreamer) EvaluateBar(open, high, low, close float64) (isDoji, isHammer bool, engulfing int) {
	body := math.Abs(close - open)
	candleRange := high - low
	lowerShadow := math.Min(open, close) - low
	upperShadow := high - math.Max(open, close)

	if candleRange > 0 && body <= candleRange*0.1 {
		isDoji = true
	}
	if candleRange > 0 && lowerShadow >= body*2.0 && upperShadow <= body*0.2 {
		isHammer = true
	}

	if c.hasPrev {
		prevBull := c.prevClose > c.prevOpen
		currBull := close > open
		if !prevBull && currBull && open <= c.prevClose && close >= c.prevOpen {
			engulfing = 100
		} else if prevBull && !currBull && open >= c.prevClose && close <= c.prevOpen {
			engulfing = -100
		}
	}

	c.prevOpen = open
	c.prevHigh = high
	c.prevLow = low
	c.prevClose = close
	c.hasPrev = true

	return isDoji, isHammer, engulfing
}

