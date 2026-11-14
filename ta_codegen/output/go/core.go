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

/* Hand-written: ta_codegen never writes this file. */

package talib

import "math"

// The sentinels and bounds of the optional parameters and indices, as C
// defines them.
const (
	RealDefault       = -4e37
	RealMin           = -3e37
	RealMax           = 3e37
	IntegerDefault    = math.MinInt32
	IntegerMin        = math.MinInt32 + 1
	IntegerMax        = math.MaxInt32
	IndexMax          = 100000000
	UnstableAutoPrec4 = IndexMax + 4
	UnstableAutoPrec8 = IndexMax + 8
)

// CandleSetting is one candlestick threshold: Factor times the average of
// RangeType over AvgPeriod previous candles (0: no averaging).
type CandleSetting struct {
	SettingType CandleSettingType
	RangeType   RangeType
	AvgPeriod   int
	Factor      float64
}

// Core holds the settings every call reads: the unstable periods and the
// candlestick settings. Create one with [NewCore]. Do not use the zero value:
// its candlestick settings are all zero, not the defaults. A Core is safe to
// share between goroutines once configured: call its setters only while no other
// goroutine uses it and no stream opened on it is in use.
type Core struct {
	unstablePeriod [funcUnstCount]int32
	candleSettings [candleSettingCount]CandleSetting
}

// NewCore returns a Core with C's defaults: no unstable period and the
// default candlestick settings.
func NewCore() *Core {
	return &Core{candleSettings: defaultCandleSettings}
}

// SetUnstablePeriod sets the extra warm-up bars of one function, or of every
// function given FuncUnstIdALL: 0 to IndexMax, or UnstableAutoPrec4 or
// UnstableAutoPrec8. A rejected call returns BadParam and changes nothing.
func (c *Core) SetUnstablePeriod(id FuncUnstId, period int) error {
	if (period < 0 || period > IndexMax) && period != UnstableAutoPrec4 && period != UnstableAutoPrec8 {
		return BadParam
	}
	if id == FuncUnstIdALL {
		for i := range c.unstablePeriod {
			c.unstablePeriod[i] = int32(period)
		}
		return nil
	}
	if id < 0 || int(id) >= funcUnstCount {
		return BadParam
	}
	c.unstablePeriod[id] = int32(period)
	return nil
}

// UnstablePeriod returns the unstable period set for one function.
func (c *Core) UnstablePeriod(id FuncUnstId) (int, error) {
	if id < 0 || int(id) >= funcUnstCount {
		return 0, BadParam
	}
	return int(c.unstablePeriod[id]), nil
}

// SetCandleSetting overrides one candlestick setting. avgPeriod is in 0 to
// IndexMax and factor is finite and not negative. A rejected call returns
// BadParam and changes nothing.
func (c *Core) SetCandleSetting(settingType CandleSettingType, rangeType RangeType, avgPeriod int, factor float64) error {
	if settingType < 0 || settingType >= candleSettingCount {
		return BadParam
	}
	if rangeType < RangeTypeRealBody || rangeType > RangeTypeShadows {
		return BadParam
	}
	if avgPeriod < 0 || avgPeriod > IndexMax {
		return BadParam
	}
	if !(factor >= 0 && factor <= math.MaxFloat64) {
		return BadParam
	}
	c.candleSettings[settingType] = CandleSetting{settingType, rangeType, avgPeriod, factor}
	return nil
}

// CandleSetting returns the candlestick setting stored for one attribute.
func (c *Core) CandleSetting(settingType CandleSettingType) (CandleSetting, error) {
	if settingType < 0 || settingType >= candleSettingCount {
		return CandleSetting{}, BadParam
	}
	return c.candleSettings[settingType], nil
}

// RestoreCandleDefault restores one candlestick setting to its default, or
// every one given CandleSettingTypeAllCandleSettings.
func (c *Core) RestoreCandleDefault(settingType CandleSettingType) error {
	if settingType == CandleSettingTypeAllCandleSettings {
		c.candleSettings = defaultCandleSettings
		return nil
	}
	if settingType < 0 || settingType >= candleSettingCount {
		return BadParam
	}
	c.candleSettings[settingType] = defaultCandleSettings[settingType]
	return nil
}

// The unstable period of one function for one call: the stored count, or
// under an Auto level that level's count.
func (c *Core) unstableCount(id FuncUnstId, prec4, prec8 int32) int32 {
	stored := c.unstablePeriod[id]
	if stored <= IndexMax {
		return stored
	}
	if stored == UnstableAutoPrec4 {
		return prec4
	}
	return prec8
}
