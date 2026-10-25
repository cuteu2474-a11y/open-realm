#!/usr/bin/env python3
"""Fetch user-owned Warcraft III test data from private Google Drive.

The only credential is a short-lived Google OAuth access token supplied as
a GitHub Actions secret. Never commit or upload downloaded game assets.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
from urllib.parse import urlencode, quote
from urllib.request import Request, urlopen

# SHA-256 hashes confirmed against this user's files on 2026-10-26.
EXPECTED = {
    'War3.mpq': ('b08095c003f657fdc3d82a2f5c1cd1abc8fd4a7c27d428a225a48489012f0e29', 557726676),
    'War3x.mpq': ('37e9ffbef26763f1e1d0960d8761210f88eed3204a37340a6f96f491cb89dda7', 381604857),
    'War3Local.mpq': ('d8d306fc341b61e4b7a09d0bc1c5ddf59f3586f017cd43414d240f45eed5ed45', 620716),
    'War3xLocal.mpq': ('40ca45cdee3aca0a3056477f822642e58e3f541caba6eea0d88e4306fced71df', 66809059),
    'War3Patch.mpq': ('9dded3dea7fb801b7665a56b1b37560e85c4226891bcb26dd05397eee774fac0', 524476),
    'ORDR_S2_2.323[C].w3x': ('0794996c9bc73eaa3020f20ebc45fb37cfa64a6e98ffe8aa0dfa3b356e77bc5c', 133379181),
}
MAP_NAME = 'ORDR_S2_2.323[C].w3x'


def api_json(token, path, query):
    url = 'https://www.googleapis.com/drive/v3/' + path + '?' + urlencode(query)
    request = Request(url, headers={'Authorization': 'Bearer ' + token,
                                    'Accept': 'application/json'})
    with urlopen(request, timeout=120) as response:
        return json.load(response)


def locate(token, name, size):
    q = "name = '" + name.replace("'", "\\'") + "' and trashed = false"
    next_page = None
    candidates = []
    while True:
        params = {'q': q, 'fields': 'nextPageToken,files(id,name,size,mimeType)',
                  'pageSize': 100, 'spaces': 'drive'}
        if next_page:
            params['pageToken'] = next_page
        result = api_json(token, 'files', params)
        candidates.extend(f for f in result.get('files', [])
                          if f.get('name') == name and int(f.get('size', -1)) == size)
        next_page = result.get('nextPageToken')
        if not next_page:
            break
    if len(candidates) > 1:
        raise RuntimeError('Ambiguous file with matching name and size: ' + name)
    return candidates[0]['id'] if candidates else None


def retrieve(token, file_id, target, expected_size, expected_sha256):
    target.parent.mkdir(parents=True, exist_ok=True)
    tmp = target.with_suffix(target.suffix + '.partial')
    digest = hashlib.sha256()
    count = 0
    request = Request('https://www.googleapis.com/drive/v3/files/' + quote(file_id, safe='') + '?alt=media',
                      headers={'Authorization': 'Bearer ' + token})
    try:
        with urlopen(request, timeout=180) as response, tmp.open('wb') as output:
            while True:
                chunk = response.read(4 * 1024 * 1024)
                if not chunk:
                    break
                output.write(chunk)
                digest.update(chunk)
                count += len(chunk)
        if count != expected_size or digest.hexdigest() != expected_sha256:
            raise RuntimeError('Private Drive data integrity mismatch: ' + target.name)
        tmp.replace(target)
    finally:
        if tmp.exists():
            tmp.unlink()
    return {'file': target.name, 'size': count, 'sha256': digest.hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--destination', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    token = os.environ.get('ORDR_DRIVE_ACCESS_TOKEN', '')
    if not token:
        parser.error('ORDR_DRIVE_ACCESS_TOKEN secret is required')
    results = []
    missing = []
    for name, (sha256, size) in EXPECTED.items():
        file_id = locate(token, name, size)
        if not file_id:
            missing.append(name)
            if name != MAP_NAME:
                raise RuntimeError('Required MPQ absent from authorized Google Drive: ' + name)
            continue
        destination = args.destination / ('Maps' if name == MAP_NAME else '') / name
        results.append(retrieve(token, file_id, destination, size, sha256))
        print('SHA-256 verified:', name, size, 'bytes', flush=True)
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps({'verified_files': results, 'missing': missing}, indent=2))
    if MAP_NAME in missing:
        print('ORDR map is not in this Drive. Real map playback is BLOCKED.', flush=True)
    else:
        print('All required private test files verified and staged.', flush=True)


if __name__ == '__main__':
    main()
