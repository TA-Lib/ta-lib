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
 *  BT       Barry Tsung
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  121005 MF     First Version
 *  022206 BT     add copy constructor
 *  072526 MF,CC  Immutable: final fields, read accessors, no CopyFrom/copy ctor.
 */

package io.github.talib;

/**
 * One candlestick-pattern setting: how a candle part is measured
 * ({@link RangeType}), over how many previous candles, and by what factor.
 *
 * <p><b>Immutable.</b> This is what makes {@link Core}'s {@code CandleSetting[]}
 * deeply immutable and therefore safe to share across threads; instances are
 * created through {@link CoreBuilder#candleSetting}.
 */
public final class CandleSetting {

    /** Creates a new instance of TA_CandleSetting */
    public CandleSetting( CandleSettingType p_settingType,
                             RangeType p_rangeType,
                             int p_avgPeriod,
                             double p_factor )
    {
        settingType = p_settingType;
        rangeType = p_rangeType;
        avgPeriod = p_avgPeriod;
        factor = p_factor;
    }

    /** Which candle attribute this setting describes. */
    public CandleSettingType settingType() { return settingType; }

    /** How the candle range is measured. */
    public RangeType rangeType() { return rangeType; }

    /** Number of previous candles averaged over (0 = no averaging). */
    public int avgPeriod() { return avgPeriod; }

    /** Multiplier applied to the average. */
    public double factor() { return factor; }

    @Override
    public String toString()
    {
       return "CandleSetting[" + settingType + ", " + rangeType
            + ", avgPeriod=" + avgPeriod + ", factor=" + factor + "]";
    }

    /* Package-private and final: the generated indicator code in Core reads these
     * fields directly (candleSettings[X.ordinal()].avgPeriod). */
    final CandleSettingType    settingType;
    final RangeType            rangeType;
    final int                     avgPeriod;
    final double                  factor;
}
