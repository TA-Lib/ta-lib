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
pub use ac::AcStream;
pub use accbands::AccbandsStream;
pub use acos::AcosStream;
pub use ad::AdStream;
pub use add::AddStream;
pub use adosc::AdoscStream;
pub use adx::AdxStream;
pub use adxr::AdxrStream;
pub use ao::AoStream;
pub use apo::ApoStream;
pub use aroon::AroonStream;
pub use aroonosc::AroonoscStream;
pub use asin::AsinStream;
pub use atan::AtanStream;
pub use atr::AtrStream;
pub use avgdev::AvgdevStream;
pub use avgprice::AvgpriceStream;
pub use bbands::BbandsStream;
pub use beta::BetaStream;
pub use bop::BopStream;
pub use cci::CciStream;
pub use cdl2crows::Cdl2crowsStream;
pub use cdl3blackcrows::Cdl3blackcrowsStream;
pub use cdl3inside::Cdl3insideStream;
pub use cdl3linestrike::Cdl3linestrikeStream;
pub use cdl3outside::Cdl3outsideStream;
pub use cdl3starsinsouth::Cdl3starsinsouthStream;
pub use cdl3whitesoldiers::Cdl3whitesoldiersStream;
pub use cdlabandonedbaby::CdlabandonedbabyStream;
pub use cdladvanceblock::CdladvanceblockStream;
pub use cdlbelthold::CdlbeltholdStream;
pub use cdlbreakaway::CdlbreakawayStream;
pub use cdlclosingmarubozu::CdlclosingmarubozuStream;
pub use cdlconcealbabyswall::CdlconcealbabyswallStream;
pub use cdlcounterattack::CdlcounterattackStream;
pub use cdldarkcloudcover::CdldarkcloudcoverStream;
pub use cdldoji::CdldojiStream;
pub use cdldojistar::CdldojistarStream;
pub use cdldragonflydoji::CdldragonflydojiStream;
pub use cdlengulfing::CdlengulfingStream;
pub use cdleveningdojistar::CdleveningdojistarStream;
pub use cdleveningstar::CdleveningstarStream;
pub use cdlgapsidesidewhite::CdlgapsidesidewhiteStream;
pub use cdlgravestonedoji::CdlgravestonedojiStream;
pub use cdlhammer::CdlhammerStream;
pub use cdlhangingman::CdlhangingmanStream;
pub use cdlharami::CdlharamiStream;
pub use cdlharamicross::CdlharamicrossStream;
pub use cdlhighwave::CdlhighwaveStream;
pub use cdlhikkake::CdlhikkakeStream;
pub use cdlhikkakemod::CdlhikkakemodStream;
pub use cdlhomingpigeon::CdlhomingpigeonStream;
pub use cdlidentical3crows::Cdlidentical3crowsStream;
pub use cdlinneck::CdlinneckStream;
pub use cdlinvertedhammer::CdlinvertedhammerStream;
pub use cdlkicking::CdlkickingStream;
pub use cdlkickingbylength::CdlkickingbylengthStream;
pub use cdlladderbottom::CdlladderbottomStream;
pub use cdllongleggeddoji::CdllongleggeddojiStream;
pub use cdllongline::CdllonglineStream;
pub use cdlmarubozu::CdlmarubozuStream;
pub use cdlmatchinglow::CdlmatchinglowStream;
pub use cdlmathold::CdlmatholdStream;
pub use cdlmorningdojistar::CdlmorningdojistarStream;
pub use cdlmorningstar::CdlmorningstarStream;
pub use cdlonneck::CdlonneckStream;
pub use cdlpiercing::CdlpiercingStream;
pub use cdlrickshawman::CdlrickshawmanStream;
pub use cdlrisefall3methods::Cdlrisefall3methodsStream;
pub use cdlseparatinglines::CdlseparatinglinesStream;
pub use cdlshootingstar::CdlshootingstarStream;
pub use cdlshortline::CdlshortlineStream;
pub use cdlspinningtop::CdlspinningtopStream;
pub use cdlstalledpattern::CdlstalledpatternStream;
pub use cdlsticksandwich::CdlsticksandwichStream;
pub use cdltakuri::CdltakuriStream;
pub use cdltasukigap::CdltasukigapStream;
pub use cdlthrusting::CdlthrustingStream;
pub use cdltristar::CdltristarStream;
pub use cdlunique3river::Cdlunique3riverStream;
pub use cdlupsidegap2crows::Cdlupsidegap2crowsStream;
pub use cdlxsidegap3methods::Cdlxsidegap3methodsStream;
pub use ceil::CeilStream;
pub use cmf::CmfStream;
pub use cmo::CmoStream;
pub use cmou::CmouStream;
pub use correl::CorrelStream;
pub use cos::CosStream;
pub use cosh::CoshStream;
pub use dema::DemaStream;
pub use div::DivStream;
pub use dx::DxStream;
pub use efi::EfiStream;
pub use ema::EmaStream;
pub use exp::ExpStream;
pub use floor::FloorStream;
pub use hma::HmaStream;
pub use ht_dcperiod::HtDcperiodStream;
pub use ht_dcphase::HtDcphaseStream;
pub use ht_phasor::HtPhasorStream;
pub use ht_sine::HtSineStream;
pub use ht_trendline::HtTrendlineStream;
pub use ht_trendmode::HtTrendmodeStream;
pub use imi::ImiStream;
pub use kama::KamaStream;
pub use linearreg::LinearregStream;
pub use linearreg_angle::LinearregAngleStream;
pub use linearreg_intercept::LinearregInterceptStream;
pub use linearreg_slope::LinearregSlopeStream;
pub use ln::LnStream;
pub use log10::Log10Stream;
pub use ma::MaStream;
pub use macd::MacdStream;
pub use macdext::MacdextStream;
pub use macdfix::MacdfixStream;
pub use mama::MamaStream;
pub use marketfi::MarketfiStream;
pub use mavp::MavpStream;
pub use max::MaxStream;
pub use maxindex::MaxindexStream;
pub use medprice::MedpriceStream;
pub use mfi::MfiStream;
pub use midpoint::MidpointStream;
pub use midprice::MidpriceStream;
pub use min::MinStream;
pub use minindex::MinindexStream;
pub use minmax::MinmaxStream;
pub use minmaxindex::MinmaxindexStream;
pub use minus_di::MinusDiStream;
pub use minus_dm::MinusDmStream;
pub use mom::MomStream;
pub use mult::MultStream;
pub use natr::NatrStream;
pub use nvi::NviStream;
pub use obv::ObvStream;
pub use plus_di::PlusDiStream;
pub use plus_dm::PlusDmStream;
pub use ppo::PpoStream;
pub use pvi::PviStream;
pub use pvo::PvoStream;
pub use qstick::QstickStream;
pub use roc::RocStream;
pub use rocp::RocpStream;
pub use rocr::RocrStream;
pub use rocr100::Rocr100Stream;
pub use rsi::RsiStream;
pub use sar::SarStream;
pub use sarext::SarextStream;
pub use sin::SinStream;
pub use sinh::SinhStream;
pub use sma::SmaStream;
pub use smi::SmiStream;
pub use sqrt::SqrtStream;
pub use stddev::StddevStream;
pub use stoch::StochStream;
pub use stochf::StochfStream;
pub use stochrsi::StochrsiStream;
pub use sub::SubStream;
pub use sum::SumStream;
pub use t3::T3Stream;
pub use tan::TanStream;
pub use tanh::TanhStream;
pub use tema::TemaStream;
pub use trange::TrangeStream;
pub use trima::TrimaStream;
pub use trix::TrixStream;
pub use tsf::TsfStream;
pub use typprice::TyppriceStream;
pub use ultosc::UltoscStream;
pub use var::VarStream;
pub use vwap::VwapStream;
pub use vwma::VwmaStream;
pub use wad::WadStream;
pub use wclprice::WclpriceStream;
pub use willr::WillrStream;
pub use wma::WmaStream;
