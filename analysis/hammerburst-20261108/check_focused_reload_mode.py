"""Mode selection only: no game, preparations or input."""
import json
from pathlib import Path
import run_isolated_empty as runner
assert runner.reload_test_mode([])['empty_only']=='1'
assert runner.reload_test_mode(['--prepare-then-wait'])['empty_only']=='1'
focused=runner.reload_test_mode(['--prepare-then-wait','--focused-loaded-replacement'])
assert focused['empty_only']=='0' and 'empty reload still pending' in focused['scope']
source=Path(runner.__file__).read_text()
assert "GEARSVR_HAMMERBURST_EMPTY_RELOAD_ONLY=test_mode['empty_only']" in source
compile(source,runner.__file__,'exec')
Path(__file__).with_name('focused-reload-mode-checks.json').write_text(json.dumps(dict(passed=True,checks=4,
 scope='Explicit separate loaded replacement diagnostic; default full empty flow unchanged; no runtime/input'),indent=2))
print('4 focused/default mode and environment wiring checks passed; no runtime')
