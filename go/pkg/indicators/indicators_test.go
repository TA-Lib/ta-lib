package indicators

import (
	"math/rand"
	"testing"
)

func generatePrices(n int) []float64 {
	p := make([]float64, n)
	p[0] = 100.0
	r := rand.New(rand.NewSource(42))
	for i := 1; i < n; i++ {
		p[i] = p[i-1] + (r.Float64() - 0.495)
	}
	return p
}

func TestIndicators(t *testing.T) {
	prices := generatePrices(100)
	smaOut := make([]float64, 100)
	emaOut := make([]float64, 100)
	rsiOut := make([]float64, 100)

	// Test SMA
	smaRange := SmaBatch(prices, 10, smaOut)
	if smaRange.NbElement != 91 || smaRange.BegIdx != 9 {
		t.Fatalf("SMA failed: got %d elements, expected 91", smaRange.NbElement)
	}

	// Test EMA
	emaRange := EmaBatch(prices, 10, emaOut)
	if emaRange.NbElement != 91 {
		t.Fatalf("EMA failed: got %d elements, expected 91", emaRange.NbElement)
	}

	// Test RSI
	rsiRange := RsiBatch(prices, 14, rsiOut)
	if rsiRange.NbElement != 86 {
		t.Fatalf("RSI failed: got %d elements, expected 86", rsiRange.NbElement)
	}

	// Test Streaming SMA vs Batch SMA
	smaStream := NewSmaStream(10)
	for i := 0; i < 9; i++ {
		_, ready := smaStream.Update(prices[i])
		if ready {
			t.Fatal("SMA Stream reported ready too early")
		}
	}
	val, ready := smaStream.Update(prices[9])
	if !ready {
		t.Fatal("SMA Stream not ready at bar 10")
	}
	diff := val - smaOut[0]
	if diff < -1e-9 || diff > 1e-9 {
		t.Fatalf("Stream vs Batch mismatch: stream=%.6f, batch=%.6f", val, smaOut[0])
	}
}

func BenchmarkSmaBatch1MillionBars(b *testing.B) {
	prices := generatePrices(1_000_000)
	out := make([]float64, 1_000_000)
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		SmaBatch(prices, 20, out)
	}
}

func BenchmarkSmaStreamUpdateSingleTick(b *testing.B) {
	s := NewSmaStream(20)
	for i := 0; i < 20; i++ {
		s.Update(100.0)
	}
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		s.Update(105.0)
	}
}
