import io.github.talib.Core;
import io.github.talib.OutRange;

/** Resolves the published coordinate and calls two indicators through it. */
public class Check {
    public static void main(String[] args) {
        double[] in = new double[100];
        for (int i = 0; i < in.length; i++) {
            in[i] = 100.0 + Math.sin(i / 5.0) * 10.0;
        }

        double[] sma = new double[in.length];
        OutRange s = Core.DEFAULT.sma(0, in.length - 1, in, 30, sma);
        double[] rsi = new double[in.length];
        OutRange r = Core.DEFAULT.rsi(0, in.length - 1, in, 14, rsi);

        System.out.printf("SMA begIdx=%d count=%d first=%.6f%n", s.begIdx(), s.count(), sma[0]);
        System.out.printf("RSI begIdx=%d count=%d first=%.6f%n", r.begIdx(), r.count(), rsi[0]);

        // Lookback-derived, so a library that resolved but computes nothing fails here.
        require(s.begIdx() == 29 && s.count() == 71, "SMA(30) range");
        require(r.begIdx() == 14 && r.count() == 86, "RSI(14) range");
        require(sma[0] > 90.0 && sma[0] < 110.0, "SMA value");
        require(rsi[0] > 0.0 && rsi[0] < 100.0, "RSI value");
        System.out.println("OK");
    }

    private static void require(boolean ok, String what) {
        if (!ok) {
            throw new AssertionError("consumer check failed: " + what);
        }
    }
}
