# ORDR private Google Drive assets for iPad simulator smoke tests

The public OpenRealm repository and GitHub Actions artifact **must never contain**
licensed Warcraft III MPQs or the ORDR map. The simulator runner downloads them
directly into its temporary simulator app Documents directory from the owner's
private Google Drive, and only exports screenshots, status and sanitized logs.

## One-time file preparation

The connected Drive account already contains the five Warcraft III MPQs
(\`War3.mpq\`, \`War3x.mpq\`, \`War3Local.mpq\`, \`War3xLocal.mpq\`,
\`War3Patch.mpq\`). The original ORDR map was copied from ChatGPT Library into the **private** Drive folder `ORDR_MPQ_분할파일` as eight 16 MiB-or-smaller parts (`ORDR_S2_2.323[C].w3x.part001` through `.part008`). The runner automatically reassembles them in its temporary simulator container and verifies the original map's SHA-256. The downloader checks exact names, sizes and SHA-256 hashes before use.

## Temporary private authorization (required for each run)

1. On Google OAuth 2.0 Playground (https://developers.google.com/oauthplayground/),
   choose \`https://www.googleapis.com/auth/drive.readonly\`, authorize the
   **same Google account**, then exchange the authorization code for an access
   token. Google tokens are short-lived (often one hour).
2. In GitHub repository **Settings → Secrets and variables → Actions** create
   or update an encrypted repository secret named \`ORDR_DRIVE_ACCESS_TOKEN\`
   with that access token. Do NOT put it in an issue, chat, commit or workflow
   input. Updating this secret requires repo owner permission.
3. From Actions run **ORDR iPad simulator smoke** manually against
   \`feature/ordr-singleplayer-compat\` while the token is fresh.

No file IDs or tokens are stored in code or artifacts. File lookup uses the
Drive API and the configured token. A pre-existing
\`ORDR_TEST_ASSETS_URL\` secret may still be used as an alternative.

The workflow performs a **map-launch smoke test** with two snapshots and the
JASS log. This does not automate difficulty selection, touch input, beacon
movement, combat or an entire game session. Those require separate UI-driving
code and success criteria. No licensed game bytes are included in uploaded
diagnostics.
