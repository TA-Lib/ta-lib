# TA-Lib for Go

The official TA-Lib in pure Go: every technical analysis function, batch and streaming. No C library to install.

The Go module is planned for official release in Q4 2026.

## Import

Import it as `talib "ta-lib.github.io/go"`. It needs Go 1.22 or later.

## Batch

Every function is a method of `Core`, taking the range of bars to compute, the inputs (oldest bar first), the parameters and the output slices:

```go
core := talib.NewCore()
closes := []float64{44.34, 44.09, 44.15, 43.61, 44.33, 44.83, 45.10, 45.42, 45.84, 46.08}
out := make([]float64, len(closes))
r, err := core.SMA(0, len(closes)-1, closes, 5, out)
if err != nil {
	log.Fatal(err)
}
fmt.Printf("SMA from bar %d: %.3f\n", r.BegIdx, out[:r.NBElement])
```

## Streaming

A stream opens on the history and takes one closed bar at a time, returning the value the batch call computes at that bar:

```go
s, last, err := core.SMAOpen(closes, 5)
if err != nil {
	log.Fatal(err)
}
next, err := s.Update(46.03)
if err != nil {
	log.Fatal(err)
}
fmt.Printf("SMA %.3f, then %.3f\n", last, next)
```

## Documentation

https://ta-lib.org/api/go/, and every function's page under https://ta-lib.org/functions/.
