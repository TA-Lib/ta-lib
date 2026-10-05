# SINH

## Summary

Element-wise hyperbolic sine of the input series.

## Formula

outReal[i] = sinh(inReal[i])

## Notes

- An input beyond about 710.47 in magnitude has a result too large for a double, so those elements come out positive or negative infinity.

## Inputs

- `inReal` — Input series

## Outputs

- `outReal` — Hyperbolic sine of each input value

## Aliases

Hyperbolic Sine

## See Also

COSH · TANH

## References

- Wikipedia, *Hyperbolic functions*: [en.wikipedia.org/wiki/Hyperbolic_functions](https://en.wikipedia.org/wiki/Hyperbolic_functions)
