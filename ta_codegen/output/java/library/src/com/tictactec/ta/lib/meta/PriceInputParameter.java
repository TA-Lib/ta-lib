package com.tictactec.ta.lib.meta;

import com.tictactec.ta.lib.meta.annotation.InputFlags;

/**
 * This class is deprecated. Do not use it.
 * 
 * @deprecated
 * 
 * @author Richard Gomes
 *
 */
public class PriceInputParameter extends PriceHolder {

    private int flags;
    private int count;

    public PriceInputParameter(final int flags, double[] o, double[] h, double[] l, double[] c, double[] v, double[] i) 
            throws NullPointerException, IllegalArgumentException {
        super(o, h, l, c, v, i);
        initialize(flags);
    }
    
    public PriceInputParameter(final PriceHolder p) {
        super(p.getO(), p.getH(), p.getL(), p.getC(), p.getV(), p.getI());
        initialize(flags);
    }
    
    private void initialize(final int flags) {
        this.flags = flags;
        count = 0;
        count += ((flags&InputFlags.TA_IN_PRICE_OPEN)!=0) ? 1 : 0;
        count += ((flags&InputFlags.TA_IN_PRICE_HIGH)!=0) ? 1 : 0;
        count += ((flags&InputFlags.TA_IN_PRICE_LOW)!=0) ? 1 : 0;
        count += ((flags&InputFlags.TA_IN_PRICE_CLOSE)!=0) ? 1 : 0;
        count += ((flags&InputFlags.TA_IN_PRICE_VOLUME)!=0) ? 1 : 0;
        count += ((flags&InputFlags.TA_IN_PRICE_OPENINTEREST)!=0) ? 1 : 0;
    }
    
    /**
     * @return the InputFlags passed during construction of this class
     */
    public int getFlags() {
        return flags;
    }
    
    /**
     * Returns the number of arrays expected to be returned by toArrays().
     * This number is based on the flags passed during the construction of this object.
     * 
     * @return the number of arrays expected to be returned by toArrays()
     */
    public int getCount() {
        return count;
    }
    
    /**
     * @return the arrays corresponding to the flags passed during construction of this object.
     */
    public Object[] toArrays() {
        Object objs[] = new Object[count];
        int n = 0;
        
        if ((flags&InputFlags.TA_IN_PRICE_OPEN)!=0) {
            objs[n++] = getO();
        }
        if ((flags&InputFlags.TA_IN_PRICE_HIGH)!=0) {
            objs[n++] = getH();
        }
        if ((flags&InputFlags.TA_IN_PRICE_LOW)!=0) {
            objs[n++] = getL();
        }
        if ((flags&InputFlags.TA_IN_PRICE_CLOSE)!=0) {
            objs[n++] = getC();
        }
        if ((flags&InputFlags.TA_IN_PRICE_VOLUME)!=0) {
            objs[n++] = getV();
        }
        if ((flags&InputFlags.TA_IN_PRICE_OPENINTEREST)!=0) {
            objs[n++] = getI();
        }

        return objs;
    }

}
