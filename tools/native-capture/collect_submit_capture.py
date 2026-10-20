"""Copy stopped evidence and count records with bounded memory."""
import argparse
import collections
import csv
import json
import pathlib
import shutil
import time


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pid',type=int,required=True)
    p.add_argument('--build-dir',type=pathlib.Path,required=True)
    p.add_argument('--output',type=pathlib.Path,required=True)
    p.add_argument('--timeout',type=float,default=30)
    a=p.parse_args();deadline=time.monotonic()+a.timeout
    while True:
        try:status=(a.build_dir/'capture-status.txt').read_text()
        except (PermissionError,FileNotFoundError):status=''
        if f'capturing pid={a.pid} ' in status and 'stopped disable_status=' in status:break
        if time.monotonic()>=deadline:raise RuntimeError('Stop not yet confirmed; inspect the same live process before retrying')
        time.sleep(.5)
    if 'stopped disable_status=0 active=0' not in status:raise RuntimeError('Capture did not stop cleanly')
    a.output.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(a.build_dir/'capture.csv',a.output.with_suffix('.csv'))
    a.output.with_suffix('.status.txt').write_text(status)
    counts=collections.Counter()
    with a.output.with_suffix('.csv').open(newline='') as stream:
        for row in csv.DictReader(stream):counts[row['kind']]+=1
    report={'pid':a.pid,'format':'streamed_kind_counts','samples':sum(counts.values()),'kinds':dict(counts),
        'limits':'Compact collection only; run the submission analyzer and independent entry verification.'}
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
