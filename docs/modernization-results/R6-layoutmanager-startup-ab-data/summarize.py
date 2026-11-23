import argparse
from collections import defaultdict
from itertools import product
import json
from pathlib import Path
import random
from statistics import median, quantiles

parser=argparse.ArgumentParser()
parser.add_argument('series', nargs='+', type=Path)
args=parser.parse_args()

def load(path):
    pairs=defaultdict(dict)
    for line in path.read_text().splitlines():
        row=json.loads(line)
        assert row['ksycoca_stable'] and row['clean_exit'], row
        assert len(row['docks'])==2 and all(d['applets']==6 and d['retries']==0 for d in row['docks']), row
        assert {d['enumerations'] for d in row['docks']}=={13 if row['variant']=='A' else 1}, row
        if row['pair']>=0:
            assert row['variant'] not in pairs[row['pair']]
            pairs[row['pair']][row['variant']]=row
    assert pairs and all(set(pair)=={'A','B'} for pair in pairs.values())
    return list(pairs.values())

def get(row, phase):
    if phase=='launch_to_restore_ns':
        return row[phase]/1e6
    fields=('enumerate_ns','order_ns') if phase=='prep_ns' else (phase,)
    return sum(dock[field] for dock,field in product(row['docks'],fields))/1e6

def summarize(name,pairs):
    print(name, len(pairs),'pairs')
    for phase in ('enumerate_ns','order_ns','prep_ns','create_ns','restore_ns','launch_to_restore_ns'):
        a=[get(p['A'],phase) for p in pairs]
        b=[get(p['B'],phase) for p in pairs]
        delta=[y-x for x,y in zip(a,b)]
        rng=random.Random(420)
        ci=quantiles([median(rng.choices(delta,k=len(delta))) for _ in range(20000)],n=40)
        print(f'{phase:21s} {median(a):9.4f} {median(b):9.4f} {median(delta):+9.4f} '
              f'[{ci[0]:+9.4f},{ci[-1]:+9.4f}] {sum(d<0 for d in delta)}/{len(delta)} '
              f'[{min(delta):+9.4f},{max(delta):+9.4f}]')

all_pairs=[]
for path in args.series:
    pairs=load(path)
    summarize(path.parent.name,pairs)
    all_pairs.extend(pairs)
if len(args.series)>1:
    summarize('pooled',all_pairs)
