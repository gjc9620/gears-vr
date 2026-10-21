"""Interpreter discovery only: never executes game startup actions."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).with_name('Start-DebugGame.ps1')
ROOT = SCRIPT.parents[2]


@unittest.skipUnless(os.name == 'nt', 'Windows PowerShell locator')
class PythonDiscoveryTests(unittest.TestCase):
    def run_ps(self, arguments, env):
        return subprocess.run(['powershell', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', *arguments],
            env=env, capture_output=True, text=True, encoding='utf-8', timeout=20,
            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))

    def check_only(self, preferred):
        env = dict(os.environ)
        if preferred is None:
            env.pop('GEARSVR_PYTHON', None)
        else:
            env['GEARSVR_PYTHON'] = preferred
        return self.run_ps(['-File', str(SCRIPT), '-PythonCheckOnly'], env)

    def test_explicit_python_and_quoted_expanded_path(self):
        for preferred in (sys.executable, '"'+sys.executable+'"'):
            with self.subTest(preferred=preferred):
                result = self.check_only(preferred)
                self.assertEqual(result.returncode, 0, result.stderr)
                value = json.loads(result.stdout)
                self.assertTrue(value['check_only'])
                self.assertEqual(os.path.normcase(value['python']), os.path.normcase(sys.executable))
        env = dict(os.environ, GEARSVR_TEST_PYTHON=sys.executable, GEARSVR_PYTHON='%GEARSVR_TEST_PYTHON%')
        result = self.run_ps(['-File', str(SCRIPT), '-PythonCheckOnly'], env)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(os.path.normcase(json.loads(result.stdout)['python']), os.path.normcase(sys.executable))

    def test_invalid_explicit_selection_never_falls_back(self):
        with tempfile.TemporaryDirectory(prefix='Python locator with spaces ') as folder:
            invalid = Path(folder)/'python.exe'
            for exists in (False, True):
                if exists:
                    invalid.write_bytes(b'')  # Invalid stub, never a copied runtime.
                result = self.check_only(str(invalid))
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('GEARSVR_PYTHON is not a usable Python 3', result.stderr)
                self.assertNotIn('"check_only":true', result.stdout)

    def test_packaged_then_development_candidate_order_without_copying_runtime(self):
        # Parse/load only the function definition. Test-Path records candidates
        # and returns false, so no process is constructed and no game code runs.
        command = r'''
$ErrorActionPreference='Stop'
$tokens=$null;$parseErrors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($env:GEARSVR_TEST_SOURCE,[ref]$tokens,[ref]$parseErrors)
if($parseErrors.Count){throw 'PowerShell parse error'}
$definition=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Find-GearsVrPython'},$true)
. ([scriptblock]::Create($definition.Extent.Text))
$script:seen=@()
function Test-Path {param([string]$LiteralPath,[string]$PathType) $script:seen+=$LiteralPath;return $false}
try { Find-GearsVrPython $env:GEARSVR_TEST_ROOT | Out-Null } catch {}
ConvertTo-Json -InputObject @($script:seen) -Compress
'''
        portable_root = r'C:\Portable VR Kit'
        env = dict(os.environ, GEARSVR_TEST_SOURCE=str(SCRIPT), GEARSVR_TEST_ROOT=portable_root)
        env.pop('GEARSVR_PYTHON', None)
        result = self.run_ps(['-Command', command], env)
        self.assertEqual(result.returncode, 0, result.stderr)
        expected = [str(Path(portable_root)/'runtime/python/python.exe'),
                    str(Path(os.environ['USERPROFILE'])/'.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe')]
        self.assertEqual([os.path.normpath(x) for x in json.loads(result.stdout)], expected)
        env['GEARSVR_PYTHON'] = r'"C:\Explicit Python With Spaces\python.exe"'
        result = self.run_ps(['-Command', command], env)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout), [r'C:\Explicit Python With Spaces\python.exe'])

    def test_default_installed_candidate_probe_is_read_only(self):
        result = self.check_only(None)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(json.loads(result.stdout)['check_only'])


if __name__ == '__main__':
    unittest.main()
