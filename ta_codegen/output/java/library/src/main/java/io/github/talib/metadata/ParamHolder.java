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

/* GENERATED FILE — do not edit. Produced by ta_codegen
 * (generator/src/backends/java_metadata.rs) from ta_codegen/input/.
 * MF,CC
 */

package io.github.talib.metadata;

import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

/**
 * Binds arguments to a function chosen at run time, then calls it.
 *
 * <p>The counterpart of C's {@code TA_ParamHolder}, for an application that does
 * not know at compile time which indicator it will run — a charting UI listing
 * every study, a parameter sweep. Obtain one from
 * {@link FunctionInfo#newCall()}:
 *
 * <pre>{@code
 * FunctionInfo f = Functions.byName("SMA");
 * OutRange r = f.newCall()
 *     .setInput(0, close)
 *     .setOptInput(0, 30)
 *     .setOutput(0, out)
 *     .call(0, close.length - 1);
 * }</pre>
 *
 * <p>Everything is validated against the {@link FunctionInfo} row: an index out
 * of bounds, a type that does not match the declared parameter, or an unset
 * parameter at {@link #call} time throws {@link IllegalArgumentException}. The
 * call itself then behaves exactly like the typed method — including throwing
 * on misuse and returning an empty {@link OutRange} when the range is shorter
 * than the lookback.
 *
 * <p>Not thread-safe: confine one holder to one thread, or build one per call.
 */
public final class ParamHolder {

   private final FunctionInfo info;
   private final Core core;

   /** Per input slot: a real series, an int series, or the six price components. */
   private final double[][] realInputs;
   private final int[][] intInputs;
   private final double[][][] priceInputs;   // [slot][component] in OHLCV order

   private final double[] realOpts;
   private final int[] intOpts;
   private final MAType[] maTypeOpts;
   private final boolean[] optSet;

   private final double[][] realOutputs;
   private final int[][] intOutputs;

   ParamHolder(FunctionInfo info, Core core) {
      this.info = info;
      this.core = core;
      int ni = info.inputs().size();
      int no = info.optInputs().size();
      int nout = info.outputs().size();
      this.realInputs = new double[ni][];
      this.intInputs = new int[ni][];
      this.priceInputs = new double[ni][][];
      this.realOpts = new double[no];
      this.intOpts = new int[no];
      this.maTypeOpts = new MAType[no];
      this.optSet = new boolean[no];
      this.realOutputs = new double[nout][];
      this.intOutputs = new int[nout][];
   }

   /** The function this holder calls. */
   public FunctionInfo info() {
      return info;
   }

   private void checkInput(int idx, InputType expected) {
      if (idx < 0 || idx >= info.inputs().size()) {
         throw new IllegalArgumentException(
            info.name() + ": input index " + idx + " out of range [0, " + info.inputs().size() + ")");
      }
      InputType actual = info.inputs().get(idx).type();
      if (actual != expected) {
         throw new IllegalArgumentException(
            info.name() + " input " + idx + " (" + info.inputs().get(idx).paramName()
            + ") is " + actual + ", not " + expected);
      }
   }

   /** Binds a {@link InputType#REAL} input. */
   public ParamHolder setInput(int idx, double[] series) {
      checkInput(idx, InputType.REAL);
      realInputs[idx] = require(series, "input " + idx);
      return this;
   }

   /** Binds an {@link InputType#INTEGER} input. */
   public ParamHolder setInput(int idx, int[] series) {
      checkInput(idx, InputType.INTEGER);
      intInputs[idx] = require(series, "input " + idx);
      return this;
   }

   /**
    * Binds an {@link InputType#PRICE} input. Pass {@code null} for any component
    * the function does not consume — {@link InputInfo#flags()} says which it does
    * (see {@link InputFlags}). Mirrors C's {@code TA_SetInputParamPricePtr}.
    */
   public ParamHolder setPriceInput(int idx, double[] open, double[] high, double[] low,
                                    double[] close, double[] volume, double[] openInterest) {
      checkInput(idx, InputType.PRICE);
      double[][] c = { open, high, low, close, volume, openInterest };
      int flags = info.inputs().get(idx).flags();
      int[] bits = { InputFlags.PRICE_OPEN, InputFlags.PRICE_HIGH, InputFlags.PRICE_LOW,
                     InputFlags.PRICE_CLOSE, InputFlags.PRICE_VOLUME, InputFlags.PRICE_OPENINTEREST };
      String[] names = { "open", "high", "low", "close", "volume", "openInterest" };
      for (int k = 0; k < c.length; k++) {
         if ((flags & bits[k]) != 0 && c[k] == null) {
            throw new IllegalArgumentException(
               info.name() + " input " + idx + " requires " + names[k]);
         }
      }
      priceInputs[idx] = c;
      return this;
   }

   private void checkOpt(int idx, OptInputType... expected) {
      if (idx < 0 || idx >= info.optInputs().size()) {
         throw new IllegalArgumentException(
            info.name() + ": optInput index " + idx + " out of range [0, "
            + info.optInputs().size() + ")");
      }
      OptInputType actual = info.optInputs().get(idx).type();
      for (OptInputType e : expected) {
         if (actual == e) {
            return;
         }
      }
      throw new IllegalArgumentException(
         info.name() + " optInput " + idx + " (" + info.optInputs().get(idx).paramName()
         + ") is " + actual + ", not " + java.util.Arrays.toString(expected));
   }

   /** Binds an {@link OptInputType#INTEGER_RANGE} or {@link OptInputType#INTEGER_LIST} parameter. */
   public ParamHolder setOptInput(int idx, int value) {
      checkOpt(idx, OptInputType.INTEGER_RANGE, OptInputType.INTEGER_LIST);
      if (info.optInputs().get(idx).type() == OptInputType.INTEGER_LIST) {
         MAType[] all = MAType.values();
         /* Setting a parameter to its documented default THROUGH the abstract
            interface is part of the ABI: C's TA_SetOptInputParamInteger accepts
            TA_INTEGER_DEFAULT and the function substitutes the declared default.
            Java cannot carry the sentinel any further than here -- Core takes a
            real MAType -- so it resolves at this boundary, which is exactly where
            an UNSET choice list already resolves (see the constructor). Leaving
            the two to disagree was issue #164's first finding. */
         if (value == Core.INTEGER_DEFAULT) {
            int declared = (int) info.optInputs().get(idx).defaultValue();
            maTypeOpts[idx] = all[declared];
            intOpts[idx] = declared;
         } else if (value < 0 || value >= all.length) {
            /* Ahead of the write below, not after it. Reversed, a rejected ordinal
               left `value` in intOpts -- unobservable, since resolveUnsetOptInputs
               rewrites an unset slot and Dispatch reads a choice list through
               maTypeOpt(), never intOpt() -- but it broke the same rule
               setPriceInput breaks visibly: a rejected setter must leave the
               holder as it found it (issue #266). */
            throw new IllegalArgumentException(
               info.name() + " optInput " + idx + ": " + value + " is not a valid MAType ordinal");
         } else {
            maTypeOpts[idx] = all[value];
            intOpts[idx] = value;
         }
      } else {
         intOpts[idx] = value;
      }
      optSet[idx] = true;
      return this;
   }

   /** Binds an {@link OptInputType#REAL_RANGE} parameter. */
   public ParamHolder setOptInput(int idx, double value) {
      checkOpt(idx, OptInputType.REAL_RANGE, OptInputType.REAL_LIST);
      realOpts[idx] = value;
      optSet[idx] = true;
      return this;
   }

   /** Binds an {@link OptInputType#INTEGER_LIST} (moving-average type) parameter. */
   public ParamHolder setOptInput(int idx, MAType value) {
      checkOpt(idx, OptInputType.INTEGER_LIST);
      maTypeOpts[idx] = require(value, "optInput " + idx);
      intOpts[idx] = value.ordinal();
      optSet[idx] = true;
      return this;
   }

   private void checkOutput(int idx, OutputType expected) {
      if (idx < 0 || idx >= info.outputs().size()) {
         throw new IllegalArgumentException(
            info.name() + ": output index " + idx + " out of range [0, "
            + info.outputs().size() + ")");
      }
      OutputType actual = info.outputs().get(idx).type();
      if (actual != expected) {
         throw new IllegalArgumentException(
            info.name() + " output " + idx + " (" + info.outputs().get(idx).paramName()
            + ") is " + actual + ", not " + expected);
      }
   }

   /** Binds an {@link OutputType#REAL} output array. */
   public ParamHolder setOutput(int idx, double[] out) {
      checkOutput(idx, OutputType.REAL);
      realOutputs[idx] = require(out, "output " + idx);
      return this;
   }

   /** Binds an {@link OutputType#INTEGER} output array. */
   public ParamHolder setOutput(int idx, int[] out) {
      checkOutput(idx, OutputType.INTEGER);
      intOutputs[idx] = require(out, "output " + idx);
      return this;
   }

   /**
    * The first index at which this function produces output, for the optional
    * parameters bound so far.
    *
    * <p>The counterpart of C's {@code TA_GetLookback} and C#'s
    * {@code FunctionCall.Lookback()}. Inputs and outputs need not be bound --
    * a lookback depends only on the optional parameters, which is what makes it
    * useful for sizing the output arrays before binding them.
    *
    * @return the lookback, or {@code -1} if a parameter is out of range
    */
   public int lookback() {
      resolveUnsetOptInputs();
      return Dispatch.lookback(this);
   }

   /**
    * Calls the function over {@code [startIdx, endIdx]}.
    *
    * <p>Unbound parameters that carry a documented default are filled in with it;
    * unbound inputs or outputs are an error.
    *
    * @throws IllegalArgumentException if a required parameter was never bound
    */
   public OutRange call(int startIdx, int endIdx) {
      for (int i = 0; i < info.inputs().size(); i++) {
         boolean bound = switch (info.inputs().get(i).type()) {
            case REAL -> realInputs[i] != null;
            case INTEGER -> intInputs[i] != null;
            case PRICE -> priceInputs[i] != null;
         };
         if (!bound) {
            throw new IllegalArgumentException(
               info.name() + ": input " + i + " (" + info.inputs().get(i).paramName() + ") not set");
         }
      }
      for (int i = 0; i < info.outputs().size(); i++) {
         boolean bound = info.outputs().get(i).type() == OutputType.REAL
            ? realOutputs[i] != null : intOutputs[i] != null;
         if (!bound) {
            throw new IllegalArgumentException(
               info.name() + ": output " + i + " (" + info.outputs().get(i).paramName() + ") not set");
         }
      }
      resolveUnsetOptInputs();
      return Dispatch.call(this, startIdx, endIdx);
   }

   /* Unset optional parameters take the cross-language default sentinel, which
      every generated function maps to its documented default. Shared by call()
      and lookback(): it used to live inside call(), so a lookback taken before
      the first call read zero-initialised slots and came back -1 for every
      function with an optional parameter. */
   private void resolveUnsetOptInputs() {
      for (int i = 0; i < info.optInputs().size(); i++) {
         if (optSet[i]) {
            continue;
         }
         switch (info.optInputs().get(i).type()) {
            case REAL_RANGE, REAL_LIST -> realOpts[i] = -4e37;
            case INTEGER_RANGE -> intOpts[i] = Integer.MIN_VALUE;
            /* Both slots, not just maTypeOpts: setOptInput's sentinel branch keeps
               them in step, and leaving unset to record intOpts=0 while recording
               maTypeOpts=Ema (APO/PPO/PVO default to 1) would reintroduce the very
               unset-vs-sentinel divergence this pair of methods exists to remove. */
            case INTEGER_LIST -> {
               int declared = (int) info.optInputs().get(i).defaultValue();
               maTypeOpts[i] = MAType.values()[declared];
               intOpts[i] = declared;
            }
         }
      }
   }

   private static <T> T require(T v, String what) {
      if (v == null) {
         throw new IllegalArgumentException(info(what));
      }
      return v;
   }

   private static String info(String what) {
      return what + " must not be null";
   }

   /* Accessors used by the generated Dispatch switch. */

   Core core() { return core; }
   double[] realInput(int i) { return realInputs[i]; }
   int[] intInput(int i) { return intInputs[i]; }
   double[] price(int slot, int component) { return priceInputs[slot][component]; }
   double realOpt(int i) { return realOpts[i]; }
   int intOpt(int i) { return intOpts[i]; }
   MAType maTypeOpt(int i) { return maTypeOpts[i]; }
   double[] realOutput(int i) { return realOutputs[i]; }
   int[] intOutput(int i) { return intOutputs[i]; }
}
