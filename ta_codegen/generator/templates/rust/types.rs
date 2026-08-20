/// Return codes for TA-Lib function calls.
///
/// `#[must_use]` because a `RetCode` is the whole report of what a call did: a
/// rejected parameter writes no output and leaves whatever was already in the
/// caller's slices, so a discarded code reads as a successful call over stale
/// data (#179 C10). The public tiers return it inside a `Result`, itself
/// `#[must_use]`; this covers the code once separated from one.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
#[must_use = "a failed call writes no output, so the output slices still hold whatever was in them before"]
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
    /// The history given to a stream opener is shorter than `lookback + 1` bars.
    ///
    /// The library's one **recoverable** condition: accumulate more bars and
    /// retry, rather than fix the call. Streaming only — the batch tier answers
    /// a range shorter than its lookback with `Ok` and a zero count.
    InsufficientHistory,
}

/// Where a successful call's output starts and how many values it wrote.
///
/// Returned by every batch entry point and by the abstraction layer's
/// [`ParamHolder::call`](crate::abstract_api::ParamHolder::call). A valid range
/// shorter than the function's lookback is a **success with no values**
/// (`count == 0`), not an error — the same contract as C's `TA_SUCCESS` with
/// `outNBElement == 0`, and the same type Java and C# return.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct OutRange {
    /// Index of the first computed value, in the input series' coordinates.
    pub beg_idx: usize,
    /// How many values were written, starting at `beg_idx`.
    pub count: usize,
}

impl OutRange {
    /// An empty result — a successful call that produced no values.
    pub const EMPTY: Self = Self { beg_idx: 0, count: 0 };

    /// Whether this range holds no values.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.count == 0
    }
}

impl RetCode {
    /// This code's `TA_RetCode` integer — the value C returns for the same
    /// condition (`include/ta_defs.h`).
    ///
    /// It lives here, and not in whichever crate needs to serialize it, because
    /// `#[non_exhaustive]` does not apply inside the defining crate: the match
    /// below must stay total, so a new variant is a compile error. Downstream the
    /// same match would need a wildcard arm, and a new variant would silently
    /// report as whatever that arm says.
    pub const fn as_c_int(self) -> i32 {
        match self {
            RetCode::Success => 0,
            RetCode::BadParam => 2,
            RetCode::AllocErr => 3,
            RetCode::OutOfRangeStartIndex => 12,
            RetCode::OutOfRangeEndIndex => 13,
            RetCode::InsufficientHistory => 17,
            RetCode::InternalError => 5000,
        }
    }
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
            RetCode::InsufficientHistory => "history shorter than the lookback",
        };
        f.write_str(s)
    }
}

/// `RetCode` is the error type of every tier's `Result` — batch, streaming and
/// the abstraction layer.
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
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
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

impl FuncUnstId {
    /// Number of [`FuncUnstId`] function ids — the size of the unstable-period
    /// table. Not an id, and not [`FuncUnstId::ALL`]. Mirrors C's
    /// `TA_FUNC_UNST_COUNT` and Java's `FuncUnstId.COUNT`.
    pub const COUNT: usize = 24;
}

/// What a candlestick setting measures a candle against. Mirrors the C
/// `TA_RangeType`, and the `RangeType` enum of the Java and C# ports.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum RangeType {
    /// The real body: `|close - open|`.
    RealBody = 0,
    /// The whole candle: `high - low`.
    HighLow = 1,
    /// The shadows only: `high - low` less the real body.
    Shadows = 2,
}

impl TryFrom<i32> for RangeType {
    type Error = RetCode;

    /// Convert a raw range type, as C, the JSON-RPC server and a configuration
    /// file hold it.
    ///
    /// # Errors
    ///
    /// [`RetCode::BadParam`] if the value names no member — the same domain C
    /// rejects with `TA_BAD_PARAM`.
    fn try_from(value: i32) -> Result<Self, Self::Error> {
        Ok(match value {
            0 => Self::RealBody,
            1 => Self::HighLow,
            2 => Self::Shadows,
            _ => return Err(RetCode::BadParam),
        })
    }
}

/// A single candlestick setting entry.
///
/// `PartialEq` but not `Eq`: `factor` is an `f64`.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct CandleSetting {
    /// What the candle is measured against.
    pub range_type: RangeType,
    /// Period length for averaging.
    pub avg_period: i32,
    /// Scaling factor.
    pub factor: f64,
}

/// All candlestick settings used by CDL* pattern indicators.
#[derive(Debug, Clone, Copy, PartialEq)]
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
            body_long:         CandleSetting { range_type: RangeType::RealBody, avg_period: 10, factor: 1.0 },
            body_very_long:    CandleSetting { range_type: RangeType::RealBody, avg_period: 10, factor: 3.0 },
            body_short:        CandleSetting { range_type: RangeType::RealBody, avg_period: 10, factor: 1.0 },
            body_doji:         CandleSetting { range_type: RangeType::HighLow,  avg_period: 10, factor: 0.1 },
            shadow_long:       CandleSetting { range_type: RangeType::RealBody, avg_period:  0, factor: 1.0 },
            shadow_very_long:  CandleSetting { range_type: RangeType::RealBody, avg_period:  0, factor: 2.0 },
            shadow_short:      CandleSetting { range_type: RangeType::Shadows,  avg_period: 10, factor: 1.0 },
            shadow_very_short: CandleSetting { range_type: RangeType::HighLow,  avg_period: 10, factor: 0.1 },
            near:              CandleSetting { range_type: RangeType::HighLow,  avg_period:  5, factor: 0.2 },
            far:               CandleSetting { range_type: RangeType::HighLow,  avg_period:  5, factor: 0.6 },
            equal:             CandleSetting { range_type: RangeType::HighLow,  avg_period:  5, factor: 0.05 },
        }
    }
}

/// Identifies which candlestick setting to configure via
/// [`CoreBuilder::candle_setting`]. Mirrors the C `TA_CandleSettingType`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
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
///     .build()?;
/// # Ok::<(), ta_lib::RetCode>(())
/// ```
///
/// To change a setting, build a new `Core` — cloning is cheap (it is a small
/// `[i32; N]` array plus two small fields). [`Core::to_builder`] seeds a builder
/// from an existing `Core` for clone-and-modify.
#[derive(Debug, Clone, PartialEq)]
pub struct Core {
    /// Unstable period for each function identified by [`FuncUnstId`].
    pub(crate) unstable_period: [i32; FuncUnstId::COUNT],
    /// Compatibility mode (default: `Compatibility::Default`).
    pub(crate) compatibility: Compatibility,
    /// Candlestick pattern settings.
    pub(crate) candle_settings: CandleSettings,
}

impl Core {
    /// Pass this for a `f64` optional parameter to select its documented default —
    /// C's `TA_REAL_DEFAULT`. The value sits deliberately outside
    /// [`Core::REAL_MIN`]`..=`[`Core::REAL_MAX`], so it can never collide with
    /// real data.
    pub const REAL_DEFAULT: f64 = -4e37;

    /// Pass this for an `i32` optional parameter to select its documented default —
    /// C's `TA_INTEGER_DEFAULT`. One below [`Core::INTEGER_MIN`], for the same reason.
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

    /// Create a new `Core` with default settings.
    ///
    /// Infallible, unlike [`CoreBuilder::build`]: there is no argument to reject,
    /// so this is what `Core::builder().build().unwrap()` would give you.
    pub fn new() -> Self {
        Self {
            unstable_period: [0; FuncUnstId::COUNT],
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
    /// clone-and-modify: `core.to_builder().unstable_period(...).build()?`.
    ///
    /// The round trip is total: a `Core` only exists because it validated, so a
    /// builder seeded from one starts with no rejection latched and rebuilds
    /// without error unless a later setter is given something bad.
    pub fn to_builder(&self) -> CoreBuilder {
        CoreBuilder {
            unstable_period: self.unstable_period,
            compatibility: self.compatibility,
            candle_settings: self.candle_settings,
            err: None,
        }
    }

    /// Get the unstable period for a specific function.
    ///
    /// # Errors
    ///
    /// [`RetCode::BadParam`] if `id` is [`FuncUnstId::ALL`]. That variant is the
    /// set-all wildcard accepted by [`CoreBuilder::unstable_period`]; it names
    /// no single function, so there is no value to return.
    ///
    /// This is a deliberate divergence from the C reference, whose
    /// `TA_GetUnstablePeriod` answers `0` for the wildcard and cannot report an
    /// error at all: `0` is itself a legal period, so C's answer is
    /// indistinguishable from a genuine reading.
    pub fn get_unstable_period(&self, id: FuncUnstId) -> Result<u32, RetCode> {
        if (id as usize) >= FuncUnstId::COUNT {
            return Err(RetCode::BadParam);
        }
        // Stored as i32 for the generated indicators' signed lookback arithmetic;
        // every value that reaches the array came through the builder's
        // `0..=MAX_INDEX` guard, so the conversion cannot fail.
        Ok(self.unstable_period[id as usize] as u32)
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
///     .build()?;
/// # Ok::<(), ta_lib::RetCode>(())
/// ```
///
/// The setters are infallible so that they chain; a rejected argument is
/// reported once, by [`build`](CoreBuilder::build).
#[derive(Debug, Clone, PartialEq)]
pub struct CoreBuilder {
    unstable_period: [i32; FuncUnstId::COUNT],
    compatibility: Compatibility,
    candle_settings: CandleSettings,
    /// The first rejection seen by any setter, surfaced by [`CoreBuilder::build`].
    ///
    /// Setters stay infallible and chainable, so a rejection has nowhere to go at
    /// the point it happens; it is latched here instead. **First** error rather
    /// than last: a later correction repairs the stored value but must not erase
    /// the report, or a misuse would be silently swallowed — which is the failure
    /// this whole guard exists to prevent.
    err: Option<RetCode>,
}

impl CoreBuilder {
    /// Create a builder initialized with TA-Lib defaults.
    pub fn new() -> Self {
        Self {
            unstable_period: [0; FuncUnstId::COUNT],
            compatibility: Compatibility::Default,
            candle_settings: CandleSettings::default_settings(),
            err: None,
        }
    }

    /// Latch `code` unless an earlier rejection is already recorded.
    fn reject(mut self, code: RetCode) -> Self {
        if self.err.is_none() {
            self.err = Some(code);
        }
        self
    }

    /// Set the unstable period for a specific function.
    ///
    /// Passing [`FuncUnstId::ALL`] sets the unstable period for *every*
    /// function at once (mirroring the C `TA_SetUnstablePeriod` wildcard).
    ///
    /// `period` is a `u32` because C's parameter is an `unsigned int`: a negative
    /// warm-up is not a value to reject but one that cannot be written down.
    ///
    /// # Errors
    ///
    /// A `period` above [`Core::MAX_INDEX`] is rejected, and [`CoreBuilder::build`]
    /// then reports [`RetCode::BadParam`]. The period is added to a lookback that
    /// is then used as an index, so an unbounded one overflows that lookback
    /// negative and the function indexes far past the end of its input.
    /// `MAX_INDEX` is the ceiling the index space already enforces on
    /// `startIdx`/`endIdx`; a warm-up longer than the largest addressable series
    /// could never produce output, so nothing legitimate is refused.
    ///
    /// A rejected call writes nothing — every slot keeps the value it had.
    #[must_use]
    pub fn unstable_period(mut self, id: FuncUnstId, period: u32) -> Self {
        // Widened both sides rather than narrowing MAX_INDEX: u32 and usize are
        // the same width on a 32-bit target, so a `MAX_INDEX as u32` would be a
        // truncating cast there and a lint everywhere.
        if u64::from(period) > Core::MAX_INDEX as u64 {
            return self.reject(RetCode::BadParam);
        }
        // In range by the check above, so it round-trips through the i32 storage
        // the generated indicators do their signed lookback arithmetic in.
        let period = period as i32;
        if id == FuncUnstId::ALL {
            for slot in self.unstable_period.iter_mut() {
                *slot = period;
            }
        } else if (id as usize) < FuncUnstId::COUNT {
            self.unstable_period[id as usize] = period;
        } else {
            return self.reject(RetCode::BadParam);
        }
        self
    }

    /// Override a single candlestick setting (mirrors the C
    /// `TA_SetCandleSettings`).
    ///
    /// # Errors
    ///
    /// [`CoreBuilder::build`] reports [`RetCode::BadParam`] unless `setting_type`
    /// names a single setting, `setting.avg_period` is between `0` and
    /// [`Core::MAX_INDEX`], and `setting.factor` is not NaN. The range type needs no
    /// check: [`RangeType`] has no out-of-domain value to reject, which is the
    /// point of it being an enum rather than the `int` C carries.
    /// `avg_period` is the lookback of every CDL\*
    /// function that reads the setting, so it is bounded like one; `factor`
    /// scales a threshold and takes any finite value. C rejects the same values
    /// with `TA_BAD_PARAM`.
    ///
    /// [`CandleSettingType::AllCandleSettings`] is a wildcard, not a setting, so
    /// there is nothing for it to override — C returns `TA_BAD_PARAM` and Java
    /// throws for the same call.
    ///
    /// A rejected call writes nothing — every setting keeps the value it had.
    #[must_use]
    pub fn candle_setting(mut self, setting_type: CandleSettingType, setting: CandleSetting) -> Self {
        // Reported rather than asserted (#186): a library aborting the process on
        // a value that can arrive from a config file is a worse failure than one
        // the caller can handle, and every check here precedes the write, so a
        // rejection leaves the settings exactly as they were.
        if setting.avg_period < 0 || setting.avg_period as i64 > Core::MAX_INDEX as i64 {
            return self.reject(RetCode::BadParam);
        }
        if setting.factor.is_nan() {
            return self.reject(RetCode::BadParam);
        }
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
            CandleSettingType::AllCandleSettings => return self.reject(RetCode::BadParam),
        }
        self
    }

    /// Restore one candlestick setting to its TA-Lib default, or every setting
    /// when given [`CandleSettingType::AllCandleSettings`].
    ///
    /// **This does not reset anything.** C's `TA_RestoreCandleDefaultSettings`
    /// writes over a process-wide global that every later call then reads; there
    /// is no such global here. A [`Core`] is immutable and its settings are
    /// fixed at [`build`](CoreBuilder::build), so the operation this names is
    /// *deriving a new `Core`* whose settings happen to be the defaults again:
    ///
    /// ```
    /// use ta_lib::{CandleSetting, CandleSettingType, Core, RangeType};
    ///
    /// let tuned = Core::builder()
    ///     .candle_setting(
    ///         CandleSettingType::BodyLong,
    ///         CandleSetting { range_type: RangeType::Shadows, avg_period: 20, factor: 1.5 },
    ///     )
    ///     .build()?;
    ///
    /// // A SECOND Core. `tuned` still has its override -- nothing was reset.
    /// let plain = tuned
    ///     .to_builder()
    ///     .restore_candle_default(CandleSettingType::BodyLong)
    ///     .build()?;
    /// # Ok::<(), ta_lib::RetCode>(())
    /// ```
    ///
    /// On a fresh [`Core::builder()`] it is a no-op, and correctly so — that
    /// builder is already sitting on the defaults, so the way to "not override"
    /// a setting there is simply not to call
    /// [`candle_setting`](CoreBuilder::candle_setting) for it. The call earns its
    /// place only after [`Core::to_builder`], which is also the only thing the
    /// JSON-RPC server uses it for.
    ///
    /// Unlike [`candle_setting`](CoreBuilder::candle_setting) this cannot fail:
    /// the wildcard is a legal argument here rather than a rejected one, and an
    /// out-of-domain `setting_type` is unrepresentable in the enum — where C has
    /// to bound-check an `int` and C# has to throw, Rust has nothing left to
    /// reject. It records no error, so a builder that was already poisoned by an
    /// earlier rejection stays poisoned.
    #[must_use]
    pub fn restore_candle_default(mut self, setting_type: CandleSettingType) -> Self {
        let defaults = CandleSettings::default_settings();
        match setting_type {
            CandleSettingType::BodyLong => self.candle_settings.body_long = defaults.body_long,
            CandleSettingType::BodyVeryLong => {
                self.candle_settings.body_very_long = defaults.body_very_long;
            }
            CandleSettingType::BodyShort => self.candle_settings.body_short = defaults.body_short,
            CandleSettingType::BodyDoji => self.candle_settings.body_doji = defaults.body_doji,
            CandleSettingType::ShadowLong => {
                self.candle_settings.shadow_long = defaults.shadow_long;
            }
            CandleSettingType::ShadowVeryLong => {
                self.candle_settings.shadow_very_long = defaults.shadow_very_long;
            }
            CandleSettingType::ShadowShort => {
                self.candle_settings.shadow_short = defaults.shadow_short;
            }
            CandleSettingType::ShadowVeryShort => {
                self.candle_settings.shadow_very_short = defaults.shadow_very_short;
            }
            CandleSettingType::Near => self.candle_settings.near = defaults.near,
            CandleSettingType::Far => self.candle_settings.far = defaults.far,
            CandleSettingType::Equal => self.candle_settings.equal = defaults.equal,
            CandleSettingType::AllCandleSettings => self.candle_settings = defaults,
        }
        self
    }

    /// Consume the builder and produce an immutable [`Core`].
    ///
    /// # Errors
    ///
    /// [`RetCode::BadParam`] if any setter rejected its argument. The setters are
    /// infallible so that they can chain, so this is where a rejection is
    /// reported; the code is the first one recorded.
    pub fn build(self) -> Result<Core, RetCode> {
        if let Some(code) = self.err {
            return Err(code);
        }
        Ok(Core {
            unstable_period: self.unstable_period,
            compatibility: self.compatibility,
            candle_settings: self.candle_settings,
        })
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

    /// A stream opened on too little history answers
    /// [`RetCode::InsufficientHistory`], not the catch-all.
    ///
    /// The distinction is the whole point of the member: a short history means
    /// "send more bars" and is recoverable, while `BadParam` always means the
    /// call itself is wrong. Before it existed, both answered `BadParam` and a
    /// caller could not tell them apart (docs/error-handling-spec.md, rule S-6).
    ///
    /// Three arms, because the first alone would pass against a body that
    /// answered `InsufficientHistory` for everything: the second shows one more
    /// bar is all it wanted, the third that a genuinely bad parameter still
    /// reports the catch-all.
    #[test]
    fn a_short_history_open_reports_insufficient_history() {
        let core = Core::new();
        let lookback = core.SMA_Lookback(30);
        assert!(lookback > 0, "the probe needs a function that consumes bars");

        let one_short = vec![1.0_f64; lookback];
        assert_eq!(
            core.SMA_Open(&one_short, 30).err(),
            Some(RetCode::InsufficientHistory),
        );

        let just_enough = vec![1.0_f64; lookback + 1];
        assert!(core.SMA_Open(&just_enough, 30).is_ok());

        assert_eq!(core.SMA_Open(&just_enough, 0).err(), Some(RetCode::BadParam));

        // The number C puts on the wire for the same condition. Pinned here
        // because nothing else in this crate compares the two vocabularies, and
        // the cross-language harness reads exactly this integer.
        assert_eq!(RetCode::InsufficientHistory.as_c_int(), 17);
    }

    #[test]
    fn new_default_and_empty_builder_are_all_defaults() {
        for core in [Core::new(), Core::default(), Core::builder().build().unwrap()] {
            assert_eq!(core.compatibility, Compatibility::Default);
            assert!(core.unstable_period.iter().all(|&p| p == 0));
            // A representative candle default (BodyDoji: HighLow range, 10, 0.1).
            assert_eq!(core.candle_settings.body_doji.range_type, RangeType::HighLow);
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
            .unwrap()
            .to_builder()
            .unstable_period(FuncUnstId::RSI, 5)
            .build()
            .unwrap();
        assert_eq!(derived.compatibility, Compatibility::Default);
    }

    #[test]
    fn builder_sets_a_single_unstable_period() {
        let core = Core::builder().unstable_period(FuncUnstId::EMA, 10).build().unwrap();
        assert_eq!(core.get_unstable_period(FuncUnstId::EMA), Ok(10));
        assert_eq!(core.get_unstable_period(FuncUnstId::RSI), Ok(0));
        // Exactly one slot changed.
        let changed: Vec<usize> = (0..core.unstable_period.len())
            .filter(|&i| core.unstable_period[i] != 0)
            .collect();
        assert_eq!(changed, vec![FuncUnstId::EMA as usize]);
    }

    #[test]
    fn builder_unstable_period_wildcard_sets_every_function() {
        let core = Core::builder().unstable_period(FuncUnstId::ALL, 7).build().unwrap();
        assert!(core.unstable_period.iter().all(|&p| p == 7));
        assert_eq!(core.get_unstable_period(FuncUnstId::EMA), Ok(7));
        assert_eq!(core.get_unstable_period(FuncUnstId::T3), Ok(7));
    }

    #[test]
    fn get_unstable_period_rejects_the_wildcard() {
        // The wildcard is one past the end of the backing array, so an unguarded
        // read indexed out of bounds instead of reporting the misuse (#144).
        // It reports rather than panics (#186): the caller gets a RetCode, not an
        // abort, and never C's ambiguous 0 -- which is also a legal period.
        assert_eq!(Core::new().get_unstable_period(FuncUnstId::ALL), Err(RetCode::BadParam));
    }

    #[test]
    fn get_unstable_period_accepts_the_last_real_function() {
        // The other side of the guard's boundary: T3 is the last real variant,
        // one below the wildcard, so a guard tightened by one rejects it here.
        // Reads go through the getter on purpose -- indexing the array directly
        // would exercise the field, not the check.
        let core = Core::builder().unstable_period(FuncUnstId::ALL, 4).build().unwrap();
        for id in [FuncUnstId::ADX, FuncUnstId::EMA, FuncUnstId::RSI, FuncUnstId::T3] {
            assert_eq!(core.get_unstable_period(id), Ok(4));
        }
    }

    #[test]
    fn unstable_period_rejects_above_max_index() {
        // The period is added to a lookback that is then used as an index, so an
        // unbounded one overflows that lookback negative and the function indexes
        // far past its input. C rejects the same values (ta_utility.c).
        let too_big = u32::try_from(Core::MAX_INDEX).unwrap() + 1;
        for id in [FuncUnstId::EMA, FuncUnstId::ALL] {
            let err = Core::builder().unstable_period(id, too_big).build().unwrap_err();
            assert_eq!(err, RetCode::BadParam, "{id:?} must reject MAX_INDEX + 1");
            let err = Core::builder().unstable_period(id, u32::MAX).build().unwrap_err();
            assert_eq!(err, RetCode::BadParam, "{id:?} must reject u32::MAX");
        }
    }

    #[test]
    fn unstable_period_bound_is_a_bound_not_an_off_by_one() {
        // MAX_INDEX itself is legal -- C accepts it and rejects MAX_INDEX + 1, so
        // a guard tightened by one would be caught here rather than shipping.
        let ceiling = u32::try_from(Core::MAX_INDEX).unwrap();
        let core = Core::builder().unstable_period(FuncUnstId::EMA, ceiling).build().unwrap();
        assert_eq!(core.get_unstable_period(FuncUnstId::EMA), Ok(ceiling));
    }

    #[test]
    fn a_rejected_setter_writes_nothing() {
        // The half of the contract that an "it errors" assertion cannot see. C
        // checks before every store (ta_utility.c), so a rejected call is a true
        // no-op; latching the error must not come at the cost of a partial write.
        let ceiling = u32::try_from(Core::MAX_INDEX).unwrap();
        let builder = Core::builder()
            .unstable_period(FuncUnstId::ALL, 3)
            .unstable_period(FuncUnstId::EMA, ceiling + 1);
        // The error is latched, so the builder no longer yields a Core -- inspect
        // the values through a clone that clears the latch by rebuilding them.
        assert_eq!(builder.clone().build().unwrap_err(), RetCode::BadParam);
        // The test module is a child of this one, so the private slots are
        // readable here -- the point is the stored bytes, not the public API.
        assert_eq!(
            builder.unstable_period,
            [3_i32; FuncUnstId::COUNT],
            "a rejected period must leave every slot at its previous value"
        );
    }

    #[test]
    fn two_different_misuses_still_report_bad_param() {
        // Deliberately NOT named "the first rejection wins": BadParam is the only
        // code either misuse can produce, so which one was latched is not
        // observable through the public API and a test claiming otherwise would
        // assert nothing. What is worth pinning is that combining them does not
        // cancel out into a successful build.
        let ceiling = u32::try_from(Core::MAX_INDEX).unwrap();
        let err = Core::builder()
            .unstable_period(FuncUnstId::EMA, ceiling + 1)
            .candle_setting(
                CandleSettingType::AllCandleSettings,
                CandleSetting { range_type: RangeType::HighLow, avg_period: 1, factor: 1.0 },
            )
            .build()
            .unwrap_err();
        assert_eq!(err, RetCode::BadParam);
    }

    #[test]
    fn a_later_good_value_does_not_clear_the_latch() {
        // A corrected value repairs the STATE but not the report: a misuse that
        // was silently swallowed is exactly what #186 exists to stop.
        let ceiling = u32::try_from(Core::MAX_INDEX).unwrap();
        let err = Core::builder()
            .unstable_period(FuncUnstId::EMA, ceiling + 1)
            .unstable_period(FuncUnstId::EMA, 5)
            .build()
            .unwrap_err();
        assert_eq!(err, RetCode::BadParam);
    }

    #[test]
    fn to_builder_round_trip_is_total() {
        // A Core only exists if it validated, so rebuilding one can never fail --
        // including at the ceiling, the only value where the bound is in play.
        let ceiling = u32::try_from(Core::MAX_INDEX).unwrap();
        let core = Core::builder().unstable_period(FuncUnstId::ALL, ceiling).build().unwrap();
        let again = core.to_builder().build().expect("a built Core must always rebuild");
        assert_eq!(again.get_unstable_period(FuncUnstId::T3), Ok(ceiling));
    }

    #[test]
    fn builder_chains_and_last_write_wins() {
        let core = Core::builder()
            .unstable_period(FuncUnstId::ALL, 7) // all -> 7
            .unstable_period(FuncUnstId::EMA, 3)         // then EMA -> 3
            .build()
            .unwrap();
        assert_eq!(core.get_unstable_period(FuncUnstId::EMA), Ok(3));
        assert_eq!(core.get_unstable_period(FuncUnstId::RSI), Ok(7));
    }

    #[test]
    fn builder_candle_setting_overrides_one_leaves_rest() {
        let custom = CandleSetting { range_type: RangeType::Shadows, avg_period: 20, factor: 1.5 };
        let core =
            Core::builder().candle_setting(CandleSettingType::BodyLong, custom).build().unwrap();
        assert_eq!(core.candle_settings.body_long.range_type, RangeType::Shadows);
        assert_eq!(core.candle_settings.body_long.avg_period, 20);
        assert_eq!(core.candle_settings.body_long.factor, 1.5);
        // A different setting keeps its default.
        assert_eq!(core.candle_settings.body_doji.avg_period, 10);
    }

    #[test]
    fn restore_candle_default_derives_a_new_core_and_leaves_the_old_one_alone() {
        // The operation is "build a second Core", not "reset the first". Nothing
        // in this crate can reach into a built Core, and this is the assertion
        // that says so: `tuned` must still carry its override afterwards. C
        // cannot make the same claim -- its restore writes a process-wide global.
        let custom = CandleSetting { range_type: RangeType::Shadows, avg_period: 20, factor: 1.5 };
        let other = CandleSetting { range_type: RangeType::RealBody, avg_period: 7, factor: 4.0 };
        let tuned = Core::builder()
            .candle_setting(CandleSettingType::BodyLong, custom)
            .candle_setting(CandleSettingType::BodyDoji, other)
            .build()
            .unwrap();

        let derived = tuned
            .to_builder()
            .restore_candle_default(CandleSettingType::BodyLong)
            .build()
            .unwrap();

        // The derived Core has that one slot back at its default...
        assert_eq!(derived.candle_settings.body_long, CandleSetting {
            range_type: RangeType::RealBody,
            avg_period: 10,
            factor: 1.0
        });
        // ...the other override survived, so this is a single-slot restore and
        // not a rebuild from defaults...
        assert_eq!(derived.candle_settings.body_doji, other);
        // ...and the Core it was derived FROM is untouched.
        assert_eq!(tuned.candle_settings.body_long, custom);
    }

    #[test]
    fn restore_candle_default_is_a_no_op_on_a_fresh_builder() {
        // Stated as a test because it is the shape of the API, not an accident:
        // a fresh builder already sits on the defaults, so there is nothing for
        // this call to undo there. Anyone reaching for it on `Core::builder()`
        // wants to simply not call candle_setting.
        let plain = Core::builder().build().unwrap();
        let restored = Core::builder()
            .restore_candle_default(CandleSettingType::AllCandleSettings)
            .build()
            .unwrap();
        assert_eq!(plain.candle_settings, restored.candle_settings);
    }

    #[test]
    fn restore_candle_default_takes_the_wildcard_that_candle_setting_rejects() {
        // The asymmetry is the point, and it mirrors C exactly: the same
        // TA_AllCandleSettings that TA_SetCandleSettings answers TA_BAD_PARAM to
        // is a legal selector for TA_RestoreCandleDefaultSettings.
        let custom = CandleSetting { range_type: RangeType::Shadows, avg_period: 20, factor: 1.5 };
        let tuned = Core::builder()
            .candle_setting(CandleSettingType::BodyLong, custom)
            .candle_setting(CandleSettingType::Equal, custom)
            .build()
            .unwrap();
        let derived = tuned
            .to_builder()
            .restore_candle_default(CandleSettingType::AllCandleSettings)
            .build()
            .unwrap();
        assert_eq!(derived.candle_settings, CandleSettings::default_settings());
        // Again: the source Core kept both overrides.
        assert_eq!(tuned.candle_settings.body_long, custom);
        assert_eq!(tuned.candle_settings.equal, custom);
    }

    #[test]
    fn restore_candle_default_does_not_clear_a_latched_rejection() {
        // Restoring is not an apology for an earlier bad argument. `err` is
        // first-wins precisely so a later valid call cannot swallow the report.
        // The bad field is the period; the range type can no longer be one.
        let bad = CandleSetting { range_type: RangeType::RealBody, avg_period: -1, factor: 1.0 };
        let err = Core::builder()
            .candle_setting(CandleSettingType::BodyLong, bad)
            .restore_candle_default(CandleSettingType::AllCandleSettings)
            .build()
            .unwrap_err();
        assert_eq!(err, RetCode::BadParam);
    }

    #[test]
    fn candle_setting_rejects_the_wildcard() {
        // Silently ignoring it left the caller believing all eleven settings had
        // been overridden while none had; C returns TA_BAD_PARAM and Java throws
        // for the same call (#144). Rust reports it at build() rather than
        // aborting (#186) -- note the `.build()`, without which this test would
        // assert nothing at all now that the setter no longer panics.
        let custom = CandleSetting { range_type: RangeType::Shadows, avg_period: 99, factor: 9.0 };
        let err = Core::builder()
            .candle_setting(CandleSettingType::AllCandleSettings, custom)
            .build()
            .unwrap_err();
        assert_eq!(err, RetCode::BadParam);
    }

    #[test]
    fn a_rejected_candle_setting_writes_nothing() {
        // Same no-write rule as the period: the wildcard names no slot, so a
        // rejection must not have scribbled into one on the way out.
        let custom = CandleSetting { range_type: RangeType::Shadows, avg_period: 99, factor: 9.0 };
        let builder = Core::builder().candle_setting(CandleSettingType::AllCandleSettings, custom);
        let defaults = CandleSettings::default_settings();
        assert_eq!(builder.candle_settings.body_long.avg_period, defaults.body_long.avg_period);
        assert_eq!(builder.candle_settings.equal.factor, defaults.equal.factor);
        assert_eq!(builder.candle_settings.body_doji.range_type, defaults.body_doji.range_type);
    }

    #[test]
    fn candle_setting_rejects_a_negative_avg_period() {
        // The period is subtracted from startIdx to seed the trailing average,
        // so a negative one starts the main loop that many bars late while
        // outBegIdx still reports startIdx: every value shifted underneath a
        // correct-looking index, and a lookback that reports negative while the
        // call succeeds (#185). Reported, not asserted (#186) -- note the
        // `.build()`, without which this test would assert nothing now that the
        // setter latches instead of panicking.
        let custom = CandleSetting { range_type: RangeType::HighLow, avg_period: -1, factor: 0.1 };
        let err = Core::builder()
            .candle_setting(CandleSettingType::BodyDoji, custom)
            .build()
            .unwrap_err();
        assert_eq!(err, RetCode::BadParam);
    }

    #[test]
    fn candle_setting_accepts_a_zero_avg_period() {
        // The boundary on the legal side: zero means "no averaging", which every
        // CDL* body handles, so a guard written as `<= 0` would refuse a valid
        // setting.
        let custom = CandleSetting { range_type: RangeType::HighLow, avg_period: 0, factor: 0.1 };
        let core = Core::builder()
            .candle_setting(CandleSettingType::BodyDoji, custom)
            .build()
            .expect("a zero avg_period is legal");
        assert_eq!(core.candle_settings.body_doji.avg_period, 0);
    }

    #[test]
    fn candle_setting_rejects_an_avg_period_past_max_index() {
        // Same ceiling the unstable period already carries: an average longer
        // than the largest addressable series could never produce output, and an
        // unbounded one overflows the lookback it feeds.
        let custom = CandleSetting {
            range_type: RangeType::HighLow,
            avg_period: (Core::MAX_INDEX as i32).saturating_add(1),
            factor: 0.1,
        };
        let err = Core::builder()
            .candle_setting(CandleSettingType::BodyDoji, custom)
            .build()
            .unwrap_err();
        assert_eq!(err, RetCode::BadParam);
    }

    #[test]
    fn range_type_rejects_an_out_of_domain_value() {
        // A choice list's domain is its member list. Anything else would fall
        // through every arm of the generated candle-range match to 0.0 — every
        // range zero, every threshold zero, and a silently meaningless answer.
        //
        // The builder cannot be handed one: `RangeType` has no such value to
        // construct. This is the boundary where a raw integer still arrives —
        // the JSON-RPC server, a config file, C interop — so it is where the
        // domain is enforced, and the three legal values must survive it.
        assert_eq!(RangeType::try_from(3), Err(RetCode::BadParam));
        assert_eq!(RangeType::try_from(-1), Err(RetCode::BadParam));
        assert_eq!(RangeType::try_from(i32::MIN), Err(RetCode::BadParam));
        assert_eq!(RangeType::try_from(0), Ok(RangeType::RealBody));
        assert_eq!(RangeType::try_from(1), Ok(RangeType::HighLow));
        assert_eq!(RangeType::try_from(2), Ok(RangeType::Shadows));
    }

    #[test]
    fn candle_setting_accepts_the_ceilings() {
        // The upper boundary on the legal side, for both bounded fields.
        let custom = CandleSetting {
            range_type: RangeType::Shadows,
            avg_period: i32::try_from(Core::MAX_INDEX).unwrap(),
            factor: 0.1,
        };
        let core = Core::builder()
            .candle_setting(CandleSettingType::BodyDoji, custom)
            .build()
            .expect("the ceilings are on the legal side of the bound");
        assert_eq!(core.candle_settings.body_doji.avg_period, i32::try_from(Core::MAX_INDEX).unwrap());
        assert_eq!(core.candle_settings.body_doji.range_type, RangeType::Shadows);
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
                    CandleSetting { range_type: RangeType::HighLow, avg_period, factor: 0.1 },
                )
                .build()
                .expect("every avg_period in this sweep is inside the bound");
            let lookback = core.CDLDOJI_Lookback();
            assert!(lookback <= Core::MAX_INDEX, "avg_period {avg_period} gave lookback {lookback}");

            let mut out = vec![0_i32; n];
            let r = core
                .CDLDOJI(0, n - 1, &open, &high, &low, &close, &mut out)
                .expect("defaults are in range");
            if lookback > n - 1 {
                assert_eq!(r, OutRange::EMPTY, "avg_period {avg_period}");
            } else {
                assert_eq!(r.beg_idx, lookback, "avg_period {avg_period}");
                assert_eq!(r.count, n - lookback, "avg_period {avg_period}");
            }
        }
    }

    #[test]
    fn candle_setting_rejects_a_nan_factor() {
        // NaN makes every comparison it feeds false, so the patterns simply stop
        // matching -- indistinguishable from "this shape never occurs" unless the
        // setter refuses it.
        let custom = CandleSetting { range_type: RangeType::HighLow, avg_period: 10, factor: f64::NAN };
        let err = Core::builder()
            .candle_setting(CandleSettingType::BodyDoji, custom)
            .build()
            .unwrap_err();
        assert_eq!(err, RetCode::BadParam);
    }

    #[test]
    fn candle_setting_accepts_a_negative_factor() {
        // Only NaN is refused: a negative factor is a legal, if unusual,
        // threshold scale, so a guard written as `< 0.0 || is_nan()` would be
        // wrong. C accepts it too.
        let custom = CandleSetting { range_type: RangeType::HighLow, avg_period: 10, factor: -1.5 };
        let core = Core::builder()
            .candle_setting(CandleSettingType::BodyDoji, custom)
            .build()
            .expect("a negative factor is legal");
        assert_eq!(core.candle_settings.body_doji.factor, -1.5);
    }

    #[test]
    fn candle_setting_accepts_the_last_real_setting() {
        // The other side of the guard's boundary: Equal is the variant declared
        // immediately before the wildcard, so a guard widened by one rejects it.
        let custom = CandleSetting { range_type: RangeType::Shadows, avg_period: 99, factor: 9.0 };
        let core =
            Core::builder().candle_setting(CandleSettingType::Equal, custom).build().unwrap();
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
            let r = core
                .CDLDOJI(0, n - 1, &open, &high, &low, &close, &mut out)
                .expect("defaults are in range");
            out[..r.count].to_vec()
        };
        let default_out = run(&Core::new());
        let tuned = Core::builder()
            .candle_setting(
                CandleSettingType::BodyDoji,
                CandleSetting { range_type: RangeType::HighLow, avg_period: 10, factor: 1.0e9 },
            )
            .build()
            .unwrap();
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
                CandleSetting { range_type: RangeType::Shadows, avg_period: 20, factor: 1.5 },
            )
            .build()
            .unwrap();
        // Clone-and-modify: derive a Core that additionally tunes EMA.
        let derived =
            original.to_builder().unstable_period(FuncUnstId::EMA, 9).build().unwrap();
        // The original is immutable and unchanged.
        assert_eq!(original.get_unstable_period(FuncUnstId::EMA), Ok(0));
        assert_eq!(original.get_unstable_period(FuncUnstId::RSI), Ok(5));
        // The derived Core inherits the settings (candle_settings included, which
        // guards against to_builder dropping a field), plus the new one.
        assert_eq!(derived.get_unstable_period(FuncUnstId::RSI), Ok(5));
        assert_eq!(derived.get_unstable_period(FuncUnstId::EMA), Ok(9));
        // candle_settings survived the round-trip (default avg_period would be 10).
        assert_eq!(derived.candle_settings.body_long.avg_period, 20);
        assert_eq!(derived.candle_settings.body_long.factor, 1.5);
        assert_eq!(derived.candle_settings.body_long.range_type, RangeType::Shadows);
    }

    #[test]
    fn unstable_period_setting_changes_lookback() {
        let base = Core::new();
        let tuned = Core::builder().unstable_period(FuncUnstId::EMA, 5).build().unwrap();
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
        let core = Arc::new(Core::builder().unstable_period(FuncUnstId::EMA, 2).build().unwrap());
        let close: Vec<f64> = (0..64).map(|i| 100.0 + f64::from(i % 7)).collect();
        let mut handles = Vec::new();
        for _ in 0..4 {
            let core = Arc::clone(&core);
            let close = close.clone();
            handles.push(thread::spawn(move || {
                let mut out = vec![0.0; close.len()];
                core.EMA(0, close.len() - 1, &close, 10, &mut out)
                    .expect("period 10 is in range");
                out[0]
            }));
        }
        let expected = handles.pop().unwrap().join().unwrap();
        for h in handles {
            assert_eq!(h.join().unwrap(), expected, "concurrent shared-Core calls must agree");
        }
    }
}
