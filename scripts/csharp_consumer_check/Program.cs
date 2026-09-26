using System.Reflection;
using TALib;
using TALib.Metadata;

const int n = 100;
var close = new double[n];
var closeF = new float[n];
for (int i = 0; i < n; i++)
{
    close[i] = 100.0 + Math.Sin(i / 5.0) * 10.0;
    closeF[i] = (float)close[i];
}

var core = Core.Default;
Require(Core.IndexMax >= n, "Core.IndexMax");

var sma = new double[n];
OutRange s = core.Sma(0, n - 1, close, 30, sma);
int smaLookback = core.SmaLookback(30);
Require(s.BegIdx == smaLookback && s.Count == n - smaLookback, "SMA(30) range");
Require(Math.Abs(sma[0] - close[..30].Average()) < 1e-9, "SMA(30) first value");

// The float overload is a separately transcribed body: widened input must give the same values.
var rsi = new double[n];
var rsiFOut = new double[n];
OutRange rf = core.Rsi(0, n - 1, closeF, 14, rsiFOut);
OutRange rd = core.Rsi(0, n - 1, closeF.Select(x => (double)x).ToArray(), 14, rsi);
int rsiLookback = core.RsiLookback(14);
Require(rf == rd && rf.BegIdx == rsiLookback && rf.Count == n - rsiLookback, "RSI(14) range");
Require(rsi.AsSpan(0, rd.Count).SequenceEqual(rsiFOut.AsSpan(0, rf.Count)), "RSI float overload");
Require(rsi[0] > 0.0 && rsi[0] < 100.0, "RSI value");

Core.SmaStream stream = core.SmaOpen(close.AsSpan(0, n - 1), 30);
Require(stream.Value.Equals(sma[s.Count - 2]), "stream Value");
Require(stream.Peek(close[n - 1]).Equals(sma[s.Count - 1]), "stream Peek");
Require(stream.Update(close[n - 1]).Equals(sma[s.Count - 1]), "stream Update");
Require(stream.OutRange == s, "stream OutRange");

var viaCatalog = new double[n];
ParamHolder call = Core.Functions["sma"].CreateCall(core)
    .SetInput(0, close)
    .SetOptInput(0, 30)
    .SetOutput(0, viaCatalog);
Require(call.Lookback() == smaLookback, "catalog lookback");
Require(call.Call(0, n - 1) == s && viaCatalog.AsSpan().SequenceEqual(sma), "catalog dispatch");

var info = typeof(Core).Assembly.GetCustomAttribute<AssemblyInformationalVersionAttribute>();
Console.WriteLine($"TALib {info?.InformationalVersion}");
Console.WriteLine($"SMA {s} first={sma[0]:R} last={sma[s.Count - 1]:R}");
Console.WriteLine($"RSI {rf} first={rsi[0]:R} last={rsi[rf.Count - 1]:R}");
Console.WriteLine("OK");

static void Require(bool ok, string what)
{
    if (!ok)
    {
        throw new InvalidOperationException("consumer check failed: " + what);
    }
}
