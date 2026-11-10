"""Runtime-only idle check; isolated encoding never owns the game window."""
import os
import cover_launch_preflight as preflight


def idle():
    return not preflight.blocking_processes(preflight.process_inventory(), os.getpid())
