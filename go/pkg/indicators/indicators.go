package indicators

// OutRange represents the output index bounds returned by batch indicators.
type OutRange struct {
	BegIdx    int // First valid output index relative to input
	NbElement int // Number of output elements calculated
}

// Indicator defines common properties for technical indicators.
type Indicator interface {
	Name() string
	Lookback() int
}

