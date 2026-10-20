"""Validate the combined unsubmitted view and independent-state lifecycle."""
import argparse
import json
import pathlib
from analyze_constructor_shadow import analyze as constructor
from analyze_shadow_state import analyze as state


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('capture','restored','output'):
        p.add_argument('--'+name,type=pathlib.Path,required=True)
    a=p.parse_args()
    status=a.capture.with_suffix('.status.txt').read_text()
    restored=json.loads(a.restored.read_text())
    view=constructor(status,restored,independent=True)
    history=state(status,restored,a.capture.with_suffix('.csv'))
    report={'passed':view['passed'] and history['passed'],'view':view,'state':history,
            'limits':['One unsubmitted view with distinct state; no rendered independent eye history.',
                      'View cleanup and state deleting-wrapper completion observed, not GPU completion.',
                      'Release ordering is enforced by the diagnostic branch; no timed GPU queue ownership proof.']}
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))
    return int(not report['passed'])


if __name__=='__main__':
    raise SystemExit(main())
