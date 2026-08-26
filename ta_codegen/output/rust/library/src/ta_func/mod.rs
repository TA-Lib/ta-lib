//! Generated technical-analysis functions — one private module per indicator,
//! all exposed as methods on [`Core`].

// Types and Core struct are in types.rs (hand-written, not generated).
mod types;
pub use types::*;

/// Moving-average type selected by an `optInMAType` parameter.
///
/// The values are pinned ABI, shared with C's `TA_MAType` and the Java and
/// C# `MAType`; the list is append-only. Convert a raw value in with
/// [`TryFrom<i32>`](MAType::try_from), and out with `as i32`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
#[allow(non_camel_case_types)]
pub enum MAType {
    /// The `TA_MAType_SMA` moving average.
    SMA = 0,
    /// The `TA_MAType_EMA` moving average.
    EMA = 1,
    /// The `TA_MAType_WMA` moving average.
    WMA = 2,
    /// The `TA_MAType_DEMA` moving average.
    DEMA = 3,
    /// The `TA_MAType_TEMA` moving average.
    TEMA = 4,
    /// The `TA_MAType_TRIMA` moving average.
    TRIMA = 5,
    /// The `TA_MAType_KAMA` moving average.
    KAMA = 6,
    /// The `TA_MAType_MAMA` moving average.
    MAMA = 7,
    /// The `TA_MAType_T3` moving average.
    T3 = 8,
    /// The `TA_MAType_HMA` moving average.
    HMA = 9,
    /// Not a moving average: the input is copied through unchanged.
    DISABLED = 10,
    /// Not a moving average: selects the documented default of whichever parameter it is passed to.
    DEFAULT = 11,
}

impl TryFrom<i32> for MAType {
    type Error = RetCode;

    /// Convert a raw parameter value, as the abstract layer and the JSON-RPC
    /// server hold it.
    ///
    /// `i32::MIN` — C's `TA_INTEGER_DEFAULT` — resolves to the `DEFAULT`
    /// member, so a value arriving through the abstract layer still selects
    /// that parameter's documented default exactly as it does in C. Every
    /// other out-of-domain value is `BadParam`, which is what keeps the
    /// rejection in this crate rather than in a caller that wraps it.
    ///
    /// # Errors
    ///
    /// [`RetCode::BadParam`] if the value names no member.
    fn try_from(value: i32) -> Result<Self, Self::Error> {
        Ok(match value {
            0 => Self::SMA,
            1 => Self::EMA,
            2 => Self::WMA,
            3 => Self::DEMA,
            4 => Self::TEMA,
            5 => Self::TRIMA,
            6 => Self::KAMA,
            7 => Self::MAMA,
            8 => Self::T3,
            9 => Self::HMA,
            10 => Self::DISABLED,
            11 => Self::DEFAULT,
            i32::MIN => Self::DEFAULT,
            _ => return Err(RetCode::BadParam),
        })
    }
}

// Hand-written test-only modules (not generated; see templates/rust/).
#[cfg(test)]
mod div_zero;
#[cfg(test)]
mod scratch_election;
#[cfg(test)]
mod stream_finite;
#[cfg(test)]
mod stream_out_range;

// Generated test-only modules.
#[cfg(test)]
mod no_phantom_io;

// Generated indicator modules:
mod ac;
mod accbands;
mod acos;
mod ad;
mod add;
mod adosc;
mod adx;
mod adxr;
mod ao;
mod apo;
mod aroon;
mod aroonosc;
mod asin;
mod atan;
mod atr;
mod avgdev;
mod avgprice;
mod bbands;
mod beta;
mod bop;
mod cci;
mod cdl2crows;
mod cdl3blackcrows;
mod cdl3inside;
mod cdl3linestrike;
mod cdl3outside;
mod cdl3starsinsouth;
mod cdl3whitesoldiers;
mod cdlabandonedbaby;
mod cdladvanceblock;
mod cdlbelthold;
mod cdlbreakaway;
mod cdlclosingmarubozu;
mod cdlconcealbabyswall;
mod cdlcounterattack;
mod cdldarkcloudcover;
mod cdldoji;
mod cdldojistar;
mod cdldragonflydoji;
mod cdlengulfing;
mod cdleveningdojistar;
mod cdleveningstar;
mod cdlgapsidesidewhite;
mod cdlgravestonedoji;
mod cdlhammer;
mod cdlhangingman;
mod cdlharami;
mod cdlharamicross;
mod cdlhighwave;
mod cdlhikkake;
mod cdlhikkakemod;
mod cdlhomingpigeon;
mod cdlidentical3crows;
mod cdlinneck;
mod cdlinvertedhammer;
mod cdlkicking;
mod cdlkickingbylength;
mod cdlladderbottom;
mod cdllongleggeddoji;
mod cdllongline;
mod cdlmarubozu;
mod cdlmatchinglow;
mod cdlmathold;
mod cdlmorningdojistar;
mod cdlmorningstar;
mod cdlonneck;
mod cdlpiercing;
mod cdlrickshawman;
mod cdlrisefall3methods;
mod cdlseparatinglines;
mod cdlshootingstar;
mod cdlshortline;
mod cdlspinningtop;
mod cdlstalledpattern;
mod cdlsticksandwich;
mod cdltakuri;
mod cdltasukigap;
mod cdlthrusting;
mod cdltristar;
mod cdlunique3river;
mod cdlupsidegap2crows;
mod cdlxsidegap3methods;
mod ceil;
mod cmf;
mod cmo;
mod cmou;
mod correl;
mod cos;
mod cosh;
mod dema;
mod div;
mod dx;
mod efi;
mod ema;
mod exp;
mod floor;
mod hma;
mod ht_dcperiod;
mod ht_dcphase;
mod ht_phasor;
mod ht_sine;
mod ht_trendline;
mod ht_trendmode;
mod imi;
mod kama;
mod linearreg;
mod linearreg_angle;
mod linearreg_intercept;
mod linearreg_slope;
mod ln;
mod log10;
mod ma;
mod macd;
mod macdext;
mod macdfix;
mod mama;
mod marketfi;
mod mavp;
mod max;
mod maxindex;
mod medprice;
mod mfi;
mod midpoint;
mod midprice;
mod min;
mod minindex;
mod minmax;
mod minmaxindex;
mod minus_di;
mod minus_dm;
mod mom;
mod mult;
mod natr;
mod nvi;
mod obv;
mod plus_di;
mod plus_dm;
mod ppo;
mod pvi;
mod pvo;
mod qstick;
mod roc;
mod rocp;
mod rocr;
mod rocr100;
mod rsi;
mod sar;
mod sarext;
mod sin;
mod sinh;
mod sma;
mod smi;
mod sqrt;
mod stddev;
mod stoch;
mod stochf;
mod stochrsi;
mod sub;
mod sum;
mod t3;
mod tan;
mod tanh;
mod tema;
mod trange;
mod trima;
mod trix;
mod tsf;
mod typprice;
mod ultosc;
mod var;
mod vwap;
mod vwma;
mod wad;
mod wclprice;
mod willr;
mod wma;

// Generated stream handles (one per streamable indicator):
pub use ac::AC_Stream;
pub use accbands::ACCBANDS_Stream;
pub use acos::ACOS_Stream;
pub use ad::AD_Stream;
pub use add::ADD_Stream;
pub use adosc::ADOSC_Stream;
pub use adx::ADX_Stream;
pub use adxr::ADXR_Stream;
pub use ao::AO_Stream;
pub use apo::APO_Stream;
pub use aroon::AROON_Stream;
pub use aroonosc::AROONOSC_Stream;
pub use asin::ASIN_Stream;
pub use atan::ATAN_Stream;
pub use atr::ATR_Stream;
pub use avgdev::AVGDEV_Stream;
pub use avgprice::AVGPRICE_Stream;
pub use bbands::BBANDS_Stream;
pub use beta::BETA_Stream;
pub use bop::BOP_Stream;
pub use cci::CCI_Stream;
pub use cdl2crows::CDL2CROWS_Stream;
pub use cdl3blackcrows::CDL3BLACKCROWS_Stream;
pub use cdl3inside::CDL3INSIDE_Stream;
pub use cdl3linestrike::CDL3LINESTRIKE_Stream;
pub use cdl3outside::CDL3OUTSIDE_Stream;
pub use cdl3starsinsouth::CDL3STARSINSOUTH_Stream;
pub use cdl3whitesoldiers::CDL3WHITESOLDIERS_Stream;
pub use cdlabandonedbaby::CDLABANDONEDBABY_Stream;
pub use cdladvanceblock::CDLADVANCEBLOCK_Stream;
pub use cdlbelthold::CDLBELTHOLD_Stream;
pub use cdlbreakaway::CDLBREAKAWAY_Stream;
pub use cdlclosingmarubozu::CDLCLOSINGMARUBOZU_Stream;
pub use cdlconcealbabyswall::CDLCONCEALBABYSWALL_Stream;
pub use cdlcounterattack::CDLCOUNTERATTACK_Stream;
pub use cdldarkcloudcover::CDLDARKCLOUDCOVER_Stream;
pub use cdldoji::CDLDOJI_Stream;
pub use cdldojistar::CDLDOJISTAR_Stream;
pub use cdldragonflydoji::CDLDRAGONFLYDOJI_Stream;
pub use cdlengulfing::CDLENGULFING_Stream;
pub use cdleveningdojistar::CDLEVENINGDOJISTAR_Stream;
pub use cdleveningstar::CDLEVENINGSTAR_Stream;
pub use cdlgapsidesidewhite::CDLGAPSIDESIDEWHITE_Stream;
pub use cdlgravestonedoji::CDLGRAVESTONEDOJI_Stream;
pub use cdlhammer::CDLHAMMER_Stream;
pub use cdlhangingman::CDLHANGINGMAN_Stream;
pub use cdlharami::CDLHARAMI_Stream;
pub use cdlharamicross::CDLHARAMICROSS_Stream;
pub use cdlhighwave::CDLHIGHWAVE_Stream;
pub use cdlhikkake::CDLHIKKAKE_Stream;
pub use cdlhikkakemod::CDLHIKKAKEMOD_Stream;
pub use cdlhomingpigeon::CDLHOMINGPIGEON_Stream;
pub use cdlidentical3crows::CDLIDENTICAL3CROWS_Stream;
pub use cdlinneck::CDLINNECK_Stream;
pub use cdlinvertedhammer::CDLINVERTEDHAMMER_Stream;
pub use cdlkicking::CDLKICKING_Stream;
pub use cdlkickingbylength::CDLKICKINGBYLENGTH_Stream;
pub use cdlladderbottom::CDLLADDERBOTTOM_Stream;
pub use cdllongleggeddoji::CDLLONGLEGGEDDOJI_Stream;
pub use cdllongline::CDLLONGLINE_Stream;
pub use cdlmarubozu::CDLMARUBOZU_Stream;
pub use cdlmatchinglow::CDLMATCHINGLOW_Stream;
pub use cdlmathold::CDLMATHOLD_Stream;
pub use cdlmorningdojistar::CDLMORNINGDOJISTAR_Stream;
pub use cdlmorningstar::CDLMORNINGSTAR_Stream;
pub use cdlonneck::CDLONNECK_Stream;
pub use cdlpiercing::CDLPIERCING_Stream;
pub use cdlrickshawman::CDLRICKSHAWMAN_Stream;
pub use cdlrisefall3methods::CDLRISEFALL3METHODS_Stream;
pub use cdlseparatinglines::CDLSEPARATINGLINES_Stream;
pub use cdlshootingstar::CDLSHOOTINGSTAR_Stream;
pub use cdlshortline::CDLSHORTLINE_Stream;
pub use cdlspinningtop::CDLSPINNINGTOP_Stream;
pub use cdlstalledpattern::CDLSTALLEDPATTERN_Stream;
pub use cdlsticksandwich::CDLSTICKSANDWICH_Stream;
pub use cdltakuri::CDLTAKURI_Stream;
pub use cdltasukigap::CDLTASUKIGAP_Stream;
pub use cdlthrusting::CDLTHRUSTING_Stream;
pub use cdltristar::CDLTRISTAR_Stream;
pub use cdlunique3river::CDLUNIQUE3RIVER_Stream;
pub use cdlupsidegap2crows::CDLUPSIDEGAP2CROWS_Stream;
pub use cdlxsidegap3methods::CDLXSIDEGAP3METHODS_Stream;
pub use ceil::CEIL_Stream;
pub use cmf::CMF_Stream;
pub use cmo::CMO_Stream;
pub use cmou::CMOU_Stream;
pub use correl::CORREL_Stream;
pub use cos::COS_Stream;
pub use cosh::COSH_Stream;
pub use dema::DEMA_Stream;
pub use div::DIV_Stream;
pub use dx::DX_Stream;
pub use efi::EFI_Stream;
pub use ema::EMA_Stream;
pub use exp::EXP_Stream;
pub use floor::FLOOR_Stream;
pub use hma::HMA_Stream;
pub use ht_dcperiod::HT_DCPERIOD_Stream;
pub use ht_dcphase::HT_DCPHASE_Stream;
pub use ht_phasor::HT_PHASOR_Stream;
pub use ht_sine::HT_SINE_Stream;
pub use ht_trendline::HT_TRENDLINE_Stream;
pub use ht_trendmode::HT_TRENDMODE_Stream;
pub use imi::IMI_Stream;
pub use kama::KAMA_Stream;
pub use linearreg::LINEARREG_Stream;
pub use linearreg_angle::LINEARREG_ANGLE_Stream;
pub use linearreg_intercept::LINEARREG_INTERCEPT_Stream;
pub use linearreg_slope::LINEARREG_SLOPE_Stream;
pub use ln::LN_Stream;
pub use log10::LOG10_Stream;
pub use ma::MA_Stream;
pub use macd::MACD_Stream;
pub use macdext::MACDEXT_Stream;
pub use macdfix::MACDFIX_Stream;
pub use mama::MAMA_Stream;
pub use marketfi::MARKETFI_Stream;
pub use mavp::MAVP_Stream;
pub use max::MAX_Stream;
pub use maxindex::MAXINDEX_Stream;
pub use medprice::MEDPRICE_Stream;
pub use mfi::MFI_Stream;
pub use midpoint::MIDPOINT_Stream;
pub use midprice::MIDPRICE_Stream;
pub use min::MIN_Stream;
pub use minindex::MININDEX_Stream;
pub use minmax::MINMAX_Stream;
pub use minmaxindex::MINMAXINDEX_Stream;
pub use minus_di::MINUS_DI_Stream;
pub use minus_dm::MINUS_DM_Stream;
pub use mom::MOM_Stream;
pub use mult::MULT_Stream;
pub use natr::NATR_Stream;
pub use nvi::NVI_Stream;
pub use obv::OBV_Stream;
pub use plus_di::PLUS_DI_Stream;
pub use plus_dm::PLUS_DM_Stream;
pub use ppo::PPO_Stream;
pub use pvi::PVI_Stream;
pub use pvo::PVO_Stream;
pub use qstick::QSTICK_Stream;
pub use roc::ROC_Stream;
pub use rocp::ROCP_Stream;
pub use rocr::ROCR_Stream;
pub use rocr100::ROCR100_Stream;
pub use rsi::RSI_Stream;
pub use sar::SAR_Stream;
pub use sarext::SAREXT_Stream;
pub use sin::SIN_Stream;
pub use sinh::SINH_Stream;
pub use sma::SMA_Stream;
pub use smi::SMI_Stream;
pub use sqrt::SQRT_Stream;
pub use stddev::STDDEV_Stream;
pub use stoch::STOCH_Stream;
pub use stochf::STOCHF_Stream;
pub use stochrsi::STOCHRSI_Stream;
pub use sub::SUB_Stream;
pub use sum::SUM_Stream;
pub use t3::T3_Stream;
pub use tan::TAN_Stream;
pub use tanh::TANH_Stream;
pub use tema::TEMA_Stream;
pub use trange::TRANGE_Stream;
pub use trima::TRIMA_Stream;
pub use trix::TRIX_Stream;
pub use tsf::TSF_Stream;
pub use typprice::TYPPRICE_Stream;
pub use ultosc::ULTOSC_Stream;
pub use var::VAR_Stream;
pub use vwap::VWAP_Stream;
pub use vwma::VWMA_Stream;
pub use wad::WAD_Stream;
pub use wclprice::WCLPRICE_Stream;
pub use willr::WILLR_Stream;
pub use wma::WMA_Stream;
