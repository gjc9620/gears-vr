"""Startup confirmation is a stereo receipt plus live exact process identities."""


def confirmation(state, report, identity):
    state, report = state or {}, report or {}
    game, xr = report.get('game_identity'), report.get('xr_identity')
    owner = state.get('owner_identity')
    def alive(value):
        return bool(isinstance(value, dict) and type(value.get('pid')) is int
                    and identity(value['pid']) == value)
    same_game = bool(game and game == state.get('game_identity'))
    live = same_game and all(alive(value) for value in (owner, game, xr))
    stereo = bool(report.get('startup_timings', {}).get('first_stereo_submission'))
    confirmed = bool(live and stereo and report.get('native_started') is True
                     and report.get('phase') == 'running')
    return dict(startup_confirmed=confirmed, live_processes_matched=bool(live),
                actual_stereo_started=stereo,
                session_phase=report.get('phase'),
                readiness='stereo_started' if confirmed else 'not_confirmed')
