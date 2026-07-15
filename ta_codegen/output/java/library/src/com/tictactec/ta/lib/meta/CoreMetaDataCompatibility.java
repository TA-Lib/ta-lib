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
 *  RG       Richard Gomes
 *
 * Change history:
 *
 *  YYYYMMDD BY     Description
 *  -------------------------------------------------------------------
 *  20070311 RG     First Version
 */

package com.tictactec.ta.lib.meta;

import java.lang.annotation.IncompleteAnnotationException;

import com.tictactec.ta.lib.RetCode;
import com.tictactec.ta.lib.meta.annotation.FuncInfo;
import com.tictactec.ta.lib.meta.annotation.InputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.OptInputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.OutputParameterInfo;

/**
 * This class is intended to application developers willing to translate "C" application code
 * which makes use of <i>ta_abstract.h</i>.
 * 
 * In Java, applications and API make a wide use of exceptions whilst in "C" code the common approach is returning status
 * codes. This class adds a layer over class CoreMetaData which handles and consumes exceptions and, instead of exceptions,
 * returns status codes. 
 * 
 * <p><b>IMPORTANT:</b> This class is provided as it is by the author as a starting point for "C" application code translators.
 * Use this class on your own risk as this class was not tested by the author.
 * 
 * <p><b>IMPORTANT:</b> Some methods from <i>ta_abstract.h</i> where not implemented. The author do not intend to
 * provide these methods as there's low or even no interest on this kind of compatibility mode. As already said, this is
 * only a starting point for applications translators which could be interested to further improve this class. 
 * 
 * @author Richard Gomes
 *
 */
public class CoreMetaDataCompatibility extends CoreMetaData {

    static RetCode taGetFuncHandle(final String name, CoreMetaData retHandle) {
        try {
            retHandle = getFuncHandle(name);
            return RetCode.Success;
        } catch (NoSuchMethodException e) {
            return RetCode.BadParam;
        }
    }
    

    RetCode taGetFuncInfo(FuncInfo retFuncInfo) {
        try {
            retFuncInfo = super.getFuncInfo();
            return RetCode.Success;
        } catch (IncompleteAnnotationException e) {
            return RetCode.InternalError;
        }
    }
    

    RetCode taGetInputParameterInfo(final int paramIndex, InputParameterInfo retInputParameterInfo) {
        try {
            retInputParameterInfo = super.getInputParameterInfo(paramIndex);
            return RetCode.Success;
        } catch (IllegalArgumentException e) {
            return RetCode.BadParam;
        }
    }


    RetCode taGetInputParameterInfo(final int paramIndex, OptInputParameterInfo retOptInputParameterInfo) {
        try {
            retOptInputParameterInfo = super.getOptInputParameterInfo(paramIndex);
            return RetCode.Success;
        } catch (IllegalArgumentException e) {
            return RetCode.BadParam;
        }
    }


    RetCode taGetOutputParameterInfo(final int paramIndex, OutputParameterInfo retOutputParameterInfo) {
        try {
            retOutputParameterInfo = super.getOutputParameterInfo(paramIndex);
            return RetCode.Success;
        } catch (IllegalArgumentException e) {
            return RetCode.BadParam;
        }
    }


    RetCode taSetInputParamIntegerPtr(final int paramIndex, final int[] value ) {
        try {
            super.setInputParamInteger(paramIndex, value);
            return RetCode.Success;
        } catch (NullPointerException e) {
            return RetCode.BadParam;
        }
    }

    
    RetCode taSetInputParamRealPtr(final int paramIndex, final double[] value ) {
        try {
            super.setInputParamReal(paramIndex, value);
            return RetCode.Success;
        } catch (NullPointerException e) {
            return RetCode.BadParam;
        }
    }

    
    RetCode taSetInputParamPricePtr(final int paramIndex,
            final double[] open,
            final double[] high,
            final double[] low,
            final double[] close,
            final double[] volume,
            final double[] openInterest) {
        try {
            super.setInputParamPrice(paramIndex, open, high, low, close, volume, openInterest);
            return RetCode.Success;
        } catch (NullPointerException e) {
            return RetCode.BadParam;
        }
    }
    
    
    RetCode taSetOptInputParamInteger(final int paramIndex, final int optInValue) {
        try {
            setOptInputParamInteger(paramIndex, optInValue);
            return RetCode.Success;
        } catch (IllegalArgumentException e) {
            return RetCode.BadParam;
        }
    }
        
    
    RetCode taSetOptInputParamReal(final int paramIndex, final double optInValue) {
        try {
            setOptInputParamReal(paramIndex, optInValue);
            return RetCode.Success;
        } catch (IllegalArgumentException e) {
            return RetCode.BadParam;
        }
    }
    
   
    RetCode taSetOutputParamIntegerPtr(final int paramIndex, int[] outArray) {
        if (outArray == null) return RetCode.BadParam;
        try {
            setOutputParamInteger(paramIndex, outArray);
            return RetCode.Success;
        } catch (IllegalArgumentException e) {
            return RetCode.BadParam;
        }
    }

    
    RetCode taSetOutputParamRealPtr(final int paramIndex, double[] outArray) {
        if (outArray == null) return RetCode.BadParam;
        try {
            setOutputParamReal(paramIndex, outArray);
            return RetCode.Success;
        } catch (IllegalArgumentException e) {
            return RetCode.BadParam;
        }
    }


}
