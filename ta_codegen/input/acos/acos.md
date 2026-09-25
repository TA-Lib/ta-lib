# ACOS

## Summary

Element-wise arc cosine of the input series.

## Formula

outReal[i] = acos(inReal[i])

## Notes

- Outside [-1, 1] there is no angle whose cosine is that value, so those elements come out NaN.

## Inputs

- `inReal` — input values (expected in [-1, 1])

## Outputs

- `outReal` — arc cosine of each input, in radians

## Aliases

Arc Cosine, Inverse Cosine, arccos

## See Also

COS · ASIN · ATAN

## References

- Wikipedia, *Inverse trigonometric functions*: [en.wikipedia.org/wiki/Inverse_trigonometric_functions](https://en.wikipedia.org/wiki/Inverse_trigonometric_functions)
