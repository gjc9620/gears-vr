import pathlib
import tempfile
import unittest
from unittest import mock

import steam_account as account


FIRST = '76561197960266962'
SECOND = '76561197960266963'


def metadata(first='"AutoLogin" "1" "WantsOfflineMode" "1"',
             second='"AutoLogin" "0" "WantsOfflineMode" "0"'):
    return ('"users" { "' + FIRST + '" {' + first + '} "' + SECOND + '" {' + second + '} }').encode()


class SteamAccountTests(unittest.TestCase):
    def test_current_and_older_offline_selection_without_names_or_timestamps(self):
        for marker in ('AutoLogin', 'MostRecent'):
            raw = metadata().replace(b'AutoLogin', marker.encode())
            self.assertEqual(account.offline_selection(raw), FIRST)
        self.assertEqual(account.offline_selection(metadata(
            '"AutoLogin" "1" "MostRecent" "1" "WantsOfflineMode" "1"',
            '"AutoLogin" "0" "MostRecent" "0" "WantsOfflineMode" "0"')), FIRST)

    def test_ambiguous_online_missing_and_conflicting_markers_refused(self):
        for raw in (metadata(second='"AutoLogin" "1" "WantsOfflineMode" "1"'),
                    metadata(first='"AutoLogin" "0" "WantsOfflineMode" "1"'),
                    metadata(first='"AutoLogin" "1" "WantsOfflineMode" "0"'),
                    metadata(first='"Timestamp" "9999" "WantsOfflineMode" "1"'),
                    metadata(first='"AutoLogin" "yes" "WantsOfflineMode" "1"'),
                    metadata(first='"AutoLogin" "1" "MostRecent" "0" "WantsOfflineMode" "1"',
                             second='"AutoLogin" "0" "MostRecent" "1" "WantsOfflineMode" "1"'),
                    metadata().replace(FIRST.encode(), b'99999999999999999')):
            with self.subTest(raw=raw):
                self.assertIsNone(account.offline_selection(raw))

    def test_strict_parser_rejects_corrupt_and_oversized_metadata(self):
        for raw in (metadata()[:-1], metadata() + b'}', b'users { }',
                    metadata(first='"AutoLogin" "1" "AutoLogin" "1"'),
                    b'x' * (1024 * 1024 + 1), b'"users" {"x" {"x" {"x" {"x" {"x" {}}}}}}'):
            with self.subTest(raw=raw[:80]), self.assertRaises(ValueError):
                account.offline_selection(raw)

    def test_names_comments_and_escaped_quotes_do_not_change_selection(self):
        raw = b'// comment\n' + metadata(first='"AutoLogin" "1" "WantsOfflineMode" "1" "PersonaName" "a\\\"b"')
        self.assertEqual(account.offline_selection(raw), FIRST)

    def test_live_client_and_unchanged_selection_required_without_writes(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = pathlib.Path(directory).resolve()
            path = folder / 'config/loginusers.vdf'
            path.parent.mkdir()
            raw = metadata()
            path.write_bytes(raw)
            state = (0, 123, folder)
            identity = dict(pid=123, creation_time=45, image=str(folder / 'steam.exe').casefold())
            cases = (([state, state], [identity, identity], FIRST),
                     ([(1, 123, folder)], [], None),
                     ([state, (1, 123, folder)], [identity], None),
                     ([state, state], [identity, dict(identity, creation_time=46)], None),
                     ([state], [dict(identity, image=str(folder / 'other.exe'))], None),
                     ([state], [None], None))
            for states, identities, expected in cases:
                with mock.patch.object(account, 'registry_state', side_effect=states), \
                        mock.patch.object(account.instance_policy, 'process_identity', side_effect=identities):
                    self.assertEqual(account.offline_steam_id(), expected)
                self.assertEqual(path.read_bytes(), raw)

    def test_client_metadata_change_during_read_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = pathlib.Path(directory).resolve()
            path = folder / 'config/loginusers.vdf'
            path.parent.mkdir()
            path.write_bytes(metadata())
            identity = dict(pid=123, creation_time=45, image=str(folder / 'steam.exe').casefold())
            def changed(raw):
                path.write_bytes(metadata(second='"AutoLogin" "1"'))
                return FIRST
            with mock.patch.object(account, 'registry_state', return_value=(0, 123, folder)), \
                    mock.patch.object(account.instance_policy, 'process_identity', return_value=identity), \
                    mock.patch.object(account, 'offline_selection', side_effect=changed):
                self.assertIsNone(account.offline_steam_id())


if __name__ == '__main__':
    unittest.main()
