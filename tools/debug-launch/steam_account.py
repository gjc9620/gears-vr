"""Read Steam account selection metadata without changing client login state."""
import pathlib
import re
import winreg

import instance_policy


def registry_state():
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam\ActiveProcess') as key:
        account, kind = winreg.QueryValueEx(key, 'ActiveUser')
        pid, pid_kind = winreg.QueryValueEx(key, 'pid')
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam') as key:
        folder, folder_kind = winreg.QueryValueEx(key, 'SteamPath')
    if (kind != winreg.REG_DWORD or type(account) is not int or not 0 <= account <= 0xffffffff
            or pid_kind != winreg.REG_DWORD or type(pid) is not int or pid <= 0
            or folder_kind != winreg.REG_SZ or not isinstance(folder, str) or not folder):
        raise ValueError('Invalid Steam client metadata')
    return account, pid, pathlib.Path(folder).resolve()


def parse_users(raw):
    """Strict, bounded KeyValues subset; duplicate/truncated fields are refused."""
    if len(raw) > 1024 * 1024:
        raise ValueError('Steam selection metadata too large')
    source = raw.decode('utf-8-sig')
    tokens = []
    pattern = re.compile(r'\s+|//[^\r\n]*|"((?:[^"\\]|\\.)*)"|([{}])')
    position = 0
    while position < len(source):
        match = pattern.match(source, position)
        if not match:
            raise ValueError('Invalid Steam selection metadata')
        position = match.end()
        if match.group(1) is not None:
            tokens.append(('text', re.sub(r'\\(.)', r'\1', match.group(1))))
        elif match.group(2):
            tokens.append((match.group(2), match.group(2)))
    index = 0

    def block(depth=0, nested=False):
        nonlocal index
        if depth > 4:
            raise ValueError('Steam selection metadata nesting limit')
        result = {}
        while index < len(tokens):
            kind, key = tokens[index]
            index += 1
            if kind == '}':
                if nested:
                    return result
                raise ValueError('Unexpected closing brace')
            if kind != 'text' or key in result or index == len(tokens):
                raise ValueError('Duplicate or missing Steam selection field')
            kind, value = tokens[index]
            index += 1
            if kind == '{':
                value = block(depth + 1, True)
            elif kind != 'text':
                raise ValueError('Missing Steam selection value')
            result[key] = value
        if nested:
            raise ValueError('Truncated Steam selection metadata')
        return result

    result = block()
    if set(result) != {'users'} or not isinstance(result['users'], dict):
        raise ValueError('Expected Steam users metadata')
    return result['users']


def offline_selection(raw):
    users = parse_users(raw)
    # Current clients use AutoLogin; older metadata can use MostRecent. If
    # both exist, their unique selections must agree. Never use timestamps.
    selections = []
    for marker in ('AutoLogin', 'MostRecent'):
        present = [key for key, value in users.items() if isinstance(value, dict) and marker in value]
        if present:
            if any(users[key][marker] not in ('0', '1') for key in present):
                return None
            selected = [key for key in present if users[key][marker] == '1']
            if len(selected) != 1:
                return None
            selections.append(selected[0])
    if not selections or len(set(selections)) != 1:
        return None
    selected = selections[0]
    if (not re.fullmatch(r'[0-9]{17}', selected)
            or not 0 < int(selected) - 0x0110000100000000 <= 0xffffffff
            or users[selected].get('WantsOfflineMode') != '1'):
        return None
    return selected


def offline_steam_id():
    """Fallback only for a live, unchanged Steam client with ActiveUser=0."""
    try:
        before = registry_state()
        account, pid, folder = before
        if account != 0:
            return None
        identity = instance_policy.process_identity(pid)
        if not identity or identity['image'].casefold() != str(folder / 'steam.exe').casefold():
            return None
        path = folder / 'config/loginusers.vdf'
        with path.open('rb') as stream:
            raw = stream.read(1024 * 1024 + 1)
        selected = offline_selection(raw)
        if registry_state() != before or instance_policy.process_identity(pid) != identity:
            return None
        with path.open('rb') as stream:
            if stream.read(1024 * 1024 + 1) != raw:
                return None
        return selected
    except (OSError, ValueError, TypeError, KeyError):
        return None
