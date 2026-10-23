"""Fresh module membership with bounded, explicitly selected name queries."""
import ctypes
from unittest import mock
import unittest

import start_capture as start
from hook_entry_attestation import WindowsReadOnly


class ModuleEnumerationTests(unittest.TestCase):
    def setUp(self):
        self.paths={0x10000:r'J:\game\GOWDE-Steam.exe',0x20000:r'C:\Windows\System32\kernel32.dll',
                    0x30000:r'J:\mod\GearsCapture.dll'}
        def enumerate_modules(handle,slots,size,needed,flags):
            for index,value in enumerate(self.paths):slots[index]=value
            needed._obj.value=len(self.paths)*ctypes.sizeof(start.w.HMODULE)
            return True
        def filename(handle,address,buffer,capacity):
            buffer.value=self.paths[address];return len(buffer.value)
        self.enumerate=mock.patch.object(start.psapi,'EnumProcessModulesEx',side_effect=enumerate_modules)
        self.filename=mock.patch.object(start.psapi,'GetModuleFileNameExW',side_effect=filename)

    def test_default_and_bound_results_match_for_the_selected_modules(self):
        with self.enumerate as enum,self.filename as names:
            full=start.modules(99)
            names.reset_mock()
            selected=start.modules(99,bases=(0x10000,0x30000))
        self.assertEqual(selected,{name:item for name,item in full.items()if item[0]in(0x10000,0x30000)})
        self.assertEqual(enum.call_count,2)
        self.assertEqual([call.args[1]for call in names.call_args_list],[0x10000,0x30000])

    def test_missing_or_unloaded_selected_base_is_never_synthesized(self):
        with self.enumerate,self.filename as names:
            first=start.modules(99,bases=(0x10000,0x30000))
            del self.paths[0x30000]
            second=start.modules(99,bases=(0x10000,0x30000))
        self.assertIn('gearscapture.dll',first);self.assertNotIn('gearscapture.dll',second)
        self.assertEqual(len(names.call_args_list),3)

    def test_selected_filename_query_failure_does_not_reuse_old_path(self):
        with self.enumerate,self.filename as names:
            self.assertIn('gearscapture.dll',start.modules(99,bases=(0x30000,)))
            names.side_effect=lambda *_:0
            self.assertEqual(start.modules(99,bases=(0x30000,)),{})

    def test_enum_failure_and_oversized_array_are_terminal(self):
        for mode in ('failure','oversized'):
            def enumeration(handle,slots,size,needed,flags):
                needed._obj.value=size+ctypes.sizeof(start.w.HMODULE)
                return mode!='failure'
            with self.subTest(mode=mode),mock.patch.object(start.psapi,'EnumProcessModulesEx',side_effect=enumeration):
                with self.assertRaises((OSError,RuntimeError)):start.modules(99,bases=(0x10000,))

    def test_invalid_selection_is_rejected_before_any_process_query(self):
        for selected in ((),(0,),(-1,),(True,),('0x10000',),(1<<64,)):
            with self.subTest(selected=selected),self.enumerate as enum:
                with self.assertRaises(ValueError):start.modules(99,bases=selected)
                enum.assert_not_called()

    def test_readonly_backend_passes_only_bound_bases_and_uses_live_handle(self):
        backend=WindowsReadOnly.__new__(WindowsReadOnly);backend.process=mock.Mock(handle=123)
        backend.module_bases=frozenset((0x10000,0x30000))
        with mock.patch.object(start,'modules',return_value={})as modules:
            self.assertEqual(backend.modules(),{})
        modules.assert_called_once_with(123,bases=backend.module_bases)


if __name__=='__main__':unittest.main()
