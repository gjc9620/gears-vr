"""Synthetic lifecycle receipts; no game/process/native API is used."""
import copy
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from run_camera_policy import PolicyRejected
from run_camera_runtime import (RuntimePolicy, DEFAULT_PROFILE, WindowsLifetimeProof,
                                LIFETIME_ACCESS, WAIT_OBJECT_0, WAIT_TIMEOUT, WAIT_FAILED)


class World:
    def __init__(self):
        self.identity = dict(pid=123, creation_time=10, image='synthetic')
        self.context = ('pawn1', 'modifier1')
        self.now = 0.
        self.events, self.readers = [], []
        self.available = True
        self.external = self.block_restore = self.fail_rebind = False
        self.fail_after_release = self.fail_apply = self.pending_apply = False
        self.recover_on_rebind = False
        self.factory_calls = 0
        self.proofs = []
        self.wait_result, self.wait_error = WAIT_TIMEOUT, None
        self.lifetime_open_error = self.lifetime_close_error = False

    def checker(self, pid):
        assert pid == 123
        return copy.deepcopy(self.identity)

    def factory(self, pid, path):
        self.factory_calls += 1
        assert pid == 123 and isinstance(path, Path)
        if not self.available:
            raise OSError('synthetic context unavailable')
        reader = Reader(self, len(self.readers))
        self.readers.append(reader)
        return SimpleNamespace(r=SimpleNamespace(pid=pid, p=reader), context=self.context)

    def policy(self, pid, checker):
        self.instance = FakePolicy(self, pid, checker)
        return self.instance

    def lifetime_factory(self, pid, identity):
        assert pid == 123 and identity == self.identity
        assert self.instance.plan.state == 'prepared' and not self.instance.plan.owned
        if self.lifetime_open_error:
            raise OSError('synthetic lifetime handle unavailable')
        proof = Lifetime(self, identity)
        self.proofs.append(proof)
        return proof

    def runtime(self):
        return RuntimePolicy(123, self.checker, clock=lambda: self.now,
                             sampler_factory=self.factory, policy_factory=self.policy,
                             lifetime_factory=self.lifetime_factory)

    def advance(self, runtime, seconds=8.):
        self.now += seconds
        runtime.tick()


class Reader:
    def __init__(self, world, index):
        self.world, self.index, self.closed = world, index, False

    def close(self):
        self.world.events.append(('reader_close', self.index))
        self.closed = True


class Lifetime:
    def __init__(self, world, identity):
        self.world, self.identity, self.closed = world, copy.deepcopy(identity), False
        self.world.events.append(('lifetime_open',))

    def wait(self):
        assert not self.closed
        self.world.events.append(('lifetime_wait', self.world.wait_result))
        if self.world.wait_error:
            raise self.world.wait_error
        return self.world.wait_result

    def close(self):
        assert not self.closed
        if self.world.lifetime_close_error:
            raise OSError('synthetic lifetime close failure')
        self.world.events.append(('lifetime_close',))
        self.closed = True


class Plan:
    def __init__(self, sampler):
        self.sampler, self.state, self.owned = sampler, 'prepared', []

    def export(self):
        return dict(state=self.state, owned_fields=list(self.owned), context=self.sampler.context)


class FakePolicy:
    """Models only resource/transaction transitions, not reflection or writes."""
    def __init__(self, world, pid, checker):
        self.world, self.checker = world, checker
        self.identity, self.plan, self.closed = checker(pid), None, False
        self.report = dict(applications=0, writes=0, restored=0, retired_fields=0,
                           external_changes=0, pending_fields=[], errors=[])

    def _reader(self):
        if self.plan is not None:
            if self.plan.sampler.r.p.closed:
                raise AssertionError('Certificate reader closed before policy release')

    def prepare(self, sampler):
        assert not sampler.r.p.closed and sampler.context == self.world.context
        assert self.checker(123) == self.identity
        assert self.plan is None or not self.plan.owned
        self.plan = Plan(sampler)
        return self.plan

    def apply(self):
        self._reader()
        assert self.plan.state == 'prepared'
        self.world.events.append(('apply', self.plan.sampler.r.p.index))
        self.plan.owned = ['ambient', 'roadie']
        self.report['writes'] += 2
        if self.world.fail_apply:
            self.world.block_restore = self.world.pending_apply
            self.restore()
            raise OSError('synthetic apply failed after partial write')
        self.plan.state = 'applied'
        self.report['applications'] += 1
        return self.plan.export()

    def check(self):
        self._reader()
        if self.world.external:
            self.report['last_check'] = dict(valid=False, reason='Run-camera desired field changed: ambient')
            return False
        valid = self.plan.state == 'applied' and self.plan.sampler.context == self.world.context
        self.report['last_check'] = dict(valid=valid, **({} if valid else {'reason': 'synthetic Pawn changed'}))
        return valid

    def restore(self):
        self._reader()
        self.world.events.append(('restore', None if self.plan is None else self.plan.sampler.r.p.index))
        if self.plan is not None:
            try:
                replaced = self.checker(123) != self.identity
            except RuntimeError:
                replaced = False
            if replaced:
                self.report['retired_fields'] += len(self.plan.owned)
                self.plan.owned = []
            elif self.plan.owned and (self.world.block_restore or self.plan.sampler.context != self.world.context):
                self.report['pending_fields'] = list(self.plan.owned)
                return dict(released=False, pending_fields=list(self.plan.owned))
            elif self.plan.owned:
                name = 'external_changes' if self.world.external else 'restored'
                self.report[name] += len(self.plan.owned)
                self.plan.owned = []
            self.plan.state = 'released'
        self.report['pending_fields'] = []
        return dict(released=True, pending_fields=[])

    def rebind(self, sampler, *, apply=True):
        assert apply is False, 'The wrapper must separate rebind from apply'
        self._reader()
        self.world.events.append(('rebind', self.plan.sampler.r.p.index, sampler.r.p.index))
        if self.world.fail_rebind:
            raise PolicyRejected('synthetic rebind rejected')
        if self.world.recover_on_rebind:
            self.plan.state = 'applied'
            self.world.context = self.plan.sampler.context
            return self.plan.export()
        if self.plan.owned:
            assert sampler.context == self.world.context
            assert sampler.context[1] == self.plan.sampler.context[1]
            self.report['restored'] += len(self.plan.owned)
            self.plan.owned = []
            self.report['pending_fields'] = []
        if self.world.fail_after_release:
            raise PolicyRejected('synthetic fresh preparation rejected after release')
        return self.prepare(sampler).export()

    def close(self):
        if self.closed:
            return copy.deepcopy(self.report)
        self._reader()
        assert self.plan is None or not self.plan.owned, 'Pending policy close loses writer responsibility'
        self.world.events.append(('policy_close',))
        self.closed = True
        return copy.deepcopy(self.report)


class NativeFunction:
    def __init__(self, callback):
        self.callback = callback

    def __call__(self, *args):
        return self.callback(*args)


class LifetimeKernel:
    """Fake kernel functions also expose ctypes signature attributes."""
    def __init__(self, identity):
        self.identity, self.calls = copy.deepcopy(identity), []
        self.result = WAIT_TIMEOUT
        self.times_ok = self.query_ok = self.close_ok = True
        for name, callback in (('OpenProcess', self.open), ('GetProcessTimes', self.times),
                               ('QueryFullProcessImageNameW', self.query),
                               ('WaitForSingleObject', self.wait), ('CloseHandle', self.close)):
            setattr(self, name, NativeFunction(callback))

    def open(self, access, inherit, pid):
        self.calls.append(('open', access, inherit, pid))
        return 0x1234

    def times(self, handle, created, *_other):
        self.calls.append(('times', handle))
        created._obj.dwLowDateTime = self.identity['creation_time'] & 0xffffffff
        created._obj.dwHighDateTime = self.identity['creation_time'] >> 32
        return self.times_ok

    def query(self, handle, flags, image, length):
        self.calls.append(('query', handle, flags))
        image.value = self.identity['image']
        length._obj.value = len(image.value)
        return self.query_ok

    def wait(self, handle, milliseconds):
        self.calls.append(('wait', handle, milliseconds))
        return self.result

    def close(self, handle):
        self.calls.append(('close', handle))
        return self.close_ok


class LifetimeProofTests(unittest.TestCase):
    def setUp(self):
        self.identity = dict(pid=123, creation_time=0x123456789abc,
                             image=str(Path('synthetic-runtime.exe').resolve()).casefold())
        self.kernel = LifetimeKernel(self.identity)

    def test_native_rights_identity_queries_and_retained_zero_timeout_handle(self):
        self.kernel.identity['image'] = self.identity['image'].upper()
        proof = WindowsLifetimeProof(123, self.identity, kernel=self.kernel)
        self.assertEqual(self.kernel.calls[0], ('open', LIFETIME_ACCESS, False, 123))
        self.assertEqual(LIFETIME_ACCESS, 0x101000)
        self.assertEqual(LIFETIME_ACCESS & (0x8 | 0x10 | 0x20), 0)
        self.assertEqual(self.kernel.calls[1:3], [('times', 0x1234), ('query', 0x1234, 0)])
        self.assertEqual(proof.wait(), WAIT_TIMEOUT)
        self.assertEqual(self.kernel.calls[-1], ('wait', 0x1234, 0))
        self.assertEqual(proof.handle, 0x1234)
        proof.close()
        proof.close()
        self.assertEqual(self.kernel.calls.count(('close', 0x1234)), 1)

    def test_native_reused_pid_and_wrong_image_are_rejected_and_handle_closed(self):
        for field, value in (('creation_time', self.identity['creation_time'] + 1),
                             ('image', str(Path('different-synthetic.exe').resolve()))):
            with self.subTest(field=field):
                kernel = LifetimeKernel(self.identity)
                kernel.identity[field] = value
                with self.assertRaisesRegex(PolicyRejected, 'different process identity'):
                    WindowsLifetimeProof(123, self.identity, kernel=kernel)
                self.assertEqual(kernel.calls[-1], ('close', 0x1234))

    def test_native_query_failures_do_not_leave_an_accepted_proof(self):
        for field in ('times_ok', 'query_ok'):
            with self.subTest(field=field):
                kernel = LifetimeKernel(self.identity)
                setattr(kernel, field, False)
                with self.assertRaises(OSError):
                    WindowsLifetimeProof(123, self.identity, kernel=kernel)
                self.assertEqual(kernel.calls[-1], ('close', 0x1234))

    def test_native_failed_wait_raises_and_keeps_handle_until_explicit_close(self):
        proof = WindowsLifetimeProof(123, self.identity, kernel=self.kernel)
        self.kernel.result = WAIT_FAILED
        with self.assertRaises(OSError):
            proof.wait()
        self.assertEqual(proof.handle, 0x1234)
        self.assertNotIn(('close', 0x1234), self.kernel.calls)
        self.kernel.result = WAIT_OBJECT_0
        self.assertEqual(proof.wait(), WAIT_OBJECT_0)
        proof.close()

    def test_native_failed_close_retains_handle_for_retry(self):
        proof = WindowsLifetimeProof(123, self.identity, kernel=self.kernel)
        self.kernel.close_ok = False
        with self.assertRaises(OSError):
            proof.close()
        self.assertEqual(proof.handle, 0x1234)
        self.kernel.close_ok = True
        proof.close()
        self.assertIsNone(proof.handle)


class RuntimeTests(unittest.TestCase):
    def setUp(self):
        self.w = World()
        self.runtime = self.w.runtime()

    def test_initial_apply_fast_check_and_poll_throttle_retain_reader(self):
        self.runtime.tick()
        old = self.w.readers[0]
        self.assertTrue(self.runtime.report['applied'])
        self.assertFalse(old.closed)
        for _ in range(100):
            self.runtime.tick()
        self.assertEqual(self.runtime.report['attempts'], 1)
        self.w.advance(self.runtime, 1.)
        self.assertEqual(self.w.factory_calls, 1)
        self.assertEqual(self.runtime.report['applications'], 1)
        self.assertFalse(old.closed)
        self.assertEqual(self.runtime.profile_path, DEFAULT_PROFILE)

    def test_changed_pawn_rebind_releases_old_before_closing_and_applies_new(self):
        self.runtime.tick()
        self.w.context = ('pawn2', 'modifier1')
        self.w.advance(self.runtime)
        self.assertTrue(self.w.readers[0].closed)
        self.assertFalse(self.w.readers[1].closed)
        self.assertEqual(self.runtime.report['restored'], 2)
        self.assertEqual(self.runtime.report['applications'], 2)
        self.assertTrue(self.runtime.report['applied'])
        events = self.w.events
        self.assertLess(events.index(('rebind', 0, 1)), events.index(('reader_close', 0)))
        self.assertLess(events.index(('reader_close', 0)), events.index(('apply', 1)))

    def test_rebind_failure_keeps_old_reader_and_pending_closes_temporary_only(self):
        self.runtime.tick()
        self.w.context = ('pawn2', 'modifier1')
        self.w.fail_rebind = True
        self.w.advance(self.runtime)
        self.assertFalse(self.w.readers[0].closed)
        self.assertTrue(self.w.readers[1].closed)
        self.assertEqual(self.runtime.report['pending_fields'], ['ambient', 'roadie'])
        report = self.runtime.close()
        self.assertFalse(report['closed'])
        self.assertFalse(report['released'])
        self.assertFalse(self.w.instance.closed)
        self.assertFalse(self.w.readers[0].closed)
        self.w.fail_rebind = False
        report = self.runtime.close()
        self.assertTrue(report['closed'])
        self.assertTrue(all(r.closed for r in self.w.readers))
        self.assertEqual(report['applications'], 1)
        self.assertEqual(report['restored'], 2)

    def test_external_change_latches_and_never_reapplies_even_new_context(self):
        self.runtime.tick()
        self.w.external = True
        self.w.advance(self.runtime)
        self.assertTrue(self.runtime.report['suspended'])
        self.assertTrue(self.runtime.report['released'])
        self.assertFalse(self.runtime.report['applied'])
        self.w.external = False
        self.w.context = ('pawn2', 'modifier2')
        for _ in range(4):
            self.w.advance(self.runtime)
        self.assertEqual(self.w.factory_calls, 1)
        self.assertEqual(self.runtime.report['applications'], 1)
        self.assertTrue(self.runtime.close()['closed'])

    def test_unavailable_identity_is_not_retirement_and_close_can_retry(self):
        self.runtime.tick()
        identity, self.w.identity = self.w.identity, None
        report = self.runtime.close()
        self.assertFalse(report['closed'])
        self.assertFalse(report['released'])
        self.assertEqual(report['retired_fields'], 0)
        self.assertEqual(report['pending_fields'], ['ambient', 'roadie'])
        self.assertEqual(report['lifetime']['state'], 'alive')
        self.assertEqual(report['lifetime']['last_wait'], WAIT_TIMEOUT)
        self.assertTrue(report['lifetime']['retained'])
        self.assertFalse(self.w.proofs[0].closed)
        self.assertFalse(self.w.readers[0].closed)
        self.w.identity = identity
        report = self.runtime.close()
        self.assertTrue(report['closed'])
        self.assertEqual(report['restored'], 2)

    def test_positive_identity_replacement_retires_without_new_apply(self):
        self.runtime.tick()
        original = copy.deepcopy(self.w.identity)
        self.w.identity['creation_time'] += 1
        self.w.advance(self.runtime)
        self.assertTrue(self.runtime.report['stopping'])
        self.assertEqual(self.runtime.report['retired_fields'], 2)
        self.assertEqual(self.w.factory_calls, 1)
        self.assertEqual(self.w.proofs[0].identity, original)
        self.assertFalse(any(event[0] == 'lifetime_wait' for event in self.w.events))
        self.assertTrue(self.runtime.close()['closed'])

    def test_acquisition_failures_have_bounded_backoff_and_diagnostics(self):
        self.w.available = False
        self.runtime.tick()
        for _ in range(100):
            self.runtime.tick()
        self.assertEqual(self.w.factory_calls, 1)
        self.w.advance(self.runtime, 1.)
        self.assertEqual(self.w.factory_calls, 2)
        self.w.advance(self.runtime, 1.)
        self.assertEqual(self.w.factory_calls, 2)
        self.w.advance(self.runtime, 1.)
        self.assertEqual(self.w.factory_calls, 3)
        for _ in range(30):
            self.w.advance(self.runtime, 8.)
        self.assertEqual(len(self.runtime.report['errors']), 16)
        self.assertIn('context unavailable', self.runtime.report['last_error']['reason'])
        self.assertLessEqual(self.runtime.next_poll - self.w.now, 8.)
        self.w.available = True
        self.w.advance(self.runtime)
        self.assertTrue(self.runtime.report['applied'])
        self.assertEqual(self.runtime.report['consecutive_failures'], 0)

    def test_pending_partial_apply_keeps_new_reader_and_no_premature_close(self):
        self.w.fail_apply = self.w.pending_apply = True
        self.runtime.tick()
        self.assertFalse(self.runtime.report['applied'])
        self.assertFalse(self.runtime.report['released'])
        self.assertFalse(self.w.readers[0].closed)
        self.assertEqual(self.runtime.report['pending_fields'], ['ambient', 'roadie'])
        self.w.available = False
        report = self.runtime.close()
        self.assertFalse(report['closed'])
        self.assertFalse(self.w.readers[0].closed)
        self.w.block_restore = False
        report = self.runtime.close()
        self.assertTrue(report['closed'])

    def test_partial_apply_after_rebind_keeps_replacement_reader_for_restore(self):
        self.runtime.tick()
        self.w.context = ('pawn2', 'modifier1')
        self.w.fail_apply = self.w.pending_apply = True
        self.w.advance(self.runtime)
        self.assertTrue(self.w.readers[0].closed)
        self.assertFalse(self.w.readers[1].closed)
        self.assertFalse(self.runtime.report['released'])
        self.assertEqual(self.runtime.report['pending_fields'], ['ambient', 'roadie'])
        self.w.available = False
        self.assertFalse(self.runtime.close()['closed'])
        self.assertFalse(self.w.readers[1].closed)
        self.w.block_restore = False
        report = self.runtime.close()
        self.assertTrue(report['closed'])
        self.assertEqual(report['restored'], 4)
        self.assertEqual(report['applications'], 1)

    def test_rejected_initial_preparation_closes_only_unused_fresh_reader(self):
        def reject(_sampler):
            raise PolicyRejected('synthetic unexpected camera ownership')
        self.w.instance.prepare = reject
        self.runtime.tick()
        self.assertTrue(self.w.readers[0].closed)
        self.assertTrue(self.runtime.report['released'])
        self.assertEqual(self.runtime.report['applications'], 0)
        self.assertIn('camera ownership', self.runtime.report['last_error']['reason'])
        self.assertTrue(self.runtime.close()['closed'])

    def test_restore_stops_future_applications_and_close_is_idempotent(self):
        self.runtime.tick()
        report = self.runtime.restore()
        self.assertTrue(report['released'])
        self.assertTrue(report['stopping'])
        report['errors'].append('detached')
        self.w.advance(self.runtime)
        self.assertEqual(self.runtime.report['applications'], 1)
        self.assertNotIn('detached', self.runtime.report['errors'])
        self.assertTrue(self.runtime.close()['closed'])
        events = list(self.w.events)
        self.assertEqual(self.runtime.close()['status'], 'closed')
        self.assertEqual(events, self.w.events)
        self.assertLess(events.index(('policy_close',)), events.index(('reader_close', 0)))

    def test_close_before_first_tick_needs_no_sampler(self):
        self.w.identity = None
        report = self.runtime.close()
        self.assertTrue(report['closed'])
        self.assertEqual(self.w.factory_calls, 0)
        self.assertEqual(self.w.proofs, [])

    def test_signaled_original_process_retires_without_restoration_writes(self):
        self.runtime.tick()
        self.w.identity, self.w.wait_result = None, WAIT_OBJECT_0
        report = self.runtime.close()
        self.assertTrue(report['closed'])
        self.assertTrue(report['released'])
        self.assertEqual(report['retired_fields'], 2)
        self.assertEqual(report['restored'], 0)
        self.assertEqual(report['writes'], 2)
        self.assertTrue(report['lifetime']['exit_confirmed'])
        self.assertFalse(report['lifetime']['retained'])
        self.assertTrue(self.w.proofs[0].closed)
        events = self.w.events
        wait = events.index(('lifetime_wait', WAIT_OBJECT_0))
        self.assertLess(wait, events.index(('policy_close',)))
        self.assertLess(events.index(('policy_close',)), events.index(('reader_close', 0)))
        self.assertLess(events.index(('reader_close', 0)), events.index(('lifetime_close',)))

    def test_failed_unknown_or_timeout_wait_never_proves_exit(self):
        for result in (WAIT_FAILED, WAIT_TIMEOUT, 0x80, 7, False, None):
            with self.subTest(result=result):
                world = World()
                runtime = world.runtime()
                runtime.tick()
                original = world.identity
                world.identity, world.wait_result = None, result
                report = runtime.close()
                self.assertFalse(report['closed'])
                self.assertFalse(report['released'])
                self.assertEqual(report['retired_fields'], 0)
                self.assertEqual(report['pending_fields'], ['ambient', 'roadie'])
                self.assertFalse(report['lifetime']['exit_confirmed'])
                self.assertTrue(report['lifetime']['retained'])
                self.assertFalse(world.proofs[0].closed)
                self.assertFalse(world.readers[0].closed)
                world.identity = original
                self.assertTrue(runtime.close()['closed'])

    def test_wait_error_is_diagnostic_pending_until_a_positive_exit_receipt(self):
        self.runtime.tick()
        self.w.identity = None
        self.w.wait_error = OSError('synthetic WAIT_FAILED')
        report = self.runtime.close()
        self.assertFalse(report['closed'])
        self.assertEqual(report['lifetime']['state'], 'wait_error')
        self.assertIn('WAIT_FAILED', report['last_error']['reason'])
        self.assertEqual(report['retired_fields'], 0)
        self.assertFalse(self.w.proofs[0].closed)
        self.w.wait_error, self.w.wait_result = None, WAIT_OBJECT_0
        report = self.runtime.close()
        self.assertTrue(report['closed'])
        self.assertEqual(report['retired_fields'], 2)

    def test_confirmed_exit_remains_bound_to_original_after_pid_reuse(self):
        self.runtime.tick()
        original = copy.deepcopy(self.w.identity)
        self.w.identity, self.w.wait_result = None, WAIT_OBJECT_0
        self.w.advance(self.runtime)
        self.assertTrue(self.runtime.report['lifetime']['exit_confirmed'])
        self.assertEqual(self.runtime.report['retired_fields'], 2)
        self.w.identity = dict(original, creation_time=11, image='synthetic-reused-pid')
        report = self.runtime.close()
        self.assertTrue(report['closed'])
        self.assertEqual(report['applications'], 1)
        self.assertEqual(self.w.factory_calls, 1)
        self.assertEqual(self.w.proofs[0].identity, original)

    def test_lifetime_open_failure_prevents_first_write_and_can_retry(self):
        self.w.lifetime_open_error = True
        self.runtime.tick()
        self.assertFalse(self.runtime.report['applied'])
        self.assertEqual(self.runtime.report['writes'], 0)
        self.assertEqual(self.runtime.report['applications'], 0)
        self.assertFalse(self.runtime.report['lifetime']['available'])
        self.assertIn('lifetime handle unavailable', self.runtime.report['last_error']['reason'])
        self.w.lifetime_open_error = False
        self.w.advance(self.runtime)
        self.assertTrue(self.runtime.report['applied'])
        self.assertEqual(len(self.w.proofs), 1)
        self.assertTrue(self.runtime.close()['closed'])

    def test_lifetime_close_failure_reports_unclosed_and_retries_without_reader_reuse(self):
        self.runtime.tick()
        self.w.lifetime_close_error = True
        report = self.runtime.close()
        self.assertTrue(report['released'])
        self.assertFalse(report['closed'])
        self.assertTrue(self.w.readers[0].closed)
        self.assertFalse(self.w.proofs[0].closed)
        self.assertIn('lifetime close failure', report['last_error']['reason'])
        self.w.lifetime_close_error = False
        self.assertTrue(self.runtime.close()['closed'])
        self.assertEqual(self.w.events.count(('reader_close', 0)), 1)
        self.assertEqual(self.w.events.count(('lifetime_close',)), 1)

    def test_factory_cannot_close_active_reader_by_returning_it_as_fresh(self):
        self.runtime.tick()
        self.w.context = ('pawn2', 'modifier1')
        old = self.w.instance.plan.sampler
        self.runtime._factory = lambda *_: old
        self.w.advance(self.runtime)
        self.assertFalse(old.r.p.closed)
        self.assertFalse(self.runtime.report['released'])
        self.assertIn('reused the active', self.runtime.report['last_error']['reason'])

    def test_failed_fresh_prepare_after_release_preserves_responsibility_until_retry(self):
        self.runtime.tick()
        self.w.context = ('pawn2', 'modifier1')
        self.w.fail_after_release = True
        self.w.advance(self.runtime)
        self.assertFalse(self.w.readers[0].closed)
        self.assertTrue(self.w.readers[1].closed)
        self.w.fail_after_release = False
        self.w.advance(self.runtime)
        self.assertTrue(self.runtime.report['applied'])
        self.assertTrue(self.w.readers[0].closed)
        self.assertEqual(self.runtime.report['restored'], 2)

    def test_recovered_old_context_during_rebind_keeps_original_certificate(self):
        self.runtime.tick()
        self.w.context = ('pawn2', 'modifier1')
        self.w.recover_on_rebind = True
        self.w.advance(self.runtime)
        self.assertTrue(self.runtime.report['applied'])
        self.assertFalse(self.w.readers[0].closed)
        self.assertTrue(self.w.readers[1].closed)
        self.assertEqual(self.runtime.report['applications'], 1)
        self.assertTrue(self.runtime.close()['closed'])

    def test_unavailable_fresh_context_preserves_old_reader_and_pending_ownership(self):
        self.runtime.tick()
        self.w.context = ('pawn2', 'modifier1')
        self.w.available = False
        self.w.advance(self.runtime)
        self.assertFalse(self.w.readers[0].closed)
        self.assertFalse(self.runtime.report['released'])
        self.assertEqual(self.runtime.report['pending_fields'], ['ambient', 'roadie'])
        self.assertEqual(self.runtime.report['applications'], 1)
        self.w.available = True
        self.w.advance(self.runtime)
        self.assertEqual(self.runtime.report['applications'], 2)

    def test_real_policy_reader_lifetime_pawn_rebind_and_unknown_identity(self):
        # The existing fully synthetic reflection fixture exercises the actual
        # Policy and its certificates, not just the lifecycle fake above.
        import run_camera_policy as policy_module
        from test_run_camera_policy import Fixture
        fixture = Fixture()
        current, readers = [fixture.sampler], []

        def factory(pid, _path):
            self.assertEqual(pid, fixture.r.pid)
            reader = Reader(self.w, len(readers))
            readers.append(reader)
            resolver = copy.copy(fixture.r)
            resolver.p = reader
            original_read = resolver.read

            def read(address, size):
                self.assertFalse(reader.closed, 'Real certificate used a closed reader')
                return original_read(address, size)

            resolver.read = read
            sampler = copy.copy(current[0])
            sampler.r = resolver
            return sampler

        with patch.object(policy_module, 'BOOL_CODE', fixture.code_cert):
            runtime = RuntimePolicy(fixture.r.pid, fixture.checker, clock=lambda: self.w.now,
                                    sampler_factory=factory,
                                    policy_factory=lambda pid, checker: policy_module.Policy(
                                        pid, checker, writer=fixture.writer),
                                    lifetime_factory=lambda _pid, identity: Lifetime(self.w, identity))
            runtime.tick()
            self.assertTrue(runtime.report['applied'])
            self.assertFalse(readers[0].closed)
            current[0] = fixture.new_world()
            self.w.advance(runtime)
            self.assertTrue(runtime.report['applied'])
            self.assertEqual(runtime.report['applications'], 2)
            self.assertEqual(runtime.report['restored'], 2)
            self.assertTrue(readers[0].closed)
            self.assertFalse(readers[1].closed)
            identity, fixture.identity = fixture.identity, None
            report = runtime.close()
            self.assertFalse(report['released'])
            self.assertFalse(report['closed'])
            self.assertEqual(report['retired_fields'], 0)
            self.assertFalse(readers[1].closed)
            fixture.identity = identity
            report = runtime.close()
            self.assertTrue(report['closed'])
            self.assertEqual(report['restored'], 4)
            self.assertTrue(readers[1].closed)
            self.assertEqual(fixture.r.u32(fixture.ambient_address), 0xa0)
            self.assertEqual(fixture.r.read(fixture.roadie_address, 4), b'\x9a\x99\x19?')

            # The actual Policy must also retire its journal when the private
            # lifetime receipt proves exit while the ordinary checker is None.
            runtime = RuntimePolicy(fixture.r.pid, fixture.checker, clock=lambda: self.w.now,
                                    sampler_factory=factory,
                                    policy_factory=lambda pid, checker: policy_module.Policy(
                                        pid, checker, writer=fixture.writer),
                                    lifetime_factory=lambda _pid, identity: Lifetime(self.w, identity))
            runtime.tick()
            writes = len(fixture.writer.calls)
            fixture.identity, self.w.wait_result = None, WAIT_OBJECT_0
            report = runtime.close()
            self.assertTrue(report['closed'])
            self.assertEqual(report['retired_fields'], 2)
            self.assertEqual(report['restored'], 0)
            self.assertEqual(len(fixture.writer.calls), writes)
            self.assertTrue(readers[-1].closed)

    def test_programming_errors_propagate_without_discarding_reader(self):
        self.runtime.tick()
        self.w.instance.check = lambda: (_ for _ in ()).throw(AssertionError('synthetic bug'))
        with self.assertRaisesRegex(AssertionError, 'synthetic bug'):
            self.w.advance(self.runtime)
        self.assertFalse(self.w.readers[0].closed)
        self.assertFalse(self.w.instance.closed)


if __name__ == '__main__':
    unittest.main()
