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

/// Core struct providing access to all TA-Lib technical analysis functions.
///
/// Create an instance with [`Core::new()`] and call functions as methods.
/// Unstable period and compatibility mode can be configured per-instance.
pub struct Core {
    /// Unstable period for each function identified by [`FuncUnstId`].
    pub unstable_period: [i32; FuncUnstId::FuncUnstAll as usize],
    /// Compatibility mode (default: [`Compatibility::Default`]).
    pub compatibility: Compatibility,
    /// Candlestick pattern settings.
    pub candle_settings: CandleSettings,
}

impl Core {
    /// Create a new Core instance with default settings.
    pub fn new() -> Self {
        Self {
            unstable_period: [0; FuncUnstId::FuncUnstAll as usize],
            compatibility: Compatibility::Default,
            candle_settings: CandleSettings::default_settings(),
        }
    }

    /// Set the unstable period for a specific function.
    pub fn set_unstable_period(&mut self, id: FuncUnstId, period: i32) {
        self.unstable_period[id as usize] = period;
    }

    /// Get the unstable period for a specific function.
    pub fn get_unstable_period(&self, id: FuncUnstId) -> i32 {
        self.unstable_period[id as usize]
    }

    /// Set the compatibility mode.
    pub fn set_compatibility(&mut self, compat: Compatibility) {
        self.compatibility = compat;
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
