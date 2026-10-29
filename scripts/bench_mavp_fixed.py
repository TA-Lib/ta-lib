"""Matched-process A/B for MAVP's normalized equal-bound shortcut.

Compile the same harness against each already-built library. Controls exercise
unchanged MA and variable-bound MAVP. Ratios are paired baseline/candidate;
process envelopes are descriptive, not confidence intervals.
"""
import argparse
import gzip
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import sys
import statistics
import subprocess
import time


def sign_pvalue(ratios):
    signs = [r > 1 for r in ratios if r != 1]
    n = len(signs)
    if not n: return 1.0
    k = min(sum(signs), n-sum(signs))
    return min(1., 2*sum(math.comb(n, i) for i in range(k+1))/2**n)



def source_record(root):
    root = root.resolve()
    digest = hashlib.sha256()
    for name in subprocess.check_output(['git','-C',str(root),'ls-files','-z']).decode().split('\0'):
        if not name: continue
        path = root/name
        if name.startswith('ta_codegen/output/rust/') or name == 'ta_codegen/input/mavp/mavp.c':
            if path.suffix in ('.rs','.toml','.lock','.c'):
                digest.update(name.encode()); digest.update(path.read_bytes())
    return {'commit':subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip(),
            'source_sha256':digest.hexdigest(),
            'dirty':bool(subprocess.check_output(['git','-C',str(root),'status','--porcelain']))}


def build_binaries(args):
    records = {}
    for side in ('baseline','candidate'):
        root = getattr(args, side+'_source').resolve()
        target = (args.output/('target-'+side)).resolve()
        binary = getattr(args,side).absolute(); binary.parent.mkdir(parents=True,exist_ok=True)
        commands = [
            ['cargo','build','--release','--locked','-p','ta-lib','--manifest-path',str(root/'ta_codegen/output/rust/Cargo.toml'),'--target-dir',str(target)],
            ['rustc','--edition=2021','-O','-C','lto=thin','-C','codegen-units=1',str(Path(__file__).with_suffix('.rs').resolve()),
             '-L','dependency='+str(target/'release/deps'),'--extern','ta_lib='+str(target/'release/libta_lib.rlib'),'-o',str(binary)],
        ]
        with (args.output/(side+'-build.log')).open('w') as log:
            for command in commands:
                log.write(json.dumps(command)+'\n'); log.flush()
                subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,cwd=root)
        records[side] = {**source_record(root),'commands':commands,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest()}
    records['harness_sha256'] = hashlib.sha256(Path(__file__).with_suffix('.rs').read_bytes()).hexdigest()
    records['rustc'] = subprocess.check_output(['rustc','--version','--verbose'],text=True)
    records['cc'] = subprocess.check_output(['cc','--version'],text=True)
    (args.output/'build-provenance.json').write_text(json.dumps(records,indent=2)+'\n')


def cpu_name():
    path = Path('/proc/cpuinfo')
    if path.exists(): return next((line.split(':',1)[1].strip() for line in path.read_text().splitlines() if line.startswith('model name')), platform.processor())
    if sys.platform == 'darwin': return subprocess.check_output(['sysctl','-n','machdep.cpu.brand_string'],text=True).strip()
    return platform.processor()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for side in ('baseline','candidate'):
        p.add_argument('--'+side, type=Path, required=True)
        p.add_argument('--'+side+'-source', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--build-only', action='store_true')
    p.add_argument('--pairs', type=int, default=12)
    p.add_argument('--repeats', type=int, default=31)
    args = p.parse_args()
    if args.pairs < 12 or args.pairs % 2 or args.repeats < 5: p.error('even >=12 pairs and >=5 samples required')
    args.output.mkdir(parents=True, exist_ok=True)
    if args.build_only:
        build_binaries(args); return
    provenance = json.loads((args.output/'build-provenance.json').read_text())
    for side in ('baseline','candidate'):
        if source_record(getattr(args,side+'_source')) != {k:provenance[side][k] for k in ('commit','source_sha256','dirty')}:
            raise RuntimeError('source changed after build')
        if hashlib.sha256(getattr(args,side).read_bytes()).hexdigest() != provenance[side]['binary_sha256']:
            raise RuntimeError('binary changed after build')
    affinity = sorted(os.sched_getaffinity(0)) if hasattr(os,'sched_getaffinity') else None
    if affinity: os.sched_setaffinity(0, affinity[:1])
    runs = {side: [] for side in ('baseline','candidate')}; order = []
    hashes = {}; capture_bytes = {}
    for side in ('baseline','candidate'):
        capture = args.output/(side+'.bin')
        subprocess.run([str(getattr(args,side).absolute()),str(args.repeats),str(capture),'--capture-only'],check=True)
        content = capture.read_bytes()
        hashes[side] = hashlib.sha256(content).hexdigest(); capture_bytes[side] = len(content)
    if hashes['baseline'] != hashes['candidate'] or not capture_bytes['baseline']:
        raise RuntimeError('full output/range/padding capture differs before timing')
    for pair in range(args.pairs):
        for side in (('baseline','candidate') if pair % 2 == 0 else ('candidate','baseline')):
            label = f'{side}-{pair+1}'; order.append(label)
            command = [str(getattr(args,side).absolute()), str(args.repeats)]
            print(time.strftime('%H:%M:%S'), label, flush=True)
            result = subprocess.run(command, check=True, capture_output=True, text=True)
            (args.output/(label+'.jsonl')).write_text(result.stdout)
            data = {entry['case']:entry for line in result.stdout.splitlines() if (entry:=json.loads(line))}
            if len(data) != 24: raise RuntimeError('case inventory must contain all24 cases')
            for entry in data.values():
                if len(entry['samples_ns']) != args.repeats or any(not math.isfinite(x) or x <= 0 for x in entry['samples_ns']):
                    raise RuntimeError('invalid samples')
            runs[side].append(data)

    rows = {}
    for name in runs['baseline'][0]:
        old = [statistics.median(r[name]['samples_ns']) for r in runs['baseline']]
        new = [statistics.median(r[name]['samples_ns']) for r in runs['candidate']]
        ratios = [a/b for a,b in zip(old,new)]
        pvalue = sign_pvalue(ratios)
        rows[name] = {'baseline_process_medians_ns':old,'candidate_process_medians_ns':new,
                      'baseline_median_ns':statistics.median(old),'candidate_median_ns':statistics.median(new),
                      'paired_ratios':ratios,'median_paired_ratio':statistics.median(ratios),
                      'ratio_envelope':[min(old)/max(new),max(old)/min(new)],
                      'two_sided_sign_pvalue':pvalue, 'bonferroni_pvalue':min(1., pvalue*24)}
    metadata = {'cpu':cpu_name(),'build_provenance':provenance,'platform':platform.platform(),'python':platform.python_version(),'pairs':args.pairs,
                'samples':args.repeats,'order':order,'affinity':affinity[:1] if affinity else None,
                'capture_sha256':hashes,'capture_bytes':capture_bytes,'results':rows}
    (args.output/'summary.json').write_text(json.dumps(metadata,indent=2)+'\n')
    for side in hashes:
        path = args.output/(side+'.bin')
        with gzip.open(str(path)+'.gz','wb') as dest: dest.write(path.read_bytes())
        path.unlink()
    lines = ['# MAVP normalized equal-bound A/B','',
             f'Same runner/toolchain/release flags; affinity {affinity[:1] if affinity else None};{args.pairs} alternating fresh-process pairs;{args.repeats} samples/process.',
             'Full output/range/padding captures are byte-identical. Controls: unchanged MA and variable-bound MAVP.',
             'Envelopes are descriptive process ranges, not confidence intervals. Exact paired sign tests use process pairs, with Bonferroni adjustment across24 cases; shared-host dependencies remain a limitation, so p-values alone do not establish independent evidence or universal speedups.','',
             '| Case | Baseline ns | Candidate ns | Paired ratio | Envelope | Adjusted p |',
             '| --- | ---: | ---: | ---: | --- | ---: |']
    for name,row in rows.items():
        lo,hi=row['ratio_envelope']
        lines.append(f"| {name} | {row['baseline_median_ns']:.1f} | {row['candidate_median_ns']:.1f} | {row['median_paired_ratio']:.3f} | {lo:.3f}–{hi:.3f} | {row['bonferroni_pvalue']:.5f} |")
    (args.output/'comparison.md').write_text('\n'.join(lines)+'\n')
    print('\n'.join(lines))


if __name__ == '__main__': main()
