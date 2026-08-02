// Auto-generated JSON-RPC server for ta_codegen C# output (managed).
// The csproj compiles the shipped library sources from ../library — the
// server's Core IS the shipped partial class, not a copy.
using System;
using System.Text.Json;
using System.Diagnostics;
using TALib;

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
        var p = root.GetProperty("params");

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
                if (id == (int)FuncUnstId.All) {
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
            else {
                return $"{{\"error\":\"Unknown method: {method}\"}}";
            }
    }

    static long ComputeLookback(string funcName, JsonElement p) {
        switch (funcName) {
        case "ACCBANDS": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.AccbandsLookback(optInTimePeriod);
        }
        case "ACOS": {
            return core.AcosLookback();
        }
        case "AD": {
            return core.AdLookback();
        }
        case "ADD": {
            return core.AddLookback();
        }
        case "ADOSC": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            return core.AdOscLookback(optInFastPeriod, optInSlowPeriod);
        }
        case "ADX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.AdxLookback(optInTimePeriod);
        }
        case "ADXR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.AdxrLookback(optInTimePeriod);
        }
        case "APO": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.ApoLookback(optInFastPeriod, optInSlowPeriod, optInMAType);
        }
        case "AROON": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.AroonLookback(optInTimePeriod);
        }
        case "AROONOSC": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.AroonOscLookback(optInTimePeriod);
        }
        case "ASIN": {
            return core.AsinLookback();
        }
        case "ATAN": {
            return core.AtanLookback();
        }
        case "ATR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.AtrLookback(optInTimePeriod);
        }
        case "AVGDEV": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.AvgDevLookback(optInTimePeriod);
        }
        case "AVGPRICE": {
            return core.AvgPriceLookback();
        }
        case "BBANDS": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            double optInNbDevUp = GetDouble(p, "optInNbDevUp", 0.0);
            double optInNbDevDn = GetDouble(p, "optInNbDevDn", 0.0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.BbandsLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
        }
        case "BETA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.BetaLookback(optInTimePeriod);
        }
        case "BOP": {
            return core.BopLookback();
        }
        case "CCI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.CciLookback(optInTimePeriod);
        }
        case "CDL2CROWS": {
            return core.Cdl2CrowsLookback();
        }
        case "CDL3BLACKCROWS": {
            return core.Cdl3BlackCrowsLookback();
        }
        case "CDL3INSIDE": {
            return core.Cdl3InsideLookback();
        }
        case "CDL3LINESTRIKE": {
            return core.Cdl3LineStrikeLookback();
        }
        case "CDL3OUTSIDE": {
            return core.Cdl3OutsideLookback();
        }
        case "CDL3STARSINSOUTH": {
            return core.Cdl3StarsInSouthLookback();
        }
        case "CDL3WHITESOLDIERS": {
            return core.Cdl3WhiteSoldiersLookback();
        }
        case "CDLABANDONEDBABY": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CdlAbandonedBabyLookback(optInPenetration);
        }
        case "CDLADVANCEBLOCK": {
            return core.CdlAdvanceBlockLookback();
        }
        case "CDLBELTHOLD": {
            return core.CdlBeltHoldLookback();
        }
        case "CDLBREAKAWAY": {
            return core.CdlBreakawayLookback();
        }
        case "CDLCLOSINGMARUBOZU": {
            return core.CdlClosingMarubozuLookback();
        }
        case "CDLCONCEALBABYSWALL": {
            return core.CdlConcealBabysWallLookback();
        }
        case "CDLCOUNTERATTACK": {
            return core.CdlCounterAttackLookback();
        }
        case "CDLDARKCLOUDCOVER": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CdlDarkCloudCoverLookback(optInPenetration);
        }
        case "CDLDOJI": {
            return core.CdlDojiLookback();
        }
        case "CDLDOJISTAR": {
            return core.CdlDojiStarLookback();
        }
        case "CDLDRAGONFLYDOJI": {
            return core.CdlDragonflyDojiLookback();
        }
        case "CDLENGULFING": {
            return core.CdlEngulfingLookback();
        }
        case "CDLEVENINGDOJISTAR": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CdlEveningDojiStarLookback(optInPenetration);
        }
        case "CDLEVENINGSTAR": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CdlEveningStarLookback(optInPenetration);
        }
        case "CDLGAPSIDESIDEWHITE": {
            return core.CdlGapSideSideWhiteLookback();
        }
        case "CDLGRAVESTONEDOJI": {
            return core.CdlGravestoneDojiLookback();
        }
        case "CDLHAMMER": {
            return core.CdlHammerLookback();
        }
        case "CDLHANGINGMAN": {
            return core.CdlHangingManLookback();
        }
        case "CDLHARAMI": {
            return core.CdlHaramiLookback();
        }
        case "CDLHARAMICROSS": {
            return core.CdlHaramiCrossLookback();
        }
        case "CDLHIGHWAVE": {
            return core.CdlHignWaveLookback();
        }
        case "CDLHIKKAKE": {
            return core.CdlHikkakeLookback();
        }
        case "CDLHIKKAKEMOD": {
            return core.CdlHikkakeModLookback();
        }
        case "CDLHOMINGPIGEON": {
            return core.CdlHomingPigeonLookback();
        }
        case "CDLIDENTICAL3CROWS": {
            return core.CdlIdentical3CrowsLookback();
        }
        case "CDLINNECK": {
            return core.CdlInNeckLookback();
        }
        case "CDLINVERTEDHAMMER": {
            return core.CdlInvertedHammerLookback();
        }
        case "CDLKICKING": {
            return core.CdlKickingLookback();
        }
        case "CDLKICKINGBYLENGTH": {
            return core.CdlKickingByLengthLookback();
        }
        case "CDLLADDERBOTTOM": {
            return core.CdlLadderBottomLookback();
        }
        case "CDLLONGLEGGEDDOJI": {
            return core.CdlLongLeggedDojiLookback();
        }
        case "CDLLONGLINE": {
            return core.CdlLongLineLookback();
        }
        case "CDLMARUBOZU": {
            return core.CdlMarubozuLookback();
        }
        case "CDLMATCHINGLOW": {
            return core.CdlMatchingLowLookback();
        }
        case "CDLMATHOLD": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CdlMatHoldLookback(optInPenetration);
        }
        case "CDLMORNINGDOJISTAR": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CdlMorningDojiStarLookback(optInPenetration);
        }
        case "CDLMORNINGSTAR": {
            double optInPenetration = GetDouble(p, "optInPenetration", 0.0);
            return core.CdlMorningStarLookback(optInPenetration);
        }
        case "CDLONNECK": {
            return core.CdlOnNeckLookback();
        }
        case "CDLPIERCING": {
            return core.CdlPiercingLookback();
        }
        case "CDLRICKSHAWMAN": {
            return core.CdlRickshawManLookback();
        }
        case "CDLRISEFALL3METHODS": {
            return core.CdlRiseFall3MethodsLookback();
        }
        case "CDLSEPARATINGLINES": {
            return core.CdlSeperatingLinesLookback();
        }
        case "CDLSHOOTINGSTAR": {
            return core.CdlShootingStarLookback();
        }
        case "CDLSHORTLINE": {
            return core.CdlShortLineLookback();
        }
        case "CDLSPINNINGTOP": {
            return core.CdlSpinningTopLookback();
        }
        case "CDLSTALLEDPATTERN": {
            return core.CdlStalledPatternLookback();
        }
        case "CDLSTICKSANDWICH": {
            return core.CdlStickSandwichLookback();
        }
        case "CDLTAKURI": {
            return core.CdlTakuriLookback();
        }
        case "CDLTASUKIGAP": {
            return core.CdlTasukiGapLookback();
        }
        case "CDLTHRUSTING": {
            return core.CdlThrustingLookback();
        }
        case "CDLTRISTAR": {
            return core.CdlTristarLookback();
        }
        case "CDLUNIQUE3RIVER": {
            return core.CdlUnique3RiverLookback();
        }
        case "CDLUPSIDEGAP2CROWS": {
            return core.CdlUpsideGap2CrowsLookback();
        }
        case "CDLXSIDEGAP3METHODS": {
            return core.CdlXSideGap3MethodsLookback();
        }
        case "CEIL": {
            return core.CeilLookback();
        }
        case "CMF": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.CmfLookback(optInTimePeriod);
        }
        case "CMO": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.CmoLookback(optInTimePeriod);
        }
        case "CMOU": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.CmouLookback(optInTimePeriod);
        }
        case "CORREL": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.CorrelLookback(optInTimePeriod);
        }
        case "COS": {
            return core.CosLookback();
        }
        case "COSH": {
            return core.CoshLookback();
        }
        case "DEMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.DemaLookback(optInTimePeriod);
        }
        case "DIV": {
            return core.DivLookback();
        }
        case "DX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.DxLookback(optInTimePeriod);
        }
        case "EMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.EmaLookback(optInTimePeriod);
        }
        case "EXP": {
            return core.ExpLookback();
        }
        case "FLOOR": {
            return core.FloorLookback();
        }
        case "HMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.HmaLookback(optInTimePeriod);
        }
        case "HT_DCPERIOD": {
            return core.HtDcPeriodLookback();
        }
        case "HT_DCPHASE": {
            return core.HtDcPhaseLookback();
        }
        case "HT_PHASOR": {
            return core.HtPhasorLookback();
        }
        case "HT_SINE": {
            return core.HtSineLookback();
        }
        case "HT_TRENDLINE": {
            return core.HtTrendlineLookback();
        }
        case "HT_TRENDMODE": {
            return core.HtTrendModeLookback();
        }
        case "IMI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.ImiLookback(optInTimePeriod);
        }
        case "KAMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.KamaLookback(optInTimePeriod);
        }
        case "LINEARREG": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.LinearRegLookback(optInTimePeriod);
        }
        case "LINEARREG_ANGLE": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.LinearRegAngleLookback(optInTimePeriod);
        }
        case "LINEARREG_INTERCEPT": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.LinearRegInterceptLookback(optInTimePeriod);
        }
        case "LINEARREG_SLOPE": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.LinearRegSlopeLookback(optInTimePeriod);
        }
        case "LN": {
            return core.LnLookback();
        }
        case "LOG10": {
            return core.Log10Lookback();
        }
        case "MA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.MovingAverageLookback(optInTimePeriod, optInMAType);
        }
        case "MACD": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            int optInSignalPeriod = GetInt(p, "optInSignalPeriod", 0);
            return core.MacdLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
        }
        case "MACDEXT": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            MAType optInFastMAType = (MAType)GetInt(p, "optInFastMAType", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            MAType optInSlowMAType = (MAType)GetInt(p, "optInSlowMAType", 0);
            int optInSignalPeriod = GetInt(p, "optInSignalPeriod", 0);
            MAType optInSignalMAType = (MAType)GetInt(p, "optInSignalMAType", 0);
            return core.MacdExtLookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
        }
        case "MACDFIX": {
            int optInSignalPeriod = GetInt(p, "optInSignalPeriod", 0);
            return core.MacdFixLookback(optInSignalPeriod);
        }
        case "MAMA": {
            double optInFastLimit = GetDouble(p, "optInFastLimit", 0.0);
            double optInSlowLimit = GetDouble(p, "optInSlowLimit", 0.0);
            return core.MamaLookback(optInFastLimit, optInSlowLimit);
        }
        case "MAVP": {
            int optInMinPeriod = GetInt(p, "optInMinPeriod", 0);
            int optInMaxPeriod = GetInt(p, "optInMaxPeriod", 0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.MovingAverageVariablePeriodLookback(optInMinPeriod, optInMaxPeriod, optInMAType);
        }
        case "MAX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MaxLookback(optInTimePeriod);
        }
        case "MAXINDEX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MaxIndexLookback(optInTimePeriod);
        }
        case "MEDPRICE": {
            return core.MedPriceLookback();
        }
        case "MFI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MfiLookback(optInTimePeriod);
        }
        case "MIDPOINT": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MidPointLookback(optInTimePeriod);
        }
        case "MIDPRICE": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MidPriceLookback(optInTimePeriod);
        }
        case "MIN": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MinLookback(optInTimePeriod);
        }
        case "MININDEX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MinIndexLookback(optInTimePeriod);
        }
        case "MINMAX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MinMaxLookback(optInTimePeriod);
        }
        case "MINMAXINDEX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MinMaxIndexLookback(optInTimePeriod);
        }
        case "MINUS_DI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MinusDILookback(optInTimePeriod);
        }
        case "MINUS_DM": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MinusDMLookback(optInTimePeriod);
        }
        case "MOM": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.MomLookback(optInTimePeriod);
        }
        case "MULT": {
            return core.MultLookback();
        }
        case "NATR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.NatrLookback(optInTimePeriod);
        }
        case "NVI": {
            return core.NviLookback();
        }
        case "OBV": {
            return core.ObvLookback();
        }
        case "PLUS_DI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.PlusDILookback(optInTimePeriod);
        }
        case "PLUS_DM": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.PlusDMLookback(optInTimePeriod);
        }
        case "PPO": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.PpoLookback(optInFastPeriod, optInSlowPeriod, optInMAType);
        }
        case "PVI": {
            return core.PviLookback();
        }
        case "PVO": {
            int optInFastPeriod = GetInt(p, "optInFastPeriod", 0);
            int optInSlowPeriod = GetInt(p, "optInSlowPeriod", 0);
            MAType optInMAType = (MAType)GetInt(p, "optInMAType", 0);
            return core.PvoLookback(optInFastPeriod, optInSlowPeriod, optInMAType);
        }
        case "ROC": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.RocLookback(optInTimePeriod);
        }
        case "ROCP": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.RocPLookback(optInTimePeriod);
        }
        case "ROCR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.RocRLookback(optInTimePeriod);
        }
        case "ROCR100": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.RocR100Lookback(optInTimePeriod);
        }
        case "RSI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.RsiLookback(optInTimePeriod);
        }
        case "SAR": {
            double optInAcceleration = GetDouble(p, "optInAcceleration", 0.0);
            double optInMaximum = GetDouble(p, "optInMaximum", 0.0);
            return core.SarLookback(optInAcceleration, optInMaximum);
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
            return core.SarExtLookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
        }
        case "SIN": {
            return core.SinLookback();
        }
        case "SINH": {
            return core.SinhLookback();
        }
        case "SMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.SmaLookback(optInTimePeriod);
        }
        case "SQRT": {
            return core.SqrtLookback();
        }
        case "STDDEV": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            double optInNbDev = GetDouble(p, "optInNbDev", 0.0);
            return core.StdDevLookback(optInTimePeriod, optInNbDev);
        }
        case "STOCH": {
            int optInFastK_Period = GetInt(p, "optInFastK_Period", 0);
            int optInSlowK_Period = GetInt(p, "optInSlowK_Period", 0);
            MAType optInSlowK_MAType = (MAType)GetInt(p, "optInSlowK_MAType", 0);
            int optInSlowD_Period = GetInt(p, "optInSlowD_Period", 0);
            MAType optInSlowD_MAType = (MAType)GetInt(p, "optInSlowD_MAType", 0);
            return core.StochLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
        }
        case "STOCHF": {
            int optInFastK_Period = GetInt(p, "optInFastK_Period", 0);
            int optInFastD_Period = GetInt(p, "optInFastD_Period", 0);
            MAType optInFastD_MAType = (MAType)GetInt(p, "optInFastD_MAType", 0);
            return core.StochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
        }
        case "STOCHRSI": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            int optInFastK_Period = GetInt(p, "optInFastK_Period", 0);
            int optInFastD_Period = GetInt(p, "optInFastD_Period", 0);
            MAType optInFastD_MAType = (MAType)GetInt(p, "optInFastD_MAType", 0);
            return core.StochRsiLookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
        }
        case "SUB": {
            return core.SubLookback();
        }
        case "SUM": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.SumLookback(optInTimePeriod);
        }
        case "T3": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            double optInVFactor = GetDouble(p, "optInVFactor", 0.0);
            return core.T3Lookback(optInTimePeriod, optInVFactor);
        }
        case "TAN": {
            return core.TanLookback();
        }
        case "TANH": {
            return core.TanhLookback();
        }
        case "TEMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.TemaLookback(optInTimePeriod);
        }
        case "TRANGE": {
            return core.TrueRangeLookback();
        }
        case "TRIMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.TrimaLookback(optInTimePeriod);
        }
        case "TRIX": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.TrixLookback(optInTimePeriod);
        }
        case "TSF": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.TsfLookback(optInTimePeriod);
        }
        case "TYPPRICE": {
            return core.TypPriceLookback();
        }
        case "ULTOSC": {
            int optInTimePeriod1 = GetInt(p, "optInTimePeriod1", 0);
            int optInTimePeriod2 = GetInt(p, "optInTimePeriod2", 0);
            int optInTimePeriod3 = GetInt(p, "optInTimePeriod3", 0);
            return core.UltOscLookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
        }
        case "VAR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            double optInNbDev = GetDouble(p, "optInNbDev", 0.0);
            return core.VarianceLookback(optInTimePeriod, optInNbDev);
        }
        case "VWMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.VwmaLookback(optInTimePeriod);
        }
        case "WCLPRICE": {
            return core.WclPriceLookback();
        }
        case "WILLR": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.WillRLookback(optInTimePeriod);
        }
        case "WMA": {
            int optInTimePeriod = GetInt(p, "optInTimePeriod", 0);
            return core.WmaLookback(optInTimePeriod);
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
            rc = core.Accbands(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
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
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AccbandsUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
            sb.Append(",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Acos(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AcosUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Ad(startIdx, endIdx, inHigh, inLow, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AdUnguarded(startIdx, endIdx, inHigh, inLow, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Add(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AddUnguarded(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.AdOsc(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInFastPeriod, optInSlowPeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AdOscUnguarded(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInFastPeriod, optInSlowPeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[0] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.Adx(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AdxUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Adxr(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AdxrUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Apo(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.ApoUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Aroon(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AroonUnguarded(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.AroonOsc(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AroonOscUnguarded(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Asin(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AsinUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Atan(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AtanUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[2] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.Atr(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AtrUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.AvgDev(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AvgDevUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.AvgPrice(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.AvgPriceUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Bbands(startIdx, endIdx, inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
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
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.BbandsUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
            sb.Append(",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Beta(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.BetaUnguarded(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Bop(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.BopUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cci(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CciUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cdl2Crows(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.Cdl2CrowsUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cdl3BlackCrows(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.Cdl3BlackCrowsUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cdl3Inside(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.Cdl3InsideUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cdl3LineStrike(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.Cdl3LineStrikeUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cdl3Outside(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.Cdl3OutsideUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cdl3StarsInSouth(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.Cdl3StarsInSouthUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cdl3WhiteSoldiers(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.Cdl3WhiteSoldiersUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlAbandonedBaby(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlAbandonedBabyUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlAdvanceBlock(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlAdvanceBlockUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlBeltHold(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlBeltHoldUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlBreakaway(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlBreakawayUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlClosingMarubozu(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlClosingMarubozuUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlConcealBabysWall(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlConcealBabysWallUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlCounterAttack(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlCounterAttackUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlDarkCloudCover(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlDarkCloudCoverUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlDoji(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlDojiUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlDojiStar(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlDojiStarUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlDragonflyDoji(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlDragonflyDojiUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlEngulfing(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlEngulfingUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlEveningDojiStar(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlEveningDojiStarUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlEveningStar(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlEveningStarUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlGapSideSideWhite(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlGapSideSideWhiteUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlGravestoneDoji(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlGravestoneDojiUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlHammer(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlHammerUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlHangingMan(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlHangingManUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlHarami(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlHaramiUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlHaramiCross(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlHaramiCrossUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlHignWave(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlHignWaveUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlHikkake(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlHikkakeUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlHikkakeMod(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlHikkakeModUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlHomingPigeon(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlHomingPigeonUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlIdentical3Crows(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlIdentical3CrowsUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlInNeck(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlInNeckUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlInvertedHammer(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlInvertedHammerUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlKicking(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlKickingUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlKickingByLength(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlKickingByLengthUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlLadderBottom(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlLadderBottomUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlLongLeggedDoji(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlLongLeggedDojiUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlLongLine(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlLongLineUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlMarubozu(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlMarubozuUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlMatchingLow(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlMatchingLowUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlMatHold(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlMatHoldUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlMorningDojiStar(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlMorningDojiStarUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlMorningStar(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlMorningStarUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInPenetration, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlOnNeck(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlOnNeckUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlPiercing(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlPiercingUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlRickshawMan(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlRickshawManUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlRiseFall3Methods(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlRiseFall3MethodsUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlSeperatingLines(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlSeperatingLinesUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlShootingStar(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlShootingStarUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlShortLine(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlShortLineUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlSpinningTop(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlSpinningTopUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlStalledPattern(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlStalledPatternUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlStickSandwich(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlStickSandwichUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlTakuri(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlTakuriUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlTasukiGap(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlTasukiGapUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlThrusting(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlThrustingUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlTristar(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlTristarUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlUnique3River(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlUnique3RiverUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlUpsideGap2Crows(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlUpsideGap2CrowsUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.CdlXSideGap3Methods(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CdlXSideGap3MethodsUnguarded(startIdx, endIdx, inOpen, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Ceil(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CeilUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cmf(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CmfUnguarded(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[3] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.Cmo(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CmoUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cmou(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CmouUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Correl(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CorrelUnguarded(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cos(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CosUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Cosh(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.CoshUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Dema(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.DemaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Div(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.DivUnguarded(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[4] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.Dx(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.DxUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[5] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.Ema(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.EmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Exp(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.ExpUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Floor(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.FloorUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Hma(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.HmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[6] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HtDcPeriod(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.HtDcPeriodUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[7] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HtDcPhase(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.HtDcPhaseUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[8] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HtPhasor(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.HtPhasorUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[9] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HtSine(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.HtSineUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[10] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HtTrendline(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.HtTrendlineUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[11] = GetInt(p, "unstablePeriod", 0);
        int[] outArr0 = new int[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.HtTrendMode(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.HtTrendModeUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Imi(startIdx, endIdx, inOpen, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.ImiUnguarded(startIdx, endIdx, inOpen, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[13] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.Kama(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.KamaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.LinearReg(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.LinearRegUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.LinearRegAngle(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.LinearRegAngleUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.LinearRegIntercept(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.LinearRegInterceptUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.LinearRegSlope(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.LinearRegSlopeUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Ln(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.LnUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Log10(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.Log10Unguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MovingAverage(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MovingAverageUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Macd(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
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
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MacdUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
            sb.Append(",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MacdExt(startIdx, endIdx, inReal, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
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
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MacdExtUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
            sb.Append(",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MacdFix(startIdx, endIdx, inReal, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
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
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MacdFixUnguarded(startIdx, endIdx, inReal, optInSignalPeriod, out outBegIdx, out outNBElement, outArr0, outArr1, outArr2);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
            sb.Append(",\"outReal2\":"); sb.Append(FormatArray(outArr2, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[14] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        double[] outArr1 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.Mama(startIdx, endIdx, inReal, optInFastLimit, optInSlowLimit, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MamaUnguarded(startIdx, endIdx, inReal, optInFastLimit, optInSlowLimit, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MovingAverageVariablePeriod(startIdx, endIdx, inReal0, inReal1, optInMinPeriod, optInMaxPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MovingAverageVariablePeriodUnguarded(startIdx, endIdx, inReal0, inReal1, optInMinPeriod, optInMaxPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Max(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MaxUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MaxIndex(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MaxIndexUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MedPrice(startIdx, endIdx, inHigh, inLow, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MedPriceUnguarded(startIdx, endIdx, inHigh, inLow, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Mfi(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MfiUnguarded(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MidPoint(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MidPointUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MidPrice(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MidPriceUnguarded(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Min(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MinUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MinIndex(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MinIndexUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MinMax(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MinMaxUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.MinMaxIndex(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashI32(_h, outArr0, outNBElement);
                _h = SvHashI32(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MinMaxIndexUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outInteger\":"); sb.Append(FormatIntArray(outArr0, outNBElement));
            sb.Append(",\"outInteger1\":"); sb.Append(FormatIntArray(outArr1, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[16] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MinusDI(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MinusDIUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[17] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.MinusDM(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MinusDMUnguarded(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Mom(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MomUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Mult(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.MultUnguarded(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[18] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.Natr(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.NatrUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Nvi(startIdx, endIdx, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.NviUnguarded(startIdx, endIdx, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Obv(startIdx, endIdx, inReal, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.ObvUnguarded(startIdx, endIdx, inReal, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[19] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.PlusDI(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.PlusDIUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[20] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.PlusDM(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.PlusDMUnguarded(startIdx, endIdx, inHigh, inLow, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Ppo(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.PpoUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Pvi(startIdx, endIdx, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.PviUnguarded(startIdx, endIdx, inClose, inVolume, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Pvo(startIdx, endIdx, inVolume, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.PvoUnguarded(startIdx, endIdx, inVolume, optInFastPeriod, optInSlowPeriod, optInMAType, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Roc(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.RocUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.RocP(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.RocPUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.RocR(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.RocRUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.RocR100(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.RocR100Unguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[21] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.Rsi(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.RsiUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Sar(startIdx, endIdx, inHigh, inLow, optInAcceleration, optInMaximum, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.SarUnguarded(startIdx, endIdx, inHigh, inLow, optInAcceleration, optInMaximum, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.SarExt(startIdx, endIdx, inHigh, inLow, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.SarExtUnguarded(startIdx, endIdx, inHigh, inLow, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Sin(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.SinUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Sinh(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.SinhUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Sma(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.SmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Sqrt(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.SqrtUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.StdDev(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.StdDevUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Stoch(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.StochUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.StochF(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.StochFUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.StochRsi(startIdx, endIdx, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
                _h = SvHashF64(_h, outArr1, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.StochRsiUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, out outBegIdx, out outNBElement, outArr0, outArr1);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
            sb.Append(",\"outReal1\":"); sb.Append(FormatArray(outArr1, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Sub(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.SubUnguarded(startIdx, endIdx, inReal0, inReal1, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Sum(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.SumUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
        core.unstablePeriod[23] = GetInt(p, "unstablePeriod", 0);
        double[] outArr0 = new double[n];
        int outBegIdx = 0, outNBElement = 0;
        RetCode rc = RetCode.Success;
        long _t0 = 0;
        for (int _bi = 0; _bi <= bench_iters; _bi++) {
            if (_bi == 1) _t0 = GetNanoTime();
            rc = core.T3(startIdx, endIdx, inReal, optInTimePeriod, optInVFactor, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.T3Unguarded(startIdx, endIdx, inReal, optInTimePeriod, optInVFactor, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Tan(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.TanUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Tanh(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.TanhUnguarded(startIdx, endIdx, inReal, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Tema(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.TemaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.TrueRange(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.TrueRangeUnguarded(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Trima(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.TrimaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Trix(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.TrixUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Tsf(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.TsfUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.TypPrice(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.TypPriceUnguarded(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.UltOsc(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.UltOscUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Variance(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.VarianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Vwma(startIdx, endIdx, inReal, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.VwmaUnguarded(startIdx, endIdx, inReal, inVolume, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.WclPrice(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.WclPriceUnguarded(startIdx, endIdx, inHigh, inLow, inClose, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.WillR(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.WillRUnguarded(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
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
            rc = core.Wma(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;
        if (GetInt(p, "want_hash", 0) != 0 && GetInt(p, "full_output", 0) == 0) {
            ulong _h = SvHashInit();
            if (rc == RetCode.Success && outNBElement > 0) {
                _h = SvHashF64(_h, outArr0, outNBElement);
            }
            _h = SvHashFin(_h);
            return $"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement},\"out_hash\":\"{_h:x16}\"}}";
        }
        long _t0u = 0;
        for (int _biu = 0; _biu <= bench_iters; _biu++) {
            if (_biu == 1) _t0u = GetNanoTime();
            rc = core.WmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, out outBegIdx, out outNBElement, outArr0);
        }
        long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;
        var sb = new System.Text.StringBuilder();
        sb.Append($"{{\"retCode\":{(int)rc},\"outBegIdx\":{outBegIdx},\"outNBElement\":{outNBElement}");
        if (GetInt(p, "no_output", 0) == 0) {
            sb.Append(",\"outReal\":"); sb.Append(FormatArray(outArr0, outNBElement));
        }
        sb.Append($",\"timing_ns\":{elapsedNs}");
        sb.Append($",\"timing_ns_unguarded\":{elapsedNsUng}");
        sb.Append("}");
        return sb.ToString();
    }

    static void Main(string[] args) {
        string? line;
        while ((line = Console.ReadLine()) != null) {
            if (string.IsNullOrWhiteSpace(line)) continue;
            Console.WriteLine(HandleRequest(line));
            Console.Out.Flush();
        }
    }
}
