// Auto-generated JSON-RPC server for ta_codegen .NET output.
// Uses P/Invoke to call the generated C shared library.
// Requires: dotnet 8.0+, libta_codegen_funcs.dylib/.so in bin/
using System;
using System.IO;
using System.Text.Json;
using System.Runtime.InteropServices;
using System.Diagnostics;

public class TaCodegenServe {
    const int MAX_ARRAY_SIZE = 200000;
    static double[] refOpen = new double[MAX_ARRAY_SIZE];
    static double[] refHigh = new double[MAX_ARRAY_SIZE];
    static double[] refLow = new double[MAX_ARRAY_SIZE];
    static double[] refClose = new double[MAX_ARRAY_SIZE];
    static double[] refVolume = new double[MAX_ARRAY_SIZE];
    static double[] refOI = new double[MAX_ARRAY_SIZE];
    static int refN = 0;

    static long GetNanoTime() {
        long ts = Stopwatch.GetTimestamp();
        long freq = Stopwatch.Frequency;
        return (ts / freq) * 1000000000L + (ts % freq) * 1000000000L / freq;
    }

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_Initialize")]
    static extern int TA_Initialize();

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SetUnstablePeriod")]
    static extern void TA_SetUnstablePeriod(int id, int period);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SetCompatibility")]
    static extern int TA_SetCompatibility(int value);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ACCBANDS")]
    static extern int TA_ACCBANDS(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1,
        double[] outArr2);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ACCBANDS_Unguarded")]
    static extern int TA_ACCBANDS_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1,
        double[] outArr2);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ACOS")]
    static extern int TA_ACOS(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ACOS_Unguarded")]
    static extern int TA_ACOS_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_AD")]
    static extern int TA_AD(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double[] inVolume,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_AD_Unguarded")]
    static extern int TA_AD_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double[] inVolume,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ADD")]
    static extern int TA_ADD(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ADD_Unguarded")]
    static extern int TA_ADD_Unguarded(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ADOSC")]
    static extern int TA_ADOSC(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double[] inVolume,
        int optInFastPeriod,
        int optInSlowPeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ADOSC_Unguarded")]
    static extern int TA_ADOSC_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double[] inVolume,
        int optInFastPeriod,
        int optInSlowPeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ADX")]
    static extern int TA_ADX(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ADX_Unguarded")]
    static extern int TA_ADX_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ADXR")]
    static extern int TA_ADXR(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ADXR_Unguarded")]
    static extern int TA_ADXR_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_APO")]
    static extern int TA_APO(
        int startIdx, int endIdx,
        double[] inReal,
        int optInFastPeriod,
        int optInSlowPeriod,
        int optInMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_APO_Unguarded")]
    static extern int TA_APO_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInFastPeriod,
        int optInSlowPeriod,
        int optInMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_AROON")]
    static extern int TA_AROON(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_AROON_Unguarded")]
    static extern int TA_AROON_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_AROONOSC")]
    static extern int TA_AROONOSC(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_AROONOSC_Unguarded")]
    static extern int TA_AROONOSC_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ASIN")]
    static extern int TA_ASIN(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ASIN_Unguarded")]
    static extern int TA_ASIN_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ATAN")]
    static extern int TA_ATAN(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ATAN_Unguarded")]
    static extern int TA_ATAN_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ATR")]
    static extern int TA_ATR(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ATR_Unguarded")]
    static extern int TA_ATR_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_AVGDEV")]
    static extern int TA_AVGDEV(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_AVGDEV_Unguarded")]
    static extern int TA_AVGDEV_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_AVGPRICE")]
    static extern int TA_AVGPRICE(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_AVGPRICE_Unguarded")]
    static extern int TA_AVGPRICE_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_BBANDS")]
    static extern int TA_BBANDS(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        double optInNbDevUp,
        double optInNbDevDn,
        int optInMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1,
        double[] outArr2);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_BBANDS_Unguarded")]
    static extern int TA_BBANDS_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        double optInNbDevUp,
        double optInNbDevDn,
        int optInMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1,
        double[] outArr2);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_BETA")]
    static extern int TA_BETA(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_BETA_Unguarded")]
    static extern int TA_BETA_Unguarded(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_BOP")]
    static extern int TA_BOP(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_BOP_Unguarded")]
    static extern int TA_BOP_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CCI")]
    static extern int TA_CCI(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CCI_Unguarded")]
    static extern int TA_CCI_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL2CROWS")]
    static extern int TA_CDL2CROWS(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL2CROWS_Unguarded")]
    static extern int TA_CDL2CROWS_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3BLACKCROWS")]
    static extern int TA_CDL3BLACKCROWS(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3BLACKCROWS_Unguarded")]
    static extern int TA_CDL3BLACKCROWS_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3INSIDE")]
    static extern int TA_CDL3INSIDE(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3INSIDE_Unguarded")]
    static extern int TA_CDL3INSIDE_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3LINESTRIKE")]
    static extern int TA_CDL3LINESTRIKE(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3LINESTRIKE_Unguarded")]
    static extern int TA_CDL3LINESTRIKE_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3OUTSIDE")]
    static extern int TA_CDL3OUTSIDE(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3OUTSIDE_Unguarded")]
    static extern int TA_CDL3OUTSIDE_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3STARSINSOUTH")]
    static extern int TA_CDL3STARSINSOUTH(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3STARSINSOUTH_Unguarded")]
    static extern int TA_CDL3STARSINSOUTH_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3WHITESOLDIERS")]
    static extern int TA_CDL3WHITESOLDIERS(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDL3WHITESOLDIERS_Unguarded")]
    static extern int TA_CDL3WHITESOLDIERS_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLABANDONEDBABY")]
    static extern int TA_CDLABANDONEDBABY(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLABANDONEDBABY_Unguarded")]
    static extern int TA_CDLABANDONEDBABY_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLADVANCEBLOCK")]
    static extern int TA_CDLADVANCEBLOCK(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLADVANCEBLOCK_Unguarded")]
    static extern int TA_CDLADVANCEBLOCK_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLBELTHOLD")]
    static extern int TA_CDLBELTHOLD(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLBELTHOLD_Unguarded")]
    static extern int TA_CDLBELTHOLD_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLBREAKAWAY")]
    static extern int TA_CDLBREAKAWAY(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLBREAKAWAY_Unguarded")]
    static extern int TA_CDLBREAKAWAY_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLCLOSINGMARUBOZU")]
    static extern int TA_CDLCLOSINGMARUBOZU(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLCLOSINGMARUBOZU_Unguarded")]
    static extern int TA_CDLCLOSINGMARUBOZU_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLCONCEALBABYSWALL")]
    static extern int TA_CDLCONCEALBABYSWALL(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLCONCEALBABYSWALL_Unguarded")]
    static extern int TA_CDLCONCEALBABYSWALL_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLCOUNTERATTACK")]
    static extern int TA_CDLCOUNTERATTACK(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLCOUNTERATTACK_Unguarded")]
    static extern int TA_CDLCOUNTERATTACK_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLDARKCLOUDCOVER")]
    static extern int TA_CDLDARKCLOUDCOVER(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLDARKCLOUDCOVER_Unguarded")]
    static extern int TA_CDLDARKCLOUDCOVER_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLDOJI")]
    static extern int TA_CDLDOJI(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLDOJI_Unguarded")]
    static extern int TA_CDLDOJI_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLDOJISTAR")]
    static extern int TA_CDLDOJISTAR(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLDOJISTAR_Unguarded")]
    static extern int TA_CDLDOJISTAR_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLDRAGONFLYDOJI")]
    static extern int TA_CDLDRAGONFLYDOJI(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLDRAGONFLYDOJI_Unguarded")]
    static extern int TA_CDLDRAGONFLYDOJI_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLENGULFING")]
    static extern int TA_CDLENGULFING(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLENGULFING_Unguarded")]
    static extern int TA_CDLENGULFING_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLEVENINGDOJISTAR")]
    static extern int TA_CDLEVENINGDOJISTAR(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLEVENINGDOJISTAR_Unguarded")]
    static extern int TA_CDLEVENINGDOJISTAR_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLEVENINGSTAR")]
    static extern int TA_CDLEVENINGSTAR(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLEVENINGSTAR_Unguarded")]
    static extern int TA_CDLEVENINGSTAR_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLGAPSIDESIDEWHITE")]
    static extern int TA_CDLGAPSIDESIDEWHITE(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLGAPSIDESIDEWHITE_Unguarded")]
    static extern int TA_CDLGAPSIDESIDEWHITE_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLGRAVESTONEDOJI")]
    static extern int TA_CDLGRAVESTONEDOJI(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLGRAVESTONEDOJI_Unguarded")]
    static extern int TA_CDLGRAVESTONEDOJI_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHAMMER")]
    static extern int TA_CDLHAMMER(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHAMMER_Unguarded")]
    static extern int TA_CDLHAMMER_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHANGINGMAN")]
    static extern int TA_CDLHANGINGMAN(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHANGINGMAN_Unguarded")]
    static extern int TA_CDLHANGINGMAN_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHARAMI")]
    static extern int TA_CDLHARAMI(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHARAMI_Unguarded")]
    static extern int TA_CDLHARAMI_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHARAMICROSS")]
    static extern int TA_CDLHARAMICROSS(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHARAMICROSS_Unguarded")]
    static extern int TA_CDLHARAMICROSS_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHIGHWAVE")]
    static extern int TA_CDLHIGHWAVE(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHIGHWAVE_Unguarded")]
    static extern int TA_CDLHIGHWAVE_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHIKKAKE")]
    static extern int TA_CDLHIKKAKE(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHIKKAKE_Unguarded")]
    static extern int TA_CDLHIKKAKE_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHIKKAKEMOD")]
    static extern int TA_CDLHIKKAKEMOD(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHIKKAKEMOD_Unguarded")]
    static extern int TA_CDLHIKKAKEMOD_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHOMINGPIGEON")]
    static extern int TA_CDLHOMINGPIGEON(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLHOMINGPIGEON_Unguarded")]
    static extern int TA_CDLHOMINGPIGEON_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLIDENTICAL3CROWS")]
    static extern int TA_CDLIDENTICAL3CROWS(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLIDENTICAL3CROWS_Unguarded")]
    static extern int TA_CDLIDENTICAL3CROWS_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLINNECK")]
    static extern int TA_CDLINNECK(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLINNECK_Unguarded")]
    static extern int TA_CDLINNECK_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLINVERTEDHAMMER")]
    static extern int TA_CDLINVERTEDHAMMER(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLINVERTEDHAMMER_Unguarded")]
    static extern int TA_CDLINVERTEDHAMMER_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLKICKING")]
    static extern int TA_CDLKICKING(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLKICKING_Unguarded")]
    static extern int TA_CDLKICKING_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLKICKINGBYLENGTH")]
    static extern int TA_CDLKICKINGBYLENGTH(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLKICKINGBYLENGTH_Unguarded")]
    static extern int TA_CDLKICKINGBYLENGTH_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLLADDERBOTTOM")]
    static extern int TA_CDLLADDERBOTTOM(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLLADDERBOTTOM_Unguarded")]
    static extern int TA_CDLLADDERBOTTOM_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLLONGLEGGEDDOJI")]
    static extern int TA_CDLLONGLEGGEDDOJI(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLLONGLEGGEDDOJI_Unguarded")]
    static extern int TA_CDLLONGLEGGEDDOJI_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLLONGLINE")]
    static extern int TA_CDLLONGLINE(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLLONGLINE_Unguarded")]
    static extern int TA_CDLLONGLINE_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLMARUBOZU")]
    static extern int TA_CDLMARUBOZU(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLMARUBOZU_Unguarded")]
    static extern int TA_CDLMARUBOZU_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLMATCHINGLOW")]
    static extern int TA_CDLMATCHINGLOW(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLMATCHINGLOW_Unguarded")]
    static extern int TA_CDLMATCHINGLOW_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLMATHOLD")]
    static extern int TA_CDLMATHOLD(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLMATHOLD_Unguarded")]
    static extern int TA_CDLMATHOLD_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLMORNINGDOJISTAR")]
    static extern int TA_CDLMORNINGDOJISTAR(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLMORNINGDOJISTAR_Unguarded")]
    static extern int TA_CDLMORNINGDOJISTAR_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLMORNINGSTAR")]
    static extern int TA_CDLMORNINGSTAR(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLMORNINGSTAR_Unguarded")]
    static extern int TA_CDLMORNINGSTAR_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double optInPenetration,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLONNECK")]
    static extern int TA_CDLONNECK(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLONNECK_Unguarded")]
    static extern int TA_CDLONNECK_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLPIERCING")]
    static extern int TA_CDLPIERCING(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLPIERCING_Unguarded")]
    static extern int TA_CDLPIERCING_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLRICKSHAWMAN")]
    static extern int TA_CDLRICKSHAWMAN(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLRICKSHAWMAN_Unguarded")]
    static extern int TA_CDLRICKSHAWMAN_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLRISEFALL3METHODS")]
    static extern int TA_CDLRISEFALL3METHODS(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLRISEFALL3METHODS_Unguarded")]
    static extern int TA_CDLRISEFALL3METHODS_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSEPARATINGLINES")]
    static extern int TA_CDLSEPARATINGLINES(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSEPARATINGLINES_Unguarded")]
    static extern int TA_CDLSEPARATINGLINES_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSHOOTINGSTAR")]
    static extern int TA_CDLSHOOTINGSTAR(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSHOOTINGSTAR_Unguarded")]
    static extern int TA_CDLSHOOTINGSTAR_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSHORTLINE")]
    static extern int TA_CDLSHORTLINE(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSHORTLINE_Unguarded")]
    static extern int TA_CDLSHORTLINE_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSPINNINGTOP")]
    static extern int TA_CDLSPINNINGTOP(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSPINNINGTOP_Unguarded")]
    static extern int TA_CDLSPINNINGTOP_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSTALLEDPATTERN")]
    static extern int TA_CDLSTALLEDPATTERN(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSTALLEDPATTERN_Unguarded")]
    static extern int TA_CDLSTALLEDPATTERN_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSTICKSANDWICH")]
    static extern int TA_CDLSTICKSANDWICH(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLSTICKSANDWICH_Unguarded")]
    static extern int TA_CDLSTICKSANDWICH_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLTAKURI")]
    static extern int TA_CDLTAKURI(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLTAKURI_Unguarded")]
    static extern int TA_CDLTAKURI_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLTASUKIGAP")]
    static extern int TA_CDLTASUKIGAP(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLTASUKIGAP_Unguarded")]
    static extern int TA_CDLTASUKIGAP_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLTHRUSTING")]
    static extern int TA_CDLTHRUSTING(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLTHRUSTING_Unguarded")]
    static extern int TA_CDLTHRUSTING_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLTRISTAR")]
    static extern int TA_CDLTRISTAR(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLTRISTAR_Unguarded")]
    static extern int TA_CDLTRISTAR_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLUNIQUE3RIVER")]
    static extern int TA_CDLUNIQUE3RIVER(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLUNIQUE3RIVER_Unguarded")]
    static extern int TA_CDLUNIQUE3RIVER_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLUPSIDEGAP2CROWS")]
    static extern int TA_CDLUPSIDEGAP2CROWS(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLUPSIDEGAP2CROWS_Unguarded")]
    static extern int TA_CDLUPSIDEGAP2CROWS_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLXSIDEGAP3METHODS")]
    static extern int TA_CDLXSIDEGAP3METHODS(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CDLXSIDEGAP3METHODS_Unguarded")]
    static extern int TA_CDLXSIDEGAP3METHODS_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CEIL")]
    static extern int TA_CEIL(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CEIL_Unguarded")]
    static extern int TA_CEIL_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CMO")]
    static extern int TA_CMO(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CMO_Unguarded")]
    static extern int TA_CMO_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CORREL")]
    static extern int TA_CORREL(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_CORREL_Unguarded")]
    static extern int TA_CORREL_Unguarded(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_COS")]
    static extern int TA_COS(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_COS_Unguarded")]
    static extern int TA_COS_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_COSH")]
    static extern int TA_COSH(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_COSH_Unguarded")]
    static extern int TA_COSH_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_DEMA")]
    static extern int TA_DEMA(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_DEMA_Unguarded")]
    static extern int TA_DEMA_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_DIV")]
    static extern int TA_DIV(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_DIV_Unguarded")]
    static extern int TA_DIV_Unguarded(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_DX")]
    static extern int TA_DX(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_DX_Unguarded")]
    static extern int TA_DX_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_EMA")]
    static extern int TA_EMA(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_EMA_Unguarded")]
    static extern int TA_EMA_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_EXP")]
    static extern int TA_EXP(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_EXP_Unguarded")]
    static extern int TA_EXP_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_FLOOR")]
    static extern int TA_FLOOR(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_FLOOR_Unguarded")]
    static extern int TA_FLOOR_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_DCPERIOD")]
    static extern int TA_HT_DCPERIOD(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_DCPERIOD_Unguarded")]
    static extern int TA_HT_DCPERIOD_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_DCPHASE")]
    static extern int TA_HT_DCPHASE(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_DCPHASE_Unguarded")]
    static extern int TA_HT_DCPHASE_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_PHASOR")]
    static extern int TA_HT_PHASOR(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_PHASOR_Unguarded")]
    static extern int TA_HT_PHASOR_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_SINE")]
    static extern int TA_HT_SINE(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_SINE_Unguarded")]
    static extern int TA_HT_SINE_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_TRENDLINE")]
    static extern int TA_HT_TRENDLINE(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_TRENDLINE_Unguarded")]
    static extern int TA_HT_TRENDLINE_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_TRENDMODE")]
    static extern int TA_HT_TRENDMODE(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_HT_TRENDMODE_Unguarded")]
    static extern int TA_HT_TRENDMODE_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_IMI")]
    static extern int TA_IMI(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_IMI_Unguarded")]
    static extern int TA_IMI_Unguarded(
        int startIdx, int endIdx,
        double[] inOpen,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_KAMA")]
    static extern int TA_KAMA(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_KAMA_Unguarded")]
    static extern int TA_KAMA_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LINEARREG")]
    static extern int TA_LINEARREG(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LINEARREG_Unguarded")]
    static extern int TA_LINEARREG_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LINEARREG_ANGLE")]
    static extern int TA_LINEARREG_ANGLE(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LINEARREG_ANGLE_Unguarded")]
    static extern int TA_LINEARREG_ANGLE_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LINEARREG_INTERCEPT")]
    static extern int TA_LINEARREG_INTERCEPT(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LINEARREG_INTERCEPT_Unguarded")]
    static extern int TA_LINEARREG_INTERCEPT_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LINEARREG_SLOPE")]
    static extern int TA_LINEARREG_SLOPE(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LINEARREG_SLOPE_Unguarded")]
    static extern int TA_LINEARREG_SLOPE_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LN")]
    static extern int TA_LN(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LN_Unguarded")]
    static extern int TA_LN_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LOG10")]
    static extern int TA_LOG10(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_LOG10_Unguarded")]
    static extern int TA_LOG10_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MA")]
    static extern int TA_MA(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        int optInMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MA_Unguarded")]
    static extern int TA_MA_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        int optInMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MACD")]
    static extern int TA_MACD(
        int startIdx, int endIdx,
        double[] inReal,
        int optInFastPeriod,
        int optInSlowPeriod,
        int optInSignalPeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1,
        double[] outArr2);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MACD_Unguarded")]
    static extern int TA_MACD_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInFastPeriod,
        int optInSlowPeriod,
        int optInSignalPeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1,
        double[] outArr2);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MACDEXT")]
    static extern int TA_MACDEXT(
        int startIdx, int endIdx,
        double[] inReal,
        int optInFastPeriod,
        int optInFastMAType,
        int optInSlowPeriod,
        int optInSlowMAType,
        int optInSignalPeriod,
        int optInSignalMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1,
        double[] outArr2);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MACDEXT_Unguarded")]
    static extern int TA_MACDEXT_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInFastPeriod,
        int optInFastMAType,
        int optInSlowPeriod,
        int optInSlowMAType,
        int optInSignalPeriod,
        int optInSignalMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1,
        double[] outArr2);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MACDFIX")]
    static extern int TA_MACDFIX(
        int startIdx, int endIdx,
        double[] inReal,
        int optInSignalPeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1,
        double[] outArr2);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MACDFIX_Unguarded")]
    static extern int TA_MACDFIX_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInSignalPeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1,
        double[] outArr2);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MAMA")]
    static extern int TA_MAMA(
        int startIdx, int endIdx,
        double[] inReal,
        double optInFastLimit,
        double optInSlowLimit,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MAMA_Unguarded")]
    static extern int TA_MAMA_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        double optInFastLimit,
        double optInSlowLimit,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MAVP")]
    static extern int TA_MAVP(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        int optInMinPeriod,
        int optInMaxPeriod,
        int optInMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MAVP_Unguarded")]
    static extern int TA_MAVP_Unguarded(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        int optInMinPeriod,
        int optInMaxPeriod,
        int optInMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MAX")]
    static extern int TA_MAX(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MAX_Unguarded")]
    static extern int TA_MAX_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MAXINDEX")]
    static extern int TA_MAXINDEX(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MAXINDEX_Unguarded")]
    static extern int TA_MAXINDEX_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MEDPRICE")]
    static extern int TA_MEDPRICE(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MEDPRICE_Unguarded")]
    static extern int TA_MEDPRICE_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MFI")]
    static extern int TA_MFI(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double[] inVolume,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MFI_Unguarded")]
    static extern int TA_MFI_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        double[] inVolume,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MIDPOINT")]
    static extern int TA_MIDPOINT(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MIDPOINT_Unguarded")]
    static extern int TA_MIDPOINT_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MIDPRICE")]
    static extern int TA_MIDPRICE(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MIDPRICE_Unguarded")]
    static extern int TA_MIDPRICE_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MIN")]
    static extern int TA_MIN(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MIN_Unguarded")]
    static extern int TA_MIN_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MININDEX")]
    static extern int TA_MININDEX(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MININDEX_Unguarded")]
    static extern int TA_MININDEX_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        int[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MINMAX")]
    static extern int TA_MINMAX(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MINMAX_Unguarded")]
    static extern int TA_MINMAX_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MINMAXINDEX")]
    static extern int TA_MINMAXINDEX(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        int[] outArr0,
        int[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MINMAXINDEX_Unguarded")]
    static extern int TA_MINMAXINDEX_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        int[] outArr0,
        int[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MINUS_DI")]
    static extern int TA_MINUS_DI(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MINUS_DI_Unguarded")]
    static extern int TA_MINUS_DI_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MINUS_DM")]
    static extern int TA_MINUS_DM(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MINUS_DM_Unguarded")]
    static extern int TA_MINUS_DM_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MOM")]
    static extern int TA_MOM(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MOM_Unguarded")]
    static extern int TA_MOM_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MULT")]
    static extern int TA_MULT(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_MULT_Unguarded")]
    static extern int TA_MULT_Unguarded(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_NATR")]
    static extern int TA_NATR(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_NATR_Unguarded")]
    static extern int TA_NATR_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_OBV")]
    static extern int TA_OBV(
        int startIdx, int endIdx,
        double[] inReal,
        double[] inVolume,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_OBV_Unguarded")]
    static extern int TA_OBV_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        double[] inVolume,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_PLUS_DI")]
    static extern int TA_PLUS_DI(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_PLUS_DI_Unguarded")]
    static extern int TA_PLUS_DI_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_PLUS_DM")]
    static extern int TA_PLUS_DM(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_PLUS_DM_Unguarded")]
    static extern int TA_PLUS_DM_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_PPO")]
    static extern int TA_PPO(
        int startIdx, int endIdx,
        double[] inReal,
        int optInFastPeriod,
        int optInSlowPeriod,
        int optInMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_PPO_Unguarded")]
    static extern int TA_PPO_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInFastPeriod,
        int optInSlowPeriod,
        int optInMAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ROC")]
    static extern int TA_ROC(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ROC_Unguarded")]
    static extern int TA_ROC_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ROCP")]
    static extern int TA_ROCP(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ROCP_Unguarded")]
    static extern int TA_ROCP_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ROCR")]
    static extern int TA_ROCR(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ROCR_Unguarded")]
    static extern int TA_ROCR_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ROCR100")]
    static extern int TA_ROCR100(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ROCR100_Unguarded")]
    static extern int TA_ROCR100_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_RSI")]
    static extern int TA_RSI(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_RSI_Unguarded")]
    static extern int TA_RSI_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SAR")]
    static extern int TA_SAR(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double optInAcceleration,
        double optInMaximum,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SAR_Unguarded")]
    static extern int TA_SAR_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double optInAcceleration,
        double optInMaximum,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SAREXT")]
    static extern int TA_SAREXT(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double optInStartValue,
        double optInOffsetOnReverse,
        double optInAccelerationInitLong,
        double optInAccelerationLong,
        double optInAccelerationMaxLong,
        double optInAccelerationInitShort,
        double optInAccelerationShort,
        double optInAccelerationMaxShort,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SAREXT_Unguarded")]
    static extern int TA_SAREXT_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double optInStartValue,
        double optInOffsetOnReverse,
        double optInAccelerationInitLong,
        double optInAccelerationLong,
        double optInAccelerationMaxLong,
        double optInAccelerationInitShort,
        double optInAccelerationShort,
        double optInAccelerationMaxShort,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SIN")]
    static extern int TA_SIN(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SIN_Unguarded")]
    static extern int TA_SIN_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SINH")]
    static extern int TA_SINH(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SINH_Unguarded")]
    static extern int TA_SINH_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SMA")]
    static extern int TA_SMA(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SMA_Unguarded")]
    static extern int TA_SMA_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SQRT")]
    static extern int TA_SQRT(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SQRT_Unguarded")]
    static extern int TA_SQRT_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_STDDEV")]
    static extern int TA_STDDEV(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        double optInNbDev,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_STDDEV_Unguarded")]
    static extern int TA_STDDEV_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        double optInNbDev,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_STOCH")]
    static extern int TA_STOCH(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInFastK_Period,
        int optInSlowK_Period,
        int optInSlowK_MAType,
        int optInSlowD_Period,
        int optInSlowD_MAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_STOCH_Unguarded")]
    static extern int TA_STOCH_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInFastK_Period,
        int optInSlowK_Period,
        int optInSlowK_MAType,
        int optInSlowD_Period,
        int optInSlowD_MAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_STOCHF")]
    static extern int TA_STOCHF(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInFastK_Period,
        int optInFastD_Period,
        int optInFastD_MAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_STOCHF_Unguarded")]
    static extern int TA_STOCHF_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInFastK_Period,
        int optInFastD_Period,
        int optInFastD_MAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_STOCHRSI")]
    static extern int TA_STOCHRSI(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        int optInFastK_Period,
        int optInFastD_Period,
        int optInFastD_MAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_STOCHRSI_Unguarded")]
    static extern int TA_STOCHRSI_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        int optInFastK_Period,
        int optInFastD_Period,
        int optInFastD_MAType,
        out int outBegIdx, out int outNBElement,
        double[] outArr0,
        double[] outArr1);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SUB")]
    static extern int TA_SUB(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SUB_Unguarded")]
    static extern int TA_SUB_Unguarded(
        int startIdx, int endIdx,
        double[] inReal0,
        double[] inReal1,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SUM")]
    static extern int TA_SUM(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_SUM_Unguarded")]
    static extern int TA_SUM_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_T3")]
    static extern int TA_T3(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        double optInVFactor,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_T3_Unguarded")]
    static extern int TA_T3_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        double optInVFactor,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TAN")]
    static extern int TA_TAN(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TAN_Unguarded")]
    static extern int TA_TAN_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TANH")]
    static extern int TA_TANH(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TANH_Unguarded")]
    static extern int TA_TANH_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TEMA")]
    static extern int TA_TEMA(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TEMA_Unguarded")]
    static extern int TA_TEMA_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TRANGE")]
    static extern int TA_TRANGE(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TRANGE_Unguarded")]
    static extern int TA_TRANGE_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TRIMA")]
    static extern int TA_TRIMA(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TRIMA_Unguarded")]
    static extern int TA_TRIMA_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TRIX")]
    static extern int TA_TRIX(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TRIX_Unguarded")]
    static extern int TA_TRIX_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TSF")]
    static extern int TA_TSF(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TSF_Unguarded")]
    static extern int TA_TSF_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TYPPRICE")]
    static extern int TA_TYPPRICE(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_TYPPRICE_Unguarded")]
    static extern int TA_TYPPRICE_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ULTOSC")]
    static extern int TA_ULTOSC(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod1,
        int optInTimePeriod2,
        int optInTimePeriod3,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_ULTOSC_Unguarded")]
    static extern int TA_ULTOSC_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod1,
        int optInTimePeriod2,
        int optInTimePeriod3,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_VAR")]
    static extern int TA_VAR(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        double optInNbDev,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_VAR_Unguarded")]
    static extern int TA_VAR_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        double optInNbDev,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_WCLPRICE")]
    static extern int TA_WCLPRICE(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_WCLPRICE_Unguarded")]
    static extern int TA_WCLPRICE_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_WILLR")]
    static extern int TA_WILLR(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_WILLR_Unguarded")]
    static extern int TA_WILLR_Unguarded(
        int startIdx, int endIdx,
        double[] inHigh,
        double[] inLow,
        double[] inClose,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_WMA")]
    static extern int TA_WMA(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    [DllImport("ta_codegen_funcs", EntryPoint = "TA_WMA_Unguarded")]
    static extern int TA_WMA_Unguarded(
        int startIdx, int endIdx,
        double[] inReal,
        int optInTimePeriod,
        out int outBegIdx, out int outNBElement,
        double[] outArr0);

    static string HandleRequest(string json) {
        try {
            using var doc = JsonDocument.Parse(json);
            var root = doc.RootElement;
            string method = root.GetProperty("method").GetString()!;
            var p = root.GetProperty("params");

            if (method == "load_data") {
                double[] tmpOpen = GetDoubleArray(p, "open");
                refN = tmpOpen.Length;
                Array.Copy(tmpOpen, refOpen, refN);
                Array.Copy(GetDoubleArray(p, "high"), refHigh, refN);
                Array.Copy(GetDoubleArray(p, "low"), refLow, refN);
                Array.Copy(GetDoubleArray(p, "close"), refClose, refN);
                Array.Copy(GetDoubleArray(p, "volume"), refVolume, refN);
                Array.Copy(GetDoubleArray(p, "openInterest"), refOI, refN);
                return $"{{\"status\":\"ok\",\"n\":{refN}}}";
            }

            int startIdx = p.TryGetProperty("startIdx", out var _startIdxEl) ? _startIdxEl.GetInt32() : 0;
            int endIdx = p.TryGetProperty("endIdx", out var _endIdxEl) ? _endIdxEl.GetInt32() : 0;
            int n = endIdx - startIdx + 1;

            if (method == "TA_ACCBANDS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 20;
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                double[] outArr2 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ACCBANDS(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                        _h = SvHashF64(_h, outArr2, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ACCBANDS_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ACOS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ACOS(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ACOS_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_AD") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                double[] inVolume = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                    inVolume = new double[refN]; Array.Copy(refVolume, inVolume, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                    inVolume = GetDoubleArray(p, "inVolume");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_AD(startIdx, endIdx, inHigh, inLow, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_AD_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ADD") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal0 = Array.Empty<double>();
                double[] inReal1 = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
                    inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
                } else {
                    inReal0 = GetDoubleArray(p, "inReal0");
                    inReal1 = GetDoubleArray(p, "inReal1");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ADD(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ADD_Unguarded(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ADOSC") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                double[] inVolume = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                    inVolume = new double[refN]; Array.Copy(refVolume, inVolume, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                    inVolume = GetDoubleArray(p, "inVolume");
                }
                int optInFastPeriod = p.TryGetProperty("optInFastPeriod", out var _optInFastPeriodVal) ? _optInFastPeriodVal.GetInt32() : 3;
                int optInSlowPeriod = p.TryGetProperty("optInSlowPeriod", out var _optInSlowPeriodVal) ? _optInSlowPeriodVal.GetInt32() : 10;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ADOSC(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInFastPeriod, optInSlowPeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ADOSC_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInFastPeriod, optInSlowPeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ADX") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(0, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ADX(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ADX_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ADXR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(1, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ADXR(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ADXR_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_APO") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInFastPeriod = p.TryGetProperty("optInFastPeriod", out var _optInFastPeriodVal) ? _optInFastPeriodVal.GetInt32() : 12;
                int optInSlowPeriod = p.TryGetProperty("optInSlowPeriod", out var _optInSlowPeriodVal) ? _optInSlowPeriodVal.GetInt32() : 26;
                int optInMAType = p.TryGetProperty("optInMAType", out var _optInMATypeVal) ? _optInMATypeVal.GetInt32() : 0;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_APO(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_APO_Unguarded(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_AROON") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_AROON(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_AROON_Unguarded(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_AROONOSC") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_AROONOSC(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_AROONOSC_Unguarded(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ASIN") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ASIN(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ASIN_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ATAN") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ATAN(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ATAN_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ATR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(2, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ATR(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ATR_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_AVGDEV") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_AVGDEV(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_AVGDEV_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_AVGPRICE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_AVGPRICE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_AVGPRICE_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_BBANDS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 5;
                double optInNbDevUp = p.TryGetProperty("optInNbDevUp", out var _optInNbDevUpVal) ? _optInNbDevUpVal.GetDouble() : 2;
                double optInNbDevDn = p.TryGetProperty("optInNbDevDn", out var _optInNbDevDnVal) ? _optInNbDevDnVal.GetDouble() : 2;
                int optInMAType = p.TryGetProperty("optInMAType", out var _optInMATypeVal) ? _optInMATypeVal.GetInt32() : 0;
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                double[] outArr2 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_BBANDS(startIdx, endIdx, inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                        _h = SvHashF64(_h, outArr2, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_BBANDS_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_BETA") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal0 = Array.Empty<double>();
                double[] inReal1 = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
                    inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
                } else {
                    inReal0 = GetDoubleArray(p, "inReal0");
                    inReal1 = GetDoubleArray(p, "inReal1");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 5;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_BETA(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_BETA_Unguarded(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_BOP") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_BOP(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_BOP_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CCI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CCI(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CCI_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDL2CROWS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDL2CROWS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDL2CROWS_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDL3BLACKCROWS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDL3BLACKCROWS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDL3BLACKCROWS_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDL3INSIDE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDL3INSIDE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDL3INSIDE_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDL3LINESTRIKE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDL3LINESTRIKE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDL3LINESTRIKE_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDL3OUTSIDE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDL3OUTSIDE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDL3OUTSIDE_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDL3STARSINSOUTH") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDL3STARSINSOUTH(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDL3STARSINSOUTH_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDL3WHITESOLDIERS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDL3WHITESOLDIERS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDL3WHITESOLDIERS_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLABANDONEDBABY") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double optInPenetration = p.TryGetProperty("optInPenetration", out var _optInPenetrationVal) ? _optInPenetrationVal.GetDouble() : 0.3;
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLABANDONEDBABY(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLABANDONEDBABY_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLADVANCEBLOCK") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLADVANCEBLOCK(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLADVANCEBLOCK_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLBELTHOLD") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLBELTHOLD(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLBELTHOLD_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLBREAKAWAY") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLBREAKAWAY(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLBREAKAWAY_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLCLOSINGMARUBOZU") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLCLOSINGMARUBOZU(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLCLOSINGMARUBOZU_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLCONCEALBABYSWALL") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLCONCEALBABYSWALL(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLCONCEALBABYSWALL_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLCOUNTERATTACK") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLCOUNTERATTACK(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLCOUNTERATTACK_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLDARKCLOUDCOVER") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double optInPenetration = p.TryGetProperty("optInPenetration", out var _optInPenetrationVal) ? _optInPenetrationVal.GetDouble() : 0.5;
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLDARKCLOUDCOVER(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLDARKCLOUDCOVER_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLDOJI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLDOJI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLDOJI_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLDOJISTAR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLDOJISTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLDOJISTAR_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLDRAGONFLYDOJI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLDRAGONFLYDOJI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLDRAGONFLYDOJI_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLENGULFING") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLENGULFING(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLENGULFING_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLEVENINGDOJISTAR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double optInPenetration = p.TryGetProperty("optInPenetration", out var _optInPenetrationVal) ? _optInPenetrationVal.GetDouble() : 0.3;
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLEVENINGDOJISTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLEVENINGDOJISTAR_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLEVENINGSTAR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double optInPenetration = p.TryGetProperty("optInPenetration", out var _optInPenetrationVal) ? _optInPenetrationVal.GetDouble() : 0.3;
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLEVENINGSTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLEVENINGSTAR_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLGAPSIDESIDEWHITE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLGAPSIDESIDEWHITE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLGAPSIDESIDEWHITE_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLGRAVESTONEDOJI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLGRAVESTONEDOJI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLGRAVESTONEDOJI_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLHAMMER") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLHAMMER(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLHAMMER_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLHANGINGMAN") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLHANGINGMAN(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLHANGINGMAN_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLHARAMI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLHARAMI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLHARAMI_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLHARAMICROSS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLHARAMICROSS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLHARAMICROSS_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLHIGHWAVE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLHIGHWAVE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLHIGHWAVE_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLHIKKAKE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLHIKKAKE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLHIKKAKE_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLHIKKAKEMOD") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLHIKKAKEMOD(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLHIKKAKEMOD_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLHOMINGPIGEON") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLHOMINGPIGEON(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLHOMINGPIGEON_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLIDENTICAL3CROWS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLIDENTICAL3CROWS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLIDENTICAL3CROWS_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLINNECK") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLINNECK(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLINNECK_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLINVERTEDHAMMER") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLINVERTEDHAMMER(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLINVERTEDHAMMER_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLKICKING") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLKICKING(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLKICKING_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLKICKINGBYLENGTH") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLKICKINGBYLENGTH(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLKICKINGBYLENGTH_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLLADDERBOTTOM") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLLADDERBOTTOM(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLLADDERBOTTOM_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLLONGLEGGEDDOJI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLLONGLEGGEDDOJI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLLONGLEGGEDDOJI_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLLONGLINE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLLONGLINE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLLONGLINE_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLMARUBOZU") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLMARUBOZU(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLMARUBOZU_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLMATCHINGLOW") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLMATCHINGLOW(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLMATCHINGLOW_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLMATHOLD") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double optInPenetration = p.TryGetProperty("optInPenetration", out var _optInPenetrationVal) ? _optInPenetrationVal.GetDouble() : 0.5;
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLMATHOLD(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLMATHOLD_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLMORNINGDOJISTAR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double optInPenetration = p.TryGetProperty("optInPenetration", out var _optInPenetrationVal) ? _optInPenetrationVal.GetDouble() : 0.3;
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLMORNINGDOJISTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLMORNINGDOJISTAR_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLMORNINGSTAR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double optInPenetration = p.TryGetProperty("optInPenetration", out var _optInPenetrationVal) ? _optInPenetrationVal.GetDouble() : 0.3;
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLMORNINGSTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLMORNINGSTAR_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLONNECK") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLONNECK(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLONNECK_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLPIERCING") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLPIERCING(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLPIERCING_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLRICKSHAWMAN") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLRICKSHAWMAN(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLRICKSHAWMAN_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLRISEFALL3METHODS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLRISEFALL3METHODS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLRISEFALL3METHODS_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLSEPARATINGLINES") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLSEPARATINGLINES(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLSEPARATINGLINES_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLSHOOTINGSTAR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLSHOOTINGSTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLSHOOTINGSTAR_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLSHORTLINE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLSHORTLINE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLSHORTLINE_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLSPINNINGTOP") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLSPINNINGTOP(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLSPINNINGTOP_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLSTALLEDPATTERN") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLSTALLEDPATTERN(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLSTALLEDPATTERN_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLSTICKSANDWICH") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLSTICKSANDWICH(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLSTICKSANDWICH_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLTAKURI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLTAKURI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLTAKURI_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLTASUKIGAP") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLTASUKIGAP(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLTASUKIGAP_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLTHRUSTING") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLTHRUSTING(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLTHRUSTING_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLTRISTAR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLTRISTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLTRISTAR_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLUNIQUE3RIVER") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLUNIQUE3RIVER(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLUNIQUE3RIVER_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLUPSIDEGAP2CROWS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLUPSIDEGAP2CROWS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLUPSIDEGAP2CROWS_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CDLXSIDEGAP3METHODS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CDLXSIDEGAP3METHODS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CDLXSIDEGAP3METHODS_Unguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CEIL") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CEIL(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CEIL_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CMO") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(3, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CMO(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CMO_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_CORREL") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal0 = Array.Empty<double>();
                double[] inReal1 = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
                    inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
                } else {
                    inReal0 = GetDoubleArray(p, "inReal0");
                    inReal1 = GetDoubleArray(p, "inReal1");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_CORREL(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_CORREL_Unguarded(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_COS") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_COS(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_COS_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_COSH") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_COSH(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_COSH_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_DEMA") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_DEMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_DEMA_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_DIV") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal0 = Array.Empty<double>();
                double[] inReal1 = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
                    inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
                } else {
                    inReal0 = GetDoubleArray(p, "inReal0");
                    inReal1 = GetDoubleArray(p, "inReal1");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_DIV(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_DIV_Unguarded(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_DX") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(4, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_DX(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_DX_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_EMA") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(5, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_EMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_EMA_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_EXP") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_EXP(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_EXP_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_FLOOR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_FLOOR(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_FLOOR_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_HT_DCPERIOD") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(6, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_HT_DCPERIOD(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_HT_DCPERIOD_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_HT_DCPHASE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(7, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_HT_DCPHASE(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_HT_DCPHASE_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_HT_PHASOR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(8, unstablePeriod);
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_HT_PHASOR(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_HT_PHASOR_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_HT_SINE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(9, unstablePeriod);
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_HT_SINE(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_HT_SINE_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_HT_TRENDLINE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(10, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_HT_TRENDLINE(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_HT_TRENDLINE_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_HT_TRENDMODE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(11, unstablePeriod);
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_HT_TRENDMODE(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_HT_TRENDMODE_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_IMI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inOpen = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inOpen = GetDoubleArray(p, "inOpen");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_IMI(startIdx, endIdx, inOpen, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_IMI_Unguarded(startIdx, endIdx, inOpen, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_KAMA") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(13, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_KAMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_KAMA_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_LINEARREG") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_LINEARREG(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_LINEARREG_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_LINEARREG_ANGLE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_LINEARREG_ANGLE(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_LINEARREG_ANGLE_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_LINEARREG_INTERCEPT") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_LINEARREG_INTERCEPT(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_LINEARREG_INTERCEPT_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_LINEARREG_SLOPE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_LINEARREG_SLOPE(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_LINEARREG_SLOPE_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_LN") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_LN(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_LN_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_LOG10") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_LOG10(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_LOG10_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MA") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                int optInMAType = p.TryGetProperty("optInMAType", out var _optInMATypeVal) ? _optInMATypeVal.GetInt32() : 0;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MA(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MA_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MACD") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInFastPeriod = p.TryGetProperty("optInFastPeriod", out var _optInFastPeriodVal) ? _optInFastPeriodVal.GetInt32() : 12;
                int optInSlowPeriod = p.TryGetProperty("optInSlowPeriod", out var _optInSlowPeriodVal) ? _optInSlowPeriodVal.GetInt32() : 26;
                int optInSignalPeriod = p.TryGetProperty("optInSignalPeriod", out var _optInSignalPeriodVal) ? _optInSignalPeriodVal.GetInt32() : 9;
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                double[] outArr2 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MACD(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                        _h = SvHashF64(_h, outArr2, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MACD_Unguarded(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MACDEXT") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInFastPeriod = p.TryGetProperty("optInFastPeriod", out var _optInFastPeriodVal) ? _optInFastPeriodVal.GetInt32() : 12;
                int optInFastMAType = p.TryGetProperty("optInFastMAType", out var _optInFastMATypeVal) ? _optInFastMATypeVal.GetInt32() : 0;
                int optInSlowPeriod = p.TryGetProperty("optInSlowPeriod", out var _optInSlowPeriodVal) ? _optInSlowPeriodVal.GetInt32() : 26;
                int optInSlowMAType = p.TryGetProperty("optInSlowMAType", out var _optInSlowMATypeVal) ? _optInSlowMATypeVal.GetInt32() : 0;
                int optInSignalPeriod = p.TryGetProperty("optInSignalPeriod", out var _optInSignalPeriodVal) ? _optInSignalPeriodVal.GetInt32() : 9;
                int optInSignalMAType = p.TryGetProperty("optInSignalMAType", out var _optInSignalMATypeVal) ? _optInSignalMATypeVal.GetInt32() : 0;
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                double[] outArr2 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MACDEXT(startIdx, endIdx, inReal, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                        _h = SvHashF64(_h, outArr2, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MACDEXT_Unguarded(startIdx, endIdx, inReal, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MACDFIX") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInSignalPeriod = p.TryGetProperty("optInSignalPeriod", out var _optInSignalPeriodVal) ? _optInSignalPeriodVal.GetInt32() : 9;
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                double[] outArr2 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MACDFIX(startIdx, endIdx, inReal, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                        _h = SvHashF64(_h, outArr2, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MACDFIX_Unguarded(startIdx, endIdx, inReal, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MAMA") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double optInFastLimit = p.TryGetProperty("optInFastLimit", out var _optInFastLimitVal) ? _optInFastLimitVal.GetDouble() : 0.5;
                double optInSlowLimit = p.TryGetProperty("optInSlowLimit", out var _optInSlowLimitVal) ? _optInSlowLimitVal.GetDouble() : 0.05;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(14, unstablePeriod);
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MAMA(startIdx, endIdx, inReal, optInFastLimit, optInSlowLimit, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MAMA_Unguarded(startIdx, endIdx, inReal, optInFastLimit, optInSlowLimit, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MAVP") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal0 = Array.Empty<double>();
                double[] inReal1 = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
                    inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
                } else {
                    inReal0 = GetDoubleArray(p, "inReal0");
                    inReal1 = GetDoubleArray(p, "inReal1");
                }
                int optInMinPeriod = p.TryGetProperty("optInMinPeriod", out var _optInMinPeriodVal) ? _optInMinPeriodVal.GetInt32() : 2;
                int optInMaxPeriod = p.TryGetProperty("optInMaxPeriod", out var _optInMaxPeriodVal) ? _optInMaxPeriodVal.GetInt32() : 30;
                int optInMAType = p.TryGetProperty("optInMAType", out var _optInMATypeVal) ? _optInMATypeVal.GetInt32() : 0;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MAVP(startIdx, endIdx, inReal0, inReal1, optInMinPeriod, optInMaxPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MAVP_Unguarded(startIdx, endIdx, inReal0, inReal1, optInMinPeriod, optInMaxPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MAX") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MAX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MAX_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MAXINDEX") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MAXINDEX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MAXINDEX_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MEDPRICE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MEDPRICE(startIdx, endIdx, inHigh, inLow, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MEDPRICE_Unguarded(startIdx, endIdx, inHigh, inLow, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MFI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                double[] inVolume = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                    inVolume = new double[refN]; Array.Copy(refVolume, inVolume, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                    inVolume = GetDoubleArray(p, "inVolume");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MFI(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MFI_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MIDPOINT") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MIDPOINT(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MIDPOINT_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MIDPRICE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MIDPRICE(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MIDPRICE_Unguarded(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MIN") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MIN(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MIN_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MININDEX") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                int[] outArr0 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MININDEX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MININDEX_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MINMAX") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MINMAX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MINMAX_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MINMAXINDEX") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                int[] outArr0 = new int[n];
                int[] outArr1 = new int[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MINMAXINDEX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashI32(_h, outArr0, outNBElement);
                        _h = SvHashI32(_h, outArr1, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MINMAXINDEX_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
                sb.Append($",\"outInteger1\":"); sb.Append(FormatIntArray(outArr1, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MINUS_DI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(16, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MINUS_DI(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MINUS_DI_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MINUS_DM") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(17, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MINUS_DM(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MINUS_DM_Unguarded(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MOM") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 10;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MOM(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MOM_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_MULT") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal0 = Array.Empty<double>();
                double[] inReal1 = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
                    inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
                } else {
                    inReal0 = GetDoubleArray(p, "inReal0");
                    inReal1 = GetDoubleArray(p, "inReal1");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_MULT(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_MULT_Unguarded(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_NATR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(18, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_NATR(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_NATR_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_OBV") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                double[] inVolume = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                    inVolume = new double[refN]; Array.Copy(refVolume, inVolume, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                    inVolume = GetDoubleArray(p, "inVolume");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_OBV(startIdx, endIdx, inReal, inVolume, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_OBV_Unguarded(startIdx, endIdx, inReal, inVolume, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_PLUS_DI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(19, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_PLUS_DI(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_PLUS_DI_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_PLUS_DM") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(20, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_PLUS_DM(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_PLUS_DM_Unguarded(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_PPO") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInFastPeriod = p.TryGetProperty("optInFastPeriod", out var _optInFastPeriodVal) ? _optInFastPeriodVal.GetInt32() : 12;
                int optInSlowPeriod = p.TryGetProperty("optInSlowPeriod", out var _optInSlowPeriodVal) ? _optInSlowPeriodVal.GetInt32() : 26;
                int optInMAType = p.TryGetProperty("optInMAType", out var _optInMATypeVal) ? _optInMATypeVal.GetInt32() : 0;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_PPO(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_PPO_Unguarded(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ROC") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 10;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ROC(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ROC_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ROCP") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 10;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ROCP(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ROCP_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ROCR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 10;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ROCR(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ROCR_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ROCR100") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 10;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ROCR100(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ROCR100_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_RSI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(21, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_RSI(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_RSI_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_SAR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                }
                double optInAcceleration = p.TryGetProperty("optInAcceleration", out var _optInAccelerationVal) ? _optInAccelerationVal.GetDouble() : 0.02;
                double optInMaximum = p.TryGetProperty("optInMaximum", out var _optInMaximumVal) ? _optInMaximumVal.GetDouble() : 0.2;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_SAR(startIdx, endIdx, inHigh, inLow, optInAcceleration, optInMaximum, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_SAR_Unguarded(startIdx, endIdx, inHigh, inLow, optInAcceleration, optInMaximum, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_SAREXT") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                }
                double optInStartValue = p.TryGetProperty("optInStartValue", out var _optInStartValueVal) ? _optInStartValueVal.GetDouble() : 0;
                double optInOffsetOnReverse = p.TryGetProperty("optInOffsetOnReverse", out var _optInOffsetOnReverseVal) ? _optInOffsetOnReverseVal.GetDouble() : 0;
                double optInAccelerationInitLong = p.TryGetProperty("optInAccelerationInitLong", out var _optInAccelerationInitLongVal) ? _optInAccelerationInitLongVal.GetDouble() : 0.02;
                double optInAccelerationLong = p.TryGetProperty("optInAccelerationLong", out var _optInAccelerationLongVal) ? _optInAccelerationLongVal.GetDouble() : 0.02;
                double optInAccelerationMaxLong = p.TryGetProperty("optInAccelerationMaxLong", out var _optInAccelerationMaxLongVal) ? _optInAccelerationMaxLongVal.GetDouble() : 0.2;
                double optInAccelerationInitShort = p.TryGetProperty("optInAccelerationInitShort", out var _optInAccelerationInitShortVal) ? _optInAccelerationInitShortVal.GetDouble() : 0.02;
                double optInAccelerationShort = p.TryGetProperty("optInAccelerationShort", out var _optInAccelerationShortVal) ? _optInAccelerationShortVal.GetDouble() : 0.02;
                double optInAccelerationMaxShort = p.TryGetProperty("optInAccelerationMaxShort", out var _optInAccelerationMaxShortVal) ? _optInAccelerationMaxShortVal.GetDouble() : 0.2;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_SAREXT(startIdx, endIdx, inHigh, inLow, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_SAREXT_Unguarded(startIdx, endIdx, inHigh, inLow, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_SIN") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_SIN(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_SIN_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_SINH") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_SINH(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_SINH_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_SMA") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_SMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_SMA_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_SQRT") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_SQRT(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_SQRT_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_STDDEV") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 5;
                double optInNbDev = p.TryGetProperty("optInNbDev", out var _optInNbDevVal) ? _optInNbDevVal.GetDouble() : 1;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_STDDEV(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_STDDEV_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_STOCH") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInFastK_Period = p.TryGetProperty("optInFastK_Period", out var _optInFastK_PeriodVal) ? _optInFastK_PeriodVal.GetInt32() : 5;
                int optInSlowK_Period = p.TryGetProperty("optInSlowK_Period", out var _optInSlowK_PeriodVal) ? _optInSlowK_PeriodVal.GetInt32() : 3;
                int optInSlowK_MAType = p.TryGetProperty("optInSlowK_MAType", out var _optInSlowK_MATypeVal) ? _optInSlowK_MATypeVal.GetInt32() : 0;
                int optInSlowD_Period = p.TryGetProperty("optInSlowD_Period", out var _optInSlowD_PeriodVal) ? _optInSlowD_PeriodVal.GetInt32() : 3;
                int optInSlowD_MAType = p.TryGetProperty("optInSlowD_MAType", out var _optInSlowD_MATypeVal) ? _optInSlowD_MATypeVal.GetInt32() : 0;
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_STOCH(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_STOCH_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_STOCHF") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInFastK_Period = p.TryGetProperty("optInFastK_Period", out var _optInFastK_PeriodVal) ? _optInFastK_PeriodVal.GetInt32() : 5;
                int optInFastD_Period = p.TryGetProperty("optInFastD_Period", out var _optInFastD_PeriodVal) ? _optInFastD_PeriodVal.GetInt32() : 3;
                int optInFastD_MAType = p.TryGetProperty("optInFastD_MAType", out var _optInFastD_MATypeVal) ? _optInFastD_MATypeVal.GetInt32() : 0;
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_STOCHF(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_STOCHF_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_STOCHRSI") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                int optInFastK_Period = p.TryGetProperty("optInFastK_Period", out var _optInFastK_PeriodVal) ? _optInFastK_PeriodVal.GetInt32() : 5;
                int optInFastD_Period = p.TryGetProperty("optInFastD_Period", out var _optInFastD_PeriodVal) ? _optInFastD_PeriodVal.GetInt32() : 3;
                int optInFastD_MAType = p.TryGetProperty("optInFastD_MAType", out var _optInFastD_MATypeVal) ? _optInFastD_MATypeVal.GetInt32() : 0;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(22, unstablePeriod);
                double[] outArr0 = new double[n];
                double[] outArr1 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_STOCHRSI(startIdx, endIdx, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                        _h = SvHashF64(_h, outArr1, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_STOCHRSI_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_SUB") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal0 = Array.Empty<double>();
                double[] inReal1 = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
                    inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
                } else {
                    inReal0 = GetDoubleArray(p, "inReal0");
                    inReal1 = GetDoubleArray(p, "inReal1");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_SUB(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_SUB_Unguarded(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_SUM") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_SUM(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_SUM_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_T3") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 5;
                double optInVFactor = p.TryGetProperty("optInVFactor", out var _optInVFactorVal) ? _optInVFactorVal.GetDouble() : 0.7;
                int unstablePeriod = p.TryGetProperty("unstablePeriod", out var _upVal) ? _upVal.GetInt32() : 0;
                TA_SetUnstablePeriod(23, unstablePeriod);
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_T3(startIdx, endIdx, inReal, optInTimePeriod, optInVFactor, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_T3_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, optInVFactor, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_TAN") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_TAN(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_TAN_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_TANH") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_TANH(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_TANH_Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_TEMA") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_TEMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_TEMA_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_TRANGE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_TRANGE(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_TRANGE_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_TRIMA") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_TRIMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_TRIMA_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_TRIX") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_TRIX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_TRIX_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_TSF") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_TSF(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_TSF_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_TYPPRICE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_TYPPRICE(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_TYPPRICE_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_ULTOSC") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod1 = p.TryGetProperty("optInTimePeriod1", out var _optInTimePeriod1Val) ? _optInTimePeriod1Val.GetInt32() : 7;
                int optInTimePeriod2 = p.TryGetProperty("optInTimePeriod2", out var _optInTimePeriod2Val) ? _optInTimePeriod2Val.GetInt32() : 14;
                int optInTimePeriod3 = p.TryGetProperty("optInTimePeriod3", out var _optInTimePeriod3Val) ? _optInTimePeriod3Val.GetInt32() : 28;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_ULTOSC(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_ULTOSC_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_VAR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 5;
                double optInNbDev = p.TryGetProperty("optInNbDev", out var _optInNbDevVal) ? _optInNbDevVal.GetDouble() : 1;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_VAR(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_VAR_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_WCLPRICE") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_WCLPRICE(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_WCLPRICE_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_WILLR") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inHigh = Array.Empty<double>();
                double[] inLow = Array.Empty<double>();
                double[] inClose = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
                    inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
                    inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
                } else {
                    inHigh = GetDoubleArray(p, "inHigh");
                    inLow = GetDoubleArray(p, "inLow");
                    inClose = GetDoubleArray(p, "inClose");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 14;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_WILLR(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_WILLR_Unguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "TA_WMA") {
                int use_preloaded = p.TryGetProperty("use_preloaded", out var _upre) ? _upre.GetInt32() : 0;
                int bench_iters = p.TryGetProperty("iters", out var _iters) ? _iters.GetInt32() : 1;
                if (bench_iters < 1) bench_iters = 1;
                double[] inReal = Array.Empty<double>();
                if (use_preloaded != 0 && refN > 0) {
                    inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
                } else {
                    inReal = GetDoubleArray(p, "inReal");
                }
                int optInTimePeriod = p.TryGetProperty("optInTimePeriod", out var _optInTimePeriodVal) ? _optInTimePeriodVal.GetInt32() : 30;
                double[] outArr0 = new double[n];
                int rc = 0;
                int outBegIdx = 0, outNBElement = 0;
                long _t0 = GetNanoTime();
                for (int _bi = 0; _bi < bench_iters; _bi++) {
                rc = TA_WMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
                if ((p.TryGetProperty("want_hash", out var _wh) ? _wh.GetInt32() : 0) != 0 &&
                    (p.TryGetProperty("full_output", out var _fo) ? _fo.GetInt32() : 0) == 0) {
                    ulong _h = SvHashInit();
                    if (rc == 0 && outNBElement > 0) {
                        _h = SvHashF64(_h, outArr0, outNBElement);
                    }
                    _h = SvHashFin(_h);
                    return $"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
                }
                long _t0u = GetNanoTime();
                for (int _biu = 0; _biu < bench_iters; _biu++) {
                rc = TA_WMA_Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
                }
                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
                var sb = new System.Text.StringBuilder();
                sb.Append($"{{\"retCode\":{rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
                sb.Append($",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
                sb.Append($",\"timing_ns\":{elapsedNs}");
                sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
                sb.Append("}");
                return sb.ToString();
            }
            else if (method == "list_functions") {
                var sb = new System.Text.StringBuilder("{\"functions\":[");
                sb.Append("\"TA_ACCBANDS\"");
                sb.Append(",");
                sb.Append("\"TA_ACOS\"");
                sb.Append(",");
                sb.Append("\"TA_AD\"");
                sb.Append(",");
                sb.Append("\"TA_ADD\"");
                sb.Append(",");
                sb.Append("\"TA_ADOSC\"");
                sb.Append(",");
                sb.Append("\"TA_ADX\"");
                sb.Append(",");
                sb.Append("\"TA_ADXR\"");
                sb.Append(",");
                sb.Append("\"TA_APO\"");
                sb.Append(",");
                sb.Append("\"TA_AROON\"");
                sb.Append(",");
                sb.Append("\"TA_AROONOSC\"");
                sb.Append(",");
                sb.Append("\"TA_ASIN\"");
                sb.Append(",");
                sb.Append("\"TA_ATAN\"");
                sb.Append(",");
                sb.Append("\"TA_ATR\"");
                sb.Append(",");
                sb.Append("\"TA_AVGDEV\"");
                sb.Append(",");
                sb.Append("\"TA_AVGPRICE\"");
                sb.Append(",");
                sb.Append("\"TA_BBANDS\"");
                sb.Append(",");
                sb.Append("\"TA_BETA\"");
                sb.Append(",");
                sb.Append("\"TA_BOP\"");
                sb.Append(",");
                sb.Append("\"TA_CCI\"");
                sb.Append(",");
                sb.Append("\"TA_CDL2CROWS\"");
                sb.Append(",");
                sb.Append("\"TA_CDL3BLACKCROWS\"");
                sb.Append(",");
                sb.Append("\"TA_CDL3INSIDE\"");
                sb.Append(",");
                sb.Append("\"TA_CDL3LINESTRIKE\"");
                sb.Append(",");
                sb.Append("\"TA_CDL3OUTSIDE\"");
                sb.Append(",");
                sb.Append("\"TA_CDL3STARSINSOUTH\"");
                sb.Append(",");
                sb.Append("\"TA_CDL3WHITESOLDIERS\"");
                sb.Append(",");
                sb.Append("\"TA_CDLABANDONEDBABY\"");
                sb.Append(",");
                sb.Append("\"TA_CDLADVANCEBLOCK\"");
                sb.Append(",");
                sb.Append("\"TA_CDLBELTHOLD\"");
                sb.Append(",");
                sb.Append("\"TA_CDLBREAKAWAY\"");
                sb.Append(",");
                sb.Append("\"TA_CDLCLOSINGMARUBOZU\"");
                sb.Append(",");
                sb.Append("\"TA_CDLCONCEALBABYSWALL\"");
                sb.Append(",");
                sb.Append("\"TA_CDLCOUNTERATTACK\"");
                sb.Append(",");
                sb.Append("\"TA_CDLDARKCLOUDCOVER\"");
                sb.Append(",");
                sb.Append("\"TA_CDLDOJI\"");
                sb.Append(",");
                sb.Append("\"TA_CDLDOJISTAR\"");
                sb.Append(",");
                sb.Append("\"TA_CDLDRAGONFLYDOJI\"");
                sb.Append(",");
                sb.Append("\"TA_CDLENGULFING\"");
                sb.Append(",");
                sb.Append("\"TA_CDLEVENINGDOJISTAR\"");
                sb.Append(",");
                sb.Append("\"TA_CDLEVENINGSTAR\"");
                sb.Append(",");
                sb.Append("\"TA_CDLGAPSIDESIDEWHITE\"");
                sb.Append(",");
                sb.Append("\"TA_CDLGRAVESTONEDOJI\"");
                sb.Append(",");
                sb.Append("\"TA_CDLHAMMER\"");
                sb.Append(",");
                sb.Append("\"TA_CDLHANGINGMAN\"");
                sb.Append(",");
                sb.Append("\"TA_CDLHARAMI\"");
                sb.Append(",");
                sb.Append("\"TA_CDLHARAMICROSS\"");
                sb.Append(",");
                sb.Append("\"TA_CDLHIGHWAVE\"");
                sb.Append(",");
                sb.Append("\"TA_CDLHIKKAKE\"");
                sb.Append(",");
                sb.Append("\"TA_CDLHIKKAKEMOD\"");
                sb.Append(",");
                sb.Append("\"TA_CDLHOMINGPIGEON\"");
                sb.Append(",");
                sb.Append("\"TA_CDLIDENTICAL3CROWS\"");
                sb.Append(",");
                sb.Append("\"TA_CDLINNECK\"");
                sb.Append(",");
                sb.Append("\"TA_CDLINVERTEDHAMMER\"");
                sb.Append(",");
                sb.Append("\"TA_CDLKICKING\"");
                sb.Append(",");
                sb.Append("\"TA_CDLKICKINGBYLENGTH\"");
                sb.Append(",");
                sb.Append("\"TA_CDLLADDERBOTTOM\"");
                sb.Append(",");
                sb.Append("\"TA_CDLLONGLEGGEDDOJI\"");
                sb.Append(",");
                sb.Append("\"TA_CDLLONGLINE\"");
                sb.Append(",");
                sb.Append("\"TA_CDLMARUBOZU\"");
                sb.Append(",");
                sb.Append("\"TA_CDLMATCHINGLOW\"");
                sb.Append(",");
                sb.Append("\"TA_CDLMATHOLD\"");
                sb.Append(",");
                sb.Append("\"TA_CDLMORNINGDOJISTAR\"");
                sb.Append(",");
                sb.Append("\"TA_CDLMORNINGSTAR\"");
                sb.Append(",");
                sb.Append("\"TA_CDLONNECK\"");
                sb.Append(",");
                sb.Append("\"TA_CDLPIERCING\"");
                sb.Append(",");
                sb.Append("\"TA_CDLRICKSHAWMAN\"");
                sb.Append(",");
                sb.Append("\"TA_CDLRISEFALL3METHODS\"");
                sb.Append(",");
                sb.Append("\"TA_CDLSEPARATINGLINES\"");
                sb.Append(",");
                sb.Append("\"TA_CDLSHOOTINGSTAR\"");
                sb.Append(",");
                sb.Append("\"TA_CDLSHORTLINE\"");
                sb.Append(",");
                sb.Append("\"TA_CDLSPINNINGTOP\"");
                sb.Append(",");
                sb.Append("\"TA_CDLSTALLEDPATTERN\"");
                sb.Append(",");
                sb.Append("\"TA_CDLSTICKSANDWICH\"");
                sb.Append(",");
                sb.Append("\"TA_CDLTAKURI\"");
                sb.Append(",");
                sb.Append("\"TA_CDLTASUKIGAP\"");
                sb.Append(",");
                sb.Append("\"TA_CDLTHRUSTING\"");
                sb.Append(",");
                sb.Append("\"TA_CDLTRISTAR\"");
                sb.Append(",");
                sb.Append("\"TA_CDLUNIQUE3RIVER\"");
                sb.Append(",");
                sb.Append("\"TA_CDLUPSIDEGAP2CROWS\"");
                sb.Append(",");
                sb.Append("\"TA_CDLXSIDEGAP3METHODS\"");
                sb.Append(",");
                sb.Append("\"TA_CEIL\"");
                sb.Append(",");
                sb.Append("\"TA_CMO\"");
                sb.Append(",");
                sb.Append("\"TA_CORREL\"");
                sb.Append(",");
                sb.Append("\"TA_COS\"");
                sb.Append(",");
                sb.Append("\"TA_COSH\"");
                sb.Append(",");
                sb.Append("\"TA_DEMA\"");
                sb.Append(",");
                sb.Append("\"TA_DIV\"");
                sb.Append(",");
                sb.Append("\"TA_DX\"");
                sb.Append(",");
                sb.Append("\"TA_EMA\"");
                sb.Append(",");
                sb.Append("\"TA_EXP\"");
                sb.Append(",");
                sb.Append("\"TA_FLOOR\"");
                sb.Append(",");
                sb.Append("\"TA_HT_DCPERIOD\"");
                sb.Append(",");
                sb.Append("\"TA_HT_DCPHASE\"");
                sb.Append(",");
                sb.Append("\"TA_HT_PHASOR\"");
                sb.Append(",");
                sb.Append("\"TA_HT_SINE\"");
                sb.Append(",");
                sb.Append("\"TA_HT_TRENDLINE\"");
                sb.Append(",");
                sb.Append("\"TA_HT_TRENDMODE\"");
                sb.Append(",");
                sb.Append("\"TA_IMI\"");
                sb.Append(",");
                sb.Append("\"TA_KAMA\"");
                sb.Append(",");
                sb.Append("\"TA_LINEARREG\"");
                sb.Append(",");
                sb.Append("\"TA_LINEARREG_ANGLE\"");
                sb.Append(",");
                sb.Append("\"TA_LINEARREG_INTERCEPT\"");
                sb.Append(",");
                sb.Append("\"TA_LINEARREG_SLOPE\"");
                sb.Append(",");
                sb.Append("\"TA_LN\"");
                sb.Append(",");
                sb.Append("\"TA_LOG10\"");
                sb.Append(",");
                sb.Append("\"TA_MA\"");
                sb.Append(",");
                sb.Append("\"TA_MACD\"");
                sb.Append(",");
                sb.Append("\"TA_MACDEXT\"");
                sb.Append(",");
                sb.Append("\"TA_MACDFIX\"");
                sb.Append(",");
                sb.Append("\"TA_MAMA\"");
                sb.Append(",");
                sb.Append("\"TA_MAVP\"");
                sb.Append(",");
                sb.Append("\"TA_MAX\"");
                sb.Append(",");
                sb.Append("\"TA_MAXINDEX\"");
                sb.Append(",");
                sb.Append("\"TA_MEDPRICE\"");
                sb.Append(",");
                sb.Append("\"TA_MFI\"");
                sb.Append(",");
                sb.Append("\"TA_MIDPOINT\"");
                sb.Append(",");
                sb.Append("\"TA_MIDPRICE\"");
                sb.Append(",");
                sb.Append("\"TA_MIN\"");
                sb.Append(",");
                sb.Append("\"TA_MININDEX\"");
                sb.Append(",");
                sb.Append("\"TA_MINMAX\"");
                sb.Append(",");
                sb.Append("\"TA_MINMAXINDEX\"");
                sb.Append(",");
                sb.Append("\"TA_MINUS_DI\"");
                sb.Append(",");
                sb.Append("\"TA_MINUS_DM\"");
                sb.Append(",");
                sb.Append("\"TA_MOM\"");
                sb.Append(",");
                sb.Append("\"TA_MULT\"");
                sb.Append(",");
                sb.Append("\"TA_NATR\"");
                sb.Append(",");
                sb.Append("\"TA_OBV\"");
                sb.Append(",");
                sb.Append("\"TA_PLUS_DI\"");
                sb.Append(",");
                sb.Append("\"TA_PLUS_DM\"");
                sb.Append(",");
                sb.Append("\"TA_PPO\"");
                sb.Append(",");
                sb.Append("\"TA_ROC\"");
                sb.Append(",");
                sb.Append("\"TA_ROCP\"");
                sb.Append(",");
                sb.Append("\"TA_ROCR\"");
                sb.Append(",");
                sb.Append("\"TA_ROCR100\"");
                sb.Append(",");
                sb.Append("\"TA_RSI\"");
                sb.Append(",");
                sb.Append("\"TA_SAR\"");
                sb.Append(",");
                sb.Append("\"TA_SAREXT\"");
                sb.Append(",");
                sb.Append("\"TA_SIN\"");
                sb.Append(",");
                sb.Append("\"TA_SINH\"");
                sb.Append(",");
                sb.Append("\"TA_SMA\"");
                sb.Append(",");
                sb.Append("\"TA_SQRT\"");
                sb.Append(",");
                sb.Append("\"TA_STDDEV\"");
                sb.Append(",");
                sb.Append("\"TA_STOCH\"");
                sb.Append(",");
                sb.Append("\"TA_STOCHF\"");
                sb.Append(",");
                sb.Append("\"TA_STOCHRSI\"");
                sb.Append(",");
                sb.Append("\"TA_SUB\"");
                sb.Append(",");
                sb.Append("\"TA_SUM\"");
                sb.Append(",");
                sb.Append("\"TA_T3\"");
                sb.Append(",");
                sb.Append("\"TA_TAN\"");
                sb.Append(",");
                sb.Append("\"TA_TANH\"");
                sb.Append(",");
                sb.Append("\"TA_TEMA\"");
                sb.Append(",");
                sb.Append("\"TA_TRANGE\"");
                sb.Append(",");
                sb.Append("\"TA_TRIMA\"");
                sb.Append(",");
                sb.Append("\"TA_TRIX\"");
                sb.Append(",");
                sb.Append("\"TA_TSF\"");
                sb.Append(",");
                sb.Append("\"TA_TYPPRICE\"");
                sb.Append(",");
                sb.Append("\"TA_ULTOSC\"");
                sb.Append(",");
                sb.Append("\"TA_VAR\"");
                sb.Append(",");
                sb.Append("\"TA_WCLPRICE\"");
                sb.Append(",");
                sb.Append("\"TA_WILLR\"");
                sb.Append(",");
                sb.Append("\"TA_WMA\"");
                sb.Append("]}");
                return sb.ToString();
            }
            else if (method == "set_unstable_period") {
                int id = p.GetProperty("id").GetInt32();
                int period = p.GetProperty("period").GetInt32();
                TA_SetUnstablePeriod(id, period);
                return "{\"status\":\"ok\"}";
            }
            else if (method == "set_compatibility") {
                int mode = p.GetProperty("mode").GetInt32();
                TA_SetCompatibility(mode);
                return "{\"status\":\"ok\"}";
            }
            else {
                return $"{{\"error\":\"Unknown method: {method}\"}}";
            }
        } catch (Exception ex) {
            return $"{{\"error\":\"{ex.Message.Replace("\"", "'")}\"}}";
        }
    }

    static double[] GetDoubleArray(JsonElement p, string name) {
        var arr = p.GetProperty(name);
        if (arr.ValueKind == JsonValueKind.String) {
            string hex = arr.GetString()!;
            int cnt = hex.Length / 16;
            double[] r = new double[cnt];
            for (int i = 0; i < cnt; i++)
                r[i] = BitConverter.Int64BitsToDouble(unchecked((long)Convert.ToUInt64(hex.Substring(i * 16, 16), 16)));
            return r;
        }
        double[] result = new double[arr.GetArrayLength()];
        for (int i = 0; i < result.Length; i++)
            result[i] = arr[i].GetDouble();
        return result;
    }

    static ulong SvHashInit() => 1469598103934665603UL;
    static ulong SvHashF64(ulong h, double[] a, int n) {
        for (int i = 0; i < n; i++) {
            long bits = BitConverter.DoubleToInt64Bits(a[i]);
            for (int b = 0; b < 8; b++) { h ^= (ulong)((bits >> (8 * b)) & 0xffL); h *= 1099511628211UL; }
        }
        return h;
    }
    static ulong SvHashI32(ulong h, int[] a, int n) {
        for (int i = 0; i < n; i++) {
            int bits = a[i];
            for (int b = 0; b < 4; b++) { h ^= (ulong)((bits >> (8 * b)) & 0xff); h *= 1099511628211UL; }
        }
        return h;
    }
    static ulong SvHashFin(ulong h) {
        h ^= h >> 33; h *= 0xFF51AFD7ED558CCDUL;
        h ^= h >> 33; h *= 0xC4CEB9FE1A85EC53UL;
        h ^= h >> 33; return h;
    }

    static string FormatArray(double[] arr, int count) {
        var parts = new string[count];
        for (int i = 0; i < count; i++)
            parts[i] = arr[i].ToString("G15");
        return "[" + string.Join(",", parts) + "]";
    }

    static string FormatIntArray(int[] arr, int count) {
        var parts = new string[count];
        for (int i = 0; i < count; i++)
            parts[i] = arr[i].ToString();
        return "[" + string.Join(",", parts) + "]";
    }

    static void Main(string[] args) {
        TA_Initialize();
        string? line;
        while ((line = Console.ReadLine()) != null) {
            if (string.IsNullOrWhiteSpace(line)) continue;
            Console.WriteLine(HandleRequest(line));
            Console.Out.Flush();
        }
    }
}
