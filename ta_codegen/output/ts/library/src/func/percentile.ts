/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090426 MF,CC  First version (issue #368).
 *  092226 MF,CC  O(1) read, binary search from 256 values, one shift per bar (issue #435).
 *  092326 MF,CC  Branchless update kernels, merge-sorted first window (issue #435).
 *  092426 MF,CC  Rust stream tier takes the branchless kernels too (issue #439).
 *  092426 MF,CC  Default period 100, and stack buffers sized to it (issue #437).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function percentileLookbackPositional(optInTimePeriod: number, optInPercentile: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInPercentile !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 100;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return -1;
  }
  if (optInPercentile === Core.REAL_DEFAULT) {
    optInPercentile = 50.0;
  } else if (!(optInPercentile >= 0e0 && optInPercentile <= 1e2)) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function percentileDisplayShiftPositional(optInTimePeriod: number, optInPercentile: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (percentileLookbackPositional(optInTimePeriod, optInPercentile, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function percentileImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInPercentile: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let newValue = 0;
  let oldValue = 0;
  let result = 0;
  let loV = 0;
  let hiV = 0;
  let last = 0;
  let pendNew = 0;
  let pendOld = 0;
  let held = 0;
  let cur = 0;
  let nxt = 0;
  let nx2 = 0;
  let ins = 0;
  let in2 = 0;
  let res = 0;
  let prv = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let j = 0;
  let k = 0;
  let e = 0;
  let nbSorted = 0;
  let rank = 0;
  let hiRank = 0;
  let loRank = 0;
  let s = 0;
  let t = 0;
  let w = 0;
  let lo = 0;
  let mid = 0;
  let hi = 0;
  let a = 0;
  let b = 0;
  let tail = 0;
  let lim = 0;
  let run = 0;
  let dnStep = 0;
  let same = 0;
  let dPrev = 0;
  let trend = 0;
  let jh = 0;
  let jl = 0;
  let layout = 0;
  let head = 0;
  let pendPos = 0;
  let pendDel = 0;
  let runLen = 0;
  let saving = 0;
  let gain = 0;
  let pp = 0;
  let hiSlot = 0;
  let loSlot = 0;
  let pos = 0;
  let del = 0;
  let dp = 0;
  let dq = 0;
  let len = 0;
  let half = 0;
  let o2 = 0;
  let o3 = 0;
  let t1 = 0;
  let t2 = 0;
  let t3 = 0;
  let u1 = 0;
  let u2 = 0;
  let u3 = 0;
  let freeSlot = 0;
  let maxSlot = 0;
  let nextSlot = 0;
  let bp = 0;
  let bq = 0;
  let q1 = 0;
  let q2 = 0;
  let q3 = 0;
  let p1 = 0;
  let p2 = 0;
  let p3 = 0;
  let sP = 0;
  let sQ = 0;
  let slot = 0;
  let cnt = 0;
  let dn = 0;
  let seg = 0;
  let room = 0;
  let ring!: Float64Array;
  let ring_Idx = 0;
  let maxIdx_ring = (100) - 1;
  let sorted!: Float64Array;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 100;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (optInPercentile === Core.REAL_DEFAULT) {
    optInPercentile = 50.0;
  } else if (!(optInPercentile >= 0e0 && optInPercentile <= 1e2)) {
    return RetCode.BAD_PARAM;
  }
  /* The window is carried twice: "ring" by age, "sorted" by value. */
  lookbackTotal = optInTimePeriod - 1;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring = new Float64Array(optInTimePeriod);
  maxIdx_ring = optInTimePeriod - 1;
  ring_Idx = 0;
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sorted = new Float64Array(optInTimePeriod);
  /* Never read as a value, but from 2048 values it is the first free slot and
   * travels with the ring: set so two handles over the same bars hold the same
   * state.
   */
  sorted[lookbackTotal] = 0.0;
  /* Keep the multiply left of the divide. (P*n)/100 reproduces exact integer
   * arithmetic; P/100 is inexact in binary64 and lands the product just above
   * an integer, one order statistic too high, at exactly the round
   * percentages a caller types.
   */
  rank = (Math.ceil(optInPercentile * optInTimePeriod / 100.0) | 0);
  if (rank < 1) {
    rank = 1;
  }
  if (rank > optInTimePeriod) {
    rank = optInTimePeriod;
  }
  hiRank = rank - 1;
  loRank = (hiRank > 0) ? hiRank - 1 : 0;
  /* The retained values are sorted ascending with every run of equal values
   * in age order: the departing value is then the first of its run bit for
   * bit, and a new value goes after its equals. Only the sign of a zero can
   * observe that order, and it does.
   */
  i = startIdx - lookbackTotal;
  if (lookbackTotal < 100) {
    nbSorted = 0;
    while (i < startIdx) {
      newValue = inReal[i];
      j = nbSorted;
      while (j > 0 && sorted[j - 1] > newValue) {
        sorted[j] = sorted[j - 1];
        j -= 1;
      }
      sorted[j] = newValue;
      nbSorted += 1;
      ring[ring_Idx] = newValue;
      i += 1;
      ring_Idx++;
      if (ring_Idx > maxIdx_ring) {
        ring_Idx = 0;
      }
    }
  } else {
    /* Bottom-up merge sort, stable, with ring as the other half until it
     * is filled by age below.
     */
    sorted[0] = inReal[i];
    j = 1;
    while (j < lookbackTotal && inReal[i + j - 1] <= inReal[i + j]) {
      sorted[j] = inReal[i + j];
      j += 1;
    }
    if (j < lookbackTotal) {
      s = 0;
      while (s < lookbackTotal) {
        e = s + 16;
        if (e > lookbackTotal) {
          e = lookbackTotal;
        }
        t = s;
        while (t < e) {
          newValue = inReal[i + t];
          j = t;
          while (j > s && sorted[j - 1] > newValue) {
            sorted[j] = sorted[j - 1];
            j -= 1;
          }
          sorted[j] = newValue;
          t += 1;
        }
        s = e;
      }
      w = 16;
      while (w < lookbackTotal) {
        lo = 0;
        while (lo < lookbackTotal) {
          mid = lo + w;
          if (mid > lookbackTotal) {
            mid = lookbackTotal;
          }
          hi = mid + w;
          if (hi > lookbackTotal) {
            hi = lookbackTotal;
          }
          a = lo;
          b = mid;
          t = lo;
          if (mid < hi && sorted[mid - 1] > sorted[mid]) {
            while (a < mid && b < hi) {
              if (sorted[b] < sorted[a]) {
                ring[t] = sorted[b];
                b += 1;
              } else {
                ring[t] = sorted[a];
                a += 1;
              }
              t += 1;
            }
          }
          while (a < mid) {
            ring[t] = sorted[a];
            a += 1;
            t += 1;
          }
          while (b < hi) {
            ring[t] = sorted[b];
            b += 1;
            t += 1;
          }
          lo = hi;
        }
        w += w;
        if (w < lookbackTotal) {
          lo = 0;
          while (lo < lookbackTotal) {
            mid = lo + w;
            if (mid > lookbackTotal) {
              mid = lookbackTotal;
            }
            hi = mid + w;
            if (hi > lookbackTotal) {
              hi = lookbackTotal;
            }
            a = lo;
            b = mid;
            t = lo;
            if (mid < hi && ring[mid - 1] > ring[mid]) {
              while (a < mid && b < hi) {
                if (ring[b] < ring[a]) {
                  sorted[t] = ring[b];
                  b += 1;
                } else {
                  sorted[t] = ring[a];
                  a += 1;
                }
                t += 1;
              }
            }
            while (a < mid) {
              sorted[t] = ring[a];
              a += 1;
              t += 1;
            }
            while (b < hi) {
              sorted[t] = ring[b];
              b += 1;
              t += 1;
            }
            lo = hi;
          }
          w += w;
        } else {
          t = 0;
          while (t < lookbackTotal) {
            sorted[t] = ring[t];
            t += 1;
          }
        }
      }
    }
    t = 0;
    while (t < lookbackTotal) {
      ring[t] = inReal[i + t];
      t += 1;
    }
    ring_Idx = lookbackTotal;
    i = startIdx;
  }
  /* Below 32 values: every bar rewrites the whole window with no
   * data-dependent branch, or, after a long enough run in one direction,
   * reads the two ranks it needs straight off the ring.
   */
  tail = lookbackTotal - ((lookbackTotal % 2) | 0);
  lim = (lookbackTotal > 13) ? lookbackTotal - 2 : 11;
  run = -1;
  dPrev = 0;
  trend = 0;
  last = inReal[startIdx - 1];
  /* From m = 32 retained values, layout 0 keeps slots 0..m-1 ONE BAR BEHIND:
   * the previous bar's update (remove rank pendDel, insert pendNew at pendPos
   * counted before the removal) is still pending and the spare slot m holds
   * pendNew. A bar searches that array and corrects its counts by two
   * compares, so its search does not wait on the previous bar's shift.
   * Layout 1 is a ring of all m+1 slots, rank r at slot head+r and the free
   * slot just before head; layout 2 is layout 1 with the pending update still
   * to apply. The ring pays for itself on a trending window, where a new
   * extreme arrives as the opposite one departs, and from 2048 values, where
   * moving the shorter way round halves the shift.
   */
  layout = (lookbackTotal >= 2048 ? 1 : 0);
  head = 0;
  runLen = 0;
  saving = 0;
  pendNew = sorted[0];
  pendOld = pendNew;
  pendDel = 0;
  pendPos = 1;
  hiV = (rank <= lookbackTotal) ? sorted[hiRank] : 0.0;
  loV = (rank > 1) ? sorted[loRank] : 0.0;
  /* Both scratch buffers hold copies and inReal is never read below i, so
   * inReal and outReal may be the same buffer.
   *
   * Every store to state sits BELOW the output store: the streaming peek
   * frame is this loop cut there.
   */
  outIdx = 0;
  do {
    newValue = inReal[i];
    /* The full window is the retained values with newValue inserted after
     * its equals, so its rank-th value is newValue clamped to the retained
     * values of rank rank-1 (loV) and rank (hiV).
     */
    result = newValue;
    if (rank <= lookbackTotal) {
      result = (result < hiV) ? result : hiV;
    }
    if (rank > 1) {
      result = (result < loV) ? loV : result;
    }
    outReal[outIdx] = result;
    outIdx += 1;
    ring[ring_Idx] = newValue;
    ring_Idx++;
    if (ring_Idx > maxIdx_ring) {
      ring_Idx = 0;
    }
    oldValue = ring[ring_Idx];
    if (lookbackTotal < 32) {
      if (lookbackTotal === 1) {
        sorted[0] = newValue;
        hiV = newValue;
        loV = newValue;
      } else {
        /* 0/1 arithmetic, not conditional assignments: gcc threads those
         * into a branch on newValue < last, a coin flip on random data.
         * run starts at -1 so the first step cannot count twice.
         */
        dnStep = (newValue < last ? 1 : 0);
        same = (dnStep === dPrev ? 1 : 0);
        run = (((run + 1) * same) | 0);
        dPrev = dnStep;
        last = newValue;
        if (run >= lim) {
          /* The ring is then the sorted order, from its oldest slot on
           * a rising run and from its newest on a falling one, which
           * must be strict: read newest first, equal values would come
           * out in reverse age order. sorted is left stale until the
           * run breaks.
           */
          run = lim;
          if (dnStep === 0) {
            trend = 1;
            jh = ring_Idx + rank;
            if (jh >= optInTimePeriod) {
              jh -= optInTimePeriod;
            }
            jl = (jh === 0) ? lookbackTotal : jh - 1;
          } else {
            trend = 2;
            jh = ring_Idx + (optInTimePeriod - rank);
            if (jh >= optInTimePeriod) {
              jh -= optInTimePeriod;
            }
            jl = (jh === lookbackTotal) ? 0 : jh + 1;
          }
          if (rank <= lookbackTotal) {
            hiV = ring[jh];
          }
          if (rank > 1) {
            loV = ring[jl];
          }
        } else {
          if (trend !== 0) {
            if (trend === 1) {
              j = ring_Idx;
              k = 0;
              while (k < lookbackTotal) {
                sorted[k] = ring[j];
                j = (j === lookbackTotal) ? 0 : j + 1;
                k += 1;
              }
            } else {
              j = ring_Idx + lookbackTotal - 1;
              if (j >= optInTimePeriod) {
                j -= optInTimePeriod;
              }
              k = 0;
              while (k < lookbackTotal) {
                sorted[k] = ring[j];
                j = (j === 0) ? lookbackTotal : j - 1;
                k += 1;
              }
            }
            trend = 0;
          }
          /* With D the retained values less oldValue, slot k becomes
           * max(D[k-1], min(newValue, D[k])). The spare slot stands in
           * for D past the end.
           */
          sorted[lookbackTotal] = newValue;
          cur = sorted[0];
          prv = (newValue < cur) ? newValue : cur;
          k = 0;
          do {
            nxt = sorted[k + 1];
            nx2 = sorted[k + 2];
            ins = (cur < oldValue) ? cur : nxt;
            in2 = (nxt < oldValue) ? nxt : nx2;
            res = (newValue < ins) ? newValue : ins;
            sorted[k] = (prv > res) ? prv : res;
            res = (newValue < in2) ? newValue : in2;
            sorted[k + 1] = (ins > res) ? ins : res;
            prv = in2;
            cur = nx2;
            k += 2;
          } while (k < tail);
          sorted[tail] = (prv > newValue) ? prv : newValue;
          if (rank <= lookbackTotal) {
            hiV = sorted[hiRank];
          }
          if (rank > 1) {
            loV = sorted[loRank];
          }
        }
      }
    } else {
      if (layout !== 0) {
        if (layout === 2) {
          if (pendDel < pendPos) {
            for (k = pendDel; k < pendPos - 1; k += 1) {
              sorted[k] = sorted[k + 1];
            }
            sorted[pendPos - 1] = pendNew;
          } else {
            for (k = pendDel; k > pendPos; k -= 1) {
              sorted[k] = sorted[k - 1];
            }
            sorted[pendPos] = pendNew;
          }
          layout = 1;
        }
        freeSlot = (head === 0) ? lookbackTotal : head - 1;
        maxSlot = (freeSlot === 0) ? lookbackTotal : freeSlot - 1;
        nextSlot = (maxSlot === 0) ? lookbackTotal : maxSlot - 1;
        /* A new value tied with the minimum belongs after it, so only a
         * strictly smaller one may take the front.
         */
        if (newValue >= sorted[maxSlot] && oldValue <= sorted[head]) {
          sorted[freeSlot] = newValue;
          head = (head === lookbackTotal) ? 0 : head + 1;
        } else if (newValue < sorted[head] && oldValue > sorted[nextSlot]) {
          sorted[freeSlot] = newValue;
          head = freeSlot;
        } else if (saving > 0 || lookbackTotal >= 2048) {
          /* pos = retained values <= newValue, del = retained values <
           * oldValue, both searched from the free slot as rank -1.
           */
          bp = freeSlot;
          bq = bp;
          len = optInTimePeriod;
          while (len > 2) {
            half = (((len + 3) / 4) | 0);
            o3 = len - half;
            o2 = half + half;
            q1 = (bp < optInTimePeriod - half) ? bp + half : bp + half - optInTimePeriod;
            q2 = (bp < optInTimePeriod - o2) ? bp + o2 : bp + o2 - optInTimePeriod;
            q3 = (bp < optInTimePeriod - o3) ? bp + o3 : bp + o3 - optInTimePeriod;
            p1 = (bq < optInTimePeriod - half) ? bq + half : bq + half - optInTimePeriod;
            p2 = (bq < optInTimePeriod - o2) ? bq + o2 : bq + o2 - optInTimePeriod;
            p3 = (bq < optInTimePeriod - o3) ? bq + o3 : bq + o3 - optInTimePeriod;
            bp = (sorted[q1] <= newValue) ? q1 : bp;
            bp = (sorted[q2] <= newValue) ? q2 : bp;
            bp = (sorted[q3] <= newValue) ? q3 : bp;
            bq = (sorted[p1] < oldValue) ? p1 : bq;
            bq = (sorted[p2] < oldValue) ? p2 : bq;
            bq = (sorted[p3] < oldValue) ? p3 : bq;
            len = half;
          }
          if (len > 1) {
            sP = (bp < lookbackTotal) ? bp + 1 : 0;
            sQ = (bq < lookbackTotal) ? bq + 1 : 0;
            bp = (sorted[sP] <= newValue) ? sP : bp;
            bq = (sorted[sQ] < oldValue) ? sQ : bq;
          }
          pos = (bp >= freeSlot) ? bp - freeSlot : bp + optInTimePeriod - freeSlot;
          del = (bq >= freeSlot) ? bq - freeSlot : bq + optInTimePeriod - freeSlot;
          gain = (del < pos) ? pos - 1 - del : del - pos;
          seg = lookbackTotal + ((((lookbackTotal / 4) | 0) * 3) | 0);
          saving = (saving + gain + gain > seg) ? saving + gain + gain - seg : 0;
          saving = (saving < ((lookbackTotal * 16) | 0)) ? saving : ((lookbackTotal * 16) | 0);
          /* The hole left by oldValue travels to newValue's place the
           * shorter way round; going through the free slot moves head
           * by one.
           */
          slot = head + del;
          if (slot >= optInTimePeriod) {
            slot -= optInTimePeriod;
          }
          if (del < pos) {
            cnt = pos - 1 - del;
            dn = 1;
          } else {
            cnt = del - pos;
            dn = 0;
          }
          if (cnt + cnt > lookbackTotal) {
            cnt = lookbackTotal - cnt;
            dn = 1 - dn;
            if (dn === 1) {
              head = freeSlot;
            } else {
              head = (head === lookbackTotal) ? 0 : head + 1;
            }
          }
          if (dn === 1) {
            room = lookbackTotal - slot;
            if (cnt <= room) {
              e = slot + cnt;
              for (k = slot; k < e; k += 1) {
                sorted[k] = sorted[k + 1];
              }
              slot = e;
            } else {
              for (k = slot; k < lookbackTotal; k += 1) {
                sorted[k] = sorted[k + 1];
              }
              sorted[lookbackTotal] = sorted[0];
              e = cnt - room - 1;
              for (k = 0; k < e; k += 1) {
                sorted[k] = sorted[k + 1];
              }
              slot = e;
            }
          } else if (cnt <= slot) {
            e = slot - cnt;
            for (k = slot; k > e; k -= 1) {
              sorted[k] = sorted[k - 1];
            }
            slot = e;
          } else {
            for (k = slot; k > 0; k -= 1) {
              sorted[k] = sorted[k - 1];
            }
            sorted[0] = sorted[lookbackTotal];
            e = lookbackTotal - (cnt - slot - 1);
            for (k = lookbackTotal; k > e; k -= 1) {
              sorted[k] = sorted[k - 1];
            }
            slot = e;
          }
          sorted[slot] = newValue;
        } else {
          /* Back to layout 0: rotate head to slot 0 by three
           * reversals, then carry an empty pending update.
           */
          if (head !== 0) {
            k = 0;
            e = head - 1;
            while (k < e) {
              held = sorted[k];
              sorted[k] = sorted[e];
              sorted[e] = held;
              k += 1;
              e -= 1;
            }
            k = head;
            e = lookbackTotal;
            while (k < e) {
              held = sorted[k];
              sorted[k] = sorted[e];
              sorted[e] = held;
              k += 1;
              e -= 1;
            }
            k = 0;
            e = lookbackTotal;
            while (k < e) {
              held = sorted[k];
              sorted[k] = sorted[e];
              sorted[e] = held;
              k += 1;
              e -= 1;
            }
            head = 0;
          }
          pendNew = sorted[0];
          pendOld = pendNew;
          pendDel = 0;
          pendPos = 1;
          sorted[lookbackTotal] = pendNew;
          layout = 0;
          runLen = 0;
        }
      }
      if (layout === 0) {
        /* Branchless four-way search of the lagging array, no wrap. */
        pos = 0;
        del = 0;
        len = optInTimePeriod;
        while (len > 2) {
          half = (((len + 3) / 4) | 0);
          o3 = len - half;
          o2 = half + half;
          t1 = pos + half;
          t2 = pos + o2;
          t3 = pos + o3;
          u1 = del + half;
          u2 = del + o2;
          u3 = del + o3;
          pos = (sorted[t1 - 1] <= newValue) ? t1 : pos;
          pos = (sorted[t2 - 1] <= newValue) ? t2 : pos;
          pos = (sorted[t3 - 1] <= newValue) ? t3 : pos;
          del = (sorted[u1 - 1] < oldValue) ? u1 : del;
          del = (sorted[u2 - 1] < oldValue) ? u2 : del;
          del = (sorted[u3 - 1] < oldValue) ? u3 : del;
          len = half;
        }
        /* Increments, not selects: gcc turns the select into a jump. */
        if (len > 1) {
          pos += (sorted[pos] <= newValue ? 1 : 0);
          del += (sorted[del] < oldValue ? 1 : 0);
        }
        pos += (pendNew <= newValue ? 1 : 0);
        del += (pendNew < oldValue ? 1 : 0);
        dp = (pendOld <= newValue ? 1 : 0);
        dq = (pendOld < oldValue ? 1 : 0);
        /* Clamped although exact for finite input: a NaN makes the counts
         * disagree with the corrections, and they index the shift below.
         */
        pos = (pos > dp) ? pos - dp : 0;
        del = (del > dq) ? del - dq : 0;
        pos = (pos < lookbackTotal) ? pos : lookbackTotal;
        del = (del < lookbackTotal) ? del : lookbackTotal;
        if (pendDel < pendPos) {
          for (k = pendDel; k < pendPos - 1; k += 1) {
            sorted[k] = sorted[k + 1];
          }
          sorted[pendPos - 1] = pendNew;
        } else {
          for (k = pendDel; k > pendPos; k -= 1) {
            sorted[k] = sorted[k - 1];
          }
          sorted[pendPos] = pendNew;
        }
        sorted[lookbackTotal] = newValue;
        pendPos = pos;
        pendDel = del;
        pendNew = newValue;
        pendOld = oldValue;
        /* Leave for the ring after four bars that each add a new extreme
         * as the opposite one departs, or, from 300 values, while the
         * shift the ring would save keeps exceeding 3/4 of the window.
         */
        runLen = (pos === del + lookbackTotal || del === pos + lookbackTotal - 1) ? runLen + 1 : 0;
        if (lookbackTotal >= 300) {
          gain = (del < pos) ? pos - 1 - del : del - pos;
          seg = lookbackTotal + ((((lookbackTotal / 4) | 0) * 3) | 0);
          saving = (saving + gain + gain > seg) ? saving + gain + gain - seg : 0;
        }
        if (runLen >= 4 || saving > ((lookbackTotal * 8) | 0)) {
          layout = 2;
        }
      }
      if (layout !== 1) {
        pp = (pendDel < pendPos) ? pendPos - 1 : pendPos;
        hiSlot = (hiRank > pp) ? hiRank - 1 : hiRank;
        hiSlot += (hiSlot >= pendDel ? 1 : 0);
        hiSlot = (hiRank === pp) ? lookbackTotal : hiSlot;
        loSlot = (loRank > pp) ? loRank - 1 : loRank;
        loSlot += (loSlot >= pendDel ? 1 : 0);
        loSlot = (loRank === pp) ? lookbackTotal : loSlot;
      } else {
        hiSlot = head + hiRank;
        if (hiSlot >= optInTimePeriod) {
          hiSlot -= optInTimePeriod;
        }
        loSlot = (hiSlot === 0) ? lookbackTotal : hiSlot - 1;
      }
      if (rank <= lookbackTotal) {
        hiV = sorted[hiSlot];
      }
      if (rank > 1) {
        loV = sorted[loSlot];
      }
    }
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 10000, 'in 2..10000'], ['percentile', Core.REAL_DEFAULT, 0e0, 1e2, 'in 0..100']] as const;

/** @internal */
export function percentilePositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInPercentile: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('PERCENTILE', 'startIdx', startIdx);
  endIdx = requireInt32('PERCENTILE', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('PERCENTILE', 'timePeriod', optInTimePeriod);
  optInPercentile = requireReal('PERCENTILE', 'percentile', optInPercentile);
  requireIndexRange('PERCENTILE', startIdx, endIdx);
  const guardLookback = percentileLookbackPositional(optInTimePeriod, optInPercentile, core);
  if (guardLookback < 0) {
    throw rejectedParams('PERCENTILE', PARAM_RANGES, optInTimePeriod, optInPercentile);
  }
  const guardStart = clampedStart('PERCENTILE', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('PERCENTILE', 'real', inReal, guardInLen);
  requireOutput('PERCENTILE', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('PERCENTILE', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = percentileImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInPercentile, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('PERCENTILE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link percentile}, {@link percentileLookback},
 * {@link percentileDisplayShift}, {@link percentileOpen} and
 * {@link percentileOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface PercentileOptions {
  /**
   * Number of bars in the trailing window (default 100; range 2 to 10000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Percentage position within the sorted window (default 50; range 0 to 100;
   * `Core.REAL_DEFAULT` selects the default).
   */
  percentile?: number | undefined;
  /**
   * First bar of the requested range (default 0). Bars before it are still read as
   * warm-up. Batch calls only.
   */
  startIdx?: number | undefined;
  /**
   * Last bar of the requested range, inclusive (default the last bar of the first
   * input). Batch calls only.
   */
  endIdx?: number | undefined;
  /**
   * Unstable periods and candle settings to compute with; `Core.DEFAULT` when
   * omitted.
   */
  core?: Core | undefined;
  /**
   * Buffer for `values`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outReal?: Float64Array | undefined;
}

/**
 * What {@link percentile} returns, and {@link percentileOpenAndFill} with its
 * stream.
 */
export interface PercentileResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * The value at the requested rank within the trailing window. `values[k]` is the
   * value at input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link percentile} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PercentileOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function percentileLookback(options?: PercentileOptions): number {
  const o = optionsOf('PERCENTILE lookback', options);
  return percentileLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.percentile, Core.REAL_DEFAULT), coreOf('PERCENTILE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link percentile}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PercentileOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function percentileDisplayShift(outputIdx: number, options?: PercentileOptions): number {
  const o = optionsOf('PERCENTILE displayShift', options);
  return percentileDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.percentile, Core.REAL_DEFAULT), outputIdx, coreOf('PERCENTILE displayShift', o.core));
}

/**
 * Rolling percentile by the nearest-rank method: sort the trailing window
 * ascending and report the value whose 1-based ordinal rank is the P-th percentile
 * of the window size. The result is always a value that actually occurred in the
 * window, never an interpolation, so it stays on the price scale and never invents
 * a level the series never traded at. At P = 50 with an odd window it is the
 * rolling median; at the extremes it degenerates to the rolling minimum and
 * maximum.
 *
 * Formula and more info at
 * [ta-lib.org/functions/percentile](https://ta-lib.org/functions/percentile).
 *
 * **Notes**
 * - The nearest-rank method is one of several incompatible percentile conventions.
 *   The linear-interpolation family (Hyndman & Fan type 7, the default of most
 *   statistical packages, and TradingView's `ta.percentile_linear_interpolation`)
 *   reports a weighted blend of two neighbouring order statistics and can emit a
 *   value that never occurred. That is a different indicator, not a mode of this
 *   one: PERCENTILE's parameter list is fixed at a window and a percentage, and a
 *   method selector cannot be appended to it later without changing the function's
 *   arity.
 * - A tail percentile rests on the few values beyond it: at N = 30, P = 95 is the
 *   second-largest value in the window. For a stable estimate keep about ten
 *   values beyond the percentile, N ≥ 1000 / min(P, 100 − P): 100 bars at P = 10
 *   or 90, 200 at P = 5 or 95.
 * - Every input value in the window must be finite. A NaN makes every comparison
 *   against it false, which breaks the ordering the rank index is read from.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link percentileLookback} is a success with empty outputs.
 *
 * @param real - Source series to take the percentile of.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link PercentileOptions}.
 * @returns `begIdx` and the values; see {@link PercentileResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type or a buffer sharing memory
 *   with a `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { percentile } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = percentile(data, {
 *   timePeriod: 100,
 *   percentile: 50,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`min`](https://ta-lib.org/functions/min)
 * @see [`max`](https://ta-lib.org/functions/max)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 */
export function percentile(real: ArrayLike<number>, options?: PercentileOptions): PercentileResult {
  const o = optionsOf('PERCENTILE', options);
  if (o.endIdx === undefined) {
    requireSeries('PERCENTILE', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInPercentile = orDefault(o.percentile, Core.REAL_DEFAULT);
  const core = coreOf('PERCENTILE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, percentileLookbackPositional(optInTimePeriod, optInPercentile, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = percentilePositional(startIdx, endIdx, real, optInTimePeriod, optInPercentile, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live PERCENTILE stream: one value per closed bar, bit-identical to
 * {@link percentile} over the same series. Open it with {@link percentileOpen} or
 * {@link percentileOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/percentile](https://ta-lib.org/functions/percentile).
 */
export class PercentileStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInPercentile: number = 0;
  /** @internal */
  loV: number = 0;
  /** @internal */
  hiV: number = 0;
  /** @internal */
  last: number = 0;
  /** @internal */
  pendNew: number = 0;
  /** @internal */
  pendOld: number = 0;
  /** @internal */
  lookbackTotal: number = 0;
  /** @internal */
  rank: number = 0;
  /** @internal */
  hiRank: number = 0;
  /** @internal */
  loRank: number = 0;
  /** @internal */
  tail: number = 0;
  /** @internal */
  lim: number = 0;
  /** @internal */
  run: number = 0;
  /** @internal */
  dPrev: number = 0;
  /** @internal */
  trend: number = 0;
  /** @internal */
  layout: number = 0;
  /** @internal */
  head: number = 0;
  /** @internal */
  pendPos: number = 0;
  /** @internal */
  pendDel: number = 0;
  /** @internal */
  runLen: number = 0;
  /** @internal */
  saving: number = 0;
  /** @internal */
  ring_Idx: number = 0;
  /** @internal */
  maxIdx_ring: number = 0;
  /** @internal */
  sorted_Idx: number = 0;
  /** @internal */
  maxIdx_sorted: number = 0;
  /** @internal */
  cbSize_ring: number = 0;
  /** @internal */
  cb_ring: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_sorted: number = 0;
  /** @internal */
  cb_sorted: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): PercentileStream {
    return new PercentileStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link percentile} reports them over the same bars. Each accepted `update` and
   * each `advance` adds one to `count`.
   */
  outRange(): OutRange {
    return new OutRange(this.outRangeBegIdx, this.outRangeCount);
  }

  /**
   * Counts one bar this stream was not fed: `outRange()` advances by one and
   * `value()` keeps answering the previous output.
   *
   * @throws `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has
   *   reached bar `Core.INDEX_MAX`.
   */
  advance(): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('PERCENTILE advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source series to take the percentile
   *   of.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('PERCENTILE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('PERCENTILE update', 'inReal');
    }
    percentileStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source series to take the percentile
   *   of.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('PERCENTILE peek', 'inReal');
    }
    const sp = this;
    let newValue = 0;
    let result = 0;
    let cur_outReal = 0;
    newValue = inReal;
    /* The full window is the retained values with newValue inserted after
     * its equals, so its rank-th value is newValue clamped to the retained
     * values of rank rank-1 (loV) and rank (hiV).
     */
    result = newValue;
    if (sp.rank <= sp.lookbackTotal) {
      result = (result < sp.hiV) ? result : sp.hiV;
    }
    if (sp.rank > 1) {
      result = (result < sp.loV) ? sp.loV : result;
    }
    cur_outReal = result;
    return cur_outReal;
  }

  /** The value at the last bar this stream counted; `peek` does not change it. */
  value(): number {
    return this.cur_outReal;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): PercentileStream {
    const c = new PercentileStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInPercentile = this.optInPercentile;
    c.loV = this.loV;
    c.hiV = this.hiV;
    c.last = this.last;
    c.pendNew = this.pendNew;
    c.pendOld = this.pendOld;
    c.lookbackTotal = this.lookbackTotal;
    c.rank = this.rank;
    c.hiRank = this.hiRank;
    c.loRank = this.loRank;
    c.tail = this.tail;
    c.lim = this.lim;
    c.run = this.run;
    c.dPrev = this.dPrev;
    c.trend = this.trend;
    c.layout = this.layout;
    c.head = this.head;
    c.pendPos = this.pendPos;
    c.pendDel = this.pendDel;
    c.runLen = this.runLen;
    c.saving = this.saving;
    c.ring_Idx = this.ring_Idx;
    c.maxIdx_ring = this.maxIdx_ring;
    c.sorted_Idx = this.sorted_Idx;
    c.maxIdx_sorted = this.maxIdx_sorted;
    c.cbSize_ring = this.cbSize_ring;
    c.cb_ring = this.cb_ring.slice();
    c.cbSize_sorted = this.cbSize_sorted;
    c.cb_sorted = this.cb_sorted.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function percentileStepImpl(sp: PercentileStream, inReal: number): void {
  let newValue = 0;
  let oldValue = 0;
  let result = 0;
  let held = 0;
  let cur = 0;
  let nxt = 0;
  let nx2 = 0;
  let ins = 0;
  let in2 = 0;
  let res = 0;
  let prv = 0;
  let j = 0;
  let k = 0;
  let e = 0;
  let dnStep = 0;
  let same = 0;
  let jh = 0;
  let jl = 0;
  let gain = 0;
  let pp = 0;
  let hiSlot = 0;
  let loSlot = 0;
  let pos = 0;
  let del = 0;
  let dp = 0;
  let dq = 0;
  let len = 0;
  let half = 0;
  let o2 = 0;
  let o3 = 0;
  let t1 = 0;
  let t2 = 0;
  let t3 = 0;
  let u1 = 0;
  let u2 = 0;
  let u3 = 0;
  let freeSlot = 0;
  let maxSlot = 0;
  let nextSlot = 0;
  let bp = 0;
  let bq = 0;
  let q1 = 0;
  let q2 = 0;
  let q3 = 0;
  let p1 = 0;
  let p2 = 0;
  let p3 = 0;
  let sP = 0;
  let sQ = 0;
  let slot = 0;
  let cnt = 0;
  let dn = 0;
  let seg = 0;
  let room = 0;
  newValue = inReal;
  /* The full window is the retained values with newValue inserted after
   * its equals, so its rank-th value is newValue clamped to the retained
   * values of rank rank-1 (loV) and rank (hiV).
   */
  result = newValue;
  if (sp.rank <= sp.lookbackTotal) {
    result = (result < sp.hiV) ? result : sp.hiV;
  }
  if (sp.rank > 1) {
    result = (result < sp.loV) ? sp.loV : result;
  }
  sp.cur_outReal = result;
  sp.cb_ring[sp.ring_Idx] = newValue;
  sp.ring_Idx = sp.ring_Idx + 1;
  if (sp.ring_Idx > sp.maxIdx_ring) {
    sp.ring_Idx = 0;
  }
  oldValue = sp.cb_ring[sp.ring_Idx];
  if (sp.lookbackTotal < 32) {
    if (sp.lookbackTotal === 1) {
      sp.cb_sorted[0] = newValue;
      sp.hiV = newValue;
      sp.loV = newValue;
    } else {
      /* 0/1 arithmetic, not conditional assignments: gcc threads those
       * into a branch on newValue < last, a coin flip on random data.
       * run starts at -1 so the first step cannot count twice.
       */
      dnStep = (newValue < sp.last ? 1 : 0);
      same = (dnStep === sp.dPrev ? 1 : 0);
      sp.run = (((sp.run + 1) * same) | 0);
      sp.dPrev = dnStep;
      sp.last = newValue;
      if (sp.run >= sp.lim) {
        /* The ring is then the sorted order, from its oldest slot on
         * a rising run and from its newest on a falling one, which
         * must be strict: read newest first, equal values would come
         * out in reverse age order. sorted is left stale until the
         * run breaks.
         */
        sp.run = sp.lim;
        if (dnStep === 0) {
          sp.trend = 1;
          jh = sp.ring_Idx + sp.rank;
          if (jh >= sp.optInTimePeriod) {
            jh -= sp.optInTimePeriod;
          }
          jl = (jh === 0) ? sp.lookbackTotal : jh - 1;
        } else {
          sp.trend = 2;
          jh = sp.ring_Idx + (sp.optInTimePeriod - sp.rank);
          if (jh >= sp.optInTimePeriod) {
            jh -= sp.optInTimePeriod;
          }
          jl = (jh === sp.lookbackTotal) ? 0 : jh + 1;
        }
        if (sp.rank <= sp.lookbackTotal) {
          sp.hiV = sp.cb_ring[jh];
        }
        if (sp.rank > 1) {
          sp.loV = sp.cb_ring[jl];
        }
      } else {
        if (sp.trend !== 0) {
          if (sp.trend === 1) {
            j = sp.ring_Idx;
            k = 0;
            while (k < sp.lookbackTotal) {
              sp.cb_sorted[k] = sp.cb_ring[j];
              j = (j === sp.lookbackTotal) ? 0 : j + 1;
              k += 1;
            }
          } else {
            j = sp.ring_Idx + sp.lookbackTotal - 1;
            if (j >= sp.optInTimePeriod) {
              j -= sp.optInTimePeriod;
            }
            k = 0;
            while (k < sp.lookbackTotal) {
              sp.cb_sorted[k] = sp.cb_ring[j];
              j = (j === 0) ? sp.lookbackTotal : j - 1;
              k += 1;
            }
          }
          sp.trend = 0;
        }
        /* With D the retained values less oldValue, slot k becomes
         * max(D[k-1], min(newValue, D[k])). The spare slot stands in
         * for D past the end.
         */
        sp.cb_sorted[sp.lookbackTotal] = newValue;
        cur = sp.cb_sorted[0];
        prv = (newValue < cur) ? newValue : cur;
        k = 0;
        do {
          nxt = sp.cb_sorted[k + 1];
          nx2 = sp.cb_sorted[k + 2];
          ins = (cur < oldValue) ? cur : nxt;
          in2 = (nxt < oldValue) ? nxt : nx2;
          res = (newValue < ins) ? newValue : ins;
          sp.cb_sorted[k] = (prv > res) ? prv : res;
          res = (newValue < in2) ? newValue : in2;
          sp.cb_sorted[k + 1] = (ins > res) ? ins : res;
          prv = in2;
          cur = nx2;
          k += 2;
        } while (k < sp.tail);
        sp.cb_sorted[sp.tail] = (prv > newValue) ? prv : newValue;
        if (sp.rank <= sp.lookbackTotal) {
          sp.hiV = sp.cb_sorted[sp.hiRank];
        }
        if (sp.rank > 1) {
          sp.loV = sp.cb_sorted[sp.loRank];
        }
      }
    }
  } else {
    if (sp.layout !== 0) {
      if (sp.layout === 2) {
        if (sp.pendDel < sp.pendPos) {
          for (k = sp.pendDel; k < sp.pendPos - 1; k += 1) {
            sp.cb_sorted[k] = sp.cb_sorted[k + 1];
          }
          sp.cb_sorted[sp.pendPos - 1] = sp.pendNew;
        } else {
          for (k = sp.pendDel; k > sp.pendPos; k -= 1) {
            sp.cb_sorted[k] = sp.cb_sorted[k - 1];
          }
          sp.cb_sorted[sp.pendPos] = sp.pendNew;
        }
        sp.layout = 1;
      }
      freeSlot = (sp.head === 0) ? sp.lookbackTotal : sp.head - 1;
      maxSlot = (freeSlot === 0) ? sp.lookbackTotal : freeSlot - 1;
      nextSlot = (maxSlot === 0) ? sp.lookbackTotal : maxSlot - 1;
      /* A new value tied with the minimum belongs after it, so only a
       * strictly smaller one may take the front.
       */
      if (newValue >= sp.cb_sorted[maxSlot] && oldValue <= sp.cb_sorted[sp.head]) {
        sp.cb_sorted[freeSlot] = newValue;
        sp.head = (sp.head === sp.lookbackTotal) ? 0 : sp.head + 1;
      } else if (newValue < sp.cb_sorted[sp.head] && oldValue > sp.cb_sorted[nextSlot]) {
        sp.cb_sorted[freeSlot] = newValue;
        sp.head = freeSlot;
      } else if (sp.saving > 0 || sp.lookbackTotal >= 2048) {
        /* pos = retained values <= newValue, del = retained values <
         * oldValue, both searched from the free slot as rank -1.
         */
        bp = freeSlot;
        bq = bp;
        len = sp.optInTimePeriod;
        while (len > 2) {
          half = (((len + 3) / 4) | 0);
          o3 = len - half;
          o2 = half + half;
          q1 = (bp < sp.optInTimePeriod - half) ? bp + half : bp + half - sp.optInTimePeriod;
          q2 = (bp < sp.optInTimePeriod - o2) ? bp + o2 : bp + o2 - sp.optInTimePeriod;
          q3 = (bp < sp.optInTimePeriod - o3) ? bp + o3 : bp + o3 - sp.optInTimePeriod;
          p1 = (bq < sp.optInTimePeriod - half) ? bq + half : bq + half - sp.optInTimePeriod;
          p2 = (bq < sp.optInTimePeriod - o2) ? bq + o2 : bq + o2 - sp.optInTimePeriod;
          p3 = (bq < sp.optInTimePeriod - o3) ? bq + o3 : bq + o3 - sp.optInTimePeriod;
          bp = (sp.cb_sorted[q1] <= newValue) ? q1 : bp;
          bp = (sp.cb_sorted[q2] <= newValue) ? q2 : bp;
          bp = (sp.cb_sorted[q3] <= newValue) ? q3 : bp;
          bq = (sp.cb_sorted[p1] < oldValue) ? p1 : bq;
          bq = (sp.cb_sorted[p2] < oldValue) ? p2 : bq;
          bq = (sp.cb_sorted[p3] < oldValue) ? p3 : bq;
          len = half;
        }
        if (len > 1) {
          sP = (bp < sp.lookbackTotal) ? bp + 1 : 0;
          sQ = (bq < sp.lookbackTotal) ? bq + 1 : 0;
          bp = (sp.cb_sorted[sP] <= newValue) ? sP : bp;
          bq = (sp.cb_sorted[sQ] < oldValue) ? sQ : bq;
        }
        pos = (bp >= freeSlot) ? bp - freeSlot : bp + sp.optInTimePeriod - freeSlot;
        del = (bq >= freeSlot) ? bq - freeSlot : bq + sp.optInTimePeriod - freeSlot;
        gain = (del < pos) ? pos - 1 - del : del - pos;
        seg = sp.lookbackTotal + ((((sp.lookbackTotal / 4) | 0) * 3) | 0);
        sp.saving = (sp.saving + gain + gain > seg) ? sp.saving + gain + gain - seg : 0;
        sp.saving = (sp.saving < ((sp.lookbackTotal * 16) | 0)) ? sp.saving : ((sp.lookbackTotal * 16) | 0);
        /* The hole left by oldValue travels to newValue's place the
         * shorter way round; going through the free slot moves head
         * by one.
         */
        slot = sp.head + del;
        if (slot >= sp.optInTimePeriod) {
          slot -= sp.optInTimePeriod;
        }
        if (del < pos) {
          cnt = pos - 1 - del;
          dn = 1;
        } else {
          cnt = del - pos;
          dn = 0;
        }
        if (cnt + cnt > sp.lookbackTotal) {
          cnt = sp.lookbackTotal - cnt;
          dn = 1 - dn;
          if (dn === 1) {
            sp.head = freeSlot;
          } else {
            sp.head = (sp.head === sp.lookbackTotal) ? 0 : sp.head + 1;
          }
        }
        if (dn === 1) {
          room = sp.lookbackTotal - slot;
          if (cnt <= room) {
            e = slot + cnt;
            for (k = slot; k < e; k += 1) {
              sp.cb_sorted[k] = sp.cb_sorted[k + 1];
            }
            slot = e;
          } else {
            for (k = slot; k < sp.lookbackTotal; k += 1) {
              sp.cb_sorted[k] = sp.cb_sorted[k + 1];
            }
            sp.cb_sorted[sp.lookbackTotal] = sp.cb_sorted[0];
            e = cnt - room - 1;
            for (k = 0; k < e; k += 1) {
              sp.cb_sorted[k] = sp.cb_sorted[k + 1];
            }
            slot = e;
          }
        } else if (cnt <= slot) {
          e = slot - cnt;
          for (k = slot; k > e; k -= 1) {
            sp.cb_sorted[k] = sp.cb_sorted[k - 1];
          }
          slot = e;
        } else {
          for (k = slot; k > 0; k -= 1) {
            sp.cb_sorted[k] = sp.cb_sorted[k - 1];
          }
          sp.cb_sorted[0] = sp.cb_sorted[sp.lookbackTotal];
          e = sp.lookbackTotal - (cnt - slot - 1);
          for (k = sp.lookbackTotal; k > e; k -= 1) {
            sp.cb_sorted[k] = sp.cb_sorted[k - 1];
          }
          slot = e;
        }
        sp.cb_sorted[slot] = newValue;
      } else {
        /* Back to layout 0: rotate head to slot 0 by three
         * reversals, then carry an empty pending update.
         */
        if (sp.head !== 0) {
          k = 0;
          e = sp.head - 1;
          while (k < e) {
            held = sp.cb_sorted[k];
            sp.cb_sorted[k] = sp.cb_sorted[e];
            sp.cb_sorted[e] = held;
            k += 1;
            e -= 1;
          }
          k = sp.head;
          e = sp.lookbackTotal;
          while (k < e) {
            held = sp.cb_sorted[k];
            sp.cb_sorted[k] = sp.cb_sorted[e];
            sp.cb_sorted[e] = held;
            k += 1;
            e -= 1;
          }
          k = 0;
          e = sp.lookbackTotal;
          while (k < e) {
            held = sp.cb_sorted[k];
            sp.cb_sorted[k] = sp.cb_sorted[e];
            sp.cb_sorted[e] = held;
            k += 1;
            e -= 1;
          }
          sp.head = 0;
        }
        sp.pendNew = sp.cb_sorted[0];
        sp.pendOld = sp.pendNew;
        sp.pendDel = 0;
        sp.pendPos = 1;
        sp.cb_sorted[sp.lookbackTotal] = sp.pendNew;
        sp.layout = 0;
        sp.runLen = 0;
      }
    }
    if (sp.layout === 0) {
      /* Branchless four-way search of the lagging array, no wrap. */
      pos = 0;
      del = 0;
      len = sp.optInTimePeriod;
      while (len > 2) {
        half = (((len + 3) / 4) | 0);
        o3 = len - half;
        o2 = half + half;
        t1 = pos + half;
        t2 = pos + o2;
        t3 = pos + o3;
        u1 = del + half;
        u2 = del + o2;
        u3 = del + o3;
        pos = (sp.cb_sorted[t1 - 1] <= newValue) ? t1 : pos;
        pos = (sp.cb_sorted[t2 - 1] <= newValue) ? t2 : pos;
        pos = (sp.cb_sorted[t3 - 1] <= newValue) ? t3 : pos;
        del = (sp.cb_sorted[u1 - 1] < oldValue) ? u1 : del;
        del = (sp.cb_sorted[u2 - 1] < oldValue) ? u2 : del;
        del = (sp.cb_sorted[u3 - 1] < oldValue) ? u3 : del;
        len = half;
      }
      /* Increments, not selects: gcc turns the select into a jump. */
      if (len > 1) {
        pos += (sp.cb_sorted[pos] <= newValue ? 1 : 0);
        del += (sp.cb_sorted[del] < oldValue ? 1 : 0);
      }
      pos += (sp.pendNew <= newValue ? 1 : 0);
      del += (sp.pendNew < oldValue ? 1 : 0);
      dp = (sp.pendOld <= newValue ? 1 : 0);
      dq = (sp.pendOld < oldValue ? 1 : 0);
      /* Clamped although exact for finite input: a NaN makes the counts
       * disagree with the corrections, and they index the shift below.
       */
      pos = (pos > dp) ? pos - dp : 0;
      del = (del > dq) ? del - dq : 0;
      pos = (pos < sp.lookbackTotal) ? pos : sp.lookbackTotal;
      del = (del < sp.lookbackTotal) ? del : sp.lookbackTotal;
      if (sp.pendDel < sp.pendPos) {
        for (k = sp.pendDel; k < sp.pendPos - 1; k += 1) {
          sp.cb_sorted[k] = sp.cb_sorted[k + 1];
        }
        sp.cb_sorted[sp.pendPos - 1] = sp.pendNew;
      } else {
        for (k = sp.pendDel; k > sp.pendPos; k -= 1) {
          sp.cb_sorted[k] = sp.cb_sorted[k - 1];
        }
        sp.cb_sorted[sp.pendPos] = sp.pendNew;
      }
      sp.cb_sorted[sp.lookbackTotal] = newValue;
      sp.pendPos = pos;
      sp.pendDel = del;
      sp.pendNew = newValue;
      sp.pendOld = oldValue;
      /* Leave for the ring after four bars that each add a new extreme
       * as the opposite one departs, or, from 300 values, while the
       * shift the ring would save keeps exceeding 3/4 of the window.
       */
      sp.runLen = (pos === del + sp.lookbackTotal || del === pos + sp.lookbackTotal - 1) ? sp.runLen + 1 : 0;
      if (sp.lookbackTotal >= 300) {
        gain = (del < pos) ? pos - 1 - del : del - pos;
        seg = sp.lookbackTotal + ((((sp.lookbackTotal / 4) | 0) * 3) | 0);
        sp.saving = (sp.saving + gain + gain > seg) ? sp.saving + gain + gain - seg : 0;
      }
      if (sp.runLen >= 4 || sp.saving > ((sp.lookbackTotal * 8) | 0)) {
        sp.layout = 2;
      }
    }
    if (sp.layout !== 1) {
      pp = (sp.pendDel < sp.pendPos) ? sp.pendPos - 1 : sp.pendPos;
      hiSlot = (sp.hiRank > pp) ? sp.hiRank - 1 : sp.hiRank;
      hiSlot += (hiSlot >= sp.pendDel ? 1 : 0);
      hiSlot = (sp.hiRank === pp) ? sp.lookbackTotal : hiSlot;
      loSlot = (sp.loRank > pp) ? sp.loRank - 1 : sp.loRank;
      loSlot += (loSlot >= sp.pendDel ? 1 : 0);
      loSlot = (sp.loRank === pp) ? sp.lookbackTotal : loSlot;
    } else {
      hiSlot = sp.head + sp.hiRank;
      if (hiSlot >= sp.optInTimePeriod) {
        hiSlot -= sp.optInTimePeriod;
      }
      loSlot = (hiSlot === 0) ? sp.lookbackTotal : hiSlot - 1;
    }
    if (sp.rank <= sp.lookbackTotal) {
      sp.hiV = sp.cb_sorted[hiSlot];
    }
    if (sp.rank > 1) {
      sp.loV = sp.cb_sorted[loSlot];
    }
  }
}

function percentileOpenImpl(
  sp: PercentileStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInPercentile: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let newValue = 0;
  let oldValue = 0;
  let result = 0;
  let loV = 0;
  let hiV = 0;
  let last = 0;
  let pendNew = 0;
  let pendOld = 0;
  let held = 0;
  let cur = 0;
  let nxt = 0;
  let nx2 = 0;
  let ins = 0;
  let in2 = 0;
  let res = 0;
  let prv = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let j = 0;
  let k = 0;
  let e = 0;
  let nbSorted = 0;
  let rank = 0;
  let hiRank = 0;
  let loRank = 0;
  let s = 0;
  let t = 0;
  let w = 0;
  let lo = 0;
  let mid = 0;
  let hi = 0;
  let a = 0;
  let b = 0;
  let tail = 0;
  let lim = 0;
  let run = 0;
  let dnStep = 0;
  let same = 0;
  let dPrev = 0;
  let trend = 0;
  let jh = 0;
  let jl = 0;
  let layout = 0;
  let head = 0;
  let pendPos = 0;
  let pendDel = 0;
  let runLen = 0;
  let saving = 0;
  let gain = 0;
  let pp = 0;
  let hiSlot = 0;
  let loSlot = 0;
  let pos = 0;
  let del = 0;
  let dp = 0;
  let dq = 0;
  let len = 0;
  let half = 0;
  let o2 = 0;
  let o3 = 0;
  let t1 = 0;
  let t2 = 0;
  let t3 = 0;
  let u1 = 0;
  let u2 = 0;
  let u3 = 0;
  let freeSlot = 0;
  let maxSlot = 0;
  let nextSlot = 0;
  let bp = 0;
  let bq = 0;
  let q1 = 0;
  let q2 = 0;
  let q3 = 0;
  let p1 = 0;
  let p2 = 0;
  let p3 = 0;
  let sP = 0;
  let sQ = 0;
  let slot = 0;
  let cnt = 0;
  let dn = 0;
  let seg = 0;
  let room = 0;
  let ring!: Float64Array;
  let ring_Idx = 0;
  let maxIdx_ring = (100) - 1;
  let sorted!: Float64Array;
  let sorted_Idx = 0;
  let maxIdx_sorted = (100) - 1;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 100;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (optInPercentile === Core.REAL_DEFAULT) {
    optInPercentile = 50.0;
  } else if (!(optInPercentile >= 0e0 && optInPercentile <= 1e2)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The window is carried twice: "ring" by age, "sorted" by value. */
  lookbackTotal = optInTimePeriod - 1;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring = new Float64Array(optInTimePeriod);
  maxIdx_ring = optInTimePeriod - 1;
  ring_Idx = 0;
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sorted = new Float64Array(optInTimePeriod);
  maxIdx_sorted = optInTimePeriod - 1;
  sorted_Idx = 0;
  /* Never read as a value, but from 2048 values it is the first free slot and
   * travels with the ring: set so two handles over the same bars hold the same
   * state.
   */
  sorted[lookbackTotal] = 0.0;
  /* Keep the multiply left of the divide. (P*n)/100 reproduces exact integer
   * arithmetic; P/100 is inexact in binary64 and lands the product just above
   * an integer, one order statistic too high, at exactly the round
   * percentages a caller types.
   */
  rank = (Math.ceil(optInPercentile * optInTimePeriod / 100.0) | 0);
  if (rank < 1) {
    rank = 1;
  }
  if (rank > optInTimePeriod) {
    rank = optInTimePeriod;
  }
  hiRank = rank - 1;
  loRank = (hiRank > 0) ? hiRank - 1 : 0;
  /* The retained values are sorted ascending with every run of equal values
   * in age order: the departing value is then the first of its run bit for
   * bit, and a new value goes after its equals. Only the sign of a zero can
   * observe that order, and it does.
   */
  i = startIdx - lookbackTotal;
  if (lookbackTotal < 100) {
    nbSorted = 0;
    while (i < startIdx) {
      newValue = inReal[i];
      j = nbSorted;
      while (j > 0 && sorted[j - 1] > newValue) {
        sorted[j] = sorted[j - 1];
        j -= 1;
      }
      sorted[j] = newValue;
      nbSorted += 1;
      ring[ring_Idx] = newValue;
      i += 1;
      ring_Idx++;
      if (ring_Idx > maxIdx_ring) {
        ring_Idx = 0;
      }
    }
  } else {
    /* Bottom-up merge sort, stable, with ring as the other half until it
     * is filled by age below.
     */
    sorted[0] = inReal[i];
    j = 1;
    while (j < lookbackTotal && inReal[i + j - 1] <= inReal[i + j]) {
      sorted[j] = inReal[i + j];
      j += 1;
    }
    if (j < lookbackTotal) {
      s = 0;
      while (s < lookbackTotal) {
        e = s + 16;
        if (e > lookbackTotal) {
          e = lookbackTotal;
        }
        t = s;
        while (t < e) {
          newValue = inReal[i + t];
          j = t;
          while (j > s && sorted[j - 1] > newValue) {
            sorted[j] = sorted[j - 1];
            j -= 1;
          }
          sorted[j] = newValue;
          t += 1;
        }
        s = e;
      }
      w = 16;
      while (w < lookbackTotal) {
        lo = 0;
        while (lo < lookbackTotal) {
          mid = lo + w;
          if (mid > lookbackTotal) {
            mid = lookbackTotal;
          }
          hi = mid + w;
          if (hi > lookbackTotal) {
            hi = lookbackTotal;
          }
          a = lo;
          b = mid;
          t = lo;
          if (mid < hi && sorted[mid - 1] > sorted[mid]) {
            while (a < mid && b < hi) {
              if (sorted[b] < sorted[a]) {
                ring[t] = sorted[b];
                b += 1;
              } else {
                ring[t] = sorted[a];
                a += 1;
              }
              t += 1;
            }
          }
          while (a < mid) {
            ring[t] = sorted[a];
            a += 1;
            t += 1;
          }
          while (b < hi) {
            ring[t] = sorted[b];
            b += 1;
            t += 1;
          }
          lo = hi;
        }
        w += w;
        if (w < lookbackTotal) {
          lo = 0;
          while (lo < lookbackTotal) {
            mid = lo + w;
            if (mid > lookbackTotal) {
              mid = lookbackTotal;
            }
            hi = mid + w;
            if (hi > lookbackTotal) {
              hi = lookbackTotal;
            }
            a = lo;
            b = mid;
            t = lo;
            if (mid < hi && ring[mid - 1] > ring[mid]) {
              while (a < mid && b < hi) {
                if (ring[b] < ring[a]) {
                  sorted[t] = ring[b];
                  b += 1;
                } else {
                  sorted[t] = ring[a];
                  a += 1;
                }
                t += 1;
              }
            }
            while (a < mid) {
              sorted[t] = ring[a];
              a += 1;
              t += 1;
            }
            while (b < hi) {
              sorted[t] = ring[b];
              b += 1;
              t += 1;
            }
            lo = hi;
          }
          w += w;
        } else {
          t = 0;
          while (t < lookbackTotal) {
            sorted[t] = ring[t];
            t += 1;
          }
        }
      }
    }
    t = 0;
    while (t < lookbackTotal) {
      ring[t] = inReal[i + t];
      t += 1;
    }
    ring_Idx = lookbackTotal;
    i = startIdx;
  }
  /* Below 32 values: every bar rewrites the whole window with no
   * data-dependent branch, or, after a long enough run in one direction,
   * reads the two ranks it needs straight off the ring.
   */
  tail = lookbackTotal - ((lookbackTotal % 2) | 0);
  lim = (lookbackTotal > 13) ? lookbackTotal - 2 : 11;
  run = -1;
  dPrev = 0;
  trend = 0;
  last = inReal[startIdx - 1];
  /* From m = 32 retained values, layout 0 keeps slots 0..m-1 ONE BAR BEHIND:
   * the previous bar's update (remove rank pendDel, insert pendNew at pendPos
   * counted before the removal) is still pending and the spare slot m holds
   * pendNew. A bar searches that array and corrects its counts by two
   * compares, so its search does not wait on the previous bar's shift.
   * Layout 1 is a ring of all m+1 slots, rank r at slot head+r and the free
   * slot just before head; layout 2 is layout 1 with the pending update still
   * to apply. The ring pays for itself on a trending window, where a new
   * extreme arrives as the opposite one departs, and from 2048 values, where
   * moving the shorter way round halves the shift.
   */
  layout = (lookbackTotal >= 2048 ? 1 : 0);
  head = 0;
  runLen = 0;
  saving = 0;
  pendNew = sorted[0];
  pendOld = pendNew;
  pendDel = 0;
  pendPos = 1;
  hiV = (rank <= lookbackTotal) ? sorted[hiRank] : 0.0;
  loV = (rank > 1) ? sorted[loRank] : 0.0;
  /* Both scratch buffers hold copies and inReal is never read below i, so
   * inReal and outReal may be the same buffer.
   *
   * Every store to state sits BELOW the output store: the streaming peek
   * frame is this loop cut there.
   */
  outIdx = 0;
  do {
    newValue = inReal[i];
    /* The full window is the retained values with newValue inserted after
     * its equals, so its rank-th value is newValue clamped to the retained
     * values of rank rank-1 (loV) and rank (hiV).
     */
    result = newValue;
    if (rank <= lookbackTotal) {
      result = (result < hiV) ? result : hiV;
    }
    if (rank > 1) {
      result = (result < loV) ? loV : result;
    }
    outReal[((outIdx * outStride) | 0)] = result;
    outIdx += 1;
    ring[ring_Idx] = newValue;
    ring_Idx++;
    if (ring_Idx > maxIdx_ring) {
      ring_Idx = 0;
    }
    oldValue = ring[ring_Idx];
    if (lookbackTotal < 32) {
      if (lookbackTotal === 1) {
        sorted[0] = newValue;
        hiV = newValue;
        loV = newValue;
      } else {
        /* 0/1 arithmetic, not conditional assignments: gcc threads those
         * into a branch on newValue < last, a coin flip on random data.
         * run starts at -1 so the first step cannot count twice.
         */
        dnStep = (newValue < last ? 1 : 0);
        same = (dnStep === dPrev ? 1 : 0);
        run = (((run + 1) * same) | 0);
        dPrev = dnStep;
        last = newValue;
        if (run >= lim) {
          /* The ring is then the sorted order, from its oldest slot on
           * a rising run and from its newest on a falling one, which
           * must be strict: read newest first, equal values would come
           * out in reverse age order. sorted is left stale until the
           * run breaks.
           */
          run = lim;
          if (dnStep === 0) {
            trend = 1;
            jh = ring_Idx + rank;
            if (jh >= optInTimePeriod) {
              jh -= optInTimePeriod;
            }
            jl = (jh === 0) ? lookbackTotal : jh - 1;
          } else {
            trend = 2;
            jh = ring_Idx + (optInTimePeriod - rank);
            if (jh >= optInTimePeriod) {
              jh -= optInTimePeriod;
            }
            jl = (jh === lookbackTotal) ? 0 : jh + 1;
          }
          if (rank <= lookbackTotal) {
            hiV = ring[jh];
          }
          if (rank > 1) {
            loV = ring[jl];
          }
        } else {
          if (trend !== 0) {
            if (trend === 1) {
              j = ring_Idx;
              k = 0;
              while (k < lookbackTotal) {
                sorted[k] = ring[j];
                j = (j === lookbackTotal) ? 0 : j + 1;
                k += 1;
              }
            } else {
              j = ring_Idx + lookbackTotal - 1;
              if (j >= optInTimePeriod) {
                j -= optInTimePeriod;
              }
              k = 0;
              while (k < lookbackTotal) {
                sorted[k] = ring[j];
                j = (j === 0) ? lookbackTotal : j - 1;
                k += 1;
              }
            }
            trend = 0;
          }
          /* With D the retained values less oldValue, slot k becomes
           * max(D[k-1], min(newValue, D[k])). The spare slot stands in
           * for D past the end.
           */
          sorted[lookbackTotal] = newValue;
          cur = sorted[0];
          prv = (newValue < cur) ? newValue : cur;
          k = 0;
          do {
            nxt = sorted[k + 1];
            nx2 = sorted[k + 2];
            ins = (cur < oldValue) ? cur : nxt;
            in2 = (nxt < oldValue) ? nxt : nx2;
            res = (newValue < ins) ? newValue : ins;
            sorted[k] = (prv > res) ? prv : res;
            res = (newValue < in2) ? newValue : in2;
            sorted[k + 1] = (ins > res) ? ins : res;
            prv = in2;
            cur = nx2;
            k += 2;
          } while (k < tail);
          sorted[tail] = (prv > newValue) ? prv : newValue;
          if (rank <= lookbackTotal) {
            hiV = sorted[hiRank];
          }
          if (rank > 1) {
            loV = sorted[loRank];
          }
        }
      }
    } else {
      if (layout !== 0) {
        if (layout === 2) {
          if (pendDel < pendPos) {
            for (k = pendDel; k < pendPos - 1; k += 1) {
              sorted[k] = sorted[k + 1];
            }
            sorted[pendPos - 1] = pendNew;
          } else {
            for (k = pendDel; k > pendPos; k -= 1) {
              sorted[k] = sorted[k - 1];
            }
            sorted[pendPos] = pendNew;
          }
          layout = 1;
        }
        freeSlot = (head === 0) ? lookbackTotal : head - 1;
        maxSlot = (freeSlot === 0) ? lookbackTotal : freeSlot - 1;
        nextSlot = (maxSlot === 0) ? lookbackTotal : maxSlot - 1;
        /* A new value tied with the minimum belongs after it, so only a
         * strictly smaller one may take the front.
         */
        if (newValue >= sorted[maxSlot] && oldValue <= sorted[head]) {
          sorted[freeSlot] = newValue;
          head = (head === lookbackTotal) ? 0 : head + 1;
        } else if (newValue < sorted[head] && oldValue > sorted[nextSlot]) {
          sorted[freeSlot] = newValue;
          head = freeSlot;
        } else if (saving > 0 || lookbackTotal >= 2048) {
          /* pos = retained values <= newValue, del = retained values <
           * oldValue, both searched from the free slot as rank -1.
           */
          bp = freeSlot;
          bq = bp;
          len = optInTimePeriod;
          while (len > 2) {
            half = (((len + 3) / 4) | 0);
            o3 = len - half;
            o2 = half + half;
            q1 = (bp < optInTimePeriod - half) ? bp + half : bp + half - optInTimePeriod;
            q2 = (bp < optInTimePeriod - o2) ? bp + o2 : bp + o2 - optInTimePeriod;
            q3 = (bp < optInTimePeriod - o3) ? bp + o3 : bp + o3 - optInTimePeriod;
            p1 = (bq < optInTimePeriod - half) ? bq + half : bq + half - optInTimePeriod;
            p2 = (bq < optInTimePeriod - o2) ? bq + o2 : bq + o2 - optInTimePeriod;
            p3 = (bq < optInTimePeriod - o3) ? bq + o3 : bq + o3 - optInTimePeriod;
            bp = (sorted[q1] <= newValue) ? q1 : bp;
            bp = (sorted[q2] <= newValue) ? q2 : bp;
            bp = (sorted[q3] <= newValue) ? q3 : bp;
            bq = (sorted[p1] < oldValue) ? p1 : bq;
            bq = (sorted[p2] < oldValue) ? p2 : bq;
            bq = (sorted[p3] < oldValue) ? p3 : bq;
            len = half;
          }
          if (len > 1) {
            sP = (bp < lookbackTotal) ? bp + 1 : 0;
            sQ = (bq < lookbackTotal) ? bq + 1 : 0;
            bp = (sorted[sP] <= newValue) ? sP : bp;
            bq = (sorted[sQ] < oldValue) ? sQ : bq;
          }
          pos = (bp >= freeSlot) ? bp - freeSlot : bp + optInTimePeriod - freeSlot;
          del = (bq >= freeSlot) ? bq - freeSlot : bq + optInTimePeriod - freeSlot;
          gain = (del < pos) ? pos - 1 - del : del - pos;
          seg = lookbackTotal + ((((lookbackTotal / 4) | 0) * 3) | 0);
          saving = (saving + gain + gain > seg) ? saving + gain + gain - seg : 0;
          saving = (saving < ((lookbackTotal * 16) | 0)) ? saving : ((lookbackTotal * 16) | 0);
          /* The hole left by oldValue travels to newValue's place the
           * shorter way round; going through the free slot moves head
           * by one.
           */
          slot = head + del;
          if (slot >= optInTimePeriod) {
            slot -= optInTimePeriod;
          }
          if (del < pos) {
            cnt = pos - 1 - del;
            dn = 1;
          } else {
            cnt = del - pos;
            dn = 0;
          }
          if (cnt + cnt > lookbackTotal) {
            cnt = lookbackTotal - cnt;
            dn = 1 - dn;
            if (dn === 1) {
              head = freeSlot;
            } else {
              head = (head === lookbackTotal) ? 0 : head + 1;
            }
          }
          if (dn === 1) {
            room = lookbackTotal - slot;
            if (cnt <= room) {
              e = slot + cnt;
              for (k = slot; k < e; k += 1) {
                sorted[k] = sorted[k + 1];
              }
              slot = e;
            } else {
              for (k = slot; k < lookbackTotal; k += 1) {
                sorted[k] = sorted[k + 1];
              }
              sorted[lookbackTotal] = sorted[0];
              e = cnt - room - 1;
              for (k = 0; k < e; k += 1) {
                sorted[k] = sorted[k + 1];
              }
              slot = e;
            }
          } else if (cnt <= slot) {
            e = slot - cnt;
            for (k = slot; k > e; k -= 1) {
              sorted[k] = sorted[k - 1];
            }
            slot = e;
          } else {
            for (k = slot; k > 0; k -= 1) {
              sorted[k] = sorted[k - 1];
            }
            sorted[0] = sorted[lookbackTotal];
            e = lookbackTotal - (cnt - slot - 1);
            for (k = lookbackTotal; k > e; k -= 1) {
              sorted[k] = sorted[k - 1];
            }
            slot = e;
          }
          sorted[slot] = newValue;
        } else {
          /* Back to layout 0: rotate head to slot 0 by three
           * reversals, then carry an empty pending update.
           */
          if (head !== 0) {
            k = 0;
            e = head - 1;
            while (k < e) {
              held = sorted[k];
              sorted[k] = sorted[e];
              sorted[e] = held;
              k += 1;
              e -= 1;
            }
            k = head;
            e = lookbackTotal;
            while (k < e) {
              held = sorted[k];
              sorted[k] = sorted[e];
              sorted[e] = held;
              k += 1;
              e -= 1;
            }
            k = 0;
            e = lookbackTotal;
            while (k < e) {
              held = sorted[k];
              sorted[k] = sorted[e];
              sorted[e] = held;
              k += 1;
              e -= 1;
            }
            head = 0;
          }
          pendNew = sorted[0];
          pendOld = pendNew;
          pendDel = 0;
          pendPos = 1;
          sorted[lookbackTotal] = pendNew;
          layout = 0;
          runLen = 0;
        }
      }
      if (layout === 0) {
        /* Branchless four-way search of the lagging array, no wrap. */
        pos = 0;
        del = 0;
        len = optInTimePeriod;
        while (len > 2) {
          half = (((len + 3) / 4) | 0);
          o3 = len - half;
          o2 = half + half;
          t1 = pos + half;
          t2 = pos + o2;
          t3 = pos + o3;
          u1 = del + half;
          u2 = del + o2;
          u3 = del + o3;
          pos = (sorted[t1 - 1] <= newValue) ? t1 : pos;
          pos = (sorted[t2 - 1] <= newValue) ? t2 : pos;
          pos = (sorted[t3 - 1] <= newValue) ? t3 : pos;
          del = (sorted[u1 - 1] < oldValue) ? u1 : del;
          del = (sorted[u2 - 1] < oldValue) ? u2 : del;
          del = (sorted[u3 - 1] < oldValue) ? u3 : del;
          len = half;
        }
        /* Increments, not selects: gcc turns the select into a jump. */
        if (len > 1) {
          pos += (sorted[pos] <= newValue ? 1 : 0);
          del += (sorted[del] < oldValue ? 1 : 0);
        }
        pos += (pendNew <= newValue ? 1 : 0);
        del += (pendNew < oldValue ? 1 : 0);
        dp = (pendOld <= newValue ? 1 : 0);
        dq = (pendOld < oldValue ? 1 : 0);
        /* Clamped although exact for finite input: a NaN makes the counts
         * disagree with the corrections, and they index the shift below.
         */
        pos = (pos > dp) ? pos - dp : 0;
        del = (del > dq) ? del - dq : 0;
        pos = (pos < lookbackTotal) ? pos : lookbackTotal;
        del = (del < lookbackTotal) ? del : lookbackTotal;
        if (pendDel < pendPos) {
          for (k = pendDel; k < pendPos - 1; k += 1) {
            sorted[k] = sorted[k + 1];
          }
          sorted[pendPos - 1] = pendNew;
        } else {
          for (k = pendDel; k > pendPos; k -= 1) {
            sorted[k] = sorted[k - 1];
          }
          sorted[pendPos] = pendNew;
        }
        sorted[lookbackTotal] = newValue;
        pendPos = pos;
        pendDel = del;
        pendNew = newValue;
        pendOld = oldValue;
        /* Leave for the ring after four bars that each add a new extreme
         * as the opposite one departs, or, from 300 values, while the
         * shift the ring would save keeps exceeding 3/4 of the window.
         */
        runLen = (pos === del + lookbackTotal || del === pos + lookbackTotal - 1) ? runLen + 1 : 0;
        if (lookbackTotal >= 300) {
          gain = (del < pos) ? pos - 1 - del : del - pos;
          seg = lookbackTotal + ((((lookbackTotal / 4) | 0) * 3) | 0);
          saving = (saving + gain + gain > seg) ? saving + gain + gain - seg : 0;
        }
        if (runLen >= 4 || saving > ((lookbackTotal * 8) | 0)) {
          layout = 2;
        }
      }
      if (layout !== 1) {
        pp = (pendDel < pendPos) ? pendPos - 1 : pendPos;
        hiSlot = (hiRank > pp) ? hiRank - 1 : hiRank;
        hiSlot += (hiSlot >= pendDel ? 1 : 0);
        hiSlot = (hiRank === pp) ? lookbackTotal : hiSlot;
        loSlot = (loRank > pp) ? loRank - 1 : loRank;
        loSlot += (loSlot >= pendDel ? 1 : 0);
        loSlot = (loRank === pp) ? lookbackTotal : loSlot;
      } else {
        hiSlot = head + hiRank;
        if (hiSlot >= optInTimePeriod) {
          hiSlot -= optInTimePeriod;
        }
        loSlot = (hiSlot === 0) ? lookbackTotal : hiSlot - 1;
      }
      if (rank <= lookbackTotal) {
        hiV = sorted[hiSlot];
      }
      if (rank > 1) {
        loV = sorted[loSlot];
      }
    }
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capCb_ring = maxIdx_ring + 1;
  if (capCb_ring > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_sorted = maxIdx_sorted + 1;
  if (capCb_sorted > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInPercentile = optInPercentile;
  sp.loV = loV;
  sp.hiV = hiV;
  sp.last = last;
  sp.pendNew = pendNew;
  sp.pendOld = pendOld;
  sp.lookbackTotal = lookbackTotal;
  sp.rank = rank;
  sp.hiRank = hiRank;
  sp.loRank = loRank;
  sp.tail = tail;
  sp.lim = lim;
  sp.run = run;
  sp.dPrev = dPrev;
  sp.trend = trend;
  sp.layout = layout;
  sp.head = head;
  sp.pendPos = pendPos;
  sp.pendDel = pendDel;
  sp.runLen = runLen;
  sp.saving = saving;
  sp.ring_Idx = ring_Idx;
  sp.maxIdx_ring = maxIdx_ring;
  sp.sorted_Idx = sorted_Idx;
  sp.maxIdx_sorted = maxIdx_sorted;
  sp.cbSize_ring = capCb_ring;
  sp.cb_ring = ring;
  sp.cbSize_sorted = capCb_sorted;
  sp.cb_sorted = sorted;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function percentileOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInPercentile: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): PercentileStream {
  const sp = PercentileStream.of(core);
  const retCode = percentileOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInPercentile, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PERCENTILE openAndFill', inReal.length, startIdx, percentileLookbackPositional(optInTimePeriod, optInPercentile, core));
  }
  if (retCode === RetCode.BAD_PARAM && percentileLookbackPositional(optInTimePeriod, optInPercentile, core) < 0) {
    throw rejectedParams('PERCENTILE openAndFill', PARAM_RANGES, optInTimePeriod, optInPercentile);
  }
  throw streamFailure('PERCENTILE openAndFill', retCode);
}

/** @internal */
export function percentileOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInPercentile: number,
): PercentileStream {
  const sp = PercentileStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = percentileOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInPercentile, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PERCENTILE open', inReal.length, startIdx, percentileLookbackPositional(optInTimePeriod, optInPercentile, core));
  }
  if (retCode === RetCode.BAD_PARAM && percentileLookbackPositional(optInTimePeriod, optInPercentile, core) < 0) {
    throw rejectedParams('PERCENTILE open', PARAM_RANGES, optInTimePeriod, optInPercentile);
  }
  throw streamFailure('PERCENTILE open', retCode);
}

/** @internal */
export function percentileOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInPercentile: number,
  core: Core = Core.DEFAULT,
): PercentileStream {
  optInTimePeriod = requireInt32('PERCENTILE open', 'timePeriod', optInTimePeriod);
  optInPercentile = requireReal('PERCENTILE open', 'percentile', optInPercentile);
  requireArgument('PERCENTILE open', 'real', inReal);
  requireHistory('PERCENTILE open', inReal.length);
  return percentileOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInPercentile);
}

/** @internal */
export function percentileOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInPercentile: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): PercentileStream {
  optInTimePeriod = requireInt32('PERCENTILE openAndFill', 'timePeriod', optInTimePeriod);
  optInPercentile = requireReal('PERCENTILE openAndFill', 'percentile', optInPercentile);
  requireArgument('PERCENTILE openAndFill', 'real', inReal);
  requireHistory('PERCENTILE openAndFill', inReal.length);
  const guardLookback = percentileLookbackPositional(optInTimePeriod, optInPercentile, core);
  if (guardLookback < 0) {
    throw rejectedParams('PERCENTILE openAndFill', PARAM_RANGES, optInTimePeriod, optInPercentile);
  }
  const guardOutLen = openFillCount('PERCENTILE openAndFill', inReal.length, guardLookback);
  requireOutput('PERCENTILE openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('PERCENTILE openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('PERCENTILE openAndFill', 'outReal', 'real');
  }
  return percentileOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInPercentile, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live PERCENTILE stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link percentile} there.
 *
 * @param real - Warm-up history. Source series to take the percentile of.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PercentileOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `percentileLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a real
 *   parameter that is not a number or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { percentileOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = percentileOpen(data, { timePeriod: 100, percentile: 50 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function percentileOpen(real: ArrayLike<number>, options?: PercentileOptions): PercentileStream {
  const o = optionsOf('PERCENTILE open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInPercentile = orDefault(o.percentile, Core.REAL_DEFAULT);
  const core = coreOf('PERCENTILE open', o.core);
  return percentileOpenPositional(real, optInTimePeriod, optInPercentile, core);
}

/**
 * {@link percentileOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link percentile}.
 *
 * @param real - Warm-up history. Source series to take the percentile of.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link PercentileOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link percentile} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `percentileLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range or a buffer too
 *   short, of the wrong type or starting where another buffer or a `Float64Array`
 *   input starts, or, on a history long enough to open, one sharing memory with
 *   another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { percentileOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = percentileOpenAndFill(data, {
 *   timePeriod: 100,
 *   percentile: 50,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function percentileOpenAndFill(real: ArrayLike<number>, options?: PercentileOptions): PercentileResult & { stream: PercentileStream } {
  const o = optionsOf('PERCENTILE openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInPercentile = orDefault(o.percentile, Core.REAL_DEFAULT);
  const core = coreOf('PERCENTILE openAndFill', o.core);
  const capacity = fillCapacity(percentileLookbackPositional(optInTimePeriod, optInPercentile, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = percentileOpenAndFillPositional(real, optInTimePeriod, optInPercentile, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
