# DIV

## Summary

Element-wise division of two input series.

## Formula

outReal[i] = inReal0[i] / inReal1[i]

## Notes

- Zero divided by zero gives NaN; anything else divided by zero gives positive or negative infinity. Neither is reported as an error.

## Inputs

- `inReal0` — Dividend (numerator) series
- `inReal1` — Divisor (denominator) series

## Outputs

- `outReal` — Per-element quotient inReal0/inReal1

## Aliases

Vector Arithmetic Divide, Divide

## See Also

MULT · ADD · SUB
