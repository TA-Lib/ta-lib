# Checks of the proven Auto rules against the kernels' decay laws, at seven values of K and a
# grid of periods; FISHER, whose gain has no closed form, by iterating its kernel from two starts. Not exhaustive, and the calibrated rules are not here. The two-pole
# envelopes are stated against the largest difference the seed causes.
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

# FISHER: v = 0.67*v + 0.66*(r - 0.5), r in [0,1] the channel position; v snaps to +-0.999
# beyond +-0.99 and the snapped value is fed back; fish = atanh(v) + 0.5*fish; trigger is the
# previous fish. Rule m = ceil(5*(K+c)/2). Sized on the path where the two starts never sit on
# opposite sides of the snap: there dv is exactly 0.67^age * dv0 on any input, and what is
# searched is the gain between dv and the outputs, against S, the largest output difference.
def fisher_step(v,f,r):
    v=0.33*2.0*(r-0.5)+0.67*v
    if v>0.99: v=0.999
    if v<-0.99: v=-0.999
    return v, 0.5*math.log((1.0+v)/(1.0-v))+0.5*f
FN=128                                  # ages run; the longest rule listed is 115
def fisher_pair(pre,rs_of):
    """Earlier start runs the bars `pre` first; both then run FN ages. rs_of(age,va,vb) picks
    the input. The later start carries (vb,fb); the earlier one is carried as the difference
    (d,df) so that e^-37 is not lost to rounding: va = vb + d, and d*0.67 is the recursion's
    own update. Returns the output differences per age, or None when one start snaps alone."""
    va=fa=0.0
    for r in pre: va,fa=fisher_step(va,fa,r)
    vb=fb=0.0; d=va; df=fa
    out=[]
    for age in range(FN):
        r=rs_of(age,vb+d,vb)
        u=0.33*2.0*(r-0.5)
        a=u+0.67*(vb+d); b=u+0.67*vb
        sa=a>0.99 or a<-0.99; sb=b>0.99 or b<-0.99
        if sa!=sb: return None
        prev_df=df
        vb,fb=fisher_step(vb,fb,r)
        if sa: d=0.0
        else: d=0.67*d
        df=0.5*(math.log1p(d/(1.0+vb))-math.log1p(-d/(1.0-vb)))+0.5*df
        out.append(max(abs(df),abs(prev_df)))      # fisher, and trigger = the previous fisher
    return out
def fisher_literal(pre,rs):             # the same pair with two plain runs, to check the form above
    va=fa=0.0
    for r in pre: va,fa=fisher_step(va,fa,r)
    vb=fb=0.0; out=[]
    for r in rs:
        pa,pb=fa,fb
        va,fa=fisher_step(va,fa,r); vb,fb=fisher_step(vb,fb,r)
        out.append(max(abs(fa-fb),abs(pa-pb)))
    return out
def fisher_inputs(hold,sign,level):
    """Rest at the centre for `hold` ages, where the slope of atanh is 1, then drive both
    starts to `level` on one side and keep the outer one there: the largest slope the
    no-snap path has is at 0.99, 1/(1-0.99^2) = 50.25."""
    used=[]
    def pick(age,va,vb):
        if age<hold: r=0.5
        else:
            hi=max(sign*va,sign*vb)
            u=min(0.33,max(-0.33,level-0.67*hi))
            r=0.5+sign*u/0.66
        used.append(r); return r
    return pick,used
import random
random.seed(485)
EDGE=0.99-1e-9
cases=[]                                # (pre, input picker factory)
pres=[]
# states two bars reach: a small v with a fish of either sign and of any size against it
for ve in (1e-6,1e-4,1e-2,0.03,0.1,0.2,0.3,0.4,0.5):
    for x in (-2,-1,-0.5,0,0.3,0.5,0.6,0.627,0.65,0.7,0.8,1,1.5,2,3):
        fe=-x*ve                        # the fish that cancels part of the first differences
        u1=math.tanh(2.0*(fe-math.atanh(ve))); u2=ve-0.67*u1
        if abs(u1)<=0.33 and abs(u2)<=0.33: pres.append([0.5+u1/0.66,0.5+u2/0.66])
# states a long history reaches: pinned, snapped, leaving a snap, crossing the centre
for run in ([1.0]*40,[0.995]*60,[0.0]*40,[0.9]*40,[0.7]*40,[0.2]*40):
    for tail in ([],[0.5],[0.0],[1.0],[0.0,1.0],[0.0,0.5,1.0],[0.5]*4,[0.0]*2+[1.0]*3):
        pres.append(run+tail)
for _ in range(60): pres.append([random.random() for _ in range(random.randint(1,30))])
worst=(-1e9,None); nstep=0; nrun=0; need={K:0 for K in Ks}; maxdev=0.0
for pre in pres:
    for sign in (1,-1):
        for hold in (0,1,2,3,4,6,9,13,18,25,40):
            for level in (EDGE,0.9,0.5):
                pick,used=fisher_inputs(hold,sign,level)
                o=fisher_pair(pre,pick); nrun+=1
                if o is None: nstep+=1; continue
                S=max(o)
                if S==0.0: continue
                if nrun%97==0:           # the difference form against two plain runs, above rounding
                    lit=fisher_literal(pre,used)
                    maxdev=max(maxdev,max(abs(x-y) for x,y in zip(o,lit)))
                for age in range(FN):
                    if o[age]>0.0:
                        g=math.log(o[age]/S)-age*math.log(0.67)
                        if g>worst[0] and age>=20: worst=(g,(len(pre),sign,hold,level,age))
                        for K in Ks:
                            if o[age]>math.exp(-K)*S and age+1>need[K]: need[K]=age+1
    for _ in range(4):                  # inputs with no design, as a floor for the search
        seq=[random.choice((0.0,1.0,random.random())) for _ in range(FN)]
        o=fisher_pair(pre,lambda age,va,vb:seq[age]); nrun+=1
        if o is None: nstep+=1; continue
        S=max(o)
        if S==0.0: continue
        for age in range(FN):
            for K in Ks:
                if o[age]>math.exp(-K)*S and age+1>need[K]: need[K]=age+1
# the bound the search should approach and not pass: slope 50.25, times 0.67/(0.67-0.5) for
# the 0.5 pole, over S >= 0.936*dv0 (ages 0 and 1 with slopes >= 1), plus one bar for trigger
bound=math.log(50.25*0.67/0.17/0.936)-math.log(0.67)
print('FISHER: bars per e-fold %.4f (rule 2.5); runs %d, of which %d put one start alone across the snap (left out)'%(-1/math.log(0.67),nrun,nstep))
print('  worst gain found %.3f e-folds at (prehistory bars, side, hold, level, age) %s; bound %.3f'%(worst[0],worst[1],bound))
print('  difference form against two plain runs, largest gap %.1e'%maxdev)
print('  need at K',Ks,'=',[need[K] for K in Ks])
for c in (4,5,6,7,8,9):
    print('  FISHER with K+%d: rule'%c,[(5*(K+c)+1)//2 for K in Ks],'violations at K=',[K for K in Ks if need[K]>(5*(K+c)+1)//2])
print('  smallest c with no violation:',min(c for c in range(0,30) if all(need[K]<=(5*(K+c)+1)//2 for K in Ks)))
# The snap is a step and it latches: 0.999 fed back stays above 0.99 while r > 0.9859, and a
# start that never crossed stays under it while r <= 0.995. Real channel, period 10: a run
# of new highs, then every other bar a new high and the bar between 1.7% of the range below.
def fisher_series(P,start,n=10):
    v=f=0.0; out={}
    for t in range(start+n-1,len(P)):
        w=P[t-n+1:t+1]; hi=max(w); lo=min(w)
        r=(P[t]-lo)/(hi-lo) if hi>lo else 0.5
        v,f=fisher_step(v,f,r); out[t]=f
    return out
P=[100.0+10.0*t for t in range(30)]
while len(P)<4000: P.append(round(P[-1]+10.69,2)); P.append(round(P[-1]-0.69,2))
a=fisher_series(P,0); b=fisher_series(P,40)
print('  latch: fisher at the last two bars from bar 0 %.4f %.4f, from bar 40 %.4f %.4f, %d bars after the later start'%(a[len(P)-2],a[len(P)-1],b[len(P)-2],b[len(P)-1],len(P)-40))
