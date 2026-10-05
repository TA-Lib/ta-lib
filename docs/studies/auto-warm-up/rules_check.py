# Checks of the proven Auto rules against the kernels' decay laws, at seven values of K and a
# grid of periods. Not exhaustive: KAMA and FRAMA follow from their coefficient floor and are
# not here; the two-pole envelopes are stated against the largest difference the seed causes.
import math
Ks=[7,10,14,19,20,28,37]
def ceil_div(a,b): return -(-a//b)
bad=[]
# one-pole kernels: weight (1-alpha)^m
for K in Ks:
    for n in list(range(2,2000))+[5000,10000,50000,100000]:
        m=ceil_div(K*(n+1),2)
        if m*math.log1p(-2/(n+1)) > -K: bad.append(('ema',K,n))
        m=K*n
        if n>1 and m*math.log1p(-1/n) > -K: bad.append(('wilder',K,n))
    if (2*K)*math.log(0.5) > -K: bad.append(('ha',K))
print('one-pole violations:',bad[:5], len(bad))

# SWAK double real pole: d_k <= S * e^{-x}(1+e x), x = k * (-ln om); rule m = ceil((K+5)(P+2)/9)
def om(P):
    w=2*math.pi/P; b=2.415*(1-math.cos(w)); a=-b+math.sqrt(b*b+2*b); return 1-a
worst=-1e9; bad=[]
for K in Ks:
    for P in list(range(2,3000))+[5000,10000]:
        m=ceil_div((K+5)*(P+2),9)
        x=m*(-math.log(om(P)))
        w=-x+math.log(1+math.e*x)       # log of the S-relative bound
        if w>-K: bad.append((K,P,w))
        worst=max(worst,(w+K))
print('2-pole: violations',len(bad),bad[:3],' tightest slack (K-units, <=0 ok):',worst)
# how conservative: bars at rule / bars at exact need (solve x - ln(1+e x) = K)
def need(K,P):
    lo=om(P); k=0
    while True:
        x=k*(-math.log(lo))
        if -x+math.log(1+math.e*x) <= -K: return k
        k+=1
for P in (2,5,10,20,60,200,1000,10000):
    print('  P=%5d  rule(K=14)=%5d  exact-bound need=%5d  bars/K-unit=%.3f'%(P,ceil_div(19*(P+2),9),need(14,P),1/(-math.log(om(P)))))

# SWAK_HP one pole a1=(1-sin w)/cos w ; rule m = ceil(K*P/6)
bad=[]; worst=-1e9
for K in Ks:
    for P in list(range(5,3000))+[10000,100000]:
        w=2*math.pi/P; a1=(1-math.sin(w))/math.cos(w)
        m=ceil_div(K*P,6)
        v=m*math.log(a1)+K
        worst=max(worst,v)
        if v>0: bad.append((K,P))
print('HP: violations',len(bad),' tightest slack',worst)

# SWAK_BP complex pair radius sqrt(abp); check complex for all params; rule m = ceil((K+1)*P/(6*delta))
bad=[]; realpoles=0; worst=-1e9
for K in Ks:
    for P in list(range(5,2001)):
        for delta in (0.05,0.07,0.1,0.2,0.3,0.4,0.5):
            w=2*math.pi/P; beta=math.cos(w); t=4*math.pi*delta/P
            abp=(1-math.sin(t))/math.cos(t)
            disc=(beta*(1+abp))**2-4*abp
            if disc>=0: realpoles+=1; continue
            r=math.sqrt(abp)
            m=math.ceil(((K+1)*P)/(6.0*delta))
            # envelope C r^m with C <= 2S (see derivation): need ln2 + m ln r <= -K
            v=math.log(2)+m*math.log(r)+K
            worst=max(worst,v)
            if v>0: bad.append((K,P,delta,v))
print('BP: real-pole cases',realpoles,' violations',len(bad),bad[:3],' tightest slack',worst)

# T3: six equal EMAs in lockstep; sup-norm state weight = P(Bin(m,alpha) <= 5); rule m = ceil((K+20)(n+1)/2)
def binom_tail(m,a,r=5):
    s=0.0
    for j in range(r+1):
        s+=math.exp(math.lgamma(m+1)-math.lgamma(j+1)-math.lgamma(m-j+1)+j*math.log(a)+(m-j)*math.log1p(-a))
    return s
bad=[]; worst=-1e9
for K in Ks:
    for n in list(range(2,400))+[1000,10000,100000]:
        a=2/(n+1); m=ceil_div((K+20)*(n+1),2)
        v=math.log(binom_tail(m,a))+K
        worst=max(worst,v)
        if v>0: bad.append((K,n,v))
print('T3 (K+20): violations',len(bad),bad[:3],' tightest slack',worst)
for c in (8,10,12,14,16):
    b=[(K,n) for K in Ks for n in (2,5,30,200) if math.log(binom_tail(ceil_div((K+c)*(n+1),2),2/(n+1)))+K>0]
    print('  T3 with K+%d: violations at'%c, sorted(set(k for k,_ in b)))
# two-stage lockstep (ADX-like, linear part): P(Bin(m,1/n) <= 1); rule m=(K+c)n
for c in (2,3,4,5,6):
    b=[(K,n) for K in Ks for n in (2,5,14,42,200,1000) if math.log(binom_tail((K+c)*n,1/n,1))+K>0]
    print('  2-stage Wilder with K+%d: violations at K='%c, sorted(set(k for k,_ in b)))
