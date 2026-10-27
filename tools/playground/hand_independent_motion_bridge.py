"""Private retained calls for an already loaded Playground DLL.

Top-level imports are standard library only. ``attach_only`` is the sole public
factory: it requests the parent's attach-only constructor and never initializes,
injects or unloads a DLL. A retirement receipt proves request lifetime only; it
does not prove restored engine state. Unknown calls block every new dispatch.
"""
import copy
import ctypes as c
import math
import struct
import threading
import time


class PendingCallError(RuntimeError):
    pass


class ExactProcessExited(RuntimeError):
    pass


class Win32Api:
    """Small injectable API; all waits are zero-time observations."""
    def __init__(self):
        if not hasattr(c, "WinDLL"):
            raise OSError("Win32Api requires Windows; host tests must inject an API")
        self.k = c.WinDLL("kernel32", use_last_error=True)
        H, D, P = c.c_void_p, c.c_uint32, c.c_void_p
        signatures = {
            "OpenProcess": ([D, c.c_int, D], H),
            "GetProcessId": ([H], D),
            "GetProcessTimes": ([H, P, P, P, P], c.c_int),
            "CreateRemoteThread": ([H, P, c.c_size_t, P, P, D, P], H),
            "WaitForSingleObject": ([H, D], D),
            "GetExitCodeThread": ([H, P], c.c_int),
            "CloseHandle": ([H], c.c_int),
            "VirtualAllocEx": ([H, P, c.c_size_t, D, D], P),
            "VirtualFreeEx": ([H, P, c.c_size_t, D], c.c_int),
            "WriteProcessMemory": ([H, P, P, c.c_size_t, P], c.c_int),
        }
        for name, (arguments, result) in signatures.items():
            fn = getattr(self.k, name)
            fn.argtypes, fn.restype = arguments, result

    def _identity(self, handle):
        pid = self.k.GetProcessId(handle)
        stamps = [c.c_uint64() for _ in range(4)]
        if not pid or not self.k.GetProcessTimes(handle, *(c.byref(x) for x in stamps)):
            raise c.WinError(c.get_last_error())
        return int(pid), stamps[0].value

    def retain_process(self, parent_handle, identity):
        expected = int(identity["pid"]), int(identity["creation_time"])
        if self._identity(parent_handle) != expected:
            raise RuntimeError("Parent process handle identity differs")
        handle = self.k.OpenProcess(0x100000 | 0x1000, False, expected[0])
        if not handle:
            raise c.WinError(c.get_last_error())
        try:
            if self._identity(handle) != expected:
                raise RuntimeError("Retained process creation time differs")
            return handle
        except BaseException:
            self.k.CloseHandle(handle)
            raise

    def process_exited(self, handle, identity):
        # This is the retained process object, never a PID-absence query.
        if self._identity(handle) != (int(identity["pid"]), int(identity["creation_time"])):
            raise RuntimeError("Retained process identity changed")
        state = self.k.WaitForSingleObject(handle, 0)
        if state == 258:
            return False
        if state != 0:
            raise RuntimeError("Retained process wait failed")
        return True

    def alloc(self, process, size):
        address = self.k.VirtualAllocEx(process, None, size, 0x3000, 4)
        if not address:
            raise c.WinError(c.get_last_error())
        return address

    def write(self, process, address, raw):
        buffer, count = c.create_string_buffer(raw), c.c_size_t()
        if not self.k.WriteProcessMemory(process, address, buffer, len(raw), c.byref(count)) or count.value != len(raw):
            raise c.WinError(c.get_last_error())

    def free(self, process, address):
        if not self.k.VirtualFreeEx(process, address, 0, 0x8000):
            raise c.WinError(c.get_last_error())

    def create_thread(self, process, address, parameter):
        # Zero is a known non-dispatch. An exception is conservatively unknown.
        return self.k.CreateRemoteThread(process, None, 0, address, parameter, 0, None)

    def wait_thread(self, handle):
        return self.k.WaitForSingleObject(handle, 0)

    def thread_exit_code(self, handle):
        code = c.c_uint32()
        if not self.k.GetExitCodeThread(handle, c.byref(code)):
            raise c.WinError(c.get_last_error())
        return code.value

    def close_handle(self, handle):
        if not self.k.CloseHandle(handle):
            raise c.WinError(c.get_last_error())


_FACTORY = object()


class RetainedBridge:
    def __init__(self, parent, api, *, token, allowed_exports, allowed_functions,
                 timeout, on_retire, clock, sleep):
        if token is not _FACTORY:
            raise TypeError("Use attach_only; an arbitrary initialized Bridge is not accepted")
        self._parent, self._api = parent, api
        self.identity = copy.deepcopy(parent.identity)
        self._process = api.retain_process(parent.handle, self.identity)
        self._exports = frozenset(allowed_exports)
        self._functions = frozenset(allowed_functions)
        self._timeout, self._clock, self._sleep = timeout, clock, sleep
        self._callback, self._callback_active = on_retire, False
        self._lock = threading.RLock()
        self._pending, self._deadline, self._closed = None, None, False
        self._close_state = dict(parent_attempted=False, parent_closed=False,
                                 process_attempted=False, process_closed=False)
        self._number = 0
        self.history, self.diagnostics, self.last_outputs = [], [], {}

    def __getattr__(self, name):
        if name in ("allocate", "free", "write"):
            raise AttributeError("Remote buffers are adapter-owned; use owned_export/call")
        return getattr(self._parent, name)

    @property
    def r(self):
        return self._parent.r

    @r.setter
    def r(self, reader):
        # Discovery temporarily replaces the reader. Parent.function/verify and
        # parent.close must see that same reader; discovery owns the old one.
        self._parent.r = reader

    @property
    def has_pending(self):
        with self._lock:
            return self._pending is not None

    @property
    def pending_record(self):
        with self._lock:
            return copy.deepcopy(self._pending)

    @property
    def pending_address(self):
        # Existing discovery checks this only as a non-None retry fence. An
        # argument-free export still owns a thread and must also block retries.
        with self._lock:
            if self._pending is None:
                return None
            return self._pending["request"] or -1

    def set_deadline(self, absolute_deadline):
        with self._lock:
            if absolute_deadline is not None and not math.isfinite(absolute_deadline):
                raise ValueError("Finite absolute deadline required")
            self._deadline = absolute_deadline

    def _limit(self, absolute_deadline, timeout):
        timeout = self._timeout if timeout is None else timeout
        if not isinstance(timeout, (int, float)) or not math.isfinite(timeout) or timeout <= 0:
            raise ValueError("Positive finite timeout required")
        limits = [self._clock() + timeout]
        for value in (self._deadline, absolute_deadline):
            if value is not None:
                if not math.isfinite(value):
                    raise ValueError("Finite absolute deadline required")
                limits.append(value)
        deadline = min(limits)
        if self._clock() >= deadline:
            raise TimeoutError("Absolute request deadline expired before dispatch")
        return deadline

    def _new(self, kind, name, deadline):
        if self._closed or self._close_state["parent_attempted"] or self._callback_active or self._pending is not None:
            raise PendingCallError("Closed, callback-active or pending bridge; no new dispatch")
        if self._api.process_exited(self._process, self.identity):
            raise ExactProcessExited("Exact retained game process has exited")
        self._parent.verify()
        self._number += 1
        record = dict(number=self._number, kind=kind, name=name, deadline=deadline,
                      allocations=[], request=None, thread=None, dispatch_attempted=False,
                      dispatched=False, thread_exited=False, thread_close_attempted=False,
                      thread_closed=False, terminal=False, observation_errors=[], error_count=0,
                      engine_clean_proved=False)
        self._pending = record
        return record

    def _allocate(self, record, raw):
        address = self._api.alloc(self._parent.handle, len(raw))
        if not address:
            raise RuntimeError("Allocation returned zero")
        # Register before the write: even a partial WriteProcessMemory is owned.
        record["allocations"].append(dict(address=address, release_attempted=False, released=False))
        self._api.write(self._parent.handle, address, raw)
        return address

    def _error(self, record, error):
        record["error_count"] += 1
        if len(record["observation_errors"]) < 8:
            record["observation_errors"].append(type(error).__name__ + ": " + str(error))

    def _retire(self, record, *, game_exited=False):
        if record is not self._pending:
            raise PendingCallError("Retirement identity differs")
        record["exact_game_exited"] = game_exited
        if not game_exited:
            for allocation in reversed(record["allocations"]):
                if allocation["released"]:
                    continue
                if allocation["release_attempted"]:
                    return None  # Unknown prior free is never repeated.
                allocation["release_attempted"] = True
                try:
                    self._api.free(self._parent.handle, allocation["address"])
                    allocation["released"] = True
                except BaseException as error:
                    self._error(record, error)
                    return None
        else:
            record["remote_allocations_abandoned_after_exact_exit"] = True
        if record["thread"] and not record["thread_closed"]:
            if record["thread_close_attempted"]:
                if not game_exited:
                    return None
                record["local_thread_close_unknown"] = True
            else:
                record["thread_close_attempted"] = True
                try:
                    self._api.close_handle(record["thread"])
                    record["thread_closed"] = True
                except BaseException as error:
                    self._error(record, error)
                    if not game_exited:
                        return None
                    record["local_thread_close_unknown"] = True
        receipt = copy.deepcopy(record)
        self._pending = None
        self.history.append(copy.deepcopy(receipt))
        if self._callback:
            self._callback_active = True
            try:
                self._callback(copy.deepcopy(receipt))
            except BaseException as error:
                self.diagnostics.append(dict(operation="on_retire", number=record["number"],
                                             error=type(error).__name__ + ": " + str(error)))
            finally:
                self._callback_active = False
        return receipt

    def poll_pending(self):
        """Observe the same call; None means retained, never permission to retry.

        Callback runs once after retirement is committed, under a reentry guard.
        Callback exceptions are diagnostics and cannot repeat release operations.
        """
        with self._lock:
            if self._callback_active:
                raise PendingCallError("Cannot poll from a retirement callback")
            record = self._pending
            if record is None:
                return None
            try:
                if self._api.process_exited(self._process, self.identity):
                    record["terminal"] = True
                    return self._retire(record, game_exited=True)
                if record.get("permanent_unknown"):
                    return None
                if not record["dispatched"]:
                    if record["dispatch_attempted"]:
                        return None  # CreateThread raised: no handle proof.
                    return self._retire(record)
                if not record["thread_exited"]:
                    state = self._api.wait_thread(record["thread"])
                    if state == 258:
                        return None
                    if state != 0:
                        raise RuntimeError("Owned remote thread wait failed; retained")
                    # Exit-code failure retains the handle and retries observation.
                    code = self._api.thread_exit_code(record["thread"])
                    if type(code) is not int or not 0 <= code <= 0xffffffff:
                        raise RuntimeError("Invalid remote thread exit code")
                    record.update(thread_exited=True, exit_code=code)
                if record["kind"] == "submit":
                    self._parent.verify()
                    state, size, obj, fn = struct.unpack("<iIQQ", self.r.read(record["request"], 24))
                    if (size, obj, fn) != (record["size"], record["object"], record["function"]):
                        record["permanent_unknown"] = "request_header_changed"
                        return None
                    code = record["exit_code"]
                    if code:
                        if code not in (1, 2) or state != 0:
                            record["permanent_unknown"] = "unknown_submit_acceptance"
                            return None
                        record.update(accepted=False, state=0)
                    else:
                        if state == 0:
                            return None
                        if state not in (2, -1, -2):
                            record["permanent_unknown"] = "unknown_gt_state"
                            return None
                        record.update(accepted=True, state=state)
                        if state == 2:
                            raw = self.r.read(record["request"] + 24, 4096)
                            if len(raw) != 4096:
                                raise RuntimeError("Incomplete completed request output")
                            record["outputs_hex"] = raw.hex()
                record["terminal"] = True
                return self._retire(record)
            except BaseException as error:
                self._error(record, error)
                return None

    def observe_pending(self, absolute_deadline):
        if not math.isfinite(absolute_deadline):
            raise ValueError("Finite observation deadline required")
        while self.has_pending:
            if self._clock() >= absolute_deadline:
                return None
            receipt = self.poll_pending()
            if receipt is not None:
                return receipt
            self._sleep(min(.01, max(0., absolute_deadline - self._clock())))
        return None

    def _dispatch(self, record, address):
        if self._clock() >= record["deadline"]:
            raise TimeoutError("Absolute request deadline expired before CreateThread")
        record["dispatch_attempted"] = True
        record["thread"] = self._api.create_thread(self._parent.handle, address, record["request"])
        if not record["thread"]:
            # The API's zero result is a known failure, unlike an exception.
            record.update(dispatch_attempted=False, terminal=True, dispatch_failed=True)
            self._retire(record)
            raise RuntimeError("CreateRemoteThread failed without dispatch")
        record["dispatched"] = True
        receipt = self.observe_pending(record["deadline"])
        if receipt is None:
            raise TimeoutError("Original remote call/request retained; no replay")
        if receipt.get("exact_game_exited"):
            raise ExactProcessExited("Exact game exited before the operation was retired")
        return receipt

    def export(self, name, parameter=None, *, payload=None, absolute_deadline=None, timeout=None):
        with self._lock:
            if name not in self._exports:
                raise ValueError("Export is outside the explicit private allowlist")
            if parameter is not None and (type(parameter) is not int or parameter != 0):
                raise ValueError("External remote pointer forbidden; pass owned payload bytes")
            if payload is not None and (not isinstance(payload, (bytes, bytearray, memoryview)) or not 0 < len(payload) <= 4096):
                raise ValueError("Owned export payload must contain 1..4096 bytes")
            deadline = self._limit(absolute_deadline, timeout)
            record = self._new("export", name, deadline)
            try:
                if payload is not None:
                    raw = bytes(payload)
                    record["payload_hex"] = raw.hex()
                    record["request"] = self._allocate(record, raw)
                return self._dispatch(record, self.base + self.exports[name])["exit_code"]
            except BaseException:
                if self._pending is record and not record["dispatch_attempted"]:
                    record["terminal"] = True
                    self._retire(record)
                raise

    def owned_export(self, name, payload, **kwargs):
        return self.export(name, payload=payload, **kwargs)

    def call(self, object, name, timeout=10, *, absolute_deadline=None, **values):
        with self._lock:
            if name not in self._functions:
                raise ValueError("Original function is outside the explicit private allowlist")
            deadline = self._limit(absolute_deadline, timeout)
            record = self._new("submit", name, deadline)
            try:
                obj = self.r.identity(object)
                fn = self.function(obj["cls"], name)["address"]
                size = struct.unpack("<H", self.r.read(fn + 0x106, 2))[0]
                if size > 4096:
                    raise ValueError("Playground parameter budget")
                buffer, fields, returned = bytearray(4096), [], None
                for prop in self.r.fields(fn):
                    if self._clock() >= deadline:
                        raise TimeoutError("Absolute request deadline expired during marshalling")
                    flags = self.r.u64(prop["address"] + 0x78)
                    if not flags & 0x80:
                        continue
                    kind = self.r.class_name(prop)
                    offset, width = self.r.u32(prop["address"] + 0x94), self.r.u32(prop["address"] + 0x74)
                    if offset + width > size:
                        raise RuntimeError("Function parameter outside declared size")
                    fields.append((prop["name"], flags, kind, offset, width))
                    if flags & 0x400:
                        returned = (kind, offset, width)
                        continue
                    if prop["name"] not in values:
                        continue
                    value = values.pop(prop["name"])
                    if kind in ("ObjectProperty", "ClassProperty", "ComponentProperty"):
                        raw = struct.pack("<Q", value)
                    elif kind == "NameProperty":
                        raw = struct.pack("<II", *value)
                    elif kind == "IntProperty":
                        raw = struct.pack("<i", value)
                    elif kind == "ByteProperty":
                        raw = struct.pack("<B", value)
                    elif kind == "BoolProperty":
                        raw = struct.pack("<I", self.r.u32(prop["address"] + 0xd0) if value else 0)
                    elif kind == "FloatProperty":
                        raw = struct.pack("<f", value)
                    elif kind == "StrProperty":
                        if not isinstance(value, str):
                            raise TypeError("StrProperty requires str")
                        encoded = (value + "\0").encode("utf-16-le")
                        address = self._allocate(record, encoded)
                        raw = struct.pack("<Qii", address, len(encoded) // 2, len(encoded) // 2)
                    elif kind == "StructProperty":
                        if not isinstance(value, (bytes, bytearray, memoryview)):
                            raise TypeError("StructProperty requires exact bytes")
                        raw = bytes(value)
                    else:
                        raise RuntimeError("Unsupported parameter: " + kind)
                    if len(raw) != width:
                        raise RuntimeError("Parameter element size mismatch: " + prop["name"])
                    buffer[offset:offset + width] = raw
                if values:
                    raise ValueError("Unknown parameters: " + str(sorted(values)))
                record.update(size=size, object=object, function=fn)
                record["request"] = self._allocate(record, struct.pack("<iIQQ", 0, size, object, fn) + buffer)
                receipt = self._dispatch(record, self.base + self.exports["PlaygroundSubmit"])
                if receipt["exit_code"] or receipt["state"] != 2:
                    raise RuntimeError("Original request rejected/failed: " + str(receipt.get("state")))
                raw = bytes.fromhex(receipt["outputs_hex"])
                self.last_outputs = {name: raw[offset:offset + width].hex() for name, flags, kind, offset, width in fields if flags & 0x100}
                if returned is None:
                    return None
                kind, offset, width = returned
                formats = {"ObjectProperty": "<Q", "ClassProperty": "<Q", "ComponentProperty": "<Q",
                           "IntProperty": "<i", "ByteProperty": "<B", "BoolProperty": "<I", "FloatProperty": "<f"}
                value = raw[offset:offset + width]
                if kind in formats:
                    parsed = struct.unpack(formats[kind], value)[0]
                    return bool(parsed) if kind == "BoolProperty" else parsed
                return value.hex()
            except BaseException:
                if self._pending is record and not record["dispatch_attempted"]:
                    record["terminal"] = True
                    self._retire(record)
                raise

    def close(self):
        """False leaves parent/process/thread/request intact; never a drain."""
        with self._lock:
            if self._pending is not None or self._callback_active:
                return False
            if self._closed:
                return True
            state = self._close_state
            if not state["parent_closed"]:
                if state["parent_attempted"]:
                    return False
                state["parent_attempted"] = True
                try:
                    self._parent.close()
                    state["parent_closed"] = True
                except BaseException as error:
                    self.diagnostics.append(dict(operation="parent_close", unknown=True,
                                                 error=type(error).__name__ + ": " + str(error)))
                    return False
            if not state["process_closed"]:
                if state["process_attempted"]:
                    return False
                state["process_attempted"] = True
                try:
                    self._api.close_handle(self._process)
                    state["process_closed"] = True
                except BaseException as error:
                    self.diagnostics.append(dict(operation="process_handle_close", unknown=True,
                                                 error=type(error).__name__ + ": " + str(error)))
                    return False
            self._closed = True
            return True


def attach_only(bridge_type, identity, dll, *, expected_sha256, allowed_exports,
                allowed_functions=(), api=None, timeout=10.0, on_retire=None,
                clock=time.monotonic, sleep=time.sleep):
    """Attach to an existing exact module; caller must seal class/artifact origins.

    ``api`` is a host-test seam with Win32Api's methods, never an SDK import.
    ``on_retire(receipt)`` is diagnostic-only; no receipt asserts engine cleanup.
    """
    if not expected_sha256 or not allowed_exports or not isinstance(timeout, (int, float)) or not math.isfinite(timeout) or timeout <= 0:
        raise ValueError("Exact DLL hash, export allowlist and finite timeout required")
    backend = api if api is not None else Win32Api()
    parent = bridge_type(identity, dll, expected_sha256=expected_sha256, attach_only=True)
    try:
        if any(name not in parent.exports for name in allowed_exports) or (allowed_functions and "PlaygroundSubmit" not in parent.exports):
            raise ValueError("Required export absent from exact loaded module")
        return RetainedBridge(parent, backend, token=_FACTORY, allowed_exports=allowed_exports,
                              allowed_functions=allowed_functions, timeout=timeout, on_retire=on_retire,
                              clock=clock, sleep=sleep)
    except BaseException:
        parent.close()
        raise
