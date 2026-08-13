/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  072526 MF,CC  First Version — builder for the immutable Core.
 *  081126 KL,MF,CC Bound avgPeriod above and refuse a NaN factor (#185).
 */

package io.github.talib;

/**
 * Builder for an immutable {@link Core}.
 *
 * <p>Obtain one from {@link Core#builder()} (TA-Lib defaults) or
 * {@link Core#toBuilder()} (seeded from an existing {@code Core}), configure the
 * value-affecting settings, then call {@link #build()}:
 * <pre>{@code
 * Core core = Core.builder()
 *     .unstablePeriod(FuncUnstId.EMA, 10)
 *     .build();
 * }</pre>
 *
 * <p>A builder is <b>not</b> thread-safe (the standard builder contract — confine
 * it to one thread); the {@code Core} it produces is. Builder allocation is a
 * cold path and irrelevant to indicator throughput.
 */
public final class CoreBuilder {

   private final int[] unstablePeriod;

   private final CandleSetting[] candleSettings;

   /** A builder seeded with TA-Lib's defaults. */
   public CoreBuilder() {
      this.unstablePeriod = new int[FuncUnstId.COUNT];
      // CandleSetting is immutable, so the default instances are shared, not copied.
      this.candleSettings = Core.DEFAULT_CANDLE_SETTINGS.clone();
   }

   /** A builder seeded from an existing {@code Core}'s settings (see {@link Core#toBuilder()}). */
   CoreBuilder(int[] unstablePeriod, CandleSetting[] candleSettings) {
      this.unstablePeriod = unstablePeriod.clone();
      this.candleSettings = candleSettings.clone();
   }

   /**
    * Sets the unstable period for one function.
    *
    * <p>Passing {@link FuncUnstId#ALL} sets it for <i>every</i> function at once,
    * mirroring the C {@code TA_SetUnstablePeriod} wildcard.
    *
    * @throws NullPointerException if {@code id} is null
    * @throws IllegalArgumentException if {@code period} is negative or above
    *         {@link Core#MAX_INDEX}
    */
   public CoreBuilder unstablePeriod(FuncUnstId id, int period) {
      if (id == null) {
         throw new NullPointerException("id");
      }
      if (period < 0) {
         throw new IllegalArgumentException("unstablePeriod must be >= 0, got " + period);
      }
      /* The period is added to a lookback which is then used as an index, so an
       * unbounded one overflows that lookback negative and the function indexes
       * far past the end of its input. MAX_INDEX is the ceiling the index space
       * already enforces on startIdx/endIdx; a warm-up longer than the largest
       * addressable series could never produce output, so nothing legitimate is
       * refused. C applies the same bound in TA_SetUnstablePeriod.
       */
      if (period > Core.MAX_INDEX) {
         throw new IllegalArgumentException(
            "unstablePeriod must be <= " + Core.MAX_INDEX + ", got " + period);
      }
      if (id == FuncUnstId.ALL) {
         java.util.Arrays.fill(unstablePeriod, period);
      } else {
         unstablePeriod[id.ordinal()] = period;
      }
      return this;
   }

   /**
    * Overrides one candlestick setting, mirroring the C
    * {@code TA_SetCandleSettings}.
    *
    * <p>{@code avgPeriod} is the lookback of every {@code CDL*} function that
    * reads the setting, so it is bounded like one; {@code factor} scales a
    * threshold and takes any finite value.
    *
    * @throws NullPointerException if {@code settingType} or {@code rangeType} is null
    * @throws IllegalArgumentException if {@code settingType} is
    *         {@link CandleSettingType#AllCandleSettings} (not a single-setting
    *         target), if {@code avgPeriod} is outside {@code 0..}{@link
    *         Core#MAX_INDEX}, or if {@code factor} is NaN
    */
   public CoreBuilder candleSetting(CandleSettingType settingType, RangeType rangeType,
      int avgPeriod, double factor) {
      if (settingType == null) {
         throw new NullPointerException("settingType");
      }
      if (rangeType == null) {
         throw new NullPointerException("rangeType");
      }
      if (settingType == CandleSettingType.AllCandleSettings) {
         throw new IllegalArgumentException(
            "AllCandleSettings is not a single-setting target");
      }
      if (avgPeriod < 0) {
         throw new IllegalArgumentException("avgPeriod must be >= 0, got " + avgPeriod);
      }
      if (avgPeriod > Core.MAX_INDEX) {
         throw new IllegalArgumentException(
            "avgPeriod must be <= " + Core.MAX_INDEX + ", got " + avgPeriod);
      }
      if (Double.isNaN(factor)) {
         throw new IllegalArgumentException("factor must not be NaN");
      }
      candleSettings[settingType.ordinal()] =
         new CandleSetting(settingType, rangeType, avgPeriod, factor);
      return this;
   }

   /**
    * Restores one candlestick setting to its TA-Lib default, or every setting
    * when given {@link CandleSettingType#AllCandleSettings}.
    *
    * @throws NullPointerException if {@code settingType} is null
    */
   public CoreBuilder restoreCandleDefault(CandleSettingType settingType) {
      if (settingType == null) {
         throw new NullPointerException("settingType");
      }
      if (settingType == CandleSettingType.AllCandleSettings) {
         System.arraycopy(Core.DEFAULT_CANDLE_SETTINGS, 0, candleSettings, 0,
            candleSettings.length);
      } else {
         candleSettings[settingType.ordinal()] =
            Core.DEFAULT_CANDLE_SETTINGS[settingType.ordinal()];
      }
      return this;
   }

   /** Produces the immutable {@link Core}. The builder stays usable afterwards. */
   public Core build() {
      return new Core(this);
   }

   /* Defensive copies handed to Core's constructor — the builder keeps its own
    * arrays, so later builder calls cannot reach into a built Core. */

   int[] snapshotUnstablePeriod() {
      return unstablePeriod.clone();
   }

   CandleSetting[] snapshotCandleSettings() {
      return candleSettings.clone();
   }
}
