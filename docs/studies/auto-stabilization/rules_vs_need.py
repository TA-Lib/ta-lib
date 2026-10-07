# The count the library discards at the two levels (PREC_4: K = 10, X = 4; PREC_8: K = 19, X = 8)
# against the measured need, for every function that owns an unstable id. The count is the
# probe's auto4 and auto8 columns, so it includes what the function inherits (STC's EMA).
# KAMA, FRAMA and VIDYA are compared on the trend and random-walk series only.
# usage: rules_vs_need.py <probe.tsv>
import csv, sys, collections
OWNS_ID=0x08000000   # TA_FUNC_FLG_UNST_PER
rows=[r for r in csv.DictReader(open(sys.argv[1]),delimiter='\t')
      if r['cfgkind'] in('0','1') and int(r['flags'],16)&OWNS_ID]
G=collections.defaultdict(list)
for r in rows: G[(r['func'],r['cfg'])].append(r)
TYPICAL={'KAMA','FRAMA','VIDYA'}   # sized on the trend and random-walk series only
def worst(rs,col):
    v=[int(r[col]) for r in rs if float(r['S'])>0 and not (r['func'] in TYPICAL and r['shape']=='zz')]
    return 0 if not v else (-1 if any(x<0 for x in v) else max(v))
print('%-12s %-22s | PREC_4: rule  1e-4  leg(e^-7) sig4 | PREC_8: rule  1e-8  leg(e^-16) sig8 | flags'%('id','cfg'))
for (fn,cfg),rs in G.items():
    r4,r8=int(rs[0]['auto4']),int(rs[0]['auto8'])
    a10,a7,a19,a16,c4,c8=[worst(rs,c) for c in('A10','A7','A19','A16','C4','C8')]
    fl=[]
    if a7<0 or a7>r4: fl.append('LEG4')
    if a16<0 or a16>r8: fl.append('LEG8')
    if a10<0 or a10>r4: fl.append('k4')
    if a19<0 or a19>r8: fl.append('k8')
    if c4<0 or c4>r4: fl.append('sig4')
    if c8<0 or c8>r8: fl.append('sig8')
    print('%-12s %-22s | %12d %5d %9d %5d | %12d %5d %10d %5d | %s'%(fn,cfg[:22],r4,a10,a7,c4,r8,a19,a16,c8,' '.join(fl)))
