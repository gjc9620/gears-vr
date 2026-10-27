"""User-requested 0.4 background-test plans. Pure functions; no game or file writes.

Apply only to a fresh private instance before launch. Normal launcher settings
and sealed historical test inputs must never be passed back to a writer.
"""
import copy
import re

RENDER_SCALE = 0.4
STARTUP_KEYS = tuple(prefix + 'StartupResolution' + axis
    for prefix in ('Hi', 'Low', 'Rec', '') for axis in ('X', 'Y'))

def configuration(source):
    if not isinstance(source, dict):
        raise TypeError('Expected a private test configuration object')
    result = copy.deepcopy(source)
    result['render_scale'] = RENDER_SCALE
    return result

def startup_ini(raw):
    """Scale the eight known ClientSettings startup dimensions, preserving bytes
    elsewhere. This is a prelaunch plan, not proof of the effective game size.
    Missing/duplicate/unknown dimension syntax is rejected; no guessed fallback.
    """
    if raw.startswith(b'\xff\xfe'):
        encoding = 'utf-16'
    else:
        encoding = 'utf-8-sig' if raw.startswith(b'\xef\xbb\xbf') else 'utf-8'
    text = raw.decode(encoding)
    sections = list(re.finditer(r'(?im)^\[ClientSettings\][ \t]*\r?\n', text))
    if len(sections) != 1:
        raise ValueError('Expected one private ClientSettings section')
    begin = sections[0].end()
    following = re.search(r'(?m)^\[', text[begin:])
    end = begin + following.start() if following else len(text)
    block = text[begin:end]
    changes = []
    for key in STARTUP_KEYS:
        assignments = re.findall(r'(?im)^[ \t]*' + re.escape(key) + r'[ \t]*=', block)
        if len(assignments) != 1:
            raise ValueError('Missing or duplicate private startup dimension: ' + key)
        pattern = re.compile(r'(?im)^([ \t]*' + re.escape(key) + r'[ \t]*=[ \t]*)([0-9]+)([ \t]*)(?=\r?$)')
        matches = list(pattern.finditer(block))
        if len(matches) != 1:
            raise ValueError('Expected one numeric private startup dimension: ' + key)
        original = int(matches[0][2])
        scaled = (original * 2 + 4) // 5  # ceil(original * 0.4), exact integer arithmetic.
        if not 64 <= scaled <= original <= 32768:
            raise ValueError('Private startup dimension outside supported plan range: ' + key)
        changes.append({'key': key, 'before': original, 'after': scaled})
        block = pattern.sub(lambda match: match[1] + str(scaled) + match[3], block, count=1)
    return (text[:begin] + block + text[end:]).encode(encoding), changes
