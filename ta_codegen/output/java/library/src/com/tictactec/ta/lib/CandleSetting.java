/* TA-LIB Copyright (c) 1999-2007, Mario Fortier
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
 */

package com.tictactec.ta.lib;

public class CandleSetting {
    
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

    public CandleSetting(CandleSetting that)
    {
       this.settingType = that.settingType;
       this.rangeType = that.rangeType;
       this.avgPeriod = that.avgPeriod;
       this.factor = that.factor;       
    }
    
    public void CopyFrom( CandleSetting src )
    {
       this.settingType = src.settingType;
       this.rangeType = src.rangeType;
       this.avgPeriod = src.avgPeriod;
       this.factor = src.factor;
    }
    
    CandleSettingType    settingType;
    RangeType            rangeType;
    int                     avgPeriod;
    double                  factor;    
}
