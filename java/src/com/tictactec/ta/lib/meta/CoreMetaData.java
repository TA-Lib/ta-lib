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

import java.lang.annotation.Annotation;
import java.lang.annotation.IncompleteAnnotationException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;

import com.tictactec.ta.lib.CoreAnnotated;
import com.tictactec.ta.lib.MAType;
import com.tictactec.ta.lib.MInteger;
import com.tictactec.ta.lib.RetCode;
import com.tictactec.ta.lib.meta.annotation.FuncInfo;
import com.tictactec.ta.lib.meta.annotation.InputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.InputParameterType;
import com.tictactec.ta.lib.meta.annotation.IntegerList;
import com.tictactec.ta.lib.meta.annotation.IntegerRange;
import com.tictactec.ta.lib.meta.annotation.OptInputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.OptInputParameterType;
import com.tictactec.ta.lib.meta.annotation.OutputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.OutputParameterType;
import com.tictactec.ta.lib.meta.annotation.RealList;
import com.tictactec.ta.lib.meta.annotation.RealRange;

/**
 * CoreMetaData provides low level RTTI (Run Time Type Information) for TA functions. It also provides methods
 * for those willing to call a certain TA function using late binding techniques. These two functionalities let you
 * call TA functions dinamically. Even if more TA functions are added by the time, your application code will be able
 * to call all added functions by querying TA-Lib package about:
 * 
 * <li>TA functions available;
 * <li>function groups the TA functions belong to;
 * <li>input arguments of TA functions and also their run time type information;
 * <li>output arguments of TA functions and also their run time type information;
 * <li>optional input arguments of TA functions and also their run time type information;
 *
 * CoreMetaData is mostly intended for API developers and mimic as accurately as possible the functionality exposed by
 * <i>ta_abstract.h</i> "C" header file.

 * @see com.tictactec.ta.lib.meta.helpers.SimpleHelper is a simple API level helper class based on CoreMetaData
 * @see com.tictactec.ta.lib.meta.CoreMetaDataCompatibility for a "C" style interface, mostly intended for those traslating "C" code to Java.
 * 
 * @author Richard Gomes
 */
public class CoreMetaData implements Comparable<CoreMetaData>, Cloneable {
    
    private static transient final String CONTACT_DEVELOPERS = "Contact developers";
    private static transient final String INDEX_OUT_OF_BOUNDS = "Index out of bounds";
    private static transient final String ILLEGAL_NUMBER_OF_ARGUMENTS = "Illegal number of arguments";

    private static transient final String ARRAY_IS_NULL = "Array is null";
    private static transient final String INT_ARRAY_EXPECTED = "int[] expected";
    private static transient final String DOUBLE_ARRAY_EXPECTED = "double[] expected";
    private static transient final String PRICE_EXPECTED = "PriceInputParameter object expected";
    
    private static transient final Class<CoreAnnotated> coreClass = CoreAnnotated.class;
    private static transient final String LOOKBACK_SUFFIX = "Lookback";

    private static transient CoreAnnotated taCore = null;

    private String name = null;
    private Method function = null;
    private Method lookback = null;
    
    private static transient Map<String, CoreMetaData> taFuncMap = null;
    private static transient Map<String, Set<CoreMetaData> > taGrpMap = null;

    private transient Object callInputParams[] = null;
    private transient Object callOutputParams[] = null;
    private transient Object callOptInputParams[] = null;
    
    private transient FuncInfo funcInfo;
    private transient Annotation[][] parameterAnnotations;
    
    
    protected CoreMetaData() {
        synchronized (coreClass) {
            if (taCore==null) {
                taCore = new CoreAnnotated();
            }
        }
    }

    public int compareTo(CoreMetaData arg) {
        return this.name.compareTo(arg.name);
    }

    static private Map<String, CoreMetaData> getAllFuncs() {
        synchronized (coreClass) {
            if (taFuncMap == null) {
                taFuncMap = getTaFuncMetaInfoMap();
            }
        }
        return taFuncMap;
    }

    static private Map<String, Set<CoreMetaData> > getAllGrps() {
        synchronized (coreClass) {
            if (taGrpMap == null) {
                taGrpMap = getTaGrpMetaInfoMap();
            }
        }
        return taGrpMap;
    }

    static private Map<String, Method> getLookbackMethodMap() {
        Map<String, Method> map = new HashMap<String, Method>();
        Method[] ms = coreClass.getDeclaredMethods();
        for (Method m : ms) {
            if (m.getName().endsWith(LOOKBACK_SUFFIX)) {
                map.put(m.getName(), m);
                // System.out.println("lookback="+m.getName());
            }
        }
        return map;
    }

    static private Map<String, CoreMetaData> getTaFuncMetaInfoMap() {
        Map<String, CoreMetaData> result = new TreeMap<String, CoreMetaData>();
        Method[] ms = coreClass.getDeclaredMethods();
        Map<String, Method> lookbackMap = getLookbackMethodMap();
        for (Method funcMethod : ms) {
            String fn = funcMethod.getName();
            if (funcMethod.getReturnType().equals(RetCode.class))
            // && !fn.startsWith(INT_PREFIX))
            {
                String lookbackName = fn + LOOKBACK_SUFFIX;
                Method lookbackMethod = lookbackMap.get(lookbackName);
                if (lookbackMethod != null) {
                    FuncInfo info = getFuncInfo(funcMethod);
                    String funcName = info.name();
                    CoreMetaData mi = new CoreMetaData();
                    mi.name = funcName;
                    mi.function = funcMethod;
                    mi.lookback = lookbackMethod;
                    result.put(funcName, mi);
                }
            }
        }
        return result;
    }

    static private Map<String, Set<CoreMetaData> > getTaGrpMetaInfoMap() {
        if (taFuncMap==null) getAllFuncs();
        Map<String, Set<CoreMetaData> > result = new TreeMap<String, Set<CoreMetaData> >();
        for (String func : taFuncMap.keySet()) {
            CoreMetaData mi = taFuncMap.get(func);
            String group = mi.getFuncInfo().group();
            Set<CoreMetaData> set = result.get(group);
            if (set==null) {
                set = new TreeSet<CoreMetaData>();
                result.put(group, set);
            }
            set.add(mi);
        }
        return result;
    }
    
    static private FuncInfo getFuncInfo(Method method) throws IncompleteAnnotationException {
        FuncInfo annotation = method.getAnnotation(FuncInfo.class);
        if (annotation != null) return annotation;
        throw new IncompleteAnnotationException(FuncInfo.class, "Method " + method.getName());
    }

    static CoreMetaData getFuncHandle(final String name) throws NoSuchMethodException {
        CoreMetaData mi = getAllFuncs().get(name.toUpperCase());
        if (mi == null) throw new NoSuchMethodException(name.toUpperCase());
        mi.callInputParams = null;
        mi.callOutputParams = null;
        mi.callOptInputParams = null;
        if (mi != null) return mi;
        throw new NoSuchMethodException("Function " + name);
    }

    /**
     * Returns the instance which describes a TA function. This is a
     * convenience method that simply adopts the standard "getInstance"
     * convention. This method simply calls getFuncHandle.
     * 
     * @param name
     * @return an instance of CoreMetaData
     * @throws NoSuchMethodException
     */
    static public CoreMetaData getInstance(final String name) throws NoSuchMethodException {
        return getFuncHandle(name).clone();
    }
    
    @Override
    protected CoreMetaData clone() {
        try {
			getFuncInfo();
			getParameterAnnotations();
            CoreMetaData clone = (CoreMetaData) super.clone();
            callInputParams = null;
            callOutputParams = null;
            callOptInputParams = null;
            return clone;
        } catch (CloneNotSupportedException e) {
            //we implement Cloneable interface, this exception cannot happen
            return null;
        }
    }

    /**
     * Returns an annotation which describes this TA function.
     * 
     * @return an @interface FuncInfo
     * @throws IncompleteAnnotationException
     */
    public FuncInfo getFuncInfo() throws IncompleteAnnotationException {
        if (funcInfo == null) {
            funcInfo = getFuncInfo(function);
        }
        return funcInfo;
    }

    private Annotation getParameterInfo(final int paramIndex, Class<? extends Object> paramAnnotation) {
        if (paramIndex < 0)
            throw new IllegalArgumentException(INDEX_OUT_OF_BOUNDS);
        int i = 0;
        for (Annotation[] annArray : getParameterAnnotations()) {
            for (Annotation ann : annArray) {
                if ((ann.annotationType() == paramAnnotation) && (paramIndex == i++)) {
                    return ann;
                }
            }
        }
        return null;
    }
    
    private Annotation[][] getParameterAnnotations() {
        if (parameterAnnotations == null) {
            parameterAnnotations = function.getParameterAnnotations();
        }
        return parameterAnnotations;
    }

    private Annotation getParameterInfo(final int paramIndex, Class<? extends Object> paramAnnotation, Class<? extends Object> paramExtraAnnotation) {
        if (paramIndex < 0)
            throw new IllegalArgumentException(INDEX_OUT_OF_BOUNDS);
        int i = 0;
        for (Annotation[] annArray : getParameterAnnotations()) {
            for (Annotation ann : annArray) {
                if ((ann.annotationType() == paramAnnotation) && (paramIndex == i++)) {
                    for (Annotation annExt : annArray) {
                        if (annExt.annotationType() == paramExtraAnnotation) {
                            return annExt;
                        }
                    }
                }
            }
        }
        return null;
    }

    /**
     * Returns an annotation which describes the n-th input parameter requested, if any.
     * 
     * @param paramIndex is the n-th input parameter
     * @return an @interface InputParameterInfo
     * @throws IllegalArgumentException
     */
    public InputParameterInfo getInputParameterInfo(final int paramIndex) throws IllegalArgumentException {
        return (InputParameterInfo) getParameterInfo(paramIndex, InputParameterInfo.class);
    }

    /**
     * Returns an annotation which describes the n-th output parameter requested, if any.
     * 
     * @param paramIndex is the n-th output parameter
     * @return an @interface OutputParameterInfo
     * @throws IllegalArgumentException
     */
    public OutputParameterInfo getOutputParameterInfo(final int paramIndex) throws IllegalArgumentException {
        return (OutputParameterInfo) getParameterInfo(paramIndex, OutputParameterInfo.class);
    }

    /**
     * Returns an annotation which describes the n-th optional input parameter requested, if any.
     * 
     * @param paramIndex is the n-th optional input parameter
     * @return an @interface OptInputParameterInfo
     * @throws IllegalArgumentException
     */
    public OptInputParameterInfo getOptInputParameterInfo(final int paramIndex) throws IllegalArgumentException {
        return (OptInputParameterInfo) getParameterInfo(paramIndex, OptInputParameterInfo.class);
    }

    /**
     * Returns an annotation describing an optional input parameter which type
     * is expected to be an IntegerList
     * 
     * @param paramIndex is the n-th optional input parameter
     * @return an @interface IntegerList
     * @throws IllegalArgumentException
     */
    public IntegerList getOptInputIntegerList(final int paramIndex) throws IllegalArgumentException {
        return (IntegerList) getParameterInfo(paramIndex, OptInputParameterInfo.class, IntegerList.class);
    }

    /**
     * Returns an annotation describing an optional input parameter which type
     * is expected to be an IntegerRange
     * 
     * @param paramIndex is the n-th optional input parameter
     * @return an @interface IntegerRange
     * @throws IllegalArgumentException
     */
    public IntegerRange getOptInputIntegerRange(final int paramIndex) throws IllegalArgumentException {
        return (IntegerRange) getParameterInfo(paramIndex, OptInputParameterInfo.class, IntegerRange.class);
    }

    /**
     * Returns an annotation describing an optional input parameter which type
     * is expected to be an RealList
     * 
     * @param paramIndex is the n-th optional input parameter
     * @return an @interface RealList
     * @throws IllegalArgumentException
     */
    public RealList getOptInputRealList(final int paramIndex) throws IllegalArgumentException {
        return (RealList) getParameterInfo(paramIndex, OptInputParameterInfo.class, RealList.class);
    }

    /**
     * Returns an annotation describing an optional input parameter which type
     * is expected to be an RealRange
     * 
     * @param paramIndex is the n-th optional input parameter
     * @return an @interface RealRange
     * @throws IllegalArgumentException
     */
    public RealRange getOptInputRealRange(final int paramIndex) throws IllegalArgumentException {
        return (RealRange) getParameterInfo(paramIndex, OptInputParameterInfo.class, RealRange.class);
    }

    /**
     * Assigns an <b>int</b> value to an optional input parameter
     * which is expected to be assignment compatible to <b>int</b>.
     * 
     * @param paramIndex is the n-th optional input parameter
     * @param value is the <b>int</b> value
     * @throws IllegalArgumentException
     */
    public void setOptInputParamInteger(final int paramIndex, final int value) throws IllegalArgumentException {
        OptInputParameterInfo param = getOptInputParameterInfo(paramIndex);
        if (param==null) throw new InternalError(CONTACT_DEVELOPERS);
        if (param.type()==OptInputParameterType.TA_OptInput_IntegerList) {
            IntegerList list = getOptInputIntegerList(paramIndex);
            int[] values = list.value();
            for (int i = 0; i < values.length; i++) {
                if (value==values[i]) {
                    String strValue = list.string()[i];
                    
                    // FIXME: The correct implementation should ...
                    // expose a field in @IntegerList informing
                    // which Class should be taken for introspection.
                    // Currently, all IntegerList instances implicitly depend on MAType class
                    // but it may change some day.
                    
                    MAType[] fields = MAType.values();
                    for (MAType maType : fields) {
                        if (maType.name().toUpperCase().equals(strValue.toUpperCase())) {
                            if (callOptInputParams==null) callOptInputParams = new Object[getFuncInfo().nbOptInput()];
                            callOptInputParams[paramIndex] = maType;
                            return;
                        }
                    }
                }
            }
        } else if (param.type()==OptInputParameterType.TA_OptInput_IntegerRange) {
            IntegerRange range = getOptInputIntegerRange(paramIndex);
            if ((value >= range.min())&&(value <= range.max())) {
                if (callOptInputParams==null) callOptInputParams = new Object[getFuncInfo().nbOptInput()];
                callOptInputParams[paramIndex] = value;
                return;
            }
        } 
        throw new InternalError(CONTACT_DEVELOPERS);
    }

    /**
     * Assigns an <b>int</b> value obtained from a <b>String</b> to an optional input parameter
     * which is expected to be assignment compatible to <b>int</b>.
     * 
     * @param paramIndex is the n-th optional input parameter
     * @param string is the <b>String</b> which must hold an <b>int</b> value
     * @throws IllegalArgumentException
     */
    public void setOptInputParamInteger(final int paramIndex, final String string) throws IllegalArgumentException {
        try {
            Integer v = new Integer(string);
            setOptInputParamInteger(paramIndex, v.intValue());
        } catch (NumberFormatException e) {
            OptInputParameterInfo param = getOptInputParameterInfo(paramIndex);
            if (param==null) throw new InternalError(CONTACT_DEVELOPERS);
            if (param.type()!=OptInputParameterType.TA_OptInput_IntegerList) throw new InternalError(CONTACT_DEVELOPERS);

            // FIXME: The correct implementation should ...
            // expose a field in @IntegerList informing
            // which Class should be taken for introspection.
            // Currently, all IntegerList instances implicitly depend on MAType class
            // but it may change some day.
            
            MAType[] fields = MAType.values();
            for (MAType value : fields) {
                if (value.name().toUpperCase().equals(string.toUpperCase())) {
                    if (callOptInputParams==null) callOptInputParams = new Object[getFuncInfo().nbOptInput()];
                    callOptInputParams[paramIndex] = value;
                    return;
                }
            }
            throw new InternalError(CONTACT_DEVELOPERS);
        }
    }

    /**
     * Assigns an <b>double</b> value to an optional input parameter
     * which is expected to be assignment compatible to <b>double</b>.
     * 
     * @param paramIndex is the n-th optional input parameter
     * @param value is the <b>double</b> value
     * @throws IllegalArgumentException
     */
    public void setOptInputParamReal(final int paramIndex, final double value) throws IllegalArgumentException {
        OptInputParameterInfo param = getOptInputParameterInfo(paramIndex);
        if (param.type()==OptInputParameterType.TA_OptInput_RealList) {
            RealList list = getOptInputRealList(paramIndex);
            for (double entry : list.value()) {
                if (value==entry) {
                    if (callOptInputParams==null) callOptInputParams = new Object[getFuncInfo().nbOptInput()];
                    callOptInputParams[paramIndex] = value;
                    return;
                }
            }
        } else if (param.type()==OptInputParameterType.TA_OptInput_RealRange) {
            RealRange range = getOptInputRealRange(paramIndex);
            if ((value >= range.min())&&(value <= range.max())) {
                if (callOptInputParams==null) callOptInputParams = new Object[getFuncInfo().nbOptInput()];
                callOptInputParams[paramIndex] = value;
                return;
            }
        }
        throw new InternalError(CONTACT_DEVELOPERS);
    }

    /**
     * Assigns an <b>double</b> value obtained from a <b>String</b> to an optional input parameter
     * which is expected to be assignment compatible to <b>double</b>.
     * 
     * @param paramIndex is the n-th optional input parameter
     * @param string is the <b>String</b> which must hold an <b>double</b> value
     * @throws IllegalArgumentException
     */
    public void setOptInputParamReal(final int paramIndex, final String string) throws IllegalArgumentException {
        try {
            Double v = new Double(string);
            setOptInputParamReal(paramIndex, v.doubleValue());
        } catch (NumberFormatException e) {
            OptInputParameterInfo param = getOptInputParameterInfo(paramIndex);
            if (param==null) throw new InternalError(CONTACT_DEVELOPERS);
            if (param.type()==OptInputParameterType.TA_OptInput_RealList) {
                RealList list = getOptInputRealList(paramIndex);
                for (int i=0; i<list.string().length; i++) {
                    if (string.toUpperCase().equals(list.string()[i])) {
                        if (callOptInputParams==null) callOptInputParams = new Object[getFuncInfo().nbOptInput()];
                        double value = list.value()[i];
                        callOptInputParams[paramIndex] = value;
                        return;
                    }
                }
            }
            throw new InternalError(CONTACT_DEVELOPERS);
        }
    }

    /**
     * Assigns an Object which is expected to be assignment compatible of <b>double[]</b> to
     * an input parameter which is expected to be assignment compatible of <b>double[]</b>.
     * 
     * @param paramIndex is the n-th input parameter
     * @param array is an Object expected to be assignment compatible to <b>double[]</b>
     * @throws IllegalArgumentException
     * @throws NullPointerException
     */
    public void setInputParamReal(final int paramIndex, final Object array) throws IllegalArgumentException, NullPointerException {
        if (array==null) throw new NullPointerException(ARRAY_IS_NULL);
        InputParameterInfo param = getInputParameterInfo(paramIndex);
        if ((param==null) || (param.type()!=InputParameterType.TA_Input_Real)) throw new InternalError(CONTACT_DEVELOPERS);
        if (! (array instanceof double[]) ) throw new IllegalArgumentException(DOUBLE_ARRAY_EXPECTED);
        if (callInputParams==null) callInputParams = new Object[getFuncInfo().nbInput()];
        callInputParams[paramIndex] = array;
    }

    /**
     * Assigns an Object which is expected to be assignment compatible of <b>int[]</b> to
     * an input parameter which is expected to be assignment compatible of <b>int[]</b>.
     * 
     * @param paramIndex is the n-th input parameter
     * @param array is an Object expected to be assignment compatible to <b>int[]</b>
     * @throws IllegalArgumentException
     * @throws NullPointerException
     */
    public void setInputParamInteger(final int paramIndex, final Object array) throws IllegalArgumentException, NullPointerException {
        if (array==null) throw new NullPointerException(ARRAY_IS_NULL);
        InputParameterInfo param = getInputParameterInfo(paramIndex);
        if ((param==null) || (param.type()!=InputParameterType.TA_Input_Integer)) throw new InternalError(CONTACT_DEVELOPERS);
        if (! (array instanceof int[]) ) throw new IllegalArgumentException(INT_ARRAY_EXPECTED);
        if (callInputParams==null) callInputParams = new Object[getFuncInfo().nbInput()];
        callInputParams[paramIndex] = array;
    }

    /**
     * Assigns Objects which are expected to be assignment compatible of <b>double[]</b> to
     * an input parameter which is expected to be assignment compatible of <b>PriceInputParameter</b>.
     * 
     * <p>You only need to pass those parameters strictly needed by the n-th input parameter of a certain TA function.
     * 
     * @param paramIndex is the n-th input parameter
     * @param open represents the open prices. This Object expected to be assignment compatible to <b>double[]</b>
     * @param high represents the high prices. This Object expected to be assignment compatible to <b>double[]</b>
     * @param low represents the low prices. This Object expected to be assignment compatible to <b>double[]</b>
     * @param close represents the close prices. This Object expected to be assignment compatible to <b>double[]</b>
     * @param volume represents the volume. This Object expected to be assignment compatible to <b>double[]</b>
     * @param openInterest represents the open interest. This Object expected to be assignment compatible to <b>double[]</b>
     * @throws IllegalArgumentException
     * @throws NullPointerException
     */
    public void setInputParamPrice(final int paramIndex,
                final double[] open, final double[] high, final double[] low, final double[] close,
                final double[] volume, final double[] openInterest)
            throws IllegalArgumentException, NullPointerException {
        InputParameterInfo param = getInputParameterInfo(paramIndex);
        if ((param==null) || (param.type()!=InputParameterType.TA_Input_Price)) throw new InternalError(CONTACT_DEVELOPERS);
        if (callInputParams==null) callInputParams = new Object[getFuncInfo().nbInput()];
        callInputParams[paramIndex] = new PriceHolder(param.flags(), open, high, low, close, volume, openInterest);
    }

    /**
     * Assigns an Object which are expected to be assignment compatible of <b>PriceInputParameter</b> to
     * an input parameter which is expected to be assignment compatible of <b>PriceInputParameter</b>.
     * 
     * @param paramIndex is the n-th input parameter
     * @param array is an Object expected to be assignment compatible to <b>PriceInputParameter</b>
     * @throws IllegalArgumentException
     * @throws NullPointerException
     */
    public void setInputParamPrice(final int paramIndex, final Object array) throws IllegalArgumentException, NullPointerException {
        if (array==null) throw new NullPointerException(ARRAY_IS_NULL);
        InputParameterInfo param = getInputParameterInfo(paramIndex);
        if ((param==null) || (param.type()!=InputParameterType.TA_Input_Price)) throw new InternalError(CONTACT_DEVELOPERS);
        if (! (array instanceof PriceHolder) ) throw new IllegalArgumentException(PRICE_EXPECTED);
        if (callInputParams==null) callInputParams = new Object[getFuncInfo().nbInput()];
        callInputParams[paramIndex] = array;
    }

    /**
     * Assigns an Object which are expected to be assignment compatible of <b>double[]</b> to
     * an output parameter which is expected to be assignment compatible of <b>double[]</b>.
     * 
     * @param paramIndex is the n-th output parameter
     * @param array is an Object expected to be assignment compatible to <b>double[]</b>
     * @throws IllegalArgumentException
     * @throws NullPointerException
     * @throws ClassCastException
     */
    public void setOutputParamReal(final int paramIndex, Object array) throws IllegalArgumentException, NullPointerException, ClassCastException {
        if (array==null) throw new NullPointerException(ARRAY_IS_NULL);
        OutputParameterInfo param = getOutputParameterInfo(paramIndex);
        if ((param==null)||(param.type()!=OutputParameterType.TA_Output_Real)) throw new InternalError(CONTACT_DEVELOPERS);
        if (! (array instanceof double[]) ) throw new IllegalArgumentException(DOUBLE_ARRAY_EXPECTED);
        if (callOutputParams==null) callOutputParams = new Object[getFuncInfo().nbOutput()];
        callOutputParams[paramIndex] = array;
    }

    /**
     * Assigns an Object which are expected to be assignment compatible of <b>int[]</b> to
     * an output parameter which is expected to be assignment compatible of <b>int[]</b>.
     * 
     * @param paramIndex is the n-th output parameter
     * @param array is an Object expected to be assignment compatible to <b>int[]</b>
     * @throws IllegalArgumentException
     * @throws NullPointerException
     * @throws ClassCastException
     */
    public void setOutputParamInteger(final int paramIndex, Object array) throws IllegalArgumentException, NullPointerException, ClassCastException {
        if (array==null) throw new NullPointerException(ARRAY_IS_NULL);
        OutputParameterInfo param = getOutputParameterInfo(paramIndex);
        if ((param==null)||(param.type()!=OutputParameterType.TA_Output_Integer)) throw new InternalError(CONTACT_DEVELOPERS);
        if (! (array instanceof int[]) ) throw new IllegalArgumentException(INT_ARRAY_EXPECTED);
        if (callOutputParams==null) callOutputParams = new Object[getFuncInfo().nbOutput()];
        callOutputParams[paramIndex] = array;
    }

    /**
     * For each defined TA function executes the <b>TaFuncService.execute()</b> interface method of the implementation
     * class passed ar argument. 
     * 
     * @param service is a <b>TaFuncService</b> implementation class
     * @throws Exception
     */
    static public void forEachFunc(TaFuncService service) throws Exception {
        for (CoreMetaData mi : getAllFuncs().values()) {
            service.execute(mi);
        }
    }

    /**
     * For each defined group of TA functions executes the <b>TaFuncService.execute()</b> interface method of the implementation
     * class passed ar argument.
     * 
     * @param service
     * @throws Exception
     */
    static public void forEachGrp(TaGrpService service) throws Exception {
        for (String group : getAllGrps().keySet()) {
            service.execute(group, taGrpMap.get(group));
        }
    }

    private Object[] getOptInputParameters() {
        int size = getFuncInfo().nbOptInput();
        if (callOptInputParams==null) callOptInputParams = new Object[size];
        for (int i=0; i<size; i++) {
            if (callOptInputParams[i]==null) {
                OptInputParameterInfo param = getOptInputParameterInfo(i);
                if (param==null) throw new InternalError(CONTACT_DEVELOPERS);
                if (param.type()==OptInputParameterType.TA_OptInput_IntegerList) {
                    IntegerList list = getOptInputIntegerList(i);
                    callOptInputParams[i] = list.defaultValue();
                } else if (param.type()==OptInputParameterType.TA_OptInput_IntegerRange) {
                    IntegerRange range = getOptInputIntegerRange(i);
                    callOptInputParams[i] = range.defaultValue();
                } else if (param.type()==OptInputParameterType.TA_OptInput_RealList) {
                    RealList list = getOptInputRealList(i);
                    callOptInputParams[i] = list.defaultValue();
                } else if (param.type()==OptInputParameterType.TA_OptInput_RealRange) {
                    RealRange range = getOptInputRealRange(i);
                    callOptInputParams[i] = range.defaultValue();
                } else {
                    throw new InternalError(CONTACT_DEVELOPERS);
                }
            }
        }
        return callOptInputParams;
    }
    

    /**
     * Returns the lookback.
     * 
     * <p> Lookback is the number of input data points to be consumed in order to calculate the first output data point. This value
     * is affected by the optional input arguments passed to this TA function.
     *  
     * @return the lookback number of input points to be consumed before the first output data point is produced.
     * @throws IllegalArgumentException
     * @throws IllegalAccessException
     * @throws InvocationTargetException
     */
    public int getLookback() throws IllegalArgumentException, IllegalAccessException, InvocationTargetException {
        Object[] params = getOptInputParameters();
        return (Integer) lookback.invoke(taCore, params);
    }
    
    
    /**
     * Executes the calculations defined by this TA function.
     * 
     * <p> You need to provide input arguments where this TA function will obtain data from and output arguments where this
     * TA function will write output data to.
     * 
     * @see com.tictactec.ta.lib.meta.helpers.SimpleHelper class for an example of use.
     * 
     * @param startIndex is the initial position of input data to be considered for TA function calculations
     * @param endIndex is the final position of input data to be considered for TA function calculations
     * @param outBegIdx is returned by this method and represents the initial position of output data returned by this TA function
     * @param outNbElement is returned by this method and represents the quantity of output data returned by this TA function
     * @throws IllegalArgumentException
     * @throws IllegalAccessException
     * @throws InvocationTargetException
     */
    public void callFunc(final int startIndex, final int endIndex, MInteger outBegIdx, MInteger outNbElement) 
            throws IllegalArgumentException, IllegalAccessException, InvocationTargetException {

        int count = 0;
        for (Object item : callInputParams) {
            if (PriceHolder.class.isAssignableFrom(item.getClass())) {
                count += ((PriceHolder)item).getCount();
            } else {
                count++;
            }
        }
        count += callOutputParams.length;
        
        if (callOptInputParams==null) callOptInputParams = new Object[getFuncInfo().nbOptInput()];
        count += callOptInputParams.length;
        
        Object[] params = new Object[count+4];
        count = 0;
        params[count++] = startIndex;
        params[count++] = endIndex;

        for (Object item : callInputParams) {
            if (PriceHolder.class.isAssignableFrom(item.getClass())) {
                Object objs[] = ((PriceHolder)item).toEffectiveArrays();
                for (int i=0; i<objs.length; i++) {
                    params[count++] = objs[i];
                }
            } else {
                params[count++] = item;
            }
        }

        for (Object item : callOptInputParams) {
            params[count++] = item;
        }

        params[count++] = outBegIdx;
        params[count++] = outNbElement;

        for (Object item : callOutputParams) {
            params[count++] = item;
        }
        
        Type[] types = function.getGenericParameterTypes();
        if (types.length != params.length) throw new IllegalArgumentException(ILLEGAL_NUMBER_OF_ARGUMENTS);
        //for (int i=0; i<types.length; i++) {
        //    if (! (params[i].getClass().isAssignableFrom(types[i])) ) {
        //        throw new IllegalArgumentException("Type mismatch on argument "+i+": "+types[i]);
        //    }
        //}
        
        function.invoke(taCore, params);
    }

}
