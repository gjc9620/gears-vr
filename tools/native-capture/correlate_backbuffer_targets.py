"""Stream stopped capture evidence and match observed RTV resources to Present buffers."""
import argparse
import collections
import csv
import json
import pathlib
import re


def correlate(csv_path, status_path):
    status = status_path.read_text()
    if 'stopped disable_status=0 active=0' not in status:
        raise ValueError('Capture must be stopped cleanly before correlation')
    buffers = collections.Counter()
    records = 0
    with csv_path.open(newline='') as stream:
        for row in csv.DictReader(stream):
            records += 1
            if row['kind'] not in ('3', '4') or not (int(row['valid']) & 256):
                continue
            key = (int(row['gpu_buffer'], 16), row['gpu_window'],
                   int(row['width']), int(row['height']), int(row['format']))
            buffers[key] += 1
    identities = {key[0] for key in buffers}
    targets = []
    pattern = re.compile(r'^gpu_trial_target viewport=(\d+) targets=(\d+) resource=([0-9a-fA-F]+) width=(\d+) height=(\d+) format=(\d+) mip=(\d+) calls=(\d+)$')
    for line in status.splitlines():
        match = pattern.fullmatch(line)
        if not match:
            continue
        viewport, count, resource, width, height, fmt, mip, calls = match.groups()
        resource_id = int(resource, 16)
        targets.append(dict(viewport=int(viewport), targets=int(count), resource=hex(resource_id),
                            width=int(width), height=int(height), format=int(fmt), mip=int(mip),
                            calls=int(calls), matches_present_buffer=resource_id in identities))
    return dict(records=records,
                present_buffers=[dict(resource=hex(k[0]), window=k[1], width=k[2], height=k[3],
                                      format=k[4], records=v) for k, v in sorted(buffers.items())],
                trial_targets=targets,
                limits='Identity correlation within one capture, not GPU completion or pixel correctness. Only first RTV and bounded tracked direct draws are covered.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--csv', type=pathlib.Path, required=True)
    parser.add_argument('--status', type=pathlib.Path, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    result = correlate(args.csv, args.status)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
