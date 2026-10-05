# COSH

## Summary

Element-wise hyperbolic cosine of the input series.

## Formula

outReal[i] = cosh(inReal[i]) = (e^{inReal[i]} + e^{-inReal[i]}) / 2

## Notes

- An input beyond about 710.47 in magnitude has a result too large for a double, so those elements come out positive infinity.

## Inputs

- `inReal` — Input values to transform

## Outputs

- `outReal` — Hyperbolic cosine of each input

## Aliases

Hyperbolic Cosine

## See Also

SINH · TANH · COS

## References

- Wikipedia, *Hyperbolic functions*: [en.wikipedia.org/wiki/Hyperbolic_functions](https://en.wikipedia.org/wiki/Hyperbolic_functions)
