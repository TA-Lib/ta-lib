# Rules at the two levels (PREC_4: K = 10, X = 4; PREC_8: K = 19, X = 8) against the measured
# need. KAMA, FRAMA and VIDYA are compared on the trend and random-walk series only.
# usage: levels.py <probe.tsv> <repo root>
import csv, math, sys, collections, yaml
rows=[r for r in csv.DictReader(open(sys.argv[1]),delimiter='\t') if r['cfgkind'] in('0','1')]
ROOT=sys.argv[2]; INDEX_MAX=100000000
G=collections.defaultdict(list)
for r in rows: G[(r['func'],r['cfgkind'])].append(r)
cd=lambda a,b:-(-a//b)
def rules(K):
    X={10:4,19:8}[K]; isq=math.isqrt
    E=lambda n:0 if n<=1 else cd(K*(n+1),2); Wd=lambda n:0 if n<=1 else K*n; H=HT(K)
    tp=lambda p:p['TimePeriod']
    return {'EMA':lambda p:E(tp(p)),'RMA':lambda p:Wd(tp(p)),'ATR':lambda p:Wd(tp(p)),'NATR':lambda p:Wd(tp(p)),'RSI':lambda p:Wd(tp(p)),'CMO':lambda p:Wd(tp(p)),
     'DX':lambda p:Wd(tp(p)),'PLUS_DI':lambda p:Wd(tp(p)),'MINUS_DI':lambda p:Wd(tp(p)),'PLUS_DM':lambda p:Wd(tp(p)),'MINUS_DM':lambda p:Wd(tp(p)),'RVI':lambda p:Wd(tp(p)),
     'ADX':lambda p:(K+6)*tp(p),'T3':lambda p:cd((K+20)*(tp(p)+1),2),'HA':lambda p:2*K,'SWAK_HP':lambda p:cd(K*tp(p),6),
     'SWAK_GAUSS':lambda p:cd((K+5)*(tp(p)+2),9),'SWAK_BUTTER':lambda p:cd((K+5)*(tp(p)+2),9),'SWAK_2PHP':lambda p:cd((K+5)*(tp(p)+2),9),
     'SWAK_BP':lambda p:math.ceil(((K+1)*tp(p))/(6.0*p.get('Delta',0.1))),'KAMA':lambda p:0 if tp(p)<=1 else 25*X*isq(tp(p)),'FRAMA':lambda p:80*X,
     'MAMA':lambda p:H+math.ceil((2*K)/max(p.get('FastLimit',0.5),p.get('SlowLimit',0.05))),
     'VIDYA':lambda p:0 if tp(p)<=1 else min(2*X*(tp(p)+1)*isq(p['CMOPeriod']),INDEX_MAX),'MCGD':lambda p:5*X*tp(p),
     'STC':lambda p:2*K+3*(max(p['FastPeriod'],p['SlowPeriod'])+1)+E(max(p['FastPeriod'],p['SlowPeriod'])),
     'HT_DCPERIOD':lambda p:H,'HT_DCPHASE':lambda p:H,'HT_PHASOR':lambda p:H,'HT_SINE':lambda p:H,'HT_TRENDLINE':lambda p:H,'HT_TRENDMODE':lambda p:H}
HT=lambda K:80+50*{10:4,19:8}[K]
def defaults(fn):
    y=yaml.safe_load(open('%s/ta_codegen/input/%s/%s.yaml'%(ROOT,fn.lower(),fn.lower())))
    return {o['name'][5:]:o['default'] for o in (y.get('optional_inputs') or [])}
TYPICAL={'KAMA','FRAMA','VIDYA'}   # sized on the trend and random-walk series only
def worst(rs,col):
    v=[int(r[col]) for r in rs if float(r['S'])>0 and not (r['func'] in TYPICAL and r['shape']=='zz')]
    return 0 if not v else (-1 if any(x<0 for x in v) else max(v))
R4,R8=rules(10),rules(19)
print('%-12s %-3s | PREC_4: rule  1e-4  leg(e^-7) sig4 | PREC_8: rule  1e-8  leg(e^-16) sig8 | flags'%('id','cfg'))
for fn in R4:
    d=defaults(fn)
    for kind in('0','1'):
        rs=G.get((fn,kind))
        if not rs: continue
        p=dict(d)
        if kind=='1':
            for k in p:
                if 'Period' in k and isinstance(p[k],int): p[k]*=3
        r4,r8=R4[fn](p),R8[fn](p)
        a10,a7,a19,a16,c4,c8=[worst(rs,c) for c in('A10','A7','A19','A16','C4','C8')]
        fl=[]
        if a7<0 or a7>r4: fl.append('LEG4')
        if a16<0 or a16>r8: fl.append('LEG8')
        if a10<0 or a10>r4: fl.append('k4')
        if a19<0 or a19>r8: fl.append('k8')
        if c4<0 or c4>r4: fl.append('sig4')
        if c8<0 or c8>r8: fl.append('sig8')
        print('%-12s %-3s | %12d %5d %9d %5d | %12d %5d %10d %5d | %s'%(fn,'def' if kind=='0' else 'x3',r4,a10,a7,c4,r8,a19,a16,c8,' '.join(fl)))
