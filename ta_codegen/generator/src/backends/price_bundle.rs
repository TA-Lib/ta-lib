//! Folding the expanded price components back into one `ta_abstract` descriptor.
//!
//! Two representations of the same price bar coexist, and both are load-bearing:
//!
//! - the **call signature** — one array per component, `TA_STOCH(startIdx, endIdx,
//!   inHigh, inLow, inClose, ...)`. This is what a caller writes, and what
//!   [`crate::ir::FuncDef::inputs`] holds.
//! - the **`ta_abstract` descriptor** — a *single* `TA_Input_Price` parameter named
//!   `inPriceHLC` whose flags word names the components. This is public ABI: it has been
//!   in `include/ta_abstract.h` since the 2002 initial revision, and wrappers read it
//!   (ta-lib-python renders it as `{'prices': ['high','low','close']}` with `num_inputs`
//!   of 1). It must not change.
//!
//! `parser::yaml` expands the bundle and tags each argument with a [`PriceRef`]; this
//! module folds it back. Every backend that needs the descriptor — the C tables and
//! frames, the Rust `abstract_api`, the Java `@InputParameterInfo` annotations — goes
//! through [`group`] and [`canonical_name`] here, so the name and flags a wrapper sees
//! are computed once.
//!
//! Before this module each backend regrouped by *recognising component names* in a run of
//! consecutive inputs. That is a guess about intent, and it has no oracle: every backend
//! shared the same convention, so all four would agree while all four were wrong. A
//! function taking a standalone volume series, or a `real` argument between two
//! components, would have been folded into a bundle that was never declared — silently
//! changing the abstract API. [`group`] instead reads the declaration, and rejects a
//! bundle whose components are not contiguous rather than inventing a grouping for it.

use crate::ir::{Input, PriceComponent};

/// Canonical component order for an `inPriceXXX` name, matching the historical
/// `TA_DEF_UI_Input_Price_*` constants (`OHLCV`, never `VOHLC`).
const CANONICAL_ORDER: [PriceComponent; 6] = [
    PriceComponent::Open,
    PriceComponent::High,
    PriceComponent::Low,
    PriceComponent::Close,
    PriceComponent::Volume,
    PriceComponent::OpenInterest,
];

/// A function's inputs as `ta_abstract` sees them: price components folded back together,
/// everything else one-to-one with the signature.
#[derive(Debug, Clone)]
pub enum Grouped<'a> {
    /// A plain argument — real, integer or enum.
    Single(&'a Input),
    /// One `TA_Input_Price` descriptor and the arguments it expands to, in signature order.
    Price(Vec<&'a Input>),
}

/// Fold the expanded inputs into the `ta_abstract` view.
///
/// # Panics
/// If a declared bundle's components are not contiguous in the signature. Nothing in the
/// corpus does this, and the descriptor has no way to express it: `TA_Input_Price` is one
/// parameter at one index, so an interleaved argument could not be ordered against it.
/// Failing here is the point — the alternative is emitting a plausible but wrong grouping.
#[must_use]
pub fn group(inputs: &[Input]) -> Vec<Grouped<'_>> {
    let mut result: Vec<Grouped<'_>> = Vec::new();
    let mut open: Option<usize> = None; // bundle group currently being accumulated
    let mut seen: Vec<usize> = Vec::new(); // bundle groups already closed

    for input in inputs {
        let Some(price) = input.price else {
            if let Some(prev) = open.take() {
                seen.push(prev);
            }
            result.push(Grouped::Single(input));
            continue;
        };
        if open == Some(price.group) {
            let Some(Grouped::Price(components)) = result.last_mut() else {
                unreachable!("an open bundle is always the last group pushed")
            };
            components.push(input);
            continue;
        }
        assert!(
            !seen.contains(&price.group),
            "input {:?} resumes price bundle {} after another input interrupted it — \
             ta_abstract describes a bundle as one parameter at one index, so its \
             components must be contiguous in the signature",
            input.name,
            price.group
        );
        if let Some(prev) = open {
            seen.push(prev);
        }
        open = Some(price.group);
        result.push(Grouped::Price(vec![input]));
    }
    result
}

/// The components of a folded bundle, in **signature order** — the order the function
/// takes them as arguments. Use this for anything positional (building a call, declaring
/// parameters); use [`canonical`] for the descriptor's name and flags.
///
/// # Panics
/// If any input in the slice is not a price component.
#[must_use]
pub fn signature_order(bundle: &[&Input]) -> Vec<PriceComponent> {
    bundle
        .iter()
        .map(|i| {
            i.price
                .unwrap_or_else(|| panic!("input {:?} is not a price component", i.name))
                .component
        })
        .collect()
}

/// The components of a folded bundle, in canonical `OHLCV` order — the order the
/// `TA_DEF_UI_Input_Price_*` constants and the `inPriceXXX` names have always used.
///
/// # Panics
/// If any input in the slice is not a price component.
#[must_use]
pub fn components(bundle: &[&Input]) -> Vec<PriceComponent> {
    canonical(&signature_order(bundle))
}

/// Reorder a component list into canonical `OHLCV` order, dropping duplicates.
#[must_use]
pub fn canonical(components: &[PriceComponent]) -> Vec<PriceComponent> {
    CANONICAL_ORDER
        .iter()
        .copied()
        .filter(|c| components.contains(c))
        .collect()
}

/// The `ta_abstract` parameter name for a bundle: `[High, Low, Close]` -> `inPriceHLC`.
///
/// Public ABI — wrappers key off this string (ta-lib-python maps any `inPrice*` to its
/// `'prices'` input key).
#[must_use]
pub fn canonical_name(components: &[PriceComponent]) -> String {
    let mut name = String::from("inPrice");
    for c in CANONICAL_ORDER {
        if components.contains(&c) {
            name.push(c.suffix_letter());
        }
    }
    name
}

/// The `TA_IN_PRICE_*` flags word for a bundle — public ABI.
///
/// Bitwise OR, not a sum: a repeated component must not carry into the next bit
/// (`[High, High]` summed is `0x4`, which reads as `LOW`).
#[must_use]
pub fn flags(components: &[PriceComponent]) -> u32 {
    components.iter().fold(0, |acc, c| acc | c.flag_bit())
}

/// The suffix used by the `TA_DEF_UI_Input_Price_*` C constant names (`HLC`).
#[must_use]
pub fn suffix(components: &[PriceComponent]) -> String {
    canonical_name(components)["inPrice".len()..].to_string()
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ir::{ParamType, PriceRef};

    fn comp(c: PriceComponent, group: usize) -> Input {
        Input {
            name: c.input_name().to_string(),
            param_type: ParamType::Real,
            price: Some(PriceRef { group, component: c }),
        }
    }

    fn plain(name: &str) -> Input {
        Input::new(name, ParamType::Real)
    }

    /// The 16 HLC functions: three arguments, one descriptor.
    #[test]
    fn a_bundle_folds_to_one_descriptor() {
        let inputs = vec![
            comp(PriceComponent::High, 0),
            comp(PriceComponent::Low, 0),
            comp(PriceComponent::Close, 0),
        ];
        let g = group(&inputs);
        assert_eq!(g.len(), 1);
        let Grouped::Price(bundle) = &g[0] else {
            panic!("expected a bundle")
        };
        assert_eq!(canonical_name(&components(bundle)), "inPriceHLC");
        assert_eq!(flags(&components(bundle)), 0x2 | 0x4 | 0x8);
    }

    /// OBV/VWMA: a plain `inReal` followed by a one-component bundle. The old
    /// name-recognising fold got this right only because the volume happened to come
    /// last; the declaration says so outright.
    #[test]
    fn a_plain_input_before_a_bundle_stays_separate() {
        let inputs = vec![plain("inReal"), comp(PriceComponent::Volume, 0)];
        let g = group(&inputs);
        assert_eq!(g.len(), 2);
        assert!(matches!(g[0], Grouped::Single(_)));
        let Grouped::Price(bundle) = &g[1] else {
            panic!("expected a bundle")
        };
        assert_eq!(canonical_name(&components(bundle)), "inPriceV");
    }

    /// The regression the old fold could not see: a volume argument that is *not* part of
    /// a bundle must stay its own input, not get absorbed into the neighbouring bar.
    #[test]
    fn an_undeclared_component_name_is_not_absorbed() {
        let inputs = vec![
            comp(PriceComponent::High, 0),
            comp(PriceComponent::Low, 0),
            plain("inVolume"),
        ];
        let g = group(&inputs);
        assert_eq!(g.len(), 2);
        let Grouped::Price(bundle) = &g[0] else {
            panic!("expected a bundle")
        };
        assert_eq!(canonical_name(&components(bundle)), "inPriceHL");
        assert!(matches!(g[1], Grouped::Single(_)));
    }

    /// Two independent bundles stay independent even when adjacent.
    #[test]
    fn adjacent_bundles_are_not_merged() {
        let inputs = vec![
            comp(PriceComponent::High, 0),
            comp(PriceComponent::Low, 0),
            comp(PriceComponent::Close, 1),
            comp(PriceComponent::Volume, 1),
        ];
        let g = group(&inputs);
        assert_eq!(g.len(), 2);
        let (Grouped::Price(a), Grouped::Price(b)) = (&g[0], &g[1]) else {
            panic!("expected two bundles")
        };
        assert_eq!(canonical_name(&components(a)), "inPriceHL");
        assert_eq!(canonical_name(&components(b)), "inPriceCV");
    }

    /// A descriptor sits at one input index, so an interrupted bundle has no valid
    /// encoding — refuse rather than emit a grouping nobody declared.
    #[test]
    #[should_panic(expected = "must be contiguous")]
    fn a_non_contiguous_bundle_is_rejected() {
        let inputs = vec![
            comp(PriceComponent::High, 0),
            plain("inReal"),
            comp(PriceComponent::Low, 0),
        ];
        let _ = group(&inputs);
    }

    /// The name is canonical `OHLCV`, whatever order the YAML listed the components in.
    #[test]
    fn the_canonical_name_ignores_declaration_order() {
        let cs = [
            PriceComponent::Close,
            PriceComponent::Open,
            PriceComponent::High,
            PriceComponent::Low,
        ];
        assert_eq!(canonical_name(&cs), "inPriceOHLC");
        assert_eq!(suffix(&cs), "OHLC");
    }

    /// The descriptor canonicalises; anything positional must not. A YAML declaring
    /// `price_components: [volume, close]` takes `(inVolume, inClose)` in that order, and
    /// a backend that emitted the arguments canonically would bind them to the wrong
    /// parameters — compiling cleanly and computing the wrong numbers.
    #[test]
    fn signature_order_survives_a_non_canonical_declaration() {
        let inputs = vec![
            comp(PriceComponent::Volume, 0),
            comp(PriceComponent::Close, 0),
        ];
        let g = group(&inputs);
        let Grouped::Price(bundle) = &g[0] else {
            panic!("expected a bundle")
        };
        assert_eq!(
            signature_order(bundle),
            vec![PriceComponent::Volume, PriceComponent::Close]
        );
        // ...while the ABI descriptor is canonical regardless.
        assert_eq!(canonical_name(&components(bundle)), "inPriceCV");
        assert_eq!(flags(&components(bundle)), 0x8 | 0x10);
    }

    /// A repeated component must not carry into the neighbouring bit.
    #[test]
    fn flags_are_or_ed_not_summed() {
        assert_eq!(flags(&[PriceComponent::High, PriceComponent::High]), 0x2);
    }
}
