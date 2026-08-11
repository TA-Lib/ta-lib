/// Return codes for TA-Lib function calls.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[non_exhaustive]
pub enum RetCode {
    /// Function completed successfully.
    Success,
    /// One or more parameters are invalid.
    BadParam,
    /// The start index is out of range.
    OutOfRangeStartIndex,
    /// The end index is out of range or less than start index.
    OutOfRangeEndIndex,
    /// C parity only, never returned here: an allocation failure terminates the process (#178).
    AllocErr,
    /// Internal error occurred.
    InternalError,
}

impl std::fmt::Display for RetCode {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let s = match self {
            RetCode::Success => "success",
            RetCode::BadParam => "bad parameter",
            RetCode::OutOfRangeStartIndex => "start index out of range",
            RetCode::OutOfRangeEndIndex => "end index out of range",
            RetCode::AllocErr => "allocation error",
            RetCode::InternalError => "internal error",
        };
        f.write_str(s)
    }
}

/// `RetCode` is the error type of the stream tier's `Result`s (`SMA_Open` and
/// friends), so it composes with `?` into `Box<dyn Error>`/anyhow contexts.
impl std::error::Error for RetCode {}

/// Compatibility mode for technical analysis calculations.
///
/// Crate-internal and pinned to [`Compatibility::Default`]: the variant notion is
/// not maintained, so the Rust API never exposes a way to select one. The variant
/// branches in the generated indicators are dead code pending their removal.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum Compatibility {
    /// Default TA-Lib compatibility mode.
    Default,
    /// Metastock-compatible calculation mode.
    Metastock,
}

/// Identifies functions that have an unstable period.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[non_exhaustive]
pub enum FuncUnstId {
    ADX,
    /// Reserved: was ADXR, knob was inert (#129); kept for ABI, reusable.
    UNUSED_1,
    ATR,
    CMO,
    DX,
    EMA,
    HT_DCPERIOD,
    HT_DCPHASE,
    HT_PHASOR,
    HT_SINE,
    HT_TRENDLINE,
    HT_TRENDMODE,
    /// Reserved: was IMI, reclassified stable (#14); kept for ABI, reusable.
    UNUSED_12,
    KAMA,
    MAMA,
    /// Reserved: was MFI, reclassified stable (#4); kept for ABI, reusable.
    UNUSED_15,
    MINUS_DI,
    MINUS_DM,
    NATR,
    PLUS_DI,
    PLUS_DM,
    RSI,
    /// Reserved: was STOCHRSI, knob was inert (#129); kept for ABI, reusable.
    UNUSED_22,
    T3,
    /// Wildcard: set the unstable period for all functions at once.
    ///
    /// Pinned rather than sitting one past the last function id, so that adding
    /// an indicator can never move it. Mirrors C's `TA_FUNC_UNST_ALL`.
    ALL = 65535,
}

/// Number of [`FuncUnstId`] function ids — the size of the unstable-period
/// table. Not an id, and not [`FuncUnstId::ALL`]. Mirrors C's
/// `TA_FUNC_UNST_COUNT`.
pub const FUNC_UNST_COUNT: usize = 24;

/// Pass this for a `f64` optional parameter to select its documented default —
/// C's `TA_REAL_DEFAULT`. The value sits deliberately outside
/// [`REAL_MIN`]`..=`[`REAL_MAX`], so it can never collide with real data.
pub const REAL_DEFAULT: f64 = -4e37;

/// Pass this for an `i32` optional parameter to select its documented default —
/// C's `TA_INTEGER_DEFAULT`. One below [`INTEGER_MIN`], for the same reason.
pub const INTEGER_DEFAULT: i32 = i32::MIN;

/// Lowest value a `f64` optional parameter may take (C's `TA_REAL_MIN`). A
/// parameter outside its documented range returns [`RetCode::BadParam`].
pub const REAL_MIN: f64 = -3e37;
/// Highest value a `f64` optional parameter may take (C's `TA_REAL_MAX`).
pub const REAL_MAX: f64 = 3e37;
/// Lowest value an `i32` optional parameter may take (C's `TA_INTEGER_MIN`).
pub const INTEGER_MIN: i32 = i32::MIN + 1;
/// Highest value an `i32` optional parameter may take (C's `TA_INTEGER_MAX`).
pub const INTEGER_MAX: i32 = i32::MAX;

/// Largest value `startIdx` or `endIdx` may take (C's `TA_MAX_INDEX`). Above it,
/// a call returns [`RetCode::OutOfRangeStartIndex`] or
/// [`RetCode::OutOfRangeEndIndex`] rather than computing.
///
/// This bounds the **API domain** and nothing else. In particular it is not an
/// accuracy guarantee: a handful of functions accumulate rounding error that
/// grows with the series length and are already imprecise well below this cap.
/// It is a `usize` here and an `int` in C, Java and C#, so the same call is
/// accepted or rejected identically in all four.
pub const MAX_INDEX: usize = 100_000_000;

/// A single candlestick setting entry.
#[derive(Debug, Clone, Copy)]
pub struct CandleSetting {
    /// Range type: 0 = RealBody, 1 = HighLow, 2 = Shadows.
    pub range_type: i32,
    /// Period length for averaging.
    pub avg_period: i32,
    /// Scaling factor.
    pub factor: f64,
}

/// All candlestick settings used by CDL* pattern indicators.
#[derive(Debug, Clone, Copy)]
#[allow(non_snake_case)]
pub struct CandleSettings {
    pub body_long: CandleSetting,
    pub body_very_long: CandleSetting,
    pub body_short: CandleSetting,
    pub body_doji: CandleSetting,
    pub shadow_long: CandleSetting,
    pub shadow_very_long: CandleSetting,
    pub shadow_short: CandleSetting,
    pub shadow_very_short: CandleSetting,
    pub near: CandleSetting,
    pub far: CandleSetting,
    pub equal: CandleSetting,
}

impl CandleSettings {
    /// Default candle settings matching TA-Lib C defaults.
    pub fn default_settings() -> Self {
        Self {
            body_long:         CandleSetting { range_type: 0, avg_period: 10, factor: 1.0 },
            body_very_long:    CandleSetting { range_type: 0, avg_period: 10, factor: 3.0 },
            body_short:        CandleSetting { range_type: 0, avg_period: 10, factor: 1.0 },
            body_doji:         CandleSetting { range_type: 1, avg_period: 10, factor: 0.1 },
            shadow_long:       CandleSetting { range_type: 0, avg_period:  0, factor: 1.0 },
            shadow_very_long:  CandleSetting { range_type: 0, avg_period:  0, factor: 2.0 },
            shadow_short:      CandleSetting { range_type: 2, avg_period: 10, factor: 1.0 },
            shadow_very_short: CandleSetting { range_type: 1, avg_period: 10, factor: 0.1 },
            near:              CandleSetting { range_type: 1, avg_period:  5, factor: 0.2 },
            far:               CandleSetting { range_type: 1, avg_period:  5, factor: 0.6 },
            equal:             CandleSetting { range_type: 1, avg_period:  5, factor: 0.05 },
        }
    }
}

/// Identifies which candlestick setting to configure via
/// [`CoreBuilder::candle_setting`]. Mirrors the C `TA_CandleSettingType`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[non_exhaustive]
pub enum CandleSettingType {
    BodyLong,
    BodyVeryLong,
    BodyShort,
    BodyDoji,
    ShadowLong,
    ShadowVeryLong,
    ShadowShort,
    ShadowVeryShort,
    Near,
    Far,
    Equal,
    /// Wildcard sentinel — not a valid target for a single setting.
    AllCandleSettings,
}

/// Provides access to all TA-Lib technical-analysis functions.
///
/// A `Core` is **immutable after construction**: it holds the value-affecting
/// globals — per-function unstable periods and candlestick thresholds — and
/// every indicator method takes `&self` and only *reads* them. That makes
/// `Core` deeply immutable and `Send + Sync`, so a single instance can be shared
/// read-only across threads (e.g. wrapped in an `Arc` with concurrent
/// `core.SMA(...)` calls) with no locking and no risk of configuration changing
/// mid-computation.
///
/// Construct one with [`Core::new()`] for all-defaults, or with
/// [`Core::builder()`] to configure settings up front:
///
/// ```
/// use ta_lib::{Core, FuncUnstId};
///
/// let core = Core::builder()
///     .unstable_period(FuncUnstId::EMA, 10)
///     .build();
/// ```
///
/// To change a setting, build a new `Core` — cloning is cheap (it is a small
/// `[i32; N]` array plus two small fields). [`Core::to_builder`] seeds a builder
/// from an existing `Core` for clone-and-modify.
#[derive(Debug, Clone)]
pub struct Core {
    /// Unstable period for each function identified by [`FuncUnstId`].
    pub(crate) unstable_period: [i32; FUNC_UNST_COUNT],
    /// Compatibility mode (default: `Compatibility::Default`).
    pub(crate) compatibility: Compatibility,
    /// Candlestick pattern settings.
    pub(crate) candle_settings: CandleSettings,
}

impl Core {
    /// Create a new `Core` with default settings.
    ///
    /// Equivalent to `Core::builder().build()`.
    pub fn new() -> Self {
        Self {
            unstable_period: [0; FUNC_UNST_COUNT],
            compatibility: Compatibility::Default,
            candle_settings: CandleSettings::default_settings(),
        }
    }

    /// Start building a `Core` with non-default settings.
    ///
    /// The resulting `Core` is immutable; changing a setting means building a new
    /// one. See [`CoreBuilder`].
    pub fn builder() -> CoreBuilder {
        CoreBuilder::new()
    }

    /// Seed a [`CoreBuilder`] from this `Core`'s current settings, for
    /// clone-and-modify: `core.to_builder().unstable_period(...).build()`.
    pub fn to_builder(&self) -> CoreBuilder {
        CoreBuilder {
            unstable_period: self.unstable_period,
            compatibility: self.compatibility,
            candle_settings: self.candle_settings,
        }
    }

    /// Get the unstable period for a specific function.
    ///
    /// # Panics
    ///
    /// Panics if `id` is [`FuncUnstId::ALL`]. That variant is the
    /// set-all wildcard accepted by [`CoreBuilder::unstable_period`]; it names
    /// no single function, so there is no value to return.
    pub fn get_unstable_period(&self, id: FuncUnstId) -> i32 {
        assert!(
            (id as usize) < FUNC_UNST_COUNT,
            "{id:?} is a wildcard, not a function with an unstable period"
        );
        self.unstable_period[id as usize]
    }

    /// Compute candlestick range for the given range type and OHLC values.
    #[inline(always)]
    #[allow(non_snake_case)]
    pub fn ta_candlerange(&self, rangeType: i32, open: f64, high: f64, low: f64, close: f64) -> f64 {
        match rangeType {
            0 => (close - open).abs(),
            1 => high - low,
            2 => high - low - (close - open).abs(),
            _ => 0.0,
        }
    }

    /// Compute candlestick average for the given settings and OHLC values.
    #[inline(always)]
    #[allow(non_snake_case)]
    pub fn ta_candleaverage(&self, rangeType: i32, avgPeriod: i32, factor: f64, sum: f64,
                             open: f64, high: f64, low: f64, close: f64) -> f64 {
        let avg = if avgPeriod != 0 {
            sum / (avgPeriod as f64)
        } else {
            self.ta_candlerange(rangeType, open, high, low, close)
        };
        let divisor = if rangeType == 2 { 2.0 } else { 1.0 };
        factor * avg / divisor
    }
}

impl Default for Core {
    /// All-defaults `Core`, same as [`Core::new()`].
    fn default() -> Self {
        Self::new()
    }
}

/// Builder for an immutable [`Core`].
///
/// Obtain one with [`Core::builder()`] (all defaults) or [`Core::to_builder()`]
/// (seeded from an existing `Core`), configure the value-affecting globals, then
/// call [`build`](CoreBuilder::build):
///
/// ```
/// use ta_lib::{Core, FuncUnstId};
///
/// let core = Core::builder()
///     .unstable_period(FuncUnstId::EMA, 10)
///     .build();
/// ```
#[derive(Debug, Clone)]
pub struct CoreBuilder {
    unstable_period: [i32; FUNC_UNST_COUNT],
    compatibility: Compatibility,
    candle_settings: CandleSettings,
}

impl CoreBuilder {
    /// Create a builder initialized with TA-Lib defaults.
    pub fn new() -> Self {
        Self {
            unstable_period: [0; FUNC_UNST_COUNT],
            compatibility: Compatibility::Default,
            candle_settings: CandleSettings::default_settings(),
        }
    }

    /// Set the unstable period for a specific function.
    ///
    /// Passing [`FuncUnstId::ALL`] sets the unstable period for *every*
    /// function at once (mirroring the C `TA_SetUnstablePeriod` wildcard).
    #[must_use]
    pub fn unstable_period(mut self, id: FuncUnstId, period: i32) -> Self {
        if id == FuncUnstId::ALL {
            for slot in self.unstable_period.iter_mut() {
                *slot = period;
            }
        } else {
            self.unstable_period[id as usize] = period;
        }
        self
    }

    /// Override a single candlestick setting (mirrors the C
    /// `TA_SetCandleSettings`).
    ///
    /// # Panics
    ///
    /// Panics if `setting_type` is [`CandleSettingType::AllCandleSettings`].
    /// That variant is a wildcard, not a setting, so there is nothing to
    /// override — C returns `TA_BAD_PARAM` and Java throws for the same call.
    ///
    /// Panics unless `setting.range_type` is `0`, `1` or `2`,
    /// `setting.avg_period` is between `0` and [`MAX_INDEX`], and
    /// `setting.factor` is not NaN. `avg_period` is the lookback of every CDL\*
    /// function that reads the setting, so it is bounded like one; `factor`
    /// scales a threshold and takes any finite value. C rejects the same values
    /// with `TA_BAD_PARAM`.
    #[must_use]
    pub fn candle_setting(mut self, setting_type: CandleSettingType, setting: CandleSetting) -> Self {
        assert!(
            (0..=2).contains(&setting.range_type),
            "range_type must be 0 (RealBody), 1 (HighLow) or 2 (Shadows), got {}",
            setting.range_type
        );
        assert!(
            setting.avg_period >= 0,
            "avg_period must be >= 0, got {}",
            setting.avg_period
        );
        assert!(
            (setting.avg_period as usize) <= MAX_INDEX,
            "avg_period must be <= {MAX_INDEX}, got {}",
            setting.avg_period
        );
        assert!(!setting.factor.is_nan(), "factor must not be NaN");
        match setting_type {
            CandleSettingType::BodyLong => self.candle_settings.body_long = setting,
            CandleSettingType::BodyVeryLong => self.candle_settings.body_very_long = setting,
            CandleSettingType::BodyShort => self.candle_settings.body_short = setting,
            CandleSettingType::BodyDoji => self.candle_settings.body_doji = setting,
            CandleSettingType::ShadowLong => self.candle_settings.shadow_long = setting,
            CandleSettingType::ShadowVeryLong => self.candle_settings.shadow_very_long = setting,
            CandleSettingType::ShadowShort => self.candle_settings.shadow_short = setting,
            CandleSettingType::ShadowVeryShort => self.candle_settings.shadow_very_short = setting,
            CandleSettingType::Near => self.candle_settings.near = setting,
            CandleSettingType::Far => self.candle_settings.far = setting,
            CandleSettingType::Equal => self.candle_settings.equal = setting,
            CandleSettingType::AllCandleSettings => {
                panic!("AllCandleSettings is a wildcard, not a single-setting target")
            }
        }
        self
    }

    /// Consume the builder and produce an immutable [`Core`].
    #[must_use]
    pub fn build(self) -> Core {
        Core {
            unstable_period: self.unstable_period,
            compatibility: self.compatibility,
            candle_settings: self.candle_settings,
        }
    }
}

impl Default for CoreBuilder {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new_default_and_empty_builder_are_all_defaults() {
        for core in [Core::new(), Core::default(), Core::builder().build()] {
            assert_eq!(core.compatibility, Compatibility::Default);
            assert!(core.unstable_period.iter().all(|&p| p == 0));
            // A representative candle default (BodyDoji: HighLow range, 10, 0.1).
            assert_eq!(core.candle_settings.body_doji.range_type, 1);
            assert_eq!(core.candle_settings.body_doji.avg_period, 10);
            assert_eq!(core.candle_settings.body_doji.factor, 0.1);
        }
    }

    #[test]
    fn compatibility_is_pinned_to_default() {
        // There is no public setter: every construction path — including the
        // clone-and-modify one — must leave the mode at Default, so the variant
        // branches in the generated indicators stay unreachable.
        let derived = Core::builder()
            .unstable_period(FuncUnstId::EMA, 10)
            .build()
            .to_builder()
            .unstable_period(FuncUnstId::RSI, 5)
            .build();
        assert_eq!(derived.compatibility, Compatibility::Default);
    }

    #[test]
    fn builder_sets_a_single_unstable_period() {
        let core = Core::builder().unstable_period(FuncUnstId::EMA, 10).build();
        assert_eq!(core.get_unstable_period(FuncUnstId::EMA), 10);
        assert_eq!(core.get_unstable_period(FuncUnstId::RSI), 0);
        // Exactly one slot changed.
        let changed: Vec<usize> = (0..core.unstable_period.len())
            .filter(|&i| core.unstable_period[i] != 0)
            .collect();
        assert_eq!(changed, vec![FuncUnstId::EMA as usize]);
    }

    #[test]
    fn builder_unstable_period_wildcard_sets_every_function() {
        let core = Core::builder().unstable_period(FuncUnstId::ALL, 7).build();
        assert!(core.unstable_period.iter().all(|&p| p == 7));
        assert_eq!(core.get_unstable_period(FuncUnstId::EMA), 7);
        assert_eq!(core.get_unstable_period(FuncUnstId::T3), 7);
    }

    #[test]
    #[should_panic(expected = "ALL is a wildcard")]
    fn get_unstable_period_rejects_the_wildcard() {
        // The wildcard is one past the end of the backing array, so an unguarded
        // read indexed out of bounds instead of reporting the misuse (#144).
        Core::new().get_unstable_period(FuncUnstId::ALL);
    }

    #[test]
    fn get_unstable_period_accepts_the_last_real_function() {
        // The other side of the guard's boundary: T3 is the last real variant,
        // one below the wildcard, so a guard tightened by one rejects it here.
        // Reads go through the getter on purpose -- indexing the array directly
        // would exercise the field, not the check.
        let core = Core::builder().unstable_period(FuncUnstId::ALL, 4).build();
        for id in [FuncUnstId::ADX, FuncUnstId::EMA, FuncUnstId::RSI, FuncUnstId::T3] {
            assert_eq!(core.get_unstable_period(id), 4);
        }
    }

    #[test]
    fn builder_chains_and_last_write_wins() {
        let core = Core::builder()
            .unstable_period(FuncUnstId::ALL, 7) // all -> 7
            .unstable_period(FuncUnstId::EMA, 3)         // then EMA -> 3
            .build();
        assert_eq!(core.get_unstable_period(FuncUnstId::EMA), 3);
        assert_eq!(core.get_unstable_period(FuncUnstId::RSI), 7);
    }

    #[test]
    fn builder_candle_setting_overrides_one_leaves_rest() {
        let custom = CandleSetting { range_type: 2, avg_period: 20, factor: 1.5 };
        let core = Core::builder().candle_setting(CandleSettingType::BodyLong, custom).build();
        assert_eq!(core.candle_settings.body_long.range_type, 2);
        assert_eq!(core.candle_settings.body_long.avg_period, 20);
        assert_eq!(core.candle_settings.body_long.factor, 1.5);
        // A different setting keeps its default.
        assert_eq!(core.candle_settings.body_doji.avg_period, 10);
    }

    #[test]
    #[should_panic(expected = "AllCandleSettings is a wildcard")]
    fn candle_setting_rejects_the_wildcard() {
        // Silently ignoring it left the caller believing all eleven settings had
        // been overridden while none had; C returns TA_BAD_PARAM and Java throws
        // for the same call (#144).
        let custom = CandleSetting { range_type: 2, avg_period: 99, factor: 9.0 };
        let _ = Core::builder().candle_setting(CandleSettingType::AllCandleSettings, custom);
    }

    #[test]
    #[should_panic(expected = "avg_period must be >= 0")]
    fn candle_setting_rejects_a_negative_avg_period() {
        // The period is subtracted from startIdx to seed the trailing average,
        // so a negative one starts the main loop that many bars late while
        // outBegIdx still reports startIdx: every value shifted underneath a
        // correct-looking index, and a lookback that reports negative while the
        // call succeeds (#185).
        let custom = CandleSetting { range_type: 1, avg_period: -1, factor: 0.1 };
        let _ = Core::builder().candle_setting(CandleSettingType::BodyDoji, custom);
    }

    #[test]
    fn candle_setting_accepts_a_zero_avg_period() {
        // The boundary on the legal side: zero means "no averaging", which every
        // CDL* body handles, so a guard written as `<= 0` would refuse a valid
        // setting.
        let custom = CandleSetting { range_type: 1, avg_period: 0, factor: 0.1 };
        let _ = Core::builder().candle_setting(CandleSettingType::BodyDoji, custom);
    }

    #[test]
    #[should_panic(expected = "avg_period must be <=")]
    fn candle_setting_rejects_an_avg_period_past_max_index() {
        // Same ceiling the unstable period already carries: an average longer
        // than the largest addressable series could never produce output, and an
        // unbounded one overflows the lookback it feeds.
        let custom = CandleSetting {
            range_type: 1,
            avg_period: (MAX_INDEX as i32).saturating_add(1),
            factor: 0.1,
        };
        let _ = Core::builder().candle_setting(CandleSettingType::BodyDoji, custom);
    }

    #[test]
    #[should_panic(expected = "range_type must be")]
    fn candle_setting_rejects_an_out_of_domain_range_type() {
        // A choice list's domain is its member list. Anything else falls through
        // every arm of `ta_candlerange` to 0.0 — every range zero, every
        // threshold zero, and a silently meaningless answer.
        let custom = CandleSetting { range_type: 3, avg_period: 10, factor: 0.1 };
        let _ = Core::builder().candle_setting(CandleSettingType::BodyDoji, custom);
    }

    #[test]
    fn candle_setting_accepts_the_ceilings() {
        // The upper boundary on the legal side, for both bounded fields.
        let custom = CandleSetting {
            range_type: 2,
            avg_period: i32::try_from(MAX_INDEX).unwrap(),
            factor: 0.1,
        };
        let core = Core::builder().candle_setting(CandleSettingType::BodyDoji, custom).build();
        assert_eq!(core.candle_settings.body_doji.avg_period, i32::try_from(MAX_INDEX).unwrap());
        assert_eq!(core.candle_settings.body_doji.range_type, 2);
    }

    #[test]
    fn accepted_candle_settings_keep_the_lookback_and_the_call_in_step() {
        // The property the avg_period bounds exist to preserve, stated over the
        // two tiers rather than over the setter: for every setting the builder
        // accepts, the lookback is a real index count and the call's reported
        // range agrees with it. A negative avg_period broke exactly this — here
        // it wraps `CDLDOJI_Lookback` to usize::MAX and the call silently
        // returns nothing, where C shifts the values instead (#185).
        let n = 40usize;
        let open = vec![100.0_f64; n];
        let close = vec![104.0_f64; n];
        let high = vec![105.0_f64; n];
        let low = vec![99.0_f64; n];
        for avg_period in [0, 1, 5, 39, 40, 100] {
            let core = Core::builder()
                .candle_setting(
                    CandleSettingType::BodyDoji,
                    CandleSetting { range_type: 1, avg_period, factor: 0.1 },
                )
                .build();
            let lookback = core.CDLDOJI_Lookback();
            assert!(lookback <= MAX_INDEX, "avg_period {avg_period} gave lookback {lookback}");

            let mut out = vec![0_i32; n];
            let (mut beg, mut nb) = (0usize, 0usize);
            let rc = core.CDLDOJI(0, n - 1, &open, &high, &low, &close, &mut beg, &mut nb, &mut out);
            assert_eq!(rc, RetCode::Success);
            if lookback > n - 1 {
                assert_eq!((beg, nb), (0, 0), "avg_period {avg_period}");
            } else {
                assert_eq!(beg, lookback, "avg_period {avg_period}");
                assert_eq!(nb, n - lookback, "avg_period {avg_period}");
            }
        }
    }

    #[test]
    #[should_panic(expected = "factor must not be NaN")]
    fn candle_setting_rejects_a_nan_factor() {
        // NaN makes every comparison it feeds false, so the patterns simply stop
        // matching -- indistinguishable from "this shape never occurs" unless the
        // setter refuses it.
        let custom = CandleSetting { range_type: 1, avg_period: 10, factor: f64::NAN };
        let _ = Core::builder().candle_setting(CandleSettingType::BodyDoji, custom);
    }

    #[test]
    fn candle_setting_accepts_the_last_real_setting() {
        // The other side of the guard's boundary: Equal is the variant declared
        // immediately before the wildcard, so a guard widened by one rejects it.
        let custom = CandleSetting { range_type: 2, avg_period: 99, factor: 9.0 };
        let core = Core::builder().candle_setting(CandleSettingType::Equal, custom).build();
        assert_eq!(core.candle_settings.equal.avg_period, 99);
        assert_eq!(core.candle_settings.equal.factor, 9.0);
    }

    #[test]
    fn candle_setting_flows_into_computation() {
        // A behavioral witness: prove a builder candle setting actually reaches
        // the CDL math, not just the
        // `candle_settings` struct. Identical clear candles — real body 4, high-low
        // range 6 — are never dojis at the default BodyDoji threshold (0.1), but a huge
        // factor makes the threshold enormous so every candle qualifies as a doji.
        let n = 20usize;
        let open = vec![100.0_f64; n];
        let close = vec![104.0_f64; n]; // real body = 4
        let high = vec![105.0_f64; n];
        let low = vec![99.0_f64; n]; // high-low range = 6
        let run = |core: &Core| {
            let mut out = vec![0_i32; n];
            let (mut beg, mut nb) = (0usize, 0usize);
            let rc = core.CDLDOJI(0, n - 1, &open, &high, &low, &close, &mut beg, &mut nb, &mut out);
            assert_eq!(rc, RetCode::Success);
            out[..nb].to_vec()
        };
        let default_out = run(&Core::new());
        let tuned = Core::builder()
            .candle_setting(
                CandleSettingType::BodyDoji,
                CandleSetting { range_type: 1, avg_period: 10, factor: 1.0e9 },
            )
            .build();
        let tuned_out = run(&tuned);
        assert!(
            default_out.iter().all(|&v| v == 0),
            "clear candles are not dojis at the default threshold"
        );
        assert!(
            tuned_out.iter().all(|&v| v == 100),
            "a huge BodyDoji factor marks every candle a doji"
        );
        assert_ne!(default_out, tuned_out, "candle_setting must change CDLDOJI output");
    }

    #[test]
    fn to_builder_round_trips_and_leaves_original_untouched() {
        let original = Core::builder()
            .unstable_period(FuncUnstId::RSI, 5)
            .candle_setting(
                CandleSettingType::BodyLong,
                CandleSetting { range_type: 2, avg_period: 20, factor: 1.5 },
            )
            .build();
        // Clone-and-modify: derive a Core that additionally tunes EMA.
        let derived = original.to_builder().unstable_period(FuncUnstId::EMA, 9).build();
        // The original is immutable and unchanged.
        assert_eq!(original.get_unstable_period(FuncUnstId::EMA), 0);
        assert_eq!(original.get_unstable_period(FuncUnstId::RSI), 5);
        // The derived Core inherits the settings (candle_settings included, which
        // guards against to_builder dropping a field), plus the new one.
        assert_eq!(derived.get_unstable_period(FuncUnstId::RSI), 5);
        assert_eq!(derived.get_unstable_period(FuncUnstId::EMA), 9);
        // candle_settings survived the round-trip (default avg_period would be 10).
        assert_eq!(derived.candle_settings.body_long.avg_period, 20);
        assert_eq!(derived.candle_settings.body_long.factor, 1.5);
        assert_eq!(derived.candle_settings.body_long.range_type, 2);
    }

    #[test]
    fn unstable_period_setting_changes_lookback() {
        let base = Core::new();
        let tuned = Core::builder().unstable_period(FuncUnstId::EMA, 5).build();
        // The unstable period is added to the function's lookback.
        assert_eq!(tuned.EMA_Lookback(10), base.EMA_Lookback(10) + 5);
    }

    #[test]
    fn core_and_builder_are_send_and_sync() {
        fn assert_send_sync<T: Send + Sync>() {}
        assert_send_sync::<Core>();
        assert_send_sync::<CoreBuilder>();
    }

    #[test]
    fn shared_core_runs_concurrent_batches() {
        use std::sync::Arc;
        use std::thread;
        // A single immutable Core shared read-only across threads (the concurrency
        // contract this design enables): every thread computes the same result.
        let core = Arc::new(Core::builder().unstable_period(FuncUnstId::EMA, 2).build());
        let close: Vec<f64> = (0..64).map(|i| 100.0 + f64::from(i % 7)).collect();
        let mut handles = Vec::new();
        for _ in 0..4 {
            let core = Arc::clone(&core);
            let close = close.clone();
            handles.push(thread::spawn(move || {
                let mut out = vec![0.0; close.len()];
                let (mut beg, mut n) = (0usize, 0usize);
                let rc = core.EMA(0, close.len() - 1, &close, 10, &mut beg, &mut n, &mut out);
                assert_eq!(rc, RetCode::Success);
                out[0]
            }));
        }
        let expected = handles.pop().unwrap().join().unwrap();
        for h in handles {
            assert_eq!(h.join().unwrap(), expected, "concurrent shared-Core calls must agree");
        }
    }
}
