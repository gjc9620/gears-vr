"""Candidate armspan measurement and atomic local persistence; no game writes."""
from dataclasses import dataclass
import json
import math
import os
from pathlib import Path
import tempfile


def default_path():
    return Path(os.environ.get('LOCALAPPDATA', str(Path.home() / 'AppData/Local'))) / 'GearsVR/weapon-calibration.json'


def validate(value):
    keys = {'schema', 'armspan_metres', 'hands_below_head_metres'}
    if not isinstance(value, dict) or set(value) != keys or type(value['schema']) is not int or value['schema'] != 1:
        raise ValueError('Invalid armspan schema')
    for name, low, high in (('armspan_metres', .9, 2.4), ('hands_below_head_metres', -.35, .1)):
        number = value[name]
        if type(number) not in (int, float) or not math.isfinite(number) or not low <= number <= high:
            raise ValueError('Invalid armspan ' + name)
    return dict(schema=1, armspan_metres=float(value['armspan_metres']),
                hands_below_head_metres=float(value['hands_below_head_metres']))


def _unique(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('Repeated calibration key')
        result[key] = value
    return result


@dataclass(frozen=True)
class Record:
    state: str
    value: dict | None
    original: bytes | None


def load(path):
    try:
        original = Path(path).read_bytes()
    except FileNotFoundError:
        return Record('missing', None, None)
    try:
        # Match the frozen Native consumer's UTF-8 JSON format (no BOM).
        value = validate(json.loads(original.decode('utf-8'), object_pairs_hook=_unique))
    except (ValueError, UnicodeError, TypeError, OverflowError):
        return Record('invalid', None, original)
    return Record('valid', value, original)


def save(path, value, previous):
    """Validate before touching disk; a failed replacement preserves the prior file."""
    value = validate(value)
    path = Path(path)
    current = load(path)
    if current.original != previous.original:
        raise RuntimeError('Calibration changed in another session; refusing to overwrite it')
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile('w', encoding='utf-8', dir=path.parent,
                                         prefix=path.name + '.', suffix='.tmp', delete=False) as stream:
            temporary = Path(stream.name)
            json.dump(value, stream, allow_nan=False, indent=2)
            stream.write('\n')
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return value


@dataclass(frozen=True)
class Sample:
    session: int
    sequence: int
    space_epoch: int
    profile: int
    input_epoch: int
    received_ns: int
    head: tuple
    yaw: float
    left: tuple
    right: tuple
    squeeze: tuple


class Measurement:
    """Two uninterrupted seconds and >=60 fresh, unique tracked T-pose samples."""
    def __init__(self):
        self.result = None
        self.reset_window()

    def reset_window(self):
        self.started = self.last_ns = 0
        self.last = None
        self.first = None
        self.spans = []
        self.heights = []

    @property
    def progress(self):
        return min(1., (self.last_ns - self.started) / 2e9, len(self.spans) / 60) if self.spans else 0.

    def observe(self, sample, now_ns):
        if self.result is not None:
            return self.result
        if not isinstance(sample, Sample) or type(now_ns) is not int or now_ns <= 0:
            self.reset_window()
            return None
        if (any(type(number) is not int or number <= 0 for number in
                (sample.session, sample.sequence, sample.space_epoch, sample.profile, sample.input_epoch, sample.received_ns)) or
                now_ns < sample.received_ns or now_ns - sample.received_ns > 100_000_000 or
                len(sample.squeeze) != 2 or any(not math.isfinite(v) or not .8 <= v <= 1 for v in sample.squeeze)):
            self.reset_window()
            return None
        if self.last == sample:
            return None  # Polling the same immutable ticket must not reset a good hold.
        if any(len(vector) != 3 or not all(math.isfinite(v) for v in vector)
               for vector in (sample.head, sample.left, sample.right)) or not math.isfinite(sample.yaw):
            self.reset_window()
            return None
        c, s = math.cos(sample.yaw), math.sin(sample.yaw)
        def local(point):
            x, y, z = (point[i] - sample.head[i] for i in range(3))
            return (c * x - s * z, y, s * x + c * z)
        left, right = local(sample.left), local(sample.right)
        span, height = math.dist(left, right), (left[1] + right[1]) / 2
        if (left[0] > -.35 or right[0] < .35 or not .9 <= span <= 2.4 or
                abs(left[1] - right[1]) > .15 or not -.35 <= height <= .1 or
                abs(left[2]) > .45 or abs(right[2]) > .45):
            self.reset_window()
            return None
        key = lambda v: (v.session, v.space_epoch, v.profile, v.input_epoch)
        if self.last is not None:
            if key(sample) == key(self.last) and sample.sequence <= self.last.sequence:
                self.reset_window()
                return None
            if (key(sample) != key(self.last) or now_ns <= self.last_ns or now_ns - self.last_ns > 100_000_000 or
                    math.dist(left, self.first[0]) > .08 or math.dist(right, self.first[1]) > .08):
                self.reset_window()
        if not self.spans:
            self.started = now_ns
            self.first = (left, right)
        self.last, self.last_ns = sample, now_ns
        self.spans.append(span)
        self.heights.append(height)
        if len(self.spans) >= 60 and now_ns - self.started >= 2_000_000_000:
            self.result = validate(dict(schema=1, armspan_metres=sum(self.spans) / len(self.spans),
                                        hands_below_head_metres=sum(self.heights) / len(self.heights)))
        return self.result


class Session:
    def __init__(self, path, remeasure=False):
        self.path, self.previous = Path(path), load(path)
        self.required = remeasure or self.previous.value is None
        self.measurement = Measurement()
        self.value = None if self.required else self.previous.value
        self.saved = False

    def observe(self, sample, now_ns):
        if not self.required or self.saved:
            return self.value
        value = self.measurement.observe(sample, now_ns)
        if value is not None:
            self.value = save(self.path, value, self.previous)
            self.saved = True
        return self.value
