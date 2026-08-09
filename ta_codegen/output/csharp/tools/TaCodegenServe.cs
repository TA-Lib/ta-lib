// Auto-generated JSON-RPC server for ta_codegen C# output (managed).
// The csproj compiles the shipped library sources from ../library — the
// server's Core IS the shipped partial class, not a copy.
using System;
using System.Text.Json;
using System.Diagnostics;
using TALib;
using TALib.Metadata;

public class TaCodegenServe {
    static Core core = new Core();
    const int MAX_ARRAY_SIZE = 200000;
    static double[] refOpen = new double[MAX_ARRAY_SIZE];
    static double[] refHigh = new double[MAX_ARRAY_SIZE];
    static double[] refLow = new double[MAX_ARRAY_SIZE];
    static double[] refClose = new double[MAX_ARRAY_SIZE];
    static double[] refVolume = new double[MAX_ARRAY_SIZE];
    static double[] refOI = new double[MAX_ARRAY_SIZE];
    static int refN = 0;

    static readonly JsonDocument EmptyParamsDoc = JsonDocument.Parse("{}");
    static JsonElement EmptyParams => EmptyParamsDoc.RootElement;

    static long GetNanoTime() {
        long ts = Stopwatch.GetTimestamp();
        long freq = Stopwatch.Frequency;
        return (ts / freq) * 1000000000L + (ts % freq) * 1000000000L / freq;
    }

    static int GetInt(JsonElement p, string name, int def) =>
        p.TryGetProperty(name, out var v) ? v.GetInt32() : def;

    static double GetDouble(JsonElement p, string name, double def) =>
        p.TryGetProperty(name, out var v) ? v.GetDouble() : def;

    static void LoadRef(JsonElement p, string name, double[] dst) {
        double[] tmp = GetDoubleArray(p, name);
        Array.Copy(tmp, dst, Math.Min(tmp.Length, MAX_ARRAY_SIZE));
    }

    static double[] GetDoubleArray(JsonElement p, string name) {
        if (!p.TryGetProperty(name, out var arr)) return Array.Empty<double>();
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
            parts[i] = arr[i].ToString();
        return "[" + string.Join(",", parts) + "]";
    }

    static string FormatIntArray(int[] arr, int count) {
        var parts = new string[count];
        for (int i = 0; i < count; i++)
            parts[i] = arr[i].ToString();
        return "[" + string.Join(",", parts) + "]";
    }

    static string HandleRequest(string json) {
        using var doc = JsonDocument.Parse(json);
        var root = doc.RootElement;
        string method = root.GetProperty("method").GetString()!;
        var p = root.TryGetProperty("params", out var pv) ? pv : EmptyParams;

            if (method == "load_data") {
                double[] tmpOpen = GetDoubleArray(p, "open");
                refN = Math.Min(tmpOpen.Length, MAX_ARRAY_SIZE);
                Array.Copy(tmpOpen, refOpen, refN);
                LoadRef(p, "high", refHigh);
                LoadRef(p, "low", refLow);
                LoadRef(p, "close", refClose);
                LoadRef(p, "volume", refVolume);
                LoadRef(p, "openInterest", refOI);
                return $"{{\"status\":\"ok\",\"n\":{refN}}}";
            }

            int startIdx = GetInt(p, "startIdx", 0);
            int endIdx = GetInt(p, "endIdx", 0);

            if (method == "TA_ACCBANDS") return Handle_ACCBANDS(p, startIdx, endIdx);
            else if (method == "TA_ACOS") return Handle_ACOS(p, startIdx, endIdx);
            else if (method == "TA_AD") return Handle_AD(p, startIdx, endIdx);
            else if (method == "TA_ADD") return Handle_ADD(p, startIdx, endIdx);
            else if (method == "TA_ADOSC") return Handle_ADOSC(p, startIdx, endIdx);
            else if (method == "TA_ADX") return Handle_ADX(p, startIdx, endIdx);
            else if (method == "TA_ADXR") return Handle_ADXR(p, startIdx, endIdx);
            else if (method == "TA_APO") return Handle_APO(p, startIdx, endIdx);
            else if (method == "TA_AROON") return Handle_AROON(p, startIdx, endIdx);
            else if (method == "TA_AROONOSC") return Handle_AROONOSC(p, startIdx, endIdx);
            else if (method == "TA_ASIN") return Handle_ASIN(p, startIdx, endIdx);
            else if (method == "TA_ATAN") return Handle_ATAN(p, startIdx, endIdx);
            else if (method == "TA_ATR") return Handle_ATR(p, startIdx, endIdx);
            else if (method == "TA_AVGDEV") return Handle_AVGDEV(p, startIdx, endIdx);
            else if (method == "TA_AVGPRICE") return Handle_AVGPRICE(p, startIdx, endIdx);
            else if (method == "TA_BBANDS") return Handle_BBANDS(p, startIdx, endIdx);
            else if (method == "TA_BETA") return Handle_BETA(p, startIdx, endIdx);
            else if (method == "TA_BOP") return Handle_BOP(p, startIdx, endIdx);
            else if (method == "TA_CCI") return Handle_CCI(p, startIdx, endIdx);
            else if (method == "TA_CDL2CROWS") return Handle_CDL2CROWS(p, startIdx, endIdx);
            else if (method == "TA_CDL3BLACKCROWS") return Handle_CDL3BLACKCROWS(p, startIdx, endIdx);
            else if (method == "TA_CDL3INSIDE") return Handle_CDL3INSIDE(p, startIdx, endIdx);
            else if (method == "TA_CDL3LINESTRIKE") return Handle_CDL3LINESTRIKE(p, startIdx, endIdx);
            else if (method == "TA_CDL3OUTSIDE") return Handle_CDL3OUTSIDE(p, startIdx, endIdx);
            else if (method == "TA_CDL3STARSINSOUTH") return Handle_CDL3STARSINSOUTH(p, startIdx, endIdx);
            else if (method == "TA_CDL3WHITESOLDIERS") return Handle_CDL3WHITESOLDIERS(p, startIdx, endIdx);
            else if (method == "TA_CDLABANDONEDBABY") return Handle_CDLABANDONEDBABY(p, startIdx, endIdx);
            else if (method == "TA_CDLADVANCEBLOCK") return Handle_CDLADVANCEBLOCK(p, startIdx, endIdx);
            else if (method == "TA_CDLBELTHOLD") return Handle_CDLBELTHOLD(p, startIdx, endIdx);
            else if (method == "TA_CDLBREAKAWAY") return Handle_CDLBREAKAWAY(p, startIdx, endIdx);
            else if (method == "TA_CDLCLOSINGMARUBOZU") return Handle_CDLCLOSINGMARUBOZU(p, startIdx, endIdx);
            else if (method == "TA_CDLCONCEALBABYSWALL") return Handle_CDLCONCEALBABYSWALL(p, startIdx, endIdx);
            else if (method == "TA_CDLCOUNTERATTACK") return Handle_CDLCOUNTERATTACK(p, startIdx, endIdx);
            else if (method == "TA_CDLDARKCLOUDCOVER") return Handle_CDLDARKCLOUDCOVER(p, startIdx, endIdx);
            else if (method == "TA_CDLDOJI") return Handle_CDLDOJI(p, startIdx, endIdx);
            else if (method == "TA_CDLDOJISTAR") return Handle_CDLDOJISTAR(p, startIdx, endIdx);
            else if (method == "TA_CDLDRAGONFLYDOJI") return Handle_CDLDRAGONFLYDOJI(p, startIdx, endIdx);
            else if (method == "TA_CDLENGULFING") return Handle_CDLENGULFING(p, startIdx, endIdx);
            else if (method == "TA_CDLEVENINGDOJISTAR") return Handle_CDLEVENINGDOJISTAR(p, startIdx, endIdx);
            else if (method == "TA_CDLEVENINGSTAR") return Handle_CDLEVENINGSTAR(p, startIdx, endIdx);
            else if (method == "TA_CDLGAPSIDESIDEWHITE") return Handle_CDLGAPSIDESIDEWHITE(p, startIdx, endIdx);
            else if (method == "TA_CDLGRAVESTONEDOJI") return Handle_CDLGRAVESTONEDOJI(p, startIdx, endIdx);
            else if (method == "TA_CDLHAMMER") return Handle_CDLHAMMER(p, startIdx, endIdx);
            else if (method == "TA_CDLHANGINGMAN") return Handle_CDLHANGINGMAN(p, startIdx, endIdx);
            else if (method == "TA_CDLHARAMI") return Handle_CDLHARAMI(p, startIdx, endIdx);
            else if (method == "TA_CDLHARAMICROSS") return Handle_CDLHARAMICROSS(p, startIdx, endIdx);
            else if (method == "TA_CDLHIGHWAVE") return Handle_CDLHIGHWAVE(p, startIdx, endIdx);
            else if (method == "TA_CDLHIKKAKE") return Handle_CDLHIKKAKE(p, startIdx, endIdx);
            else if (method == "TA_CDLHIKKAKEMOD") return Handle_CDLHIKKAKEMOD(p, startIdx, endIdx);
            else if (method == "TA_CDLHOMINGPIGEON") return Handle_CDLHOMINGPIGEON(p, startIdx, endIdx);
            else if (method == "TA_CDLIDENTICAL3CROWS") return Handle_CDLIDENTICAL3CROWS(p, startIdx, endIdx);
            else if (method == "TA_CDLINNECK") return Handle_CDLINNECK(p, startIdx, endIdx);
            else if (method == "TA_CDLINVERTEDHAMMER") return Handle_CDLINVERTEDHAMMER(p, startIdx, endIdx);
            else if (method == "TA_CDLKICKING") return Handle_CDLKICKING(p, startIdx, endIdx);
            else if (method == "TA_CDLKICKINGBYLENGTH") return Handle_CDLKICKINGBYLENGTH(p, startIdx, endIdx);
            else if (method == "TA_CDLLADDERBOTTOM") return Handle_CDLLADDERBOTTOM(p, startIdx, endIdx);
            else if (method == "TA_CDLLONGLEGGEDDOJI") return Handle_CDLLONGLEGGEDDOJI(p, startIdx, endIdx);
            else if (method == "TA_CDLLONGLINE") return Handle_CDLLONGLINE(p, startIdx, endIdx);
            else if (method == "TA_CDLMARUBOZU") return Handle_CDLMARUBOZU(p, startIdx, endIdx);
            else if (method == "TA_CDLMATCHINGLOW") return Handle_CDLMATCHINGLOW(p, startIdx, endIdx);
            else if (method == "TA_CDLMATHOLD") return Handle_CDLMATHOLD(p, startIdx, endIdx);
            else if (method == "TA_CDLMORNINGDOJISTAR") return Handle_CDLMORNINGDOJISTAR(p, startIdx, endIdx);
            else if (method == "TA_CDLMORNINGSTAR") return Handle_CDLMORNINGSTAR(p, startIdx, endIdx);
            else if (method == "TA_CDLONNECK") return Handle_CDLONNECK(p, startIdx, endIdx);
            else if (method == "TA_CDLPIERCING") return Handle_CDLPIERCING(p, startIdx, endIdx);
            else if (method == "TA_CDLRICKSHAWMAN") return Handle_CDLRICKSHAWMAN(p, startIdx, endIdx);
            else if (method == "TA_CDLRISEFALL3METHODS") return Handle_CDLRISEFALL3METHODS(p, startIdx, endIdx);
            else if (method == "TA_CDLSEPARATINGLINES") return Handle_CDLSEPARATINGLINES(p, startIdx, endIdx);
            else if (method == "TA_CDLSHOOTINGSTAR") return Handle_CDLSHOOTINGSTAR(p, startIdx, endIdx);
            else if (method == "TA_CDLSHORTLINE") return Handle_CDLSHORTLINE(p, startIdx, endIdx);
            else if (method == "TA_CDLSPINNINGTOP") return Handle_CDLSPINNINGTOP(p, startIdx, endIdx);
            else if (method == "TA_CDLSTALLEDPATTERN") return Handle_CDLSTALLEDPATTERN(p, startIdx, endIdx);
            else if (method == "TA_CDLSTICKSANDWICH") return Handle_CDLSTICKSANDWICH(p, startIdx, endIdx);
            else if (method == "TA_CDLTAKURI") return Handle_CDLTAKURI(p, startIdx, endIdx);
            else if (method == "TA_CDLTASUKIGAP") return Handle_CDLTASUKIGAP(p, startIdx, endIdx);
            else if (method == "TA_CDLTHRUSTING") return Handle_CDLTHRUSTING(p, startIdx, endIdx);
            else if (method == "TA_CDLTRISTAR") return Handle_CDLTRISTAR(p, startIdx, endIdx);
            else if (method == "TA_CDLUNIQUE3RIVER") return Handle_CDLUNIQUE3RIVER(p, startIdx, endIdx);
            else if (method == "TA_CDLUPSIDEGAP2CROWS") return Handle_CDLUPSIDEGAP2CROWS(p, startIdx, endIdx);
            else if (method == "TA_CDLXSIDEGAP3METHODS") return Handle_CDLXSIDEGAP3METHODS(p, startIdx, endIdx);
            else if (method == "TA_CEIL") return Handle_CEIL(p, startIdx, endIdx);
            else if (method == "TA_CMF") return Handle_CMF(p, startIdx, endIdx);
            else if (method == "TA_CMO") return Handle_CMO(p, startIdx, endIdx);
            else if (method == "TA_CMOU") return Handle_CMOU(p, startIdx, endIdx);
            else if (method == "TA_CORREL") return Handle_CORREL(p, startIdx, endIdx);
            else if (method == "TA_COS") return Handle_COS(p, startIdx, endIdx);
            else if (method == "TA_COSH") return Handle_COSH(p, startIdx, endIdx);
            else if (method == "TA_DEMA") return Handle_DEMA(p, startIdx, endIdx);
            else if (method == "TA_DIV") return Handle_DIV(p, startIdx, endIdx);
            else if (method == "TA_DX") return Handle_DX(p, startIdx, endIdx);
            else if (method == "TA_EMA") return Handle_EMA(p, startIdx, endIdx);
            else if (method == "TA_EXP") return Handle_EXP(p, startIdx, endIdx);
            else if (method == "TA_FLOOR") return Handle_FLOOR(p, startIdx, endIdx);
            else if (method == "TA_HMA") return Handle_HMA(p, startIdx, endIdx);
            else if (method == "TA_HT_DCPERIOD") return Handle_HT_DCPERIOD(p, startIdx, endIdx);
            else if (method == "TA_HT_DCPHASE") return Handle_HT_DCPHASE(p, startIdx, endIdx);
            else if (method == "TA_HT_PHASOR") return Handle_HT_PHASOR(p, startIdx, endIdx);
            else if (method == "TA_HT_SINE") return Handle_HT_SINE(p, startIdx, endIdx);
            else if (method == "TA_HT_TRENDLINE") return Handle_HT_TRENDLINE(p, startIdx, endIdx);
            else if (method == "TA_HT_TRENDMODE") return Handle_HT_TRENDMODE(p, startIdx, endIdx);
            else if (method == "TA_IMI") return Handle_IMI(p, startIdx, endIdx);
            else if (method == "TA_KAMA") return Handle_KAMA(p, startIdx, endIdx);
            else if (method == "TA_LINEARREG") return Handle_LINEARREG(p, startIdx, endIdx);
            else if (method == "TA_LINEARREG_ANGLE") return Handle_LINEARREG_ANGLE(p, startIdx, endIdx);
            else if (method == "TA_LINEARREG_INTERCEPT") return Handle_LINEARREG_INTERCEPT(p, startIdx, endIdx);
            else if (method == "TA_LINEARREG_SLOPE") return Handle_LINEARREG_SLOPE(p, startIdx, endIdx);
            else if (method == "TA_LN") return Handle_LN(p, startIdx, endIdx);
            else if (method == "TA_LOG10") return Handle_LOG10(p, startIdx, endIdx);
            else if (method == "TA_MA") return Handle_MA(p, startIdx, endIdx);
            else if (method == "TA_MACD") return Handle_MACD(p, startIdx, endIdx);
            else if (method == "TA_MACDEXT") return Handle_MACDEXT(p, startIdx, endIdx);
            else if (method == "TA_MACDFIX") return Handle_MACDFIX(p, startIdx, endIdx);
            else if (method == "TA_MAMA") return Handle_MAMA(p, startIdx, endIdx);
            else if (method == "TA_MAVP") return Handle_MAVP(p, startIdx, endIdx);
            else if (method == "TA_MAX") return Handle_MAX(p, startIdx, endIdx);
            else if (method == "TA_MAXINDEX") return Handle_MAXINDEX(p, startIdx, endIdx);
            else if (method == "TA_MEDPRICE") return Handle_MEDPRICE(p, startIdx, endIdx);
            else if (method == "TA_MFI") return Handle_MFI(p, startIdx, endIdx);
            else if (method == "TA_MIDPOINT") return Handle_MIDPOINT(p, startIdx, endIdx);
            else if (method == "TA_MIDPRICE") return Handle_MIDPRICE(p, startIdx, endIdx);
            else if (method == "TA_MIN") return Handle_MIN(p, startIdx, endIdx);
            else if (method == "TA_MININDEX") return Handle_MININDEX(p, startIdx, endIdx);
            else if (method == "TA_MINMAX") return Handle_MINMAX(p, startIdx, endIdx);
            else if (method == "TA_MINMAXINDEX") return Handle_MINMAXINDEX(p, startIdx, endIdx);
            else if (method == "TA_MINUS_DI") return Handle_MINUS_DI(p, startIdx, endIdx);
            else if (method == "TA_MINUS_DM") return Handle_MINUS_DM(p, startIdx, endIdx);
            else if (method == "TA_MOM") return Handle_MOM(p, startIdx, endIdx);
            else if (method == "TA_MULT") return Handle_MULT(p, startIdx, endIdx);
            else if (method == "TA_NATR") return Handle_NATR(p, startIdx, endIdx);
            else if (method == "TA_NVI") return Handle_NVI(p, startIdx, endIdx);
            else if (method == "TA_OBV") return Handle_OBV(p, startIdx, endIdx);
            else if (method == "TA_PLUS_DI") return Handle_PLUS_DI(p, startIdx, endIdx);
            else if (method == "TA_PLUS_DM") return Handle_PLUS_DM(p, startIdx, endIdx);
            else if (method == "TA_PPO") return Handle_PPO(p, startIdx, endIdx);
            else if (method == "TA_PVI") return Handle_PVI(p, startIdx, endIdx);
            else if (method == "TA_PVO") return Handle_PVO(p, startIdx, endIdx);
            else if (method == "TA_ROC") return Handle_ROC(p, startIdx, endIdx);
            else if (method == "TA_ROCP") return Handle_ROCP(p, startIdx, endIdx);
            else if (method == "TA_ROCR") return Handle_ROCR(p, startIdx, endIdx);
            else if (method == "TA_ROCR100") return Handle_ROCR100(p, startIdx, endIdx);
            else if (method == "TA_RSI") return Handle_RSI(p, startIdx, endIdx);
            else if (method == "TA_SAR") return Handle_SAR(p, startIdx, endIdx);
            else if (method == "TA_SAREXT") return Handle_SAREXT(p, startIdx, endIdx);
            else if (method == "TA_SIN") return Handle_SIN(p, startIdx, endIdx);
            else if (method == "TA_SINH") return Handle_SINH(p, startIdx, endIdx);
            else if (method == "TA_SMA") return Handle_SMA(p, startIdx, endIdx);
            else if (method == "TA_SQRT") return Handle_SQRT(p, startIdx, endIdx);
            else if (method == "TA_STDDEV") return Handle_STDDEV(p, startIdx, endIdx);
            else if (method == "TA_STOCH") return Handle_STOCH(p, startIdx, endIdx);
            else if (method == "TA_STOCHF") return Handle_STOCHF(p, startIdx, endIdx);
            else if (method == "TA_STOCHRSI") return Handle_STOCHRSI(p, startIdx, endIdx);
            else if (method == "TA_SUB") return Handle_SUB(p, startIdx, endIdx);
            else if (method == "TA_SUM") return Handle_SUM(p, startIdx, endIdx);
            else if (method == "TA_T3") return Handle_T3(p, startIdx, endIdx);
            else if (method == "TA_TAN") return Handle_TAN(p, startIdx, endIdx);
            else if (method == "TA_TANH") return Handle_TANH(p, startIdx, endIdx);
            else if (method == "TA_TEMA") return Handle_TEMA(p, startIdx, endIdx);
            else if (method == "TA_TRANGE") return Handle_TRANGE(p, startIdx, endIdx);
            else if (method == "TA_TRIMA") return Handle_TRIMA(p, startIdx, endIdx);
            else if (method == "TA_TRIX") return Handle_TRIX(p, startIdx, endIdx);
            else if (method == "TA_TSF") return Handle_TSF(p, startIdx, endIdx);
            else if (method == "TA_TYPPRICE") return Handle_TYPPRICE(p, startIdx, endIdx);
            else if (method == "TA_ULTOSC") return Handle_ULTOSC(p, startIdx, endIdx);
            else if (method == "TA_VAR") return Handle_VAR(p, startIdx, endIdx);
            else if (method == "TA_VWMA") return Handle_VWMA(p, startIdx, endIdx);
            else if (method == "TA_WCLPRICE") return Handle_WCLPRICE(p, startIdx, endIdx);
            else if (method == "TA_WILLR") return Handle_WILLR(p, startIdx, endIdx);
            else if (method == "TA_WMA") return Handle_WMA(p, startIdx, endIdx);
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
                sb.Append("\"TA_CMF\"");
                sb.Append(",");
                sb.Append("\"TA_CMO\"");
                sb.Append(",");
                sb.Append("\"TA_CMOU\"");
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
                sb.Append("\"TA_HMA\"");
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
                sb.Append("\"TA_NVI\"");
                sb.Append(",");
                sb.Append("\"TA_OBV\"");
                sb.Append(",");
                sb.Append("\"TA_PLUS_DI\"");
                sb.Append(",");
                sb.Append("\"TA_PLUS_DM\"");
                sb.Append(",");
                sb.Append("\"TA_PPO\"");
                sb.Append(",");
                sb.Append("\"TA_PVI\"");
                sb.Append(",");
                sb.Append("\"TA_PVO\"");
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
                sb.Append("\"TA_VWMA\"");
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
                int id = GetInt(p, "id", -1);
                int period = GetInt(p, "period", 0);
                if (id == (int)FuncUnstId.ALL) {
                    for (int i = 0; i < core.unstablePeriod.Length; i++) core.unstablePeriod[i] = period;
                    return "{\"status\":\"ok\"}";
                }
                if (id >= 0 && id < core.unstablePeriod.Length) {
                    core.unstablePeriod[id] = period;
                    return "{\"status\":\"ok\"}";
                }
                return "{\"error\":\"Invalid id\"}";
            }
            else if (method == "set_compatibility") {
                int mode = GetInt(p, "mode", 0);
                if (mode == 0) {
                    return "{\"status\":\"ok\"}";
                }
                return "{\"error\":\"csharp has no compatibility API (pinned to Default)\"}";
            }
            else if (method == "eval_predicate") {
                int which = GetInt(p, "which", 0);
                double[] values = GetDoubleArray(p, "values");
                double[] scale = GetDoubleArray(p, "scale");
                var parts = new string[values.Length];
                for (int i = 0; i < values.Length; i++) {
                    double v = values[i];
                    double sc = (i < scale.Length) ? scale[i] : 0.0;
                    bool r;
                    if (which == 1) r = (Math.Abs(v) <= 0.00000000000001 * (sc));
                    else if (which == 2) r = (v < 0.00000000000001);
                    else r = ((-0.00000000000001 < v) && (v < 0.00000000000001));
                    parts[i] = r ? "1" : "0";
                }
                return "{\"outInteger\":[" + string.Join(",", parts) + "]}";
            }
            else if (method == "abstract_get_lookback") {
                string fn = p.GetProperty("funcName").GetString()!;
                return $"{{\"lookback\":{ComputeLookback(fn, p)}}}";
            }
            else if (method == "TA_GetFuncInfo") return AbsFuncInfo(p);
            else if (method == "TA_GetInputParameterInfo") return AbsInputInfo(p);
            else if (method == "TA_GetOptInputParameterInfo") return AbsOptInputInfo(p);
            else if (method == "TA_GetOutputParameterInfo") return AbsOutputInfo(p);
            else if (method == "abstract_for_each_func") return AbsForEachFunc();
            else if (method == "TA_FunctionDescriptionXML") return AbsDescriptionXml();
            else if (method == "abstract_call") return AbsCall(p);
            else {
                return $"{{\"error\":\"Unknown method: {method}\"}}";
            }
    }

    static string AbsStr(string? v) {
        if (v is null) return "\"\"";
        var b = new System.Text.StringBuilder("\"");
        foreach (char c in v) {
            /* The full JSON string grammar, not just quote and backslash. The
               transport is NEWLINE-FRAMED (codegen_pipe reads to the next '\n'),
               so an unescaped control character in an error message would split
               one reply into two lines and hand the second to the NEXT request --
               desynchronising the stream permanently, which is worse than the
               crash the surrounding try/catch replaces. */
            switch (c) {
                case '"':  b.Append("\\\""); break;
                case '\\': b.Append("\\\\"); break;
                case '\b': b.Append("\\b"); break;
                case '\f': b.Append("\\f"); break;
                case '\n': b.Append("\\n"); break;
                case '\r': b.Append("\\r"); break;
                case '\t': b.Append("\\t"); break;
                default:
                    if (c < 0x20) b.Append("\\u").Append(((int)c).ToString("x4"));
                    else b.Append(c);
                    break;
            }
        }
        b.Append('"');
        return b.ToString();
    }

    static string R(double v) => v.ToString("R", System.Globalization.CultureInfo.InvariantCulture);

    static int DomainCode(OptInputDomain d) => d switch {
        OptInputDomain.RealRange => 0,
        OptInputDomain.RealList => 1,
        OptInputDomain.IntegerRange => 2,
        OptInputDomain.IntegerList => 3,
        _ => throw new InvalidOperationException("unhandled OptInputDomain"),
    };

    static FunctionInfo? AbsLookup(JsonElement p) =>
        FunctionCatalog.Default.TryGet(p.GetProperty("funcName").GetString()!, out var f) ? f : null;

    static string AbsFuncInfo(JsonElement p) {
        var f = AbsLookup(p);
        if (f is null) return "{\"retCode\":2}";
        return $"{{\"name\":{AbsStr(f.Name)},\"group\":{AbsStr(f.Group.ToDisplayName())}"
             + $",\"hint\":{AbsStr(f.Hint)}"
             + $",\"flags\":{(uint)f.Flags},\"nbInput\":{f.Inputs.Length}"
             + $",\"nbOptInput\":{f.OptInputs.Length},\"nbOutput\":{f.Outputs.Length}}}";
    }

    static string AbsInputInfo(JsonElement p) {
        var f = AbsLookup(p);
        int i = GetInt(p, "paramIndex", -1);
        if (f is null || i < 0 || i >= f.Inputs.Length) return "{\"retCode\":2}";
        var ii = f.Inputs[i];
        return $"{{\"type\":{(int)ii.Kind},\"paramName\":{AbsStr(ii.ParamName)},\"flags\":{(uint)ii.Components}}}";
    }

    static string AbsOutputInfo(JsonElement p) {
        var f = AbsLookup(p);
        int i = GetInt(p, "paramIndex", -1);
        if (f is null || i < 0 || i >= f.Outputs.Length) return "{\"retCode\":2}";
        var oo = f.Outputs[i];
        return $"{{\"type\":{(int)oo.Kind},\"paramName\":{AbsStr(oo.ParamName)},\"flags\":{(uint)oo.Flags}}}";
    }

    static string AbsOptInputInfo(JsonElement p) {
        var f = AbsLookup(p);
        int i = GetInt(p, "paramIndex", -1);
        if (f is null || i < 0 || i >= f.OptInputs.Length) return "{\"retCode\":2}";
        var o = f.OptInputs[i];
        var b = new System.Text.StringBuilder($"{{\"type\":{DomainCode(o.Domain)}")
            .Append($",\"paramName\":{AbsStr(o.ParamName)}")
            .Append($",\"flags\":{(uint)o.Flags}")
            .Append($",\"displayName\":{AbsStr(o.DisplayName)}")
            .Append($",\"hint\":{AbsStr(o.Hint)}")
            .Append($",\"defaultValue\":{R(o.DefaultValue)}");
        switch (o.Domain) {
            case OptInputDomain.RealRange r:
                b.Append($",\"min\":{R(r.Min)},\"max\":{R(r.Max)},\"precision\":{r.Precision}")
                 .Append($",\"suggestedStart\":{R(r.SuggestedStart)}")
                 .Append($",\"suggestedEnd\":{R(r.SuggestedEnd)}")
                 .Append($",\"suggestedIncrement\":{R(r.SuggestedIncrement)}");
                break;
            case OptInputDomain.IntegerRange r:
                b.Append($",\"min\":{r.Min},\"max\":{r.Max}")
                 .Append($",\"suggestedStart\":{r.SuggestedStart}")
                 .Append($",\"suggestedEnd\":{r.SuggestedEnd}")
                 .Append($",\"suggestedIncrement\":{r.SuggestedIncrement}");
                break;
            case OptInputDomain.IntegerList l:
                b.Append($",\"valueList\":{AbsStr(l.ToValueListString())}");
                break;
            case OptInputDomain.RealList l:
                b.Append($",\"valueList\":{AbsStr(l.ToValueListString())}");
                break;
            default:
                throw new InvalidOperationException("unhandled OptInputDomain");
        }
        b.Append('}');
        return b.ToString();
    }

    static string AbsForEachFunc() {
        var b = new System.Text.StringBuilder("{\"functions\":[");
        bool first = true;
        foreach (var f in FunctionCatalog.Default) {
            if (!first) b.Append(',');
            first = false;
            b.Append($"{{\"name\":{AbsStr(f.Name)},\"group\":{AbsStr(f.Group.ToDisplayName())}")
             .Append($",\"nbInput\":{f.Inputs.Length},\"nbOptInput\":{f.OptInputs.Length}")
             .Append($",\"nbOutput\":{f.Outputs.Length}}}");
        }
        b.Append("]}");
        return b.ToString();
    }

    /* Measured at RUN TIME from the SHIPPED FunctionDescription. Baking the two
       numbers at generation time made this leg unfailable: it compared C's real
       bytes against constants derived from the same string C's own table is
       built from (#164). Now both sides are real bytes. */
    static string AbsDescriptionXml()
    {
        string xml = TALib.Metadata.FunctionDescription.Xml;
        ulong checksum = 0;
        foreach (char c in xml) checksum += (ulong)(c & 0xFF);
        return $"{{\"length\":{xml.Length},\"checksum\":{checksum}}}";
    }

    /* The JSON key the driver sends a required input under. Price bundles are
       sent one component per set bit; a lone real input keeps its own name,
       and several become inReal0/inReal1/... by rank (test_abstract.c's
       abstract_verify_server_call and expand_input_names agree on this). */
    static string AbsRealInputKey(FunctionInfo f, int slot) {
        int totalReal = 0, rank = 0;
        for (int i = 0; i < f.Inputs.Length; i++) {
            if (f.Inputs[i].Kind != InputKind.Real) continue;
            if (i < slot) rank++;
            totalReal++;
        }
        return totalReal == 1 ? f.Inputs[slot].ParamName : $"inReal{rank}";
    }

    static string AbsComponentKey(PriceComponents c) => c switch {
        PriceComponents.Open => "inOpen",
        PriceComponents.High => "inHigh",
        PriceComponents.Low => "inLow",
        PriceComponents.Close => "inClose",
        PriceComponents.Volume => "inVolume",
        PriceComponents.OpenInterest => "inOpenInterest",
        _ => throw new ArgumentException($"not a single component: {c}"),
    };

    /* abstract_call — the fully generic path, bound through FunctionCall. This
       is a genuinely independent second implementation rather than a reroute to
       the per-function handler (which is what the Rust and Java servers do), so
       a wrong slot index or a transposed price component shows up as diverging
       VALUES against the C reference. */
    static string AbsCall(JsonElement p) {
        var f = AbsLookup(p);
        if (f is null) return "{\"error\":\"Unknown function\"}";
        int startIdx = GetInt(p, "startIdx", 0);
        int endIdx = GetInt(p, "endIdx", 0);
        // Answer the range codes BEFORE sizing anything by the range (#180).
        // `n` below drives every output allocation, so validating after it
        // would turn an out-of-range request into an 800MB-per-output
        // allocation and take the server down instead of returning a code.
        if (startIdx < 0 || startIdx > Core.MAX_INDEX)
            return "{\"binder\":1,\"lookback\":-1,\"retCode\":12,\"outBegIdx\":0,\"outNBElement\":0}";
        if (endIdx < 0 || endIdx > Core.MAX_INDEX || endIdx < startIdx)
            return "{\"binder\":1,\"lookback\":-1,\"retCode\":13,\"outBegIdx\":0,\"outNBElement\":0}";
        int n = endIdx - startIdx + 1;
        if (n < 1) n = 1;

        var call = f.CreateCall(core);
        for (int i = 0; i < f.Inputs.Length; i++) {
            var info = f.Inputs[i];
            if (info.Kind == InputKind.Price) {
                foreach (var comp in info.SignatureOrder) {
                    call.SetPriceInput(i, comp, GetDoubleArray(p, AbsComponentKey(comp)));
                }
            } else if (info.Kind == InputKind.Real) {
                call.SetInput(i, GetDoubleArray(p, AbsRealInputKey(f, i)));
            } else {
                var raw = GetDoubleArray(p, info.ParamName);
                var ints = new int[raw.Length];
                for (int k = 0; k < raw.Length; k++) ints[k] = (int)raw[k];
                call.SetInput(i, ints);
            }
        }

        if (f.UnstableId is FuncUnstId unstId) {
            core.unstablePeriod[(int)unstId] = GetInt(p, "unstablePeriod", 0);
        }

        for (int i = 0; i < f.OptInputs.Length; i++) {
            var o = f.OptInputs[i];
            if (o.Domain is OptInputDomain.RealRange or OptInputDomain.RealList) {
                call.SetOption(i, GetDouble(p, o.ParamName, o.DefaultValue));
            } else {
                call.SetOption(i, GetInt(p, o.ParamName, (int)o.DefaultValue));
            }
        }

        var realOuts = new double[f.Outputs.Length][];
        var intOuts = new int[f.Outputs.Length][];
        for (int k = 0; k < f.Outputs.Length; k++) {
            if (f.Outputs[k].Kind == OutputKind.Real) {
                realOuts[k] = new double[n];
                call.SetOutput(k, realOuts[k]);
            } else {
                intOuts[k] = new int[n];
                call.SetOutput(k, intOuts[k]);
            }
        }

        int lookback = call.Lookback();
        RetCode rc = call.TryInvoke(startIdx, endIdx, out OutRange range);

        var b = new System.Text.StringBuilder();
        b.Append($"{{\"lookback\":{lookback},\"retCode\":{(int)rc}")
         .Append($",\"outBegIdx\":{range.BegIdx},\"outNBElement\":{range.Count}");
        int realRank = 0, intRank = 0;
        for (int k = 0; k < f.Outputs.Length; k++) {
            if (f.Outputs[k].Kind == OutputKind.Real) {
                string key = realRank == 0 ? "outReal" : $"outReal{realRank}";
                realRank++;
                b.Append($",\"{key}\":").Append(FormatArray(realOuts[k], range.Count));
            } else {
                string key = intRank == 0 ? "outInteger" : $"outInteger{intRank}";
                intRank++;
                b.Append($",\"{key}\":").Append(FormatIntArray(intOuts[k], range.Count));
            }
        }
        b.Append('}');
        return b.ToString();
    }

    static long ComputeLookback(string funcName, JsonElement p) {
        switch (funcName) {
        case "ACCBANDS": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.ACCBANDS_Lookback(optInTimePeriod);
        }
        case "ACOS": {
            return core.ACOS_Lookback();
        }
        case "AD": {
            return core.AD_Lookback();
        }
        case "ADD": {
            return core.ADD_Lookback();
        }
        case "ADOSC": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            return core.ADOSC_Lookback(optInFastPeriod, optInSlowPeriod);
        }
        case "ADX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.ADX_Lookback(optInTimePeriod);
        }
        case "ADXR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.ADXR_Lookback(optInTimePeriod);
        }
        case "APO": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.APO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType);
        }
        case "AROON": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.AROON_Lookback(optInTimePeriod);
        }
        case "AROONOSC": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.AROONOSC_Lookback(optInTimePeriod);
        }
        case "ASIN": {
            return core.ASIN_Lookback();
        }
        case "ATAN": {
            return core.ATAN_Lookback();
        }
        case "ATR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.ATR_Lookback(optInTimePeriod);
        }
        case "AVGDEV": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.AVGDEV_Lookback(optInTimePeriod);
        }
        case "AVGPRICE": {
            return core.AVGPRICE_Lookback();
        }
        case "BBANDS": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            double optInNbDevUp = GetDouble(p, "optInNbDevUp", 0.0);
            double optInNbDevDn = GetDouble(p, "optInNbDevDn", 0.0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.BBANDS_Lookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
        }
        case "BETA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.BETA_Lookback(optInTimePeriod);
        }
        case "BOP": {
            return core.BOP_Lookback();
        }
        case "CCI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.CCI_Lookback(optInTimePeriod);
        }
        case "CDL2CROWS": {
            return core.CDL2CROWS_Lookback();
        }
        case "CDL3BLACKCROWS": {
            return core.CDL3BLACKCROWS_Lookback();
        }
        case "CDL3INSIDE": {
            return core.CDL3INSIDE_Lookback();
        }
        case "CDL3LINESTRIKE": {
            return core.CDL3LINESTRIKE_Lookback();
        }
        case "CDL3OUTSIDE": {
            return core.CDL3OUTSIDE_Lookback();
        }
        case "CDL3STARSINSOUTH": {
            return core.CDL3STARSINSOUTH_Lookback();
        }
        case "CDL3WHITESOLDIERS": {
            return core.CDL3WHITESOLDIERS_Lookback();
        }
        case "CDLABANDONEDBABY": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CDLABANDONEDBABY_Lookback(optInPenetration);
        }
        case "CDLADVANCEBLOCK": {
            return core.CDLADVANCEBLOCK_Lookback();
        }
        case "CDLBELTHOLD": {
            return core.CDLBELTHOLD_Lookback();
        }
        case "CDLBREAKAWAY": {
            return core.CDLBREAKAWAY_Lookback();
        }
        case "CDLCLOSINGMARUBOZU": {
            return core.CDLCLOSINGMARUBOZU_Lookback();
        }
        case "CDLCONCEALBABYSWALL": {
            return core.CDLCONCEALBABYSWALL_Lookback();
        }
        case "CDLCOUNTERATTACK": {
            return core.CDLCOUNTERATTACK_Lookback();
        }
        case "CDLDARKCLOUDCOVER": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CDLDARKCLOUDCOVER_Lookback(optInPenetration);
        }
        case "CDLDOJI": {
            return core.CDLDOJI_Lookback();
        }
        case "CDLDOJISTAR": {
            return core.CDLDOJISTAR_Lookback();
        }
        case "CDLDRAGONFLYDOJI": {
            return core.CDLDRAGONFLYDOJI_Lookback();
        }
        case "CDLENGULFING": {
            return core.CDLENGULFING_Lookback();
        }
        case "CDLEVENINGDOJISTAR": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CDLEVENINGDOJISTAR_Lookback(optInPenetration);
        }
        case "CDLEVENINGSTAR": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CDLEVENINGSTAR_Lookback(optInPenetration);
        }
        case "CDLGAPSIDESIDEWHITE": {
            return core.CDLGAPSIDESIDEWHITE_Lookback();
        }
        case "CDLGRAVESTONEDOJI": {
            return core.CDLGRAVESTONEDOJI_Lookback();
        }
        case "CDLHAMMER": {
            return core.CDLHAMMER_Lookback();
        }
        case "CDLHANGINGMAN": {
            return core.CDLHANGINGMAN_Lookback();
        }
        case "CDLHARAMI": {
            return core.CDLHARAMI_Lookback();
        }
        case "CDLHARAMICROSS": {
            return core.CDLHARAMICROSS_Lookback();
        }
        case "CDLHIGHWAVE": {
            return core.CDLHIGHWAVE_Lookback();
        }
        case "CDLHIKKAKE": {
            return core.CDLHIKKAKE_Lookback();
        }
        case "CDLHIKKAKEMOD": {
            return core.CDLHIKKAKEMOD_Lookback();
        }
        case "CDLHOMINGPIGEON": {
            return core.CDLHOMINGPIGEON_Lookback();
        }
        case "CDLIDENTICAL3CROWS": {
            return core.CDLIDENTICAL3CROWS_Lookback();
        }
        case "CDLINNECK": {
            return core.CDLINNECK_Lookback();
        }
        case "CDLINVERTEDHAMMER": {
            return core.CDLINVERTEDHAMMER_Lookback();
        }
        case "CDLKICKING": {
            return core.CDLKICKING_Lookback();
        }
        case "CDLKICKINGBYLENGTH": {
            return core.CDLKICKINGBYLENGTH_Lookback();
        }
        case "CDLLADDERBOTTOM": {
            return core.CDLLADDERBOTTOM_Lookback();
        }
        case "CDLLONGLEGGEDDOJI": {
            return core.CDLLONGLEGGEDDOJI_Lookback();
        }
        case "CDLLONGLINE": {
            return core.CDLLONGLINE_Lookback();
        }
        case "CDLMARUBOZU": {
            return core.CDLMARUBOZU_Lookback();
        }
        case "CDLMATCHINGLOW": {
            return core.CDLMATCHINGLOW_Lookback();
        }
        case "CDLMATHOLD": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CDLMATHOLD_Lookback(optInPenetration);
        }
        case "CDLMORNINGDOJISTAR": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CDLMORNINGDOJISTAR_Lookback(optInPenetration);
        }
        case "CDLMORNINGSTAR": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CDLMORNINGSTAR_Lookback(optInPenetration);
        }
        case "CDLONNECK": {
            return core.CDLONNECK_Lookback();
        }
        case "CDLPIERCING": {
            return core.CDLPIERCING_Lookback();
        }
        case "CDLRICKSHAWMAN": {
            return core.CDLRICKSHAWMAN_Lookback();
        }
        case "CDLRISEFALL3METHODS": {
            return core.CDLRISEFALL3METHODS_Lookback();
        }
        case "CDLSEPARATINGLINES": {
            return core.CDLSEPARATINGLINES_Lookback();
        }
        case "CDLSHOOTINGSTAR": {
            return core.CDLSHOOTINGSTAR_Lookback();
        }
        case "CDLSHORTLINE": {
            return core.CDLSHORTLINE_Lookback();
        }
        case "CDLSPINNINGTOP": {
            return core.CDLSPINNINGTOP_Lookback();
        }
        case "CDLSTALLEDPATTERN": {
            return core.CDLSTALLEDPATTERN_Lookback();
        }
        case "CDLSTICKSANDWICH": {
            return core.CDLSTICKSANDWICH_Lookback();
        }
        case "CDLTAKURI": {
            return core.CDLTAKURI_Lookback();
        }
        case "CDLTASUKIGAP": {
            return core.CDLTASUKIGAP_Lookback();
        }
        case "CDLTHRUSTING": {
            return core.CDLTHRUSTING_Lookback();
        }
        case "CDLTRISTAR": {
            return core.CDLTRISTAR_Lookback();
        }
        case "CDLUNIQUE3RIVER": {
            return core.CDLUNIQUE3RIVER_Lookback();
        }
        case "CDLUPSIDEGAP2CROWS": {
            return core.CDLUPSIDEGAP2CROWS_Lookback();
        }
        case "CDLXSIDEGAP3METHODS": {
            return core.CDLXSIDEGAP3METHODS_Lookback();
        }
        case "CEIL": {
            return core.CEIL_Lookback();
        }
        case "CMF": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.CMF_Lookback(optInTimePeriod);
        }
        case "CMO": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.CMO_Lookback(optInTimePeriod);
        }
        case "CMOU": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.CMOU_Lookback(optInTimePeriod);
        }
        case "CORREL": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.CORREL_Lookback(optInTimePeriod);
        }
        case "COS": {
            return core.COS_Lookback();
        }
        case "COSH": {
            return core.COSH_Lookback();
        }
        case "DEMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.DEMA_Lookback(optInTimePeriod);
        }
        case "DIV": {
            return core.DIV_Lookback();
        }
        case "DX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.DX_Lookback(optInTimePeriod);
        }
        case "EMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.EMA_Lookback(optInTimePeriod);
        }
        case "EXP": {
            return core.EXP_Lookback();
        }
        case "FLOOR": {
            return core.FLOOR_Lookback();
        }
        case "HMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.HMA_Lookback(optInTimePeriod);
        }
        case "HT_DCPERIOD": {
            return core.HT_DCPERIOD_Lookback();
        }
        case "HT_DCPHASE": {
            return core.HT_DCPHASE_Lookback();
        }
        case "HT_PHASOR": {
            return core.HT_PHASOR_Lookback();
        }
        case "HT_SINE": {
            return core.HT_SINE_Lookback();
        }
        case "HT_TRENDLINE": {
            return core.HT_TRENDLINE_Lookback();
        }
        case "HT_TRENDMODE": {
            return core.HT_TRENDMODE_Lookback();
        }
        case "IMI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.IMI_Lookback(optInTimePeriod);
        }
        case "KAMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.KAMA_Lookback(optInTimePeriod);
        }
        case "LINEARREG": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.LINEARREG_Lookback(optInTimePeriod);
        }
        case "LINEARREG_ANGLE": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.LINEARREG_ANGLE_Lookback(optInTimePeriod);
        }
        case "LINEARREG_INTERCEPT": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.LINEARREG_INTERCEPT_Lookback(optInTimePeriod);
        }
        case "LINEARREG_SLOPE": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.LINEARREG_SLOPE_Lookback(optInTimePeriod);
        }
        case "LN": {
            return core.LN_Lookback();
        }
        case "LOG10": {
            return core.LOG10_Lookback();
        }
        case "MA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.MA_Lookback(optInTimePeriod, optInMAType);
        }
        case "MACD": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            int optInSignalPeriod = GetInt(p, "optInSignalPeriod", 0);
            return core.MACD_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
        }
        case "MACDEXT": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            MAType optInFastMAType = (MAType)GetInt(p, "optInFastMAType", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            MAType optInSlowMAType = (MAType)GetInt(p, "optInSlowMAType", 0);
            int optInSignalPeriod = GetInt(p, "optInSignalPeriod", 0);
            MAType optInSignalMAType = (MAType)GetInt(p, "optInSignalMAType", 0);
            return core.MACDEXT_Lookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
        }
        case "MACDFIX": {
            int optInSignalPeriod = GetInt(p, "optInSignalPeriod", 0);
            return core.MACDFIX_Lookback(optInSignalPeriod);
        }
        case "MAMA": {
            double optInFastLimit = GetDouble(p, "optInFastLimit", 0.0);
            double optInSlowLimit = GetDouble(p, "optInSlowLimit", 0.0);
            return core.MAMA_Lookback(optInFastLimit, optInSlowLimit);
        }
        case "MAVP": {
            int optInMinPeriod = GetInt(p, "optInMinPeriod", 0);
            int optInMaxPeriod = GetInt(p, "optInMaxPeriod", 0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.MAVP_Lookback(optInMinPeriod, optInMaxPeriod, optInMAType);
        }
        case "MAX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MAX_Lookback(optInTimePeriod);
        }
        case "MAXINDEX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MAXINDEX_Lookback(optInTimePeriod);
        }
        case "MEDPRICE": {
            return core.MEDPRICE_Lookback();
        }
        case "MFI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MFI_Lookback(optInTimePeriod);
        }
        case "MIDPOINT": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MIDPOINT_Lookback(optInTimePeriod);
        }
        case "MIDPRICE": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MIDPRICE_Lookback(optInTimePeriod);
        }
        case "MIN": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MIN_Lookback(optInTimePeriod);
        }
        case "MININDEX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MININDEX_Lookback(optInTimePeriod);
        }
        case "MINMAX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MINMAX_Lookback(optInTimePeriod);
        }
        case "MINMAXINDEX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MINMAXINDEX_Lookback(optInTimePeriod);
        }
        case "MINUS_DI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MINUS_DI_Lookback(optInTimePeriod);
        }
        case "MINUS_DM": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MINUS_DM_Lookback(optInTimePeriod);
        }
        case "MOM": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MOM_Lookback(optInTimePeriod);
        }
        case "MULT": {
            return core.MULT_Lookback();
        }
        case "NATR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.NATR_Lookback(optInTimePeriod);
        }
        case "NVI": {
            return core.NVI_Lookback();
        }
        case "OBV": {
            return core.OBV_Lookback();
        }
        case "PLUS_DI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.PLUS_DI_Lookback(optInTimePeriod);
        }
        case "PLUS_DM": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.PLUS_DM_Lookback(optInTimePeriod);
        }
        case "PPO": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.PPO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType);
        }
        case "PVI": {
            return core.PVI_Lookback();
        }
        case "PVO": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.PVO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType);
        }
        case "ROC": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.ROC_Lookback(optInTimePeriod);
        }
        case "ROCP": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.ROCP_Lookback(optInTimePeriod);
        }
        case "ROCR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.ROCR_Lookback(optInTimePeriod);
        }
        case "ROCR100": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.ROCR100_Lookback(optInTimePeriod);
        }
        case "RSI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.RSI_Lookback(optInTimePeriod);
        }
        case "SAR": {
            double optInAcceleration = GetDouble(p, "optInAcceleration", 0.0);
            double optInMaximum = GetDouble(p, "optInMaximum", 0.0);
            return core.SAR_Lookback(optInAcceleration, optInMaximum);
        }
        case "SAREXT": {
            double optInStartValue = GetDouble(p, "optInStartValue", 0.0);
            double optInOffsetOnReverse = GetDouble(p, "optInOffsetOnReverse", 0.0);
            double optInAccelerationInitLong = GetDouble(p, "optInAccelerationInitLong", 0.0);
            double optInAccelerationLong = GetDouble(p, "optInAccelerationLong", 0.0);
            double optInAccelerationMaxLong = GetDouble(p, "optInAccelerationMaxLong", 0.0);
            double optInAccelerationInitShort = GetDouble(p, "optInAccelerationInitShort", 0.0);
            double optInAccelerationShort = GetDouble(p, "optInAccelerationShort", 0.0);
            double optInAccelerationMaxShort = GetDouble(p, "optInAccelerationMaxShort", 0.0);
            return core.SAREXT_Lookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
        }
        case "SIN": {
            return core.SIN_Lookback();
        }
        case "SINH": {
            return core.SINH_Lookback();
        }
        case "SMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.SMA_Lookback(optInTimePeriod);
        }
        case "SQRT": {
            return core.SQRT_Lookback();
        }
        case "STDDEV": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            double optInNbDev = GetDouble(p, "optInNbDev", 0.0);
            return core.STDDEV_Lookback(optInTimePeriod, optInNbDev);
        }
        case "STOCH": {
            int optInFastK_Period = GetInt(p, "optInFastK_Period", 0);
            int optInSlowK_Period = GetInt(p, "optInSlowK_Period", 0);
            MAType optInSlowK_MAType = (MAType)GetInt(p, "optInSlowK_MAType", 0);
            int optInSlowD_Period = GetInt(p, "optInSlowD_Period", 0);
            MAType optInSlowD_MAType = (MAType)GetInt(p, "optInSlowD_MAType", 0);
            return core.STOCH_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
        }
        case "STOCHF": {
            int optInFastK_Period = GetInt(p, "optInFastK_Period", 0);
            int optInFastD_Period = GetInt(p, "optInFastD_Period", 0);
            MAType optInFastD_MAType = (MAType)GetInt(p, "optInFastD_MAType", 0);
            return core.STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
        }
        case "STOCHRSI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            int optInFastK_Period = GetInt(p, "optInFastK_Period", 0);
            int optInFastD_Period = GetInt(p, "optInFastD_Period", 0);
            MAType optInFastD_MAType = (MAType)GetInt(p, "optInFastD_MAType", 0);
            return core.STOCHRSI_Lookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
        }
        case "SUB": {
            return core.SUB_Lookback();
        }
        case "SUM": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.SUM_Lookback(optInTimePeriod);
        }
        case "T3": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            double optInVFactor = GetDouble(p, "optInVFactor", 0.0);
            return core.T3_Lookback(optInTimePeriod, optInVFactor);
        }
        case "TAN": {
            return core.TAN_Lookback();
        }
        case "TANH": {
            return core.TANH_Lookback();
        }
        case "TEMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.TEMA_Lookback(optInTimePeriod);
        }
        case "TRANGE": {
            return core.TRANGE_Lookback();
        }
        case "TRIMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.TRIMA_Lookback(optInTimePeriod);
        }
        case "TRIX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.TRIX_Lookback(optInTimePeriod);
        }
        case "TSF": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.TSF_Lookback(optInTimePeriod);
        }
        case "TYPPRICE": {
            return core.TYPPRICE_Lookback();
        }
        case "ULTOSC": {
            int optInTimePeriod1 = GetInt(p, "optInTimePeriod1", 0);
            int optInTimePeriod2 = GetInt(p, "optInTimePeriod2", 0);
            int optInTimePeriod3 = GetInt(p, "optInTimePeriod3", 0);
            return core.ULTOSC_Lookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
        }
        case "VAR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            double optInNbDev = GetDouble(p, "optInNbDev", 0.0);
            return core.VAR_Lookback(optInTimePeriod, optInNbDev);
        }
        case "VWMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.VWMA_Lookback(optInTimePeriod);
        }
        case "WCLPRICE": {
            return core.WCLPRICE_Lookback();
        }
        case "WILLR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.WILLR_Lookback(optInTimePeriod);
        }
        case "WMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.WMA_Lookback(optInTimePeriod);
        }
        default: return -1;
        }
    }

    static string Handle_ACCBANDS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        double[] outArr2 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ACCBANDS(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.ACCBANDS(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
                _h = SvHashF64(_h, outArr2, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
            sb.Append(",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ACOS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ACOS(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.ACOS(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_AD(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        double[] inVolume;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.AD(startIdx, endIdx, inHigh, inLow, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            var f_inVolume = new float[inVolume.Length];
            for (int _fi = 0; _fi < inVolume.Length; _fi++) f_inVolume[_fi] = (float)inVolume[_fi];
            rc = core.AD(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, f_inVolume, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ADD(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal0;
        double[] inReal1;
        if (use_preloaded != 0 && refN > 0) {
            inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
            inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
        } else {
            inReal0 = GetDoubleArray(p, "inReal0");
            inReal1 = GetDoubleArray(p, "inReal1");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ADD(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal0 = new float[inReal0.Length];
            for (int _fi = 0; _fi < inReal0.Length; _fi++) f_inReal0[_fi] = (float)inReal0[_fi];
            var f_inReal1 = new float[inReal1.Length];
            for (int _fi = 0; _fi < inReal1.Length; _fi++) f_inReal1[_fi] = (float)inReal1[_fi];
            rc = core.ADD(startIdx, endIdx, f_inReal0, f_inReal1, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ADOSC(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        double[] inVolume;
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
        int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
        int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ADOSC(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInFastPeriod, optInSlowPeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            var f_inVolume = new float[inVolume.Length];
            for (int _fi = 0; _fi < inVolume.Length; _fi++) f_inVolume[_fi] = (float)inVolume[_fi];
            rc = core.ADOSC(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, f_inVolume, optInFastPeriod, optInSlowPeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ADX(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["ADX"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ADX(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.ADX(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ADXR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ADXR(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.ADXR(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_APO(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
        int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
        MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.APO(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.APO(startIdx, endIdx, f_inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_AROON(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.AROON(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            rc = core.AROON(startIdx, endIdx, f_inHigh, f_inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_AROONOSC(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.AROONOSC(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            rc = core.AROONOSC(startIdx, endIdx, f_inHigh, f_inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ASIN(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ASIN(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.ASIN(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ATAN(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ATAN(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.ATAN(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ATR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["ATR"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ATR(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.ATR(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_AVGDEV(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.AVGDEV(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.AVGDEV(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_AVGPRICE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.AVGPRICE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.AVGPRICE(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_BBANDS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double optInNbDevUp = GetDouble(p, "optInNbDevUp", 0.0);
        double optInNbDevDn = GetDouble(p, "optInNbDevDn", 0.0);
        MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        double[] outArr2 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.BBANDS(startIdx, endIdx, inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.BBANDS(startIdx, endIdx, f_inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
                _h = SvHashF64(_h, outArr2, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
            sb.Append(",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_BETA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal0;
        double[] inReal1;
        if (use_preloaded != 0 && refN > 0) {
            inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
            inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
        } else {
            inReal0 = GetDoubleArray(p, "inReal0");
            inReal1 = GetDoubleArray(p, "inReal1");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.BETA(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal0 = new float[inReal0.Length];
            for (int _fi = 0; _fi < inReal0.Length; _fi++) f_inReal0[_fi] = (float)inReal0[_fi];
            var f_inReal1 = new float[inReal1.Length];
            for (int _fi = 0; _fi < inReal1.Length; _fi++) f_inReal1[_fi] = (float)inReal1[_fi];
            rc = core.BETA(startIdx, endIdx, f_inReal0, f_inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_BOP(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.BOP(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.BOP(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CCI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CCI(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CCI(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDL2CROWS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDL2CROWS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDL2CROWS(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDL3BLACKCROWS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDL3BLACKCROWS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDL3BLACKCROWS(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDL3INSIDE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDL3INSIDE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDL3INSIDE(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDL3LINESTRIKE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDL3LINESTRIKE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDL3LINESTRIKE(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDL3OUTSIDE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDL3OUTSIDE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDL3OUTSIDE(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDL3STARSINSOUTH(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDL3STARSINSOUTH(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDL3STARSINSOUTH(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDL3WHITESOLDIERS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDL3WHITESOLDIERS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDL3WHITESOLDIERS(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLABANDONEDBABY(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLABANDONEDBABY(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLABANDONEDBABY(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLADVANCEBLOCK(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLADVANCEBLOCK(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLADVANCEBLOCK(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLBELTHOLD(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLBELTHOLD(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLBELTHOLD(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLBREAKAWAY(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLBREAKAWAY(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLBREAKAWAY(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLCLOSINGMARUBOZU(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLCLOSINGMARUBOZU(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLCLOSINGMARUBOZU(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLCONCEALBABYSWALL(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLCONCEALBABYSWALL(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLCONCEALBABYSWALL(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLCOUNTERATTACK(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLCOUNTERATTACK(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLCOUNTERATTACK(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLDARKCLOUDCOVER(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLDARKCLOUDCOVER(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLDARKCLOUDCOVER(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLDOJI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLDOJI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLDOJI(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLDOJISTAR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLDOJISTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLDOJISTAR(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLDRAGONFLYDOJI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLDRAGONFLYDOJI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLDRAGONFLYDOJI(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLENGULFING(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLENGULFING(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLENGULFING(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLEVENINGDOJISTAR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLEVENINGDOJISTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLEVENINGDOJISTAR(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLEVENINGSTAR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLEVENINGSTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLEVENINGSTAR(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLGAPSIDESIDEWHITE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLGAPSIDESIDEWHITE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLGAPSIDESIDEWHITE(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLGRAVESTONEDOJI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLGRAVESTONEDOJI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLGRAVESTONEDOJI(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLHAMMER(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLHAMMER(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLHAMMER(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLHANGINGMAN(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLHANGINGMAN(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLHANGINGMAN(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLHARAMI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLHARAMI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLHARAMI(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLHARAMICROSS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLHARAMICROSS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLHARAMICROSS(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLHIGHWAVE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLHIGHWAVE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLHIGHWAVE(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLHIKKAKE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLHIKKAKE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLHIKKAKE(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLHIKKAKEMOD(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLHIKKAKEMOD(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLHIKKAKEMOD(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLHOMINGPIGEON(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLHOMINGPIGEON(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLHOMINGPIGEON(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLIDENTICAL3CROWS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLIDENTICAL3CROWS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLIDENTICAL3CROWS(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLINNECK(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLINNECK(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLINNECK(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLINVERTEDHAMMER(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLINVERTEDHAMMER(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLINVERTEDHAMMER(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLKICKING(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLKICKING(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLKICKING(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLKICKINGBYLENGTH(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLKICKINGBYLENGTH(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLKICKINGBYLENGTH(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLLADDERBOTTOM(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLLADDERBOTTOM(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLLADDERBOTTOM(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLLONGLEGGEDDOJI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLLONGLEGGEDDOJI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLLONGLEGGEDDOJI(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLLONGLINE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLLONGLINE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLLONGLINE(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLMARUBOZU(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLMARUBOZU(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLMARUBOZU(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLMATCHINGLOW(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLMATCHINGLOW(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLMATCHINGLOW(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLMATHOLD(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLMATHOLD(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLMATHOLD(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLMORNINGDOJISTAR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLMORNINGDOJISTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLMORNINGDOJISTAR(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLMORNINGSTAR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLMORNINGSTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLMORNINGSTAR(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLONNECK(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLONNECK(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLONNECK(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLPIERCING(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLPIERCING(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLPIERCING(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLRICKSHAWMAN(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLRICKSHAWMAN(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLRICKSHAWMAN(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLRISEFALL3METHODS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLRISEFALL3METHODS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLRISEFALL3METHODS(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLSEPARATINGLINES(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLSEPARATINGLINES(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLSEPARATINGLINES(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLSHOOTINGSTAR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLSHOOTINGSTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLSHOOTINGSTAR(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLSHORTLINE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLSHORTLINE(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLSHORTLINE(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLSPINNINGTOP(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLSPINNINGTOP(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLSPINNINGTOP(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLSTALLEDPATTERN(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLSTALLEDPATTERN(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLSTALLEDPATTERN(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLSTICKSANDWICH(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLSTICKSANDWICH(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLSTICKSANDWICH(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLTAKURI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLTAKURI(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLTAKURI(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLTASUKIGAP(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLTASUKIGAP(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLTASUKIGAP(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLTHRUSTING(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLTHRUSTING(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLTHRUSTING(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLTRISTAR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLTRISTAR(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLTRISTAR(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLUNIQUE3RIVER(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLUNIQUE3RIVER(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLUNIQUE3RIVER(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLUPSIDEGAP2CROWS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLUPSIDEGAP2CROWS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLUPSIDEGAP2CROWS(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CDLXSIDEGAP3METHODS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CDLXSIDEGAP3METHODS(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.CDLXSIDEGAP3METHODS(startIdx, endIdx, f_inOpen, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CEIL(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CEIL(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.CEIL(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CMF(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        double[] inVolume;
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
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CMF(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            var f_inVolume = new float[inVolume.Length];
            for (int _fi = 0; _fi < inVolume.Length; _fi++) f_inVolume[_fi] = (float)inVolume[_fi];
            rc = core.CMF(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, f_inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CMO(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["CMO"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CMO(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.CMO(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CMOU(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CMOU(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.CMOU(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_CORREL(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal0;
        double[] inReal1;
        if (use_preloaded != 0 && refN > 0) {
            inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
            inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
        } else {
            inReal0 = GetDoubleArray(p, "inReal0");
            inReal1 = GetDoubleArray(p, "inReal1");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.CORREL(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal0 = new float[inReal0.Length];
            for (int _fi = 0; _fi < inReal0.Length; _fi++) f_inReal0[_fi] = (float)inReal0[_fi];
            var f_inReal1 = new float[inReal1.Length];
            for (int _fi = 0; _fi < inReal1.Length; _fi++) f_inReal1[_fi] = (float)inReal1[_fi];
            rc = core.CORREL(startIdx, endIdx, f_inReal0, f_inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_COS(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.COS(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.COS(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_COSH(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.COSH(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.COSH(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_DEMA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.DEMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.DEMA(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_DIV(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal0;
        double[] inReal1;
        if (use_preloaded != 0 && refN > 0) {
            inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
            inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
        } else {
            inReal0 = GetDoubleArray(p, "inReal0");
            inReal1 = GetDoubleArray(p, "inReal1");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.DIV(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal0 = new float[inReal0.Length];
            for (int _fi = 0; _fi < inReal0.Length; _fi++) f_inReal0[_fi] = (float)inReal0[_fi];
            var f_inReal1 = new float[inReal1.Length];
            for (int _fi = 0; _fi < inReal1.Length; _fi++) f_inReal1[_fi] = (float)inReal1[_fi];
            rc = core.DIV(startIdx, endIdx, f_inReal0, f_inReal1, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_DX(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["DX"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.DX(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.DX(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_EMA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["EMA"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.EMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.EMA(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_EXP(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.EXP(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.EXP(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_FLOOR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.FLOOR(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.FLOOR(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_HMA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.HMA(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_HT_DCPERIOD(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        core.unstablePeriod[(int)FunctionCatalog.Default["HT_DCPERIOD"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HT_DCPERIOD(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.HT_DCPERIOD(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_HT_DCPHASE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        core.unstablePeriod[(int)FunctionCatalog.Default["HT_DCPHASE"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HT_DCPHASE(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.HT_DCPHASE(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_HT_PHASOR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        core.unstablePeriod[(int)FunctionCatalog.Default["HT_PHASOR"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HT_PHASOR(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.HT_PHASOR(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_HT_SINE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        core.unstablePeriod[(int)FunctionCatalog.Default["HT_SINE"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HT_SINE(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.HT_SINE(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_HT_TRENDLINE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        core.unstablePeriod[(int)FunctionCatalog.Default["HT_TRENDLINE"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HT_TRENDLINE(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.HT_TRENDLINE(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_HT_TRENDMODE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        core.unstablePeriod[(int)FunctionCatalog.Default["HT_TRENDMODE"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HT_TRENDMODE(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.HT_TRENDMODE(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_IMI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inOpen;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inOpen = new double[refN]; Array.Copy(refOpen, inOpen, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inOpen = GetDoubleArray(p, "inOpen");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.IMI(startIdx, endIdx, inOpen, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inOpen = new float[inOpen.Length];
            for (int _fi = 0; _fi < inOpen.Length; _fi++) f_inOpen[_fi] = (float)inOpen[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.IMI(startIdx, endIdx, f_inOpen, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_KAMA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["KAMA"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.KAMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.KAMA(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_LINEARREG(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.LINEARREG(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.LINEARREG(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_LINEARREG_ANGLE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.LINEARREG_ANGLE(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.LINEARREG_ANGLE(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_LINEARREG_INTERCEPT(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.LINEARREG_INTERCEPT(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.LINEARREG_INTERCEPT(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_LINEARREG_SLOPE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.LINEARREG_SLOPE(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.LINEARREG_SLOPE(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_LN(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.LN(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.LN(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_LOG10(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.LOG10(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.LOG10(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MA(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MA(startIdx, endIdx, f_inReal, optInTimePeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MACD(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
        int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
        int optInSignalPeriod = GetInt(p, "optInSignalPeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        double[] outArr2 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MACD(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MACD(startIdx, endIdx, f_inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
                _h = SvHashF64(_h, outArr2, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
            sb.Append(",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MACDEXT(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
        MAType optInFastMAType = (MAType)GetInt(p, "optInFastMAType", 0);
        int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
        MAType optInSlowMAType = (MAType)GetInt(p, "optInSlowMAType", 0);
        int optInSignalPeriod = GetInt(p, "optInSignalPeriod", 0);
        MAType optInSignalMAType = (MAType)GetInt(p, "optInSignalMAType", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        double[] outArr2 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MACDEXT(startIdx, endIdx, inReal, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MACDEXT(startIdx, endIdx, f_inReal, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
                _h = SvHashF64(_h, outArr2, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
            sb.Append(",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MACDFIX(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInSignalPeriod = GetInt(p, "optInSignalPeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        double[] outArr2 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MACDFIX(startIdx, endIdx, inReal, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MACDFIX(startIdx, endIdx, f_inReal, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
                _h = SvHashF64(_h, outArr2, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
            sb.Append(",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MAMA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double optInFastLimit = GetDouble(p, "optInFastLimit", 0.0);
        double optInSlowLimit = GetDouble(p, "optInSlowLimit", 0.0);
        core.unstablePeriod[(int)FunctionCatalog.Default["MAMA"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MAMA(startIdx, endIdx, inReal, optInFastLimit, optInSlowLimit, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MAMA(startIdx, endIdx, f_inReal, optInFastLimit, optInSlowLimit, out outBegIdx, out outNBElement, outArr0, outArr1);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MAVP(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal0;
        double[] inReal1;
        if (use_preloaded != 0 && refN > 0) {
            inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
            inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
        } else {
            inReal0 = GetDoubleArray(p, "inReal0");
            inReal1 = GetDoubleArray(p, "inReal1");
        }
        int optInMinPeriod = GetInt(p, "optInMinPeriod", 0);
        int optInMaxPeriod = GetInt(p, "optInMaxPeriod", 0);
        MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MAVP(startIdx, endIdx, inReal0, inReal1, optInMinPeriod, optInMaxPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal0 = new float[inReal0.Length];
            for (int _fi = 0; _fi < inReal0.Length; _fi++) f_inReal0[_fi] = (float)inReal0[_fi];
            var f_inReal1 = new float[inReal1.Length];
            for (int _fi = 0; _fi < inReal1.Length; _fi++) f_inReal1[_fi] = (float)inReal1[_fi];
            rc = core.MAVP(startIdx, endIdx, f_inReal0, f_inReal1, optInMinPeriod, optInMaxPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MAX(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MAX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MAX(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MAXINDEX(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MAXINDEX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MAXINDEX(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MEDPRICE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MEDPRICE(startIdx, endIdx, inHigh, inLow, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            rc = core.MEDPRICE(startIdx, endIdx, f_inHigh, f_inLow, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MFI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        double[] inVolume;
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
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MFI(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            var f_inVolume = new float[inVolume.Length];
            for (int _fi = 0; _fi < inVolume.Length; _fi++) f_inVolume[_fi] = (float)inVolume[_fi];
            rc = core.MFI(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, f_inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MIDPOINT(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MIDPOINT(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MIDPOINT(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MIDPRICE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MIDPRICE(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            rc = core.MIDPRICE(startIdx, endIdx, f_inHigh, f_inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MIN(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MIN(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MIN(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MININDEX(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MININDEX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MININDEX(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MINMAX(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MINMAX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MINMAX(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MINMAXINDEX(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        int[] outArr0 = new int[n];
        int[] outArr1 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MINMAXINDEX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MINMAXINDEX(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
                _h = SvHashI32(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
            sb.Append(",\"outInteger1\":"); sb.Append(FormatIntArray(outArr1, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MINUS_DI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["MINUS_DI"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MINUS_DI(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.MINUS_DI(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MINUS_DM(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["MINUS_DM"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MINUS_DM(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            rc = core.MINUS_DM(startIdx, endIdx, f_inHigh, f_inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MOM(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MOM(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.MOM(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_MULT(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal0;
        double[] inReal1;
        if (use_preloaded != 0 && refN > 0) {
            inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
            inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
        } else {
            inReal0 = GetDoubleArray(p, "inReal0");
            inReal1 = GetDoubleArray(p, "inReal1");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MULT(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal0 = new float[inReal0.Length];
            for (int _fi = 0; _fi < inReal0.Length; _fi++) f_inReal0[_fi] = (float)inReal0[_fi];
            var f_inReal1 = new float[inReal1.Length];
            for (int _fi = 0; _fi < inReal1.Length; _fi++) f_inReal1[_fi] = (float)inReal1[_fi];
            rc = core.MULT(startIdx, endIdx, f_inReal0, f_inReal1, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_NATR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["NATR"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.NATR(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.NATR(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_NVI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inClose;
        double[] inVolume;
        if (use_preloaded != 0 && refN > 0) {
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
            inVolume = new double[refN]; Array.Copy(refVolume, inVolume, refN);
        } else {
            inClose = GetDoubleArray(p, "inClose");
            inVolume = GetDoubleArray(p, "inVolume");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.NVI(startIdx, endIdx, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            var f_inVolume = new float[inVolume.Length];
            for (int _fi = 0; _fi < inVolume.Length; _fi++) f_inVolume[_fi] = (float)inVolume[_fi];
            rc = core.NVI(startIdx, endIdx, f_inClose, f_inVolume, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_OBV(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        double[] inVolume;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
            inVolume = new double[refN]; Array.Copy(refVolume, inVolume, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
            inVolume = GetDoubleArray(p, "inVolume");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.OBV(startIdx, endIdx, inReal, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            var f_inVolume = new float[inVolume.Length];
            for (int _fi = 0; _fi < inVolume.Length; _fi++) f_inVolume[_fi] = (float)inVolume[_fi];
            rc = core.OBV(startIdx, endIdx, f_inReal, f_inVolume, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_PLUS_DI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["PLUS_DI"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.PLUS_DI(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.PLUS_DI(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_PLUS_DM(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["PLUS_DM"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.PLUS_DM(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            rc = core.PLUS_DM(startIdx, endIdx, f_inHigh, f_inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_PPO(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
        int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
        MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.PPO(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.PPO(startIdx, endIdx, f_inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_PVI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inClose;
        double[] inVolume;
        if (use_preloaded != 0 && refN > 0) {
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
            inVolume = new double[refN]; Array.Copy(refVolume, inVolume, refN);
        } else {
            inClose = GetDoubleArray(p, "inClose");
            inVolume = GetDoubleArray(p, "inVolume");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.PVI(startIdx, endIdx, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            var f_inVolume = new float[inVolume.Length];
            for (int _fi = 0; _fi < inVolume.Length; _fi++) f_inVolume[_fi] = (float)inVolume[_fi];
            rc = core.PVI(startIdx, endIdx, f_inClose, f_inVolume, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_PVO(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inVolume;
        if (use_preloaded != 0 && refN > 0) {
            inVolume = new double[refN]; Array.Copy(refVolume, inVolume, refN);
        } else {
            inVolume = GetDoubleArray(p, "inVolume");
        }
        int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
        int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
        MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.PVO(startIdx, endIdx, inVolume, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inVolume = new float[inVolume.Length];
            for (int _fi = 0; _fi < inVolume.Length; _fi++) f_inVolume[_fi] = (float)inVolume[_fi];
            rc = core.PVO(startIdx, endIdx, f_inVolume, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ROC(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ROC(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.ROC(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ROCP(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ROCP(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.ROCP(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ROCR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ROCR(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.ROCR(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ROCR100(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ROCR100(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.ROCR100(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_RSI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        core.unstablePeriod[(int)FunctionCatalog.Default["RSI"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.RSI(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.RSI(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_SAR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
        }
        double optInAcceleration = GetDouble(p, "optInAcceleration", 0.0);
        double optInMaximum = GetDouble(p, "optInMaximum", 0.0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.SAR(startIdx, endIdx, inHigh, inLow, optInAcceleration, optInMaximum, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            rc = core.SAR(startIdx, endIdx, f_inHigh, f_inLow, optInAcceleration, optInMaximum, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_SAREXT(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
        }
        double optInStartValue = GetDouble(p, "optInStartValue", 0.0);
        double optInOffsetOnReverse = GetDouble(p, "optInOffsetOnReverse", 0.0);
        double optInAccelerationInitLong = GetDouble(p, "optInAccelerationInitLong", 0.0);
        double optInAccelerationLong = GetDouble(p, "optInAccelerationLong", 0.0);
        double optInAccelerationMaxLong = GetDouble(p, "optInAccelerationMaxLong", 0.0);
        double optInAccelerationInitShort = GetDouble(p, "optInAccelerationInitShort", 0.0);
        double optInAccelerationShort = GetDouble(p, "optInAccelerationShort", 0.0);
        double optInAccelerationMaxShort = GetDouble(p, "optInAccelerationMaxShort", 0.0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.SAREXT(startIdx, endIdx, inHigh, inLow, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            rc = core.SAREXT(startIdx, endIdx, f_inHigh, f_inLow, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_SIN(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.SIN(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.SIN(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_SINH(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.SINH(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.SINH(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_SMA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.SMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.SMA(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_SQRT(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.SQRT(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.SQRT(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_STDDEV(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double optInNbDev = GetDouble(p, "optInNbDev", 0.0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.STDDEV(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.STDDEV(startIdx, endIdx, f_inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_STOCH(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInFastK_Period = GetInt(p, "optInFastK_Period", 0);
        int optInSlowK_Period = GetInt(p, "optInSlowK_Period", 0);
        MAType optInSlowK_MAType = (MAType)GetInt(p, "optInSlowK_MAType", 0);
        int optInSlowD_Period = GetInt(p, "optInSlowD_Period", 0);
        MAType optInSlowD_MAType = (MAType)GetInt(p, "optInSlowD_MAType", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.STOCH(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.STOCH(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_STOCHF(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInFastK_Period = GetInt(p, "optInFastK_Period", 0);
        int optInFastD_Period = GetInt(p, "optInFastD_Period", 0);
        MAType optInFastD_MAType = (MAType)GetInt(p, "optInFastD_MAType", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.STOCHF(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.STOCHF(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_STOCHRSI(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        int optInFastK_Period = GetInt(p, "optInFastK_Period", 0);
        int optInFastD_Period = GetInt(p, "optInFastD_Period", 0);
        MAType optInFastD_MAType = (MAType)GetInt(p, "optInFastD_MAType", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.STOCHRSI(startIdx, endIdx, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.STOCHRSI(startIdx, endIdx, f_inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_SUB(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal0;
        double[] inReal1;
        if (use_preloaded != 0 && refN > 0) {
            inReal0 = new double[refN]; Array.Copy(refClose, inReal0, refN);
            inReal1 = new double[refN]; Array.Copy(refHigh, inReal1, refN);
        } else {
            inReal0 = GetDoubleArray(p, "inReal0");
            inReal1 = GetDoubleArray(p, "inReal1");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.SUB(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal0 = new float[inReal0.Length];
            for (int _fi = 0; _fi < inReal0.Length; _fi++) f_inReal0[_fi] = (float)inReal0[_fi];
            var f_inReal1 = new float[inReal1.Length];
            for (int _fi = 0; _fi < inReal1.Length; _fi++) f_inReal1[_fi] = (float)inReal1[_fi];
            rc = core.SUB(startIdx, endIdx, f_inReal0, f_inReal1, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_SUM(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.SUM(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.SUM(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_T3(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double optInVFactor = GetDouble(p, "optInVFactor", 0.0);
        core.unstablePeriod[(int)FunctionCatalog.Default["T3"].UnstableId!.Value] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.T3(startIdx, endIdx, inReal, optInTimePeriod, optInVFactor, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.T3(startIdx, endIdx, f_inReal, optInTimePeriod, optInVFactor, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_TAN(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.TAN(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.TAN(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_TANH(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.TANH(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.TANH(startIdx, endIdx, f_inReal, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_TEMA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.TEMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.TEMA(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_TRANGE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.TRANGE(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.TRANGE(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_TRIMA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.TRIMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.TRIMA(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_TRIX(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.TRIX(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.TRIX(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_TSF(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.TSF(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.TSF(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_TYPPRICE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.TYPPRICE(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.TYPPRICE(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_ULTOSC(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod1 = GetInt(p, "optInTimePeriod1", 0);
        int optInTimePeriod2 = GetInt(p, "optInTimePeriod2", 0);
        int optInTimePeriod3 = GetInt(p, "optInTimePeriod3", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.ULTOSC(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.ULTOSC(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_VAR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double optInNbDev = GetDouble(p, "optInNbDev", 0.0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.VAR(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.VAR(startIdx, endIdx, f_inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_VWMA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        double[] inVolume;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
            inVolume = new double[refN]; Array.Copy(refVolume, inVolume, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
            inVolume = GetDoubleArray(p, "inVolume");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.VWMA(startIdx, endIdx, inReal, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            var f_inVolume = new float[inVolume.Length];
            for (int _fi = 0; _fi < inVolume.Length; _fi++) f_inVolume[_fi] = (float)inVolume[_fi];
            rc = core.VWMA(startIdx, endIdx, f_inReal, f_inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_WCLPRICE(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
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
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.WCLPRICE(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.WCLPRICE(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_WILLR(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inHigh;
        double[] inLow;
        double[] inClose;
        if (use_preloaded != 0 && refN > 0) {
            inHigh = new double[refN]; Array.Copy(refHigh, inHigh, refN);
            inLow = new double[refN]; Array.Copy(refLow, inLow, refN);
            inClose = new double[refN]; Array.Copy(refClose, inClose, refN);
        } else {
            inHigh = GetDoubleArray(p, "inHigh");
            inLow = GetDoubleArray(p, "inLow");
            inClose = GetDoubleArray(p, "inClose");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.WILLR(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inHigh = new float[inHigh.Length];
            for (int _fi = 0; _fi < inHigh.Length; _fi++) f_inHigh[_fi] = (float)inHigh[_fi];
            var f_inLow = new float[inLow.Length];
            for (int _fi = 0; _fi < inLow.Length; _fi++) f_inLow[_fi] = (float)inLow[_fi];
            var f_inClose = new float[inClose.Length];
            for (int _fi = 0; _fi < inClose.Length; _fi++) f_inClose[_fi] = (float)inClose[_fi];
            rc = core.WILLR(startIdx, endIdx, f_inHigh, f_inLow, f_inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static string Handle_WMA(JsonElement p, int startIdx, int endIdx) {
        int n = endIdx - startIdx + 1;
        int use_preloaded = GetInt(p, "use_preloaded", 0);
        int bench_iters = GetInt(p, "iters", 1);
        if (bench_iters < 1) bench_iters = 1;
        double[] inReal;
        if (use_preloaded != 0 && refN > 0) {
            inReal = new double[refN]; Array.Copy(refClose, inReal, refN);
        } else {
            inReal = GetDoubleArray(p, "inReal");
        }
        int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.WMA(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        int usedFloat = 0;
        if (GetInt(p, "use_float", 0) != 0) {
            var f_inReal = new float[inReal.Length];
            for (int _fi = 0; _fi < inReal.Length; _fi++) f_inReal[_fi] = (float)inReal[_fi];
            rc = core.WMA(startIdx, endIdx, f_inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
            usedFloat = 1;
        }
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"used_float\":{usedFloat}");
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append("}");
        return sb.ToString();
    }

    static void Main(string[] args) {
        string? line;
        while ((line = Console.ReadLine()) != null) {
            if (string.IsNullOrWhiteSpace(line)) continue;
            string reply;
            try { reply = HandleRequest(line); }
            catch (Exception e) {
                reply = "{\"error\":" + AbsStr(e.GetType().Name + ": " + e.Message) + "}";
            }
            Console.WriteLine(reply);
            Console.Out.Flush();
        }
    }
}
