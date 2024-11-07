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

import com.tictactec.ta.lib.meta.annotation.InputFlags;
import com.tictactec.ta.lib.meta.helpers.SimpleHelper;


/**
 * PriceHolder is responsible for holding prices, which are composed by OHLCVI, i.e:
 * open, high, low, close, volume and open interest.
 * 
 * <p>Indicators which take prices as input, only take subsets of OHLCVI components and one could think
 * that PriceHolder could be used for such purpose. <i>This class is not intended to hold prices passed to indicators.</i>
 * 
 * <p>In order to avoid this kind of confusion, PriceHolder cannot be initialized with null arrays.
 * PriceHoder also checks if all arrays passed to constructor have the same length.
 * 
 * @author Richard Gomes
 */
public class PriceHolder {

    private double[] o;
    private double[] h;
    private double[] l;
    private double[] c;
    private double[] v;
    private double[] i;
    public final int flags;
    public final int count;
    public final int length;
    
    /**
     * Stores all data point arrays in a PriceHolder instance
     * 
     * <p><b>Note:</b> This constructor requires that all arrays are passed in and that all their sizes
     * match. See {@link PriceHolder#PriceHolder(int, double[], double[], double[], double[], double[], double[])}
     * for a convenience constructor which requires only the arrays informed by the <code>flags</code> argument. 
     *  
     * @param o represent the open data points and is expected to be <b>double[]</b> assignment compatible.
     * @param h represent the high data points and is expected to be <b>double[]</b> assignment compatible.
     * @param l represent the low data points and is expected to be <b>double[]</b> assignment compatible.
     * @param c represent the close data points and is expected to be <b>double[]</b> assignment compatible.
     * @param v represent the volume data points and is expected to be <b>double[]</b> assignment compatible.
     * @param i represent the open interest data points and is expected to be <b>double[]</b> assignment compatible.
     * 
     * @throws NullPointerException if any arrays is null
     * @throws IllegalArgumentException if sizes of arrays dont match
     */
    public PriceHolder(double[] o, double[] h, double[] l, double[] c, double[] v, double[] i) 
            throws NullPointerException, IllegalArgumentException {

        if (o==null) throw new NullArrayException("open");
        if (h==null) throw new NullArrayException("high");
        if (l==null) throw new NullArrayException("low");
        if (c==null) throw new NullArrayException("close");
        if (v==null) throw new NullArrayException("volume");
        if (i==null) throw new NullArrayException("open interest");
        
        this.length = o.length;
        if (h.length != length) throw new WrongArrayLengthException("high");
        if (l.length != length) throw new WrongArrayLengthException("low");
        if (c.length != length) throw new WrongArrayLengthException("close");
        if (v.length != length) throw new WrongArrayLengthException("volume");
        if (i.length != length) throw new WrongArrayLengthException("open interest");
        
        this.count = 6;

        this.flags = InputFlags.TA_IN_PRICE_OPEN | InputFlags.TA_IN_PRICE_HIGH | InputFlags.TA_IN_PRICE_LOW | InputFlags.TA_IN_PRICE_CLOSE 
        			 | InputFlags.TA_IN_PRICE_VOLUME | InputFlags.TA_IN_PRICE_OPENINTEREST;
        	
        this.o = o;
        this.h = h;
        this.l = l;
        this.c = c;
        this.v = v;
        this.i = i;
    }
    
    
    public PriceHolder(int flags, double[] o, double[] h, double[] l, double[] c, double[] v, double[] i) {
            this.flags = flags;
            int count = 0;
            int length = Integer.MIN_VALUE;
            
            this.o = null;
            if ((flags&InputFlags.TA_IN_PRICE_OPEN)!=0) {
                if (o==null) throw new NullArrayException("open");
                if (length==Integer.MIN_VALUE) {
                	length = o.length;
                } else if (length!=o.length) {
                	throw new WrongArrayLengthException("open");
                }
                this.o = o;
            	count++;
            }
            
            this.h = null;
            if ((flags&InputFlags.TA_IN_PRICE_HIGH)!=0) {
                if (h==null) throw new NullArrayException("high");
                if (length==Integer.MIN_VALUE) {
                	length = h.length;
                } else if (length!=h.length) {
                	throw new WrongArrayLengthException("high");
                }
                this.h = h;
            	count++;
            }
            
            this.l = null;
            if ((flags&InputFlags.TA_IN_PRICE_LOW)!=0) {
                if (l==null) throw new NullArrayException("low");
                if (length==Integer.MIN_VALUE) {
                	length = l.length;
                } else if (length!=l.length) {
                	throw new WrongArrayLengthException("low");
                }
                this.l = l;
            	count++;
            }
            
            this.c = null;
            if ((flags&InputFlags.TA_IN_PRICE_CLOSE)!=0) {
                if (c==null) throw new NullArrayException("close");
                if (length==Integer.MIN_VALUE) {
                	length = c.length;
                } else if (length!=c.length) {
                	throw new WrongArrayLengthException("close");
                }
                this.c = c;
            	count++;
            }
            
            this.v = null;
            if ((flags&InputFlags.TA_IN_PRICE_VOLUME)!=0) {
                if (v==null) throw new NullArrayException("volume");
                if (length==Integer.MIN_VALUE) {
                	length = v.length;
                } else if (length!=v.length) {
                	throw new WrongArrayLengthException("volume");
                }
                this.v = v;
            	count++;
            }
            
            this.i = null;
            if ((flags&InputFlags.TA_IN_PRICE_OPENINTEREST)!=0) {
                if (i==null) throw new NullArrayException("open interest");
                if (length==Integer.MIN_VALUE) {
                	length = i.length;
                } else if (length!=i.length) {
                	throw new WrongArrayLengthException("open interest");
                }
                this.i = i;
            	count++;
            }

            this.count = count;
            this.length = length;
        }
    
    
    
    /**
     * This method is deprecated. Use public field "length" instead.
     * @deprecated
     * @return length
     */
    public int getSize() {
        return length;
    }
    
    /**
     * Return the number of arrays stored in this PriceHolder.
     * 
     * @return
     */
    public int getCount() {
    	return this.count;
    }
    
    /**
     * Return the effective flags for the actually stored arrays.
     * 
     * @return
     */
    public int getFlags() {
    	return this.flags;
    }
    
    /**
     * This method returns an array which contains all the arrays stored by this class.
     * Notice that some arrays may be null, dependent on the flags informed.
     * 
     * @return an Object[] which contains <b>all<b> data point arrays OHLCVI.
     * 
     * @see PriceHolder#PriceHolder(int, double[], double[], double[], double[], double[], double[])
     * @see PriceHolder#PriceHolder(double[], double[], double[], double[], double[], double[])
     * @see PriceHolder#toEffectiveArrays()
     */
    public Object[] toArrays() {
        Object objs[] = new Object[6];
        int n = 0;
        objs[n++] = o;
        objs[n++] = h;
        objs[n++] = l;
        objs[n++] = c;
        objs[n++] = v;
        objs[n++] = i;
        return objs;
    }

    /**
     * This method returns an array which contains the effective arrays passed to the constructor of this class.
     * The definition of effective arrays are those which match the flags informed, are not null and match the same
     * length among them.
     * 
     * <p>
     * This method has a behavior convenient and compatible to 
     * {@link SimpleHelper#calculate(int, int, Object[], Object[], com.tictactec.ta.lib.MInteger, com.tictactec.ta.lib.MInteger)} 
     * 
     * @return an Object[] which contains the effective arrays used for calculations
     * @see PriceHolder#PriceHolder(int, double[], double[], double[], double[], double[], double[])
     * @see PriceHolder#PriceHolder(double[], double[], double[], double[], double[], double[])
     * @see PriceHolder#toArrays()
     * @see SimpleHelper#calculate(int, int, Object[], Object[], com.tictactec.ta.lib.MInteger, com.tictactec.ta.lib.MInteger)
     */
    public Object[] toEffectiveArrays() {
        Object objs[] = new Object[this.count];

        int n = 0;
        if ((flags&InputFlags.TA_IN_PRICE_OPEN)!=0) {
            objs[n++] = o;
        }
        if ((flags&InputFlags.TA_IN_PRICE_HIGH)!=0) {
            objs[n++] = h;
        }
        if ((flags&InputFlags.TA_IN_PRICE_LOW)!=0) {
            objs[n++] = l;
        }
        if ((flags&InputFlags.TA_IN_PRICE_CLOSE)!=0) {
            objs[n++] = c;
        }
        if ((flags&InputFlags.TA_IN_PRICE_VOLUME)!=0) {
            objs[n++] = v;
        }
        if ((flags&InputFlags.TA_IN_PRICE_OPENINTEREST)!=0) {
            objs[n++] = i;
        }
        return objs;
    }

    /**
     * 
     * @return the Open component
     */
    public double[] getO() { return o; }
    
    /**
     * 
     * @return the High component
     */
    public double[] getH() { return h; }
    
    /**
     * 
     * @return the Low component
     */
    public double[] getL() { return l; }
    
    /**
     * 
     * @return the Close component
     */
    public double[] getC() { return c; }
    
    /**
     * 
     * @return the Volume component
     */
    public double[] getV() { return v; }
    
    /**
     * 
     * @return the Open Interest component
     */
    public double[] getI() { return i; }
    

    
    private class NullArrayException extends NullPointerException {
        public NullArrayException(String name) {
            super(name + "array is null");
        }
    }

    private class WrongArrayLengthException extends IllegalArgumentException {
        public WrongArrayLengthException(String name) {
            super(name + "array has wrong length");
        }
    }

}
