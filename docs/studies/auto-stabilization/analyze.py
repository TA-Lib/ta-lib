import csv, sys, collections, math
rows=list(csv.DictReader(open(sys.argv[1]),delimiter='\t'))
rows=[r for r in rows if not r['shape'].startswith('#')]
G=collections.defaultdict(list)
for r in rows:
    G[(r['func'],r['cfgkind'],r['cfg'],r['out'],r['outname'])].append(r)
def f(x): return float(x)
out={}
for k,rs in G.items():
    Smax=max(f(r['S']) for r in rs)
    Rmax=max(f(r['R']) for r in rs)
    rel=max((f(r['S'])/f(r['R']) if f(r['R'])>0 else (0 if f(r['S'])==0 else float('inf'))) for r in rs)
    def agg(col):
        v=[int(r[col]) for r in rs if f(r['S'])>0]
        if not v: return 0
        return -1 if any(x<0 for x in v) else max(v)
    A={c:agg(c) for c in ['A7','A10','A14','A16','A19','A21','A28','B10','B19','Z','C4','C8']}
    tail=max(f(r['tail']) for r in rs)
    # per shape A14
    per={}
    for sh in ['rw','zz','tr']:
        v=[int(r['A10']) for r in rs if r['shape']==sh and f(r['S'])>0]
        per[sh]=( -1 if any(x<0 for x in v) else max(v)) if v else 0
    if Smax==0: cls='EXACT'
    elif rel<1e-9: cls='ROUND'
    elif tail>1e-3 or A['A7']<0: cls='NEVER'
    elif A['A14']<0: cls='SLOW'
    else: cls='CONV'
    out[k]=dict(cls=cls,S=Smax,rel=rel,tail=tail,lb=rs[0]['lookback'],per=per,**A)
import json
json.dump({'|'.join(k):v for k,v in out.items()},open(sys.argv[2],'w'))
mode=sys.argv[3] if len(sys.argv)>3 else 'summary'
if mode=='summary':
    # per function, defaults only: worst class across outputs
    order={'EXACT':0,'ROUND':1,'CONV':2,'SLOW':3,'NEVER':4}
    byf=collections.defaultdict(lambda:'EXACT')
    for k,v in out.items():
        if k[1]!='0': continue
        if order[v['cls']]>order[byf[k[0]]]: byf[k[0]]=v['cls']
    c=collections.Counter(byf.values()); print(c)
    for cl in ['ROUND','CONV','SLOW','NEVER']:
        print(cl, ' '.join(sorted(n for n,v in byf.items() if v==cl)))
    print('EXACT non-cdl', ' '.join(sorted(n for n,v in byf.items() if v=='EXACT' and not n.startswith('CDL'))))
    print('EXACT cdl count', sum(1 for n,v in byf.items() if v=='EXACT' and n.startswith('CDL')))
else:
    print('%-13s %-28s %-14s %5s %-6s %9s %8s | %5s %5s %5s %5s %5s %5s %5s | %5s %5s | %5s %5s | %6s %8s | rw/zz/tr A10'%('func','cfg','out','lb','cls','S','S/R','A7','A10','A14','A16','A19','A21','A28','B10','B19','C4','C8','Z','tail'))
    for k in sorted(out):
        v=out[k]
        if mode=='nonexact' and v['cls'] in('EXACT',): continue
        if mode.startswith('f=') and k[0] not in mode[2:].split(','): continue
        print('%-13s %-28s %-14s %5s %-6s %9.3g %8.1e | %5d %5d %5d %5d %5d %5d %5d | %5d %5d | %5d %5d | %6d %8.1e | %d/%d/%d'%(k[0],k[2][:28],k[4][:14],v['lb'],v['cls'],v['S'],v['rel'],v['A7'],v['A10'],v['A14'],v['A16'],v['A19'],v['A21'],v['A28'],v['B10'],v['B19'],v['C4'],v['C8'],v['Z'],v['tail'],v['per']['rw'],v['per']['zz'],v['per']['tr']))
