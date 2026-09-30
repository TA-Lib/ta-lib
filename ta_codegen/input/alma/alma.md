# ALMA

## Summary

Arnaud Legoux Moving Average: the last N inputs weighted by a Gaussian whose peak sits a fraction `offset` of the way from the oldest to the newest bar. Moving the offset toward 1 puts the peak on recent bars and cuts the lag; moving it toward 0 puts the peak on old bars and lags most. Smoothing is greatest with the peak mid-window, near 0.5. Sigma sets the width: the Gaussian's standard deviation is `N / sigma` bars, so a larger sigma concentrates the weight around the peak and smooths less.

The weights are non-negative and sum to one, so the line stays within the range of its window, up to rounding. At the default shape and a period of 5 or more, it lags about half as much as an `SMA` of the same period and passes about twice its noise.

## Formula

N = optInTimePeriod, m = floor( offset * (N-1) ), s = N / sigma

g[j] = exp( -(j-m)^2 / (2 s^2) ),  j = 0 .. N-1, j = 0 the oldest bar of the window

ALMA[t] = sum_j ( g[j] / sum_k g[k] ) * x[t-N+1+j]

The first output is at bar N-1.

## Notes

- The peak is floored to a whole bar, as in the authors' code. At periods below about 5 this puts it on an older bar: at period 2 the line is almost entirely the previous bar.
- `TA_MAType_ALMA` runs this function at the default sigma and offset, so the line in `MA` and every function taking an MAType has the lag and noise stated above.
- The published paper's summary formula uses a different parameterisation; this is the form of the authors' own implementation.

## Inputs

- `inReal` — Data on which to compute the average

## Outputs

- `outReal` — Arnaud Legoux Moving Average line

## Parameters

- `optInTimePeriod` — Number of bars in the window
- `optInSigma` — Divides the period to give the Gaussian's width in bars
- `optInOffset` — Position of the peak weight, 0 at the oldest bar and 1 at the newest

## See Also

WMA · SMA · TRIMA · HMA

## References

- Arnaud Legoux and Dimitris Kouzis-Loukas, ["ALMA; In search for the perfect Moving Average"](https://web.archive.org/web/20110904091012/www.arnaudlegoux.com/wp-content/uploads/2011/03/ALMA-Arnaud-Legoux-Moving-Average.pdf), November 2009. The authors' NinjaTrader implementation (2010) fixes the operational form above.
