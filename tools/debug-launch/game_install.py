"""Read-only validation of the supported Reloaded installation."""
import argparse
import hashlib
import json
import os
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
DEFAULT_GAME = ROOT / 'Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe'
CONFIG_HASHES = frozenset((
    '6cf7a0c3a59d85ae6fedf5fc49d8bb406edda9adda7429448d27510e729a8f16',
    '799723628b465552bc2f1847f91370c10c35e98dbd9460695e5f369956da70de'))
STEAM_APP_ID = '2523720'


def launch_environment(environ=None):
    """Identify this supported title to the real Steam client in the child only.

    Without this context the original Steam API restarts via steam://run and
    exits the process whose identity the reservation owns. Steam still performs
    its ordinary signed-in account and license checks.
    """
    inherited = os.environ if environ is None else environ
    # Windows environment names are case-insensitive. Remove inherited spellings
    # before adding the title's values so its environment block has no duplicates.
    child = {key:value for key,value in inherited.items()
             if key.casefold() not in ('steamappid', 'steamgameid')}
    child.update(SteamAppId=STEAM_APP_ID, SteamGameId=STEAM_APP_ID)
    return child


def validate_game(path=None, root=ROOT, expected=None):
    path = pathlib.Path(path) if path is not None else DEFAULT_GAME
    exe = (root / path).resolve() if not path.is_absolute() else path.resolve()
    if exe.name.casefold() != 'gowde-steam.exe' or exe.parent.name.casefold() != 'binaries_x64' or not exe.is_file():
        raise RuntimeError('Select the supported Binaries_x64/GOWDE-Steam.exe file')
    profile = json.loads((root / 'tools/package-inspector/reloaded-profile.json').read_text())
    supported = profile['sha256'].lower()
    if expected is not None and expected.lower() != supported:
        raise RuntimeError('Resolver profile differs from manifest')
    if hashlib.sha256(exe.read_bytes()).hexdigest() != supported:
        raise RuntimeError('Unvalidated game build: ' + str(exe))
    config = exe.parent.parent / 'WarGame/Config/Steam/Cooked/Coalesced.ini'
    if not config.is_file() or hashlib.sha256(config.read_bytes()).hexdigest() not in CONFIG_HASHES:
        raise RuntimeError('Unsupported installation directory/config: ' + str(config))
    return exe


def debug_command(exe, root=ROOT, *, foreground=False, experimental_preserve_upscaler=False):
    return ['powershell', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', '-File',
            str(root / 'tools/debug-launch/Start-DebugGame.ps1'), '-DisableBackgroundInput',
            '-GamePath', str(exe)] + (['-Foreground'] if foreground else []) + (
                ['-ExperimentalPreserveUpscaler'] if experimental_preserve_upscaler else [])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-path', type=pathlib.Path)
    args = parser.parse_args()
    print(validate_game(args.game_path))
