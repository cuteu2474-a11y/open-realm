# ORDR Season 2 2.323[C] → OpenRealm iPad port

Checkpoint: 2026-10-16. **NOT a playable iPad ORDR release.**

## Verified using the private map
- The user-supplied W3X contains war3map.j (2,503,677 bytes). It was extracted successfully using OpenRealm common/mpq.c.
- Linux standalone JASS interpreter built, and the *full extracted ORDR script* passed parsing.
- The standalone script then reaches an expected missing host native (InitHashtable). This does NOT prove that the complete Warcraft game host can run the map.
- The W3X contains both TOC files and their FDF assets: war3mapImported/ORDRTemplates.toc → ORDR.fdf, UI/LoginDialog.toc → LoginDialog.fdf and SaveSelectDialog.fdf.

## Source and CI
- JASS lexer escaped-quote handling and comment scanning corrected.
- EXPauseUnit, EXSetUnitFacing, DzLoadToc, and four JN string natives have registrations. This is 7 of 43 extension declarations, not runtime conformance.
- The anchored-regex counter no longer interprets a repeated suffix as the start of the original string; Unicode Korean login input class is handled separately.
- GitHub Actions run 38839517765: jass-regression SUCCESS, compile-ipad SUCCESS.
- Diagnostic unsigned ZIP artifact: openrealm-ipad-unsigned, artifact ID 12020961218. It is NOT directly installable on an iPad.
- Repository CI's optional EOS release job requires credentials and can fail independently of this port.
- No private W3X, purchased game data, login credentials, or pairing files are in the repository.

## Remaining blockers
1. Test map loading in the actual game host using legitimately sourced Warcraft III data; log the first runtime failure.
2. Implement functional DzFrame dynamic UI creation, layout, rendering, touch input, and UI callbacks. Do not claim successful compatibility with inert stubs.
3. Implement local sync events where faithful for single-player and investigate online-only JN account/save services without bypassing authentication.
4. Test units and first wave with a real game session, then iPad input and performance.
5. Sign and provision the iPad app with authorized Apple credentials. Unsigned build success is not game playability.

Keep the map and commercial Warcraft assets outside the public repository.
