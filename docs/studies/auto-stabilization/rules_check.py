# Checks of the proven Auto rules against the kernels' decay laws, at seven values of K and a
# grid of periods; HA in exact fractions, T3 against the worst seed of its six stages, and
# FISHER, whose gain has no closed form, by iterating the kernel from two starts. Not exhaustive, and the calibrated rules are not here. The two-pole
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
print('one-pole violations:',bad[:5], len(bad))

# HA: open = (open + close)/2, the close is the bar's own average, so the opens of two starts
# differ by exactly 2^-age; the high and the low take a max and a min with the open. Rule
# m = ceil(13*K/9). The bound is the need itself, ceil(K/ln 2): the open shows the whole first
# difference at age 0, so S is that difference. Run in exact fractions, since 2^-54 of a
# difference is under a double's rounding of the price.
from fractions import Fraction
import random
def ha_pair(pre,bars):
    """bars: (o,h,l,c) in cents. The earlier start seeds at bars[0] and runs `pre` bars before
    the later one seeds. Returns the largest output difference at each age of the later start."""
    def out(op,b):
        cl=Fraction(b[0]+b[1]+b[2]+b[3],4)
        return (op,max(b[1],op,cl),min(b[2],op,cl),cl)
    a=Fraction(bars[0][0]+bars[0][3],2); res=[]
    for t in range(1,len(bars)+1):
        if t>pre:
            if t-1==pre: b=Fraction(bars[pre][0]+bars[pre][3],2)
            res.append(max(abs(x-y) for x,y in zip(out(a,bars[t-1]),out(b,bars[t-1]))))
            if t<len(bars): b=(b+out(b,bars[t-1])[3])/2
        if t<len(bars): a=(a+out(a,bars[t-1])[3])/2
    return res
random.seed(492)
HN=60                                    # ages run; the need at K = 37 is 54
ha_bound={K:math.ceil(K/math.log(2)) for K in Ks}
ha_need={K:0 for K in Ks}; ha_low={K:HN for K in Ks}; nrun=0; s_other=0
for trial in range(400):
    pre=random.choice((1,1,2,3,5,9,30)); c=10000; bars=[]
    kind=trial%4                         # 3: a high and a low that sit between the two opens
    for t in range(pre+HN):
        o=c+random.randint(-300,300); c=max(100,o+random.randint(-300,300))
        if kind==0: h=max(o,c)+random.randint(0,200); l=min(o,c)-random.randint(0,200)
        elif kind==1: h=max(o,c); l=min(o,c)
        elif kind==2: h=l=o=c
        else: h=l=o=c=10000+(700 if t==0 else 0)
        bars.append((o,h,l,c))
    d=ha_pair(pre,bars); S=max(d)
    if S==0: continue
    nrun+=1
    if S!=d[0]: s_other+=1
    for K in Ks:
        tol=Fraction(math.exp(-K))*S
        n=max([age+1 for age in range(HN) if d[age]>tol] or [0])
        ha_need[K]=max(ha_need[K],n); ha_low[K]=min(ha_low[K],n)
print('HA: bars per e-fold %.4f (rule 13/9 = %.4f); %d pairs, S is not the first open difference in %d'%(1/math.log(2),13/9,nrun,s_other))
print('  need at K',Ks,'=',[ha_need[K] for K in Ks],' smallest over the pairs',[ha_low[K] for K in Ks],' bound',[ha_bound[K] for K in Ks])
for name,rule in (('floor(10*K/7)',lambda K:10*K//7),('floor(13*K/9)',lambda K:13*K//9),('floor(3*K/2)',lambda K:3*K//2),
                  ('ceil(10*K/7)',lambda K:ceil_div(10*K,7)),('ceil(13*K/9)',lambda K:ceil_div(13*K,9)),
                  ('ceil(3*K/2)',lambda K:ceil_div(3*K,2)),('2*K',lambda K:2*K)):
    print('  HA with %-13s: rule'%name,[rule(K) for K in Ks],'violations at K=',[K for K in Ks if ha_need[K]>rule(K) or ha_bound[K]>rule(K)])
print('  ceil(13*K/9) against K/ln 2 for K in 1..100000: violations',sum(1 for K in range(1,100001) if ceil_div(13*K,9)*math.log(2)<K),
      ' bars over the bound, most',max(ceil_div(13*K,9)-math.ceil(K/math.log(2)) for K in Ks),'at the K listed')

# SWAK_GAUSS, SWAK_BUTTER, SWAK_2PHP: one denominator, 1 - 2*om*z^-1 + om^2*z^-2, a repeated
# real pole at om for every period (om = 1 - a2p, 0 < om < 1; none is a complex pair). The
# numerators only decide where the seed sits: two output slots, and for BUTTER and 2PHP two
# input slots that have left the forcing by age 2. So from age 0 the difference is
# d_k = (A + B*k) * om^k with A and B free. Worst case against S: a zero at z between age 0
# and the peak, with |d_0| equal to the peak, z = max over k > z of (k - z) * om^k. Past the
# peak d_m / S = (m - z)/z * om^m. Rule m = ceil((K+c)(P+2)/9).
SWAK_C=3
def swak_om(P):
    s=math.sin(math.pi/P); b=2.415*(2.0*s*s); return 1.0-(-b+math.sqrt(b*b+2.0*b))
def swak_peak(z,r):
    k=max(math.floor(z)+1,math.floor(z-1/math.log(r)))
    return max((j-z)*r**j for j in (k-1,k,k+1,k+2) if j>z)
def swak_zero(r):
    lo,hi=0.0,2.0-1/math.log(r)
    for _ in range(60):
        mid=(lo+hi)/2
        if mid<swak_peak(mid,r): lo=mid
        else: hi=mid
    return hi
def swak_slack(m,K,r,z):                # log(d_m/S) + K, or None when m is not past the peak
    if m<z-1/math.log(r): return None
    return math.log((m-z)/z)+m*math.log(r)+K
def swak_rule(K,P,c=SWAK_C,b=2): return ceil_div((K+c)*(P+b),9)
SWAK_P=list(range(2,10001))             # the range every one of the three yaml gives
swak_rz=[(P,)+(lambda r:(r,swak_zero(r)))(swak_om(P)) for P in SWAK_P]
worst=(-1e9,None); cand={(c,b):[] for c in (2,3,4,5) for b in (1,2,3)}; hi_slope=0.0
for P,r,z in swak_rz:
    hi_slope=max(hi_slope,-1/math.log(r)/(P+2))
    for K in Ks:
        for (c,b),v in cand.items():
            s=swak_slack(swak_rule(K,P,c,b),K,r,z)
            if s is None or s>0: v.append((K,P))
            elif (c,b)==(SWAK_C,2) and s>worst[0]: worst=(s,(K,P))
print('2-pole: bars per e-fold at most (P+2)/%.3f, and P/%.3f at P=10000 (rule (P+2)/9)'%(1/hi_slope,-10000*math.log(swak_rz[-1][1])))
print('  rule ceil((K+%d)(P+2)/9): violations %d, tightest slack (K-units, <=0 ok) %.4f at (K,P) %s'%(SWAK_C,len(cand[(SWAK_C,2)]),worst[0],worst[1]))
for (c,b),v in sorted(cand.items()):
    print('  ceil((K+%d)(P+%d)/9): violations %d'%(c,b,len(v)),'at K=',sorted(set(k for k,_ in v)),'P=',sorted(set(p for _,p in v))[:6])
short=sorted(set(K for K in range(1,61) for P,r,z in swak_rz if (lambda v:v is None or v>0)(swak_slack(swak_rule(K,P),K,r,z))))
print('  every K from 1 to 60: the rule is short at K=',short)
def swak_need(K,r,z):
    m=max(1,math.ceil(z-1/math.log(r)))
    while swak_slack(m,K,r,z)>0: m+=1
    return m
for P in (2,5,10,20,60,200,1000,10000):
    r=swak_om(P); z=swak_zero(r)
    print('  P=%5d  om=%.5f  bars/e-fold=%8.3f  need at K=10,19: %5d %5d  rule: %5d %5d'%(P,r,-1/math.log(r),swak_need(10,r,z),swak_need(19,r,z),swak_rule(10,P),swak_rule(19,P)))
# The three bodies as written, from two starts, on a prehistory solved to put the zero at z:
# the envelope must be reached and never passed. Inputs are 0 from the later start on, so the
# later run is exactly 0 and the earlier one is the difference, exact down to e^-37.
def swak_body(kind,P,x,start,n):
    s=math.sin(3.14159265358979323846/P); b2p=2.415*(2.0*s*s); a2p=-b2p+math.sqrt(b2p*b2p+2.0*b2p)
    om=1.0-a2p; a1=2.0*om; a2=-(om*om)
    c0={'GAUSS':a2p*a2p,'BUTTER':(a2p*a2p)/4.0,'2PHP':(1.0-a2p/2.0)*(1.0-a2p/2.0)}[kind]
    x1=x2=x[start]; y1=y2=0.0 if kind=='2PHP' else x[start]
    out=[]
    for t in range(start,start+n):
        x0=x[t] if t<len(x) else 0.0
        if kind=='GAUSS': y=a1*y1+(a2*y2+c0*x0)
        elif kind=='BUTTER': y=a1*y1+(a2*y2+c0*((x0+2.0*x1)+x2))
        else: y=a1*y1+(a2*y2+c0*((x0-2.0*x1)+x2))
        x2=x1; x1=x0; y2=y1; y1=y; out.append(y)
    return out
random.seed(486)
for kind in ('GAUSS','BUTTER','2PHP'):
    gap=0.0; over=-1e9; short=0; floor=-1e9
    for P in (2,3,4,5,7,10,20,60,200,1000):
        r=swak_om(P); z=swak_zero(r); n=swak_rule(Ks[-1],P)+2
        e0=swak_body(kind,P,[1.0,0.0,0.0],0,4)[2:]; e1=swak_body(kind,P,[0.0,1.0,0.0],0,4)[2:]
        det=e0[0]*e1[1]-e0[1]*e1[0]; t0=-z; t1=(1.0-z)*r
        pre=[(t0*e1[1]-t1*e1[0])/det,(e0[0]*t1-e0[1]*t0)/det,0.0]
        d=[abs(v) for v in swak_body(kind,P,pre,0,n+2)[2:]]
        late=swak_body(kind,P,pre,2,n)
        assert max(abs(v) for v in late)==0.0
        S=max(d)
        for m in range(math.ceil(z-1/math.log(r)),n):
            g=math.log(d[m]/S)-(math.log((m-z)/z)+m*math.log(r))
            gap=max(gap,abs(g))
        for K in Ks:
            m=swak_rule(K,P)
            v=math.log(max(d[m:])/S)+K; over=max(over,v)
            if v>0: short+=1
        for _ in range(200):            # prehistories with no design must stay under the envelope
            pre=[random.gauss(0,1) for _ in range(random.randint(2,12))]; t=len(pre); pre.append(0.0)
            d=[abs(v) for v in swak_body(kind,P,pre,0,t+n)[t:]]; S=max(d)
            if S==0.0: continue
            for m in range(swak_need(Ks[0],r,z),n,max(1,n//50)):
                if d[m]>0.0: floor=max(floor,math.log(d[m]/S)-(math.log((m-z)/z)+m*math.log(r)))
    print('  SWAK_%-6s two starts: envelope reached to %.1e e-folds; rule short at %d (K,P); tightest %.4f; random prehistories from K=7 on, against it at most %.4f'%(kind,gap,short,over,floor))

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

# T3: six EMA stages of one pole p = (n-1)/(n+1), stepped together; the output is a weighted
# sum of stages 3 to 6. The state difference of two starts moves as p times a unipotent
# matrix on any input, so the output difference at age j is p^j * q(j), q a polynomial of
# degree at most 5 (2 at a volume factor of 0), and every such q is a seed two starts can
# hold (checked below). Rule m = ceil((K+c)*n/2): -ln(p) = 2*atanh(1/n) > 2/n.
# Bound: for any six ages j0 < ... < j5 and any m beyond them, Lagrange's formula gives
#   |q(m)| p^m <= S * sum_i p^(m-j_i) |L_i(m)|,   S the largest |q(j)| p^j over all ages.
# Search: the q that takes +-p^-j_i in turn at those ages meets it when its own S is 1; an
# exchange moves the ages until it is. The need is then exact, not an estimate.
from fractions import Fraction
def t3_f(nodes,lp,j):                    # p^j q(j) for the alternating q on these ages
    s=0.0
    for i,ji in enumerate(nodes):
        l=math.exp((j-ji)*lp)
        for jh in nodes:
            if jh!=ji: l*=(j-jh)/(ji-jh)
        s+=l if i%2==0 else -l
    return s
def t3_log_bound(nodes,lp,m):
    t=[(m-ji)*lp+sum(math.log((m-jh)/abs(ji-jh)) for jh in nodes if jh!=ji) for ji in nodes]
    return max(t)+math.log(sum(math.exp(x-max(t)) for x in t))
def t3_ages(n,deg=5):
    """The six ages where the worst seed peaks, and that seed's S (1 when the bound is met)."""
    lp=math.log((n-1)/(n+1)); J=int(30/-lp)+8
    if n<40:                             # every age scanned, one age exchanged per pass
        nodes=list(range(deg+1))
        while True:
            v=[t3_f(nodes,lp,j) for j in range(J)]
            jm=max(range(J),key=lambda j:abs(v[j]))
            if abs(v[jm])<=1+1e-12: return nodes,abs(v[jm])
            i=max([h for h in range(deg+1) if nodes[h]<=jm],default=-1)
            pos=v[jm]>0
            if i<0: nodes=[jm]+(nodes[1:] if pos else nodes[:-1])
            elif (i%2==0)==pos: nodes[i]=jm
            elif i==deg: nodes=nodes[1:]+[jm]
            else: nodes[i+1]=jm
    tau=(0,0.24,1.0,2.37,4.58,8.56) if deg==5 else (0,0.61,3.0)
    nodes=[int(t/-lp+0.5) for t in tau]
    for _ in range(30):                  # each lobe's peak found between its sign changes
        cut=[]
        for i in range(deg):
            a,b=nodes[i],nodes[i+1]; sa=t3_f(nodes,lp,a)>0
            while b-a>1:
                c=(a+b)//2
                if (t3_f(nodes,lp,c)>0)==sa: a=c
                else: b=c
            cut.append(b)
        cut.append(J); new=[0]
        for i in range(deg):
            a,b=cut[i],cut[i+1]-1
            while b-a>2:
                c1=a+(b-a)//3; c2=b-(b-a)//3
                if abs(t3_f(nodes,lp,c1))<abs(t3_f(nodes,lp,c2)): a=c1
                else: b=c2
            new.append(max(range(a,b+1),key=lambda j:abs(t3_f(nodes,lp,j))))
        if new==nodes: break
        nodes=new
    S=max(abs(t3_f(nodes,lp,j)) for j in nodes+[J*i//400 for i in range(400)])
    return nodes,S
def t3_need(nodes,lp,K):                 # 1 + the last age whose bound is above e^-K
    lo=nodes[-1]+int(5/-lp)+1            # the bound only falls from here on
    assert t3_log_bound(nodes,lp,lo)>-K
    hi=2*lo
    while t3_log_bound(nodes,lp,hi)>-K: hi*=2
    while hi-lo>1:
        mid=(lo+hi)//2
        if t3_log_bound(nodes,lp,mid)>-K: lo=mid
        else: hi=mid
    return lo+1
T3N=list(range(2,120))+[150,200,300,500,1000,3000,10000,30000,100000]
t3need={}; worstS=0.0
for n in T3N:
    nodes,S=t3_ages(n); worstS=max(worstS,abs(S-1))
    lp=math.log((n-1)/(n+1))
    for K in Ks: t3need[(K,n)]=t3_need(nodes,lp,K)
gain=max((2*t3need[(K,n)]/n-K,K,n) for K in Ks for n in T3N)
print('T3: worst seed against its bound, largest S-1 over the periods %.1e (0: the search meets the bound)'%worstS)
print('  need in bars at n=5 and n=15, K',Ks,':',[t3need[(K,5)] for K in Ks],[t3need[(K,15)] for K in Ks])
print('  worst gain %.3f e-folds of n/2 bars at K=%d, n=%d; at K=10 %.3f, at K=19 %.3f'%(gain+
      tuple(max(2*t3need[(K,n)]/n-K for n in T3N) for K in (10,19))))
for c in (15,16,17,18,19,20,21):
    b=[(K,n) for K in Ks for n in T3N if t3need[(K,n)]>ceil_div((K+c)*n,2)]
    sl=min(2*(ceil_div((K+c)*n,2)-t3need[(K,n)])/n for K in Ks for n in T3N)
    print('  T3 with ceil((K+%d)*n/2): violations at K='%c,sorted(set(k for k,_ in b)),' tightest slack %.3f e-folds'%sl)
b=[(K,n) for K in Ks for n in T3N if t3need[(K,n)]>ceil_div((K+19)*(n+1),2)]
print('  T3 with ceil((K+19)*(n+1)/2): violations at K=',sorted(set(k for k,_ in b)))
# The kernel itself, in exact arithmetic: the state difference that gives the worst q, stepped
# through the six stages with the output weights of t3.c; and the seeds, from the earlier and
# the later start of the same series, span every state difference.
def t3_weights(v):
    c1=-v**3; c2=3*(v*v-c1); c3=-6*v*v-3*(v-c1); c4=1+3*v-c1+3*v*v
    return [0,0,c4,c3,c2,c1]
def t3_step(e,p,x=0):
    e=list(e); e[0]=(1-p)*x+p*e[0]
    for h in range(1,6): e[h]=(1-p)*e[h-1]+p*e[h]
    return e
def t3_solve(A,b):                       # exact Gauss-Jordan; None when singular
    m=len(A); M=[list(r)+[y] for r,y in zip(A,b)]
    for c in range(m):
        piv=next((i for i in range(c,m) if M[i][c]!=0),None)
        if piv is None: return None
        M[c],M[piv]=M[piv],M[c]; M[c]=[a/M[c][c] for a in M[c]]
        for i in range(m):
            if i!=c and M[i][c]!=0: M[i]=[a-M[i][c]*y for a,y in zip(M[i],M[c])]
    return [r[-1] for r in M]
def t3_seed(x,n):                        # the six states at the first output bar, as t3.c seeds them
    p=Fraction(n-1,n+1); e=[Fraction(0)]*6; it=iter(x)
    e[0]=sum(next(it) for _ in range(n))/n
    for s in range(1,6):
        t=e[s-1]
        for _ in range(n-1):
            y=next(it); e[0]=(1-p)*y+p*e[0]
            for h in range(1,s): e[h]=(1-p)*e[h-1]+p*e[h]
            t+=e[s-1]
        e[s]=t/n
    for y in it: e=t3_step(e,p,y)
    return e
for n,v in ((5,Fraction(7,10)),(5,Fraction(1)),(5,Fraction(1,100)),(2,Fraction(7,10)),(15,Fraction(7,10)),(5,Fraction(0))):
    deg=5 if v else 2; dim=deg+1
    nodes,_=t3_ages(n,deg); p=Fraction(n-1,n+1); w=t3_weights(v)
    def q(j): return sum((-1)**i*p**-ji*math.prod(Fraction(j-jh,ji-jh) for jh in nodes if jh!=ji) for i,ji in enumerate(nodes))
    obs=[[None]*dim for _ in range(dim)]
    for u in range(dim):
        e=[Fraction(int(h==u)) for h in range(6)]
        for j in range(dim): obs[j][u]=sum(a*y for a,y in zip(w,e)); e=t3_step(e,p)
    d=t3_solve(obs,[q(j)*p**j for j in range(dim)])+[Fraction(0)]*(6-dim)
    e=d; out=[]
    for j in range(20*n+60): out.append(abs(sum(a*y for a,y in zip(w,e)))); e=t3_step(e,p)
    S=max(out); lp=math.log(p)
    got=[max(j+1 for j,y in enumerate(out) if y>Fraction(math.exp(-K))*S) for K in (7,10,19)]
    want=[t3_need(nodes,lp,K) for K in (7,10,19)]
    rk=[]
    for D in (1,33):                     # start 0 against start D: rank of inputs -> state difference
        T=D+6*(n-1)+1; rows=[]
        for i in range(T):
            x=[Fraction(int(h==i)) for h in range(T)]
            rows.append([a-y for a,y in zip(t3_seed(x,n),t3_seed(x[D:],n))])
        r=0
        for c in range(6):
            piv=next((i for i in range(r,T) if rows[i][c]!=0),None)
            if piv is None: continue
            rows[r],rows[piv]=rows[piv],rows[r]
            for i in range(r+1,T):
                if rows[i][c]!=0: f=rows[i][c]/rows[r][c]; rows[i]=[a-f*y for a,y in zip(rows[i],rows[r])]
            r+=1
        rk.append(r)
    print('  kernel n=%d vFactor=%s: S=%s at ages %s, need at K=7,10,19 %s, bound %s; largest state difference %.1e of S; seeds span %s of 6'
          %(n,v,S,nodes,got,want,max(abs(float(y)) for y in d),rk))
# two-stage lockstep (ADX-like, linear part): P(Bin(m,1/n) <= 1); rule m=(K+c)n
def binom_tail(m,a,r=5):
    s=0.0
    for j in range(r+1):
        s+=math.exp(math.lgamma(m+1)-math.lgamma(j+1)-math.lgamma(m-j+1)+j*math.log(a)+(m-j)*math.log1p(-a))
    return s
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
