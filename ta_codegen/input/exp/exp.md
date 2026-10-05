# EXP

## Summary

Element-wise base-e exponential of the input series.

## Formula

outReal[i] = exp(inReal[i]) = e^{inReal[i]}

## Notes

- An input above about 709.78 has a result too large for a double, so those elements come out positive infinity.

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — e raised to each input value

## Aliases

exponential, e^x

## See Also

LN · SQRT

## References

- Wikipedia, *Exponential function*: [en.wikipedia.org/wiki/Exponential_function](https://en.wikipedia.org/wiki/Exponential_function)
