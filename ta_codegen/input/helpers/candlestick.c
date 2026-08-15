double ta_realbody(double close, double open) {
   return fabs(close - open);
}

int ta_candlecolor(double close, double open) {
   return (close >= open) ? 1 : -1;
}

double ta_uppershadow(double high, double close, double open) {
   return high - (close >= open ? close : open);
}

double ta_lowershadow(double low, double close, double open) {
   return (close >= open ? open : close) - low;
}

double ta_highlowrange(double high, double low) {
   return high - low;
}

int ta_realbodygapup(double open1, double close1, double open2, double close2) {
   return (fmin(open1, close1) > fmax(open2, close2)) ? 1 : 0;
}

int ta_realbodygapdown(double open1, double close1, double open2, double close2) {
   return (fmax(open1, close1) < fmin(open2, close2)) ? 1 : 0;
}

int ta_candlegapup(double low1, double high2) {
   return (low1 > high2) ? 1 : 0;
}

int ta_candlegapdown(double high1, double low2) {
   return (high1 < low2) ? 1 : 0;
}

double ta_candlerange(int rangeType, double open, double high, double low, double close) {
   switch (rangeType) {
      case 0: return fabs(close - open);
      case 1: return high - low;
      /* Shadows: upper plus lower, NOT the algebraically equal
       * high - low - fabs(close - open). The two differ by reassociation
       * whenever a bar's low sits below half its high, which is where
       * Sterbenz' lemma stops making the subtractions exact.
       *
       * C is the reference and never reads this body: backends/c.rs maps
       * ta_candlerange onto the TA_CANDLERANGE macro, which has always been
       * upper + lower. Writing it the other way here made the generated
       * Rust, Java and C# disagree with C on such bars (#217). Both
       * artifacts that have ever shipped -- the C library and the 2007
       * com.tictactec Java port, two hand-written implementations twenty
       * years apart -- use this spelling; the generated helper was the sole
       * outlier.
       *
       * Calling the two shadow helpers rather than respelling them keeps the
       * operation order tied to the macro's by construction: TA_UPPERSHADOW
       * and TA_LOWERSHADOW are these functions, term for term.
       */
      case 2: return ta_uppershadow(high, close, open) + ta_lowershadow(low, close, open);
      default: return 0.0;
   }
}

double ta_candleaverage(int rangeType, int avgPeriod, double factor, double sum,
   double open, double high, double low, double close) {
   double avg = (avgPeriod != 0)
   ? sum / avgPeriod
   : ta_candlerange(rangeType, open, high, low, close);
   double divisor = (rangeType == 2) ? 2.0 : 1.0;
   return factor * avg / divisor;
}
