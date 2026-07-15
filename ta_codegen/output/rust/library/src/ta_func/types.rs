/// Return codes for TA-Lib function calls.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RetCode {
    /// Function completed successfully.
    Success,
    /// One or more parameters are invalid.
    BadParam,
    /// The start index is out of range.
    OutOfRangeStartIndex,
    /// The end index is out of range or less than start index.
    OutOfRangeEndIndex,
    /// Memory allocation failed.
    AllocErr,
    /// Internal error occurred.
    InternalError,
}

/// Compatibility mode for technical analysis calculations.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Compatibility {
    /// Default TA-Lib compatibility mode.
    Default,
    /// Metastock-compatible calculation mode.
    Metastock,
}

/// Identifies functions that have an unstable period.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FuncUnstId {
    Adx,
    Adxr,
    Atr,
    Cmo,
    Dx,
    Ema,
    HtDcPeriod,
    HtDcPhase,
    HtPhasor,
    HtSine,
    HtTrendline,
    HtTrendMode,
    /// Reserved: was IMI, reclassified stable (#14); kept for ABI, reusable.
    Unused12,
    Kama,
    Mama,
    /// Reserved: was MFI, reclassified stable (#4); kept for ABI, reusable.
    Unused15,
    MinusDI,
    MinusDM,
    Natr,
    PlusDI,
    PlusDM,
    Rsi,
    StochRsi,
    T3,
    /// Wildcard: set the unstable period for all functions at once.
    FuncUnstAll,
}

/// A single candlestick setting entry.
#[derive(Debug, Clone, Copy)]
pub struct CandleSetting {
    /// Range type: 0 = RealBody, 1 = HighLow, 2 = Shadows.
    pub range_type: i32,
    /// Number of periods for averaging.
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
/// globals — per-function unstable periods, Metastock [`Compatibility`], and
/// candlestick thresholds — and every indicator method takes `&self` and only
/// *reads* them. That makes `Core` deeply immutable and `Send + Sync`, so a
/// single instance can be shared read-only across threads (e.g. wrapped in an
/// `Arc` with concurrent `core.sma(...)` calls) with no locking and no risk of
/// configuration changing mid-computation.
///
/// Construct one with [`Core::new()`] for all-defaults, or with
/// [`Core::builder()`] to configure settings up front:
///
/// ```
/// use ta_lib::{Core, Compatibility, FuncUnstId};
///
/// let core = Core::builder()
///     .compatibility(Compatibility::Metastock)
///     .unstable_period(FuncUnstId::Ema, 10)
///     .build();
/// ```
///
/// To change a setting, build a new `Core` — cloning is cheap (it is a small
/// `[i32; N]` array plus two small fields). [`Core::to_builder`] seeds a builder
/// from an existing `Core` for clone-and-modify.
#[derive(Debug, Clone)]
pub struct Core {
    /// Unstable period for each function identified by [`FuncUnstId`].
    pub(crate) unstable_period: [i32; FuncUnstId::FuncUnstAll as usize],
    /// Compatibility mode (default: [`Compatibility::Default`]).
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
            unstable_period: [0; FuncUnstId::FuncUnstAll as usize],
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
    /// clone-and-modify: `core.to_builder().compatibility(...).build()`.
    pub fn to_builder(&self) -> CoreBuilder {
        CoreBuilder {
            unstable_period: self.unstable_period,
            compatibility: self.compatibility,
            candle_settings: self.candle_settings,
        }
    }

    /// Get the unstable period for a specific function.
    pub fn get_unstable_period(&self, id: FuncUnstId) -> i32 {
        self.unstable_period[id as usize]
    }

    /// Get the current compatibility mode.
    pub fn get_compatibility(&self) -> Compatibility {
        self.compatibility
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
/// use ta_lib::{Core, Compatibility, FuncUnstId};
///
/// let core = Core::builder()
///     .compatibility(Compatibility::Metastock)
///     .unstable_period(FuncUnstId::Ema, 10)
///     .build();
/// ```
#[derive(Debug, Clone)]
pub struct CoreBuilder {
    unstable_period: [i32; FuncUnstId::FuncUnstAll as usize],
    compatibility: Compatibility,
    candle_settings: CandleSettings,
}

impl CoreBuilder {
    /// Create a builder initialized with TA-Lib defaults.
    pub fn new() -> Self {
        Self {
            unstable_period: [0; FuncUnstId::FuncUnstAll as usize],
            compatibility: Compatibility::Default,
            candle_settings: CandleSettings::default_settings(),
        }
    }

    /// Set the compatibility mode.
    #[must_use]
    pub fn compatibility(mut self, compatibility: Compatibility) -> Self {
        self.compatibility = compatibility;
        self
    }

    /// Set the unstable period for a specific function.
    ///
    /// Passing [`FuncUnstId::FuncUnstAll`] sets the unstable period for *every*
    /// function at once (mirroring the C `TA_SetUnstablePeriod` wildcard).
    #[must_use]
    pub fn unstable_period(mut self, id: FuncUnstId, period: i32) -> Self {
        if id as usize == FuncUnstId::FuncUnstAll as usize {
            for slot in self.unstable_period.iter_mut() {
                *slot = period;
            }
        } else {
            self.unstable_period[id as usize] = period;
        }
        self
    }

    /// Override a single candlestick setting (mirrors the C
    /// `TA_SetCandleSettings`). [`CandleSettingType::AllCandleSettings`] is not a
    /// valid single-setting target and is ignored.
    #[must_use]
    pub fn candle_setting(mut self, setting_type: CandleSettingType, setting: CandleSetting) -> Self {
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
            CandleSettingType::AllCandleSettings => {}
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
            assert_eq!(core.get_compatibility(), Compatibility::Default);
            assert!(core.unstable_period.iter().all(|&p| p == 0));
            // A representative candle default (BodyDoji: HighLow range, 10, 0.1).
            assert_eq!(core.candle_settings.body_doji.range_type, 1);
            assert_eq!(core.candle_settings.body_doji.avg_period, 10);
            assert_eq!(core.candle_settings.body_doji.factor, 0.1);
        }
    }

    #[test]
    fn builder_sets_compatibility_only() {
        let core = Core::builder().compatibility(Compatibility::Metastock).build();
        assert_eq!(core.get_compatibility(), Compatibility::Metastock);
        assert!(core.unstable_period.iter().all(|&p| p == 0));
    }

    #[test]
    fn builder_sets_a_single_unstable_period() {
        let core = Core::builder().unstable_period(FuncUnstId::Ema, 10).build();
        assert_eq!(core.get_unstable_period(FuncUnstId::Ema), 10);
        assert_eq!(core.get_unstable_period(FuncUnstId::Rsi), 0);
        // Exactly one slot changed.
        let changed: Vec<usize> = (0..core.unstable_period.len())
            .filter(|&i| core.unstable_period[i] != 0)
            .collect();
        assert_eq!(changed, vec![FuncUnstId::Ema as usize]);
    }

    #[test]
    fn builder_unstable_period_wildcard_sets_every_function() {
        let core = Core::builder().unstable_period(FuncUnstId::FuncUnstAll, 7).build();
        assert!(core.unstable_period.iter().all(|&p| p == 7));
        assert_eq!(core.get_unstable_period(FuncUnstId::Ema), 7);
        assert_eq!(core.get_unstable_period(FuncUnstId::T3), 7);
    }

    #[test]
    fn builder_chains_and_last_write_wins() {
        let core = Core::builder()
            .unstable_period(FuncUnstId::FuncUnstAll, 7) // all -> 7
            .unstable_period(FuncUnstId::Ema, 3)         // then EMA -> 3
            .compatibility(Compatibility::Metastock)
            .build();
        assert_eq!(core.get_unstable_period(FuncUnstId::Ema), 3);
        assert_eq!(core.get_unstable_period(FuncUnstId::Rsi), 7);
        assert_eq!(core.get_compatibility(), Compatibility::Metastock);
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
    fn candle_setting_all_sentinel_is_ignored() {
        let base = Core::new();
        let custom = CandleSetting { range_type: 2, avg_period: 99, factor: 9.0 };
        let core = Core::builder()
            .candle_setting(CandleSettingType::AllCandleSettings, custom)
            .build();
        assert_eq!(
            core.candle_settings.body_long.avg_period,
            base.candle_settings.body_long.avg_period
        );
        assert_eq!(core.candle_settings.equal.factor, base.candle_settings.equal.factor);
    }

    #[test]
    fn candle_setting_flows_into_computation() {
        // A behavioral witness (mirrors `compatibility_setting_changes_computed_output`):
        // prove a builder candle setting actually reaches the CDL math, not just the
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
            let rc = core.cdldoji(0, n - 1, &open, &high, &low, &close, &mut beg, &mut nb, &mut out);
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
            .compatibility(Compatibility::Metastock)
            .unstable_period(FuncUnstId::Rsi, 5)
            .candle_setting(
                CandleSettingType::BodyLong,
                CandleSetting { range_type: 2, avg_period: 20, factor: 1.5 },
            )
            .build();
        // Clone-and-modify: derive a Core that additionally tunes EMA.
        let derived = original.to_builder().unstable_period(FuncUnstId::Ema, 9).build();
        // The original is immutable and unchanged.
        assert_eq!(original.get_unstable_period(FuncUnstId::Ema), 0);
        assert_eq!(original.get_unstable_period(FuncUnstId::Rsi), 5);
        // The derived Core inherits ALL three settings (candle_settings included, which
        // guards against to_builder dropping a field), plus the new one.
        assert_eq!(derived.get_unstable_period(FuncUnstId::Rsi), 5);
        assert_eq!(derived.get_unstable_period(FuncUnstId::Ema), 9);
        assert_eq!(derived.get_compatibility(), Compatibility::Metastock);
        // candle_settings survived the round-trip (default avg_period would be 10).
        assert_eq!(derived.candle_settings.body_long.avg_period, 20);
        assert_eq!(derived.candle_settings.body_long.factor, 1.5);
        assert_eq!(derived.candle_settings.body_long.range_type, 2);
    }

    #[test]
    fn compatibility_setting_changes_computed_output() {
        // EMA is seeded differently under Metastock, so the two configs must
        // produce different values from the same input.
        let close: Vec<f64> = (0..40).map(|i| 100.0 + f64::from(i)).collect();
        let run = |core: &Core| {
            let mut out = vec![0.0; close.len()];
            let (mut beg, mut n) = (0usize, 0usize);
            let rc = core.ema(0, close.len() - 1, &close, 10, &mut beg, &mut n, &mut out);
            assert_eq!(rc, RetCode::Success);
            out
        };
        let default_out = run(&Core::new());
        let metastock_out = run(&Core::builder().compatibility(Compatibility::Metastock).build());
        assert_ne!(
            default_out[0], metastock_out[0],
            "Metastock compatibility should change EMA seeding"
        );
    }

    #[test]
    fn unstable_period_setting_changes_lookback() {
        let base = Core::new();
        let tuned = Core::builder().unstable_period(FuncUnstId::Ema, 5).build();
        // The unstable period is added to the function's lookback.
        assert_eq!(tuned.ema_lookback(10), base.ema_lookback(10) + 5);
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
        let core = Arc::new(Core::builder().unstable_period(FuncUnstId::Ema, 2).build());
        let close: Vec<f64> = (0..64).map(|i| 100.0 + f64::from(i % 7)).collect();
        let mut handles = Vec::new();
        for _ in 0..4 {
            let core = Arc::clone(&core);
            let close = close.clone();
            handles.push(thread::spawn(move || {
                let mut out = vec![0.0; close.len()];
                let (mut beg, mut n) = (0usize, 0usize);
                let rc = core.ema(0, close.len() - 1, &close, 10, &mut beg, &mut n, &mut out);
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
