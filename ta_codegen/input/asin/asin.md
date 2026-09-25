# ASIN

## Summary

Element-wise arcsine of the input series.

## Formula

outReal[i] = asin(inReal[i])

## Notes

- Outside [-1, 1] there is no angle whose sine is that value, so those elements come out NaN.

## Inputs

- `inReal` — Input values (domain [-1,1] for a real result)

## Outputs

- `outReal` — Arcsine of each input, in radians

## Aliases

arcsine, inverse sine

## See Also

ACOS · ATAN · SIN · COS

## References

- Wikipedia, *Inverse trigonometric functions*: [en.wikipedia.org/wiki/Inverse_trigonometric_functions](https://en.wikipedia.org/wiki/Inverse_trigonometric_functions)
