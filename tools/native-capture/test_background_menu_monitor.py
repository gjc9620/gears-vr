"""Deterministic single-worker scheduling and real blocked-reader ownership."""
from concurrent.futures import Future
import threading
import time
import unittest

from menu_display_monitor import BackgroundMenuDisplayMonitor,UNKNOWN,GAMEPLAY,PAUSE_MENU


class Executor:
    def __init__(self,**kwargs):
        self.options=kwargs;self.jobs=[];self.stopped=False
    def submit(self,fn):
        future=Future();self.jobs.append((fn,future));return future
    def step(self):
        fn,future=self.jobs.pop(0)
        try:future.set_result(fn())
        except BaseException as error:future.set_exception(error)
    def shutdown(self,wait):self.stopped=wait


class Monitor:
    def __init__(self):self.last=None;self.last_menu_mode=UNKNOWN;self.calls=[];self.error=None
    def poll(self):
        self.calls.append(('poll',threading.get_ident()))
        if self.error:raise self.error
        self.last=('observed',);self.last_menu_mode=GAMEPLAY
    def close(self):self.calls.append(('close',threading.get_ident()))


class BackgroundMenuTests(unittest.TestCase):
    def test_one_pending_job_owner_never_waits_or_reads_partial_state(self):
        menu=Monitor();executor=Executor();now=[0.]
        worker=BackgroundMenuDisplayMonitor(menu,clock=lambda:now[0],executor_factory=lambda **_:executor)
        worker.poll()
        for _ in range(50):worker.poll()
        self.assertEqual(len(executor.jobs),1);self.assertIsNone(worker.last)
        executor.step();self.assertIsNone(worker.last)
        worker.poll();self.assertEqual(worker.last_menu_mode,GAMEPLAY)
        self.assertEqual(worker.summary()['completed'],1)
        self.assertEqual(executor.jobs,[])
        now[0]=.1;worker.poll();self.assertEqual(len(executor.jobs),1)

    def test_cadence_starts_at_completion_and_keeps_real_observation_state(self):
        menu=Monitor();executor=Executor();now=[0.]
        def sample():
            now[0]=.2;menu.last=('pause',);menu.last_menu_mode=PAUSE_MENU
        menu.poll=sample
        worker=BackgroundMenuDisplayMonitor(menu,clock=lambda:now[0],executor_factory=lambda **_:executor)
        worker.poll();executor.step();worker.poll()
        self.assertEqual(worker.last,('pause',));self.assertAlmostEqual(worker.next_poll,.3)
        self.assertEqual(worker.summary()['maximum_ms'],200.)
        now[0]=.29;worker.poll();self.assertEqual(executor.jobs,[])
        now[0]=.31;worker.poll();self.assertEqual(len(executor.jobs),1)

    def test_worker_error_is_delivered_and_never_rescheduled(self):
        menu=Monitor();menu.error=RuntimeError('Identity changed');executor=Executor()
        worker=BackgroundMenuDisplayMonitor(menu,executor_factory=lambda **_:executor)
        worker.poll();executor.step()
        for _ in range(2):
            with self.assertRaisesRegex(RuntimeError,'Identity changed'):worker.poll()
        self.assertEqual(worker.summary()['failure'],'RuntimeError');self.assertEqual(executor.jobs,[])

    def test_close_before_start_and_repeated_close_do_not_create_worker(self):
        menu=Monitor();worker=BackgroundMenuDisplayMonitor(menu,executor_factory=lambda **_:self.fail('Unexpected worker'))
        worker.close();worker.close();worker.poll()
        self.assertEqual([kind for kind,_ in menu.calls],['close']);self.assertTrue(worker.summary()['closed'])

    def test_blocked_read_stays_off_owner_and_close_joins_before_unknown(self):
        menu=Monitor();entered=threading.Event();release=threading.Event();closing=threading.Event();closed=threading.Event()
        original=menu.poll
        def sample():
            entered.set();release.wait(2);original()
        menu.poll=sample;worker=BackgroundMenuDisplayMonitor(menu)
        worker.poll();self.assertTrue(entered.wait(1))
        self.assertIsNone(worker.last)
        for _ in range(30):worker.poll()
        def close():
            closing.set();worker.close();closed.set()
        thread=threading.Thread(target=close);thread.start();self.assertTrue(closing.wait(1))
        self.assertFalse(closed.is_set());release.set();thread.join(2)
        self.assertFalse(thread.is_alive());self.assertTrue(closed.is_set())
        self.assertEqual([kind for kind,_ in menu.calls],['poll','close'])
        self.assertEqual(menu.calls[0][1],menu.calls[1][1]);self.assertNotEqual(menu.calls[0][1],threading.get_ident())
        self.assertEqual(worker.summary()['completed'],1)

    def test_close_still_releases_reader_after_uncollected_worker_error(self):
        menu=Monitor();menu.error=RuntimeError('Sample failed');worker=BackgroundMenuDisplayMonitor(menu)
        worker.poll()
        with self.assertRaisesRegex(RuntimeError,'Sample failed'):worker.close()
        self.assertEqual([kind for kind,_ in menu.calls],['poll','close'])
        worker.close();self.assertTrue(worker.summary()['closed'])

    def test_invalid_interval(self):
        for interval in (0,-1,float('nan'),float('inf'),.501):
            with self.subTest(interval=interval),self.assertRaises(ValueError):
                BackgroundMenuDisplayMonitor(Monitor(),interval=interval)


if __name__=='__main__':unittest.main()
