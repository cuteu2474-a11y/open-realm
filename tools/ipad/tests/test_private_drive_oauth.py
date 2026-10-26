"""No real credentials, Google requests, or downloaded game assets."""
import importlib.util
import io
import json
import os
from pathlib import Path
import unittest
from unittest import mock
from urllib.error import HTTPError

path = Path(__file__).resolve().parents[1] / 'fetch_private_drive_assets.py'
spec = importlib.util.spec_from_file_location('drive_fetch', path)
drive = importlib.util.module_from_spec(spec)
spec.loader.exec_module(drive)


class DriveAuthTests(unittest.TestCase):
    def test_refresh_posts_client_credentials(self):
        body = io.BytesIO(b'{"access_token":"new-access","token_type":"Bearer"}')
        env = {'ORDR_DRIVE_REFRESH_TOKEN':'refresh-private',
               'ORDR_DRIVE_CLIENT_ID':'client-id',
               'ORDR_DRIVE_CLIENT_SECRET':'client-secret',
               'ORDR_DRIVE_ACCESS_TOKEN':'expired'}
        with mock.patch.dict(os.environ, env, clear=True), mock.patch.object(
                drive, 'urlopen', return_value=body) as open_mock:
            auth = drive.DriveAuth()
        self.assertEqual(auth.token, 'new-access')
        request = open_mock.call_args.args[0]
        self.assertEqual(request.full_url, 'https://oauth2.googleapis.com/token')
        self.assertEqual(request.get_method(), 'POST')
        from urllib.parse import parse_qs
        params = parse_qs(request.data.decode('utf-8'))
        self.assertEqual(params['grant_type'], ['refresh_token'])
        self.assertEqual(params['client_id'], ['client-id'])
        self.assertEqual(params['client_secret'], ['client-secret'])
        self.assertEqual(params['refresh_token'], ['refresh-private'])

    def test_legacy_access_token_supported(self):
        with mock.patch.dict(os.environ,
                {'ORDR_DRIVE_ACCESS_TOKEN':'legacy'}, clear=True):
            self.assertEqual(drive.DriveAuth().token, 'legacy')

    def test_partial_refresh_config_never_falls_back_silently(self):
        with mock.patch.dict(os.environ, {
            'ORDR_DRIVE_REFRESH_TOKEN':'refresh',
            'ORDR_DRIVE_ACCESS_TOKEN':'old'
        }, clear=True):
            with self.assertRaisesRegex(RuntimeError, 'requires'):
                drive.DriveAuth()

    def test_missing_credentials_fails(self):
        with mock.patch.dict(os.environ, {}, clear=True):
            with self.assertRaisesRegex(RuntimeError, 'Missing Google Drive'):
                drive.DriveAuth()

    def test_drive_401_refreshes_and_retries(self):
        first = io.BytesIO(b'{"access_token":"A","token_type":"Bearer"}')
        second = io.BytesIO(b'{"access_token":"B","token_type":"Bearer"}')
        unauthorized = HTTPError('https://www.googleapis.com/drive/v3/files',
                                 401, 'Unauthorized', {}, None)
        data = io.BytesIO(b'{"files":[]}')
        with mock.patch.dict(os.environ, {
            'ORDR_DRIVE_REFRESH_TOKEN':'refresh',
            'ORDR_DRIVE_CLIENT_ID':'client',
            'ORDR_DRIVE_CLIENT_SECRET':'secret',
        }, clear=True), mock.patch.object(drive, 'urlopen',
                    side_effect=[first, unauthorized, second, data]) as opened:
            auth = drive.DriveAuth()
            result = drive.api_json(auth, 'files', {'q':'name'})
        self.assertEqual(result, {'files':[]})
        self.assertEqual(opened.call_count, 4)
        self.assertEqual(auth.token, 'B')
        self.assertEqual(opened.call_args.args[0].get_header('Authorization'),
                         'Bearer B')

    def test_rejected_refresh_does_not_log_secrets(self):
        failure = HTTPError('https://oauth2.googleapis.com/token',
                            400, 'Bad Request', {}, None)
        with mock.patch.dict(os.environ, {
            'ORDR_DRIVE_REFRESH_TOKEN':'super-private-refresh',
            'ORDR_DRIVE_CLIENT_ID':'client',
            'ORDR_DRIVE_CLIENT_SECRET':'super-private-secret',
        }, clear=True), mock.patch.object(drive, 'urlopen',
                                          side_effect=failure):
            with self.assertRaises(RuntimeError) as err:
                drive.DriveAuth()
        self.assertIn('HTTP 400', str(err.exception))
        self.assertNotIn('super-private-refresh', str(err.exception))
        self.assertNotIn('super-private-secret', str(err.exception))


if __name__ == '__main__':
    unittest.main()
