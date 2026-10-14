"""Owned target-prime ABI and exact pending-status checks."""
import struct


def owner_argument(pid, creation_time):
    if not isinstance(pid,int) or not 0<pid<=0xffffffff or not isinstance(creation_time,int) or not 0<creation_time<=0xffffffffffffffff:
        raise ValueError('Valid owner PID and FILETIME creation identity required')
    return struct.pack('<IIIIQ',24,1,pid,0,creation_time)


def pending_prime(status,pid,owner_pid=None,owner_creation_time=None,require_owned=False):
    lines=status.splitlines()
    if any(line.startswith(('stopped','handed_off','prime_setup_failed')) for line in lines):
        return False
    captures=[line for line in lines if line.startswith('capturing ')]
    if len(captures)!=1:return False
    fields=dict(token.split('=',1) for token in captures[0].split()[1:] if '=' in token)
    if fields.get('pid')!=str(pid):return False
    if fields.get('lifetime')=='owner':
        return (owner_pid is not None and owner_creation_time is not None and
                fields.get('owner_pid')==str(owner_pid) and
                fields.get('owner_creation_time')==str(owner_creation_time))
    return not require_owned and fields.get('seconds')=='180' and 'lifetime' not in fields


def graphics_prime_count(status, required=False, menu_lock=False):
    """Require the requested policy, without treating arbitrary native hooks as safe."""
    if menu_lock and not required:raise RuntimeError('Menu lock requires startup graphics protection')
    captures=[line for line in status.splitlines() if line.startswith('capturing ')]
    if len(captures)!=1:raise RuntimeError('Expected one target-prime capture record')
    fields=dict(token.split('=',1) for token in captures[0].split()[1:] if '=' in token)
    expected='4' if menu_lock else '3' if required else '0'
    if (fields.get('native_hooks_enabled')!=expected or
            fields.get('graphics_hooks_enabled','0')!=expected or fields.get('scene_hooks_enabled','0')!='0'):
        raise RuntimeError('Target-prime graphics policy differs from requested manifest')
    if required and fields.get('lifetime')!='owner':raise RuntimeError('Startup graphics lock requires owned lifetime')
    return int(expected)
