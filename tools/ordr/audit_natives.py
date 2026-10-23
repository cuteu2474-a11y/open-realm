#!/usr/bin/env python3
"""Conservative ORDR native inventory. Nonempty bodies are NOT proof of parity."""
import argparse
from collections import Counter
import hashlib
import json
import re
from pathlib import Path


def mask(source):
    pattern = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/'
    return re.sub(pattern, lambda m: re.sub(r'[^\n]', ' ', m[0]), source)


def functions(source, language='c'):
    clean = mask(source)
    result = {}
    if language == 'jass':
        for m in re.finditer(r'\bfunction\s+(\w+)\s+takes\b[\s\S]*?\bendfunction\b', clean):
            result[m[1]] = clean[m.start():m.end()]
        return result
    for m in re.finditer(r'\buint32_t\s+(\w+)\s*\(\s*jass_t\s*\*\s*j\s*\)\s*\{', clean):
        end, depth = m.end(), 1
        while end < len(clean) and depth:
            depth += (clean[end] == '{') - (clean[end] == '}')
            end += 1
        if depth:
            raise ValueError('unbalanced native: ' + m[1])
        result[m[1]] = (source[m.end():end-1], source.count('\n', 0, m.start()) + 1)
    return result


def placeholder(body):
    clean = mask(body)
    clean = re.sub(r'\(void\)\s*(?:jass_check\w+\([^;]*\)|\w+)\s*;', '', clean)
    clean = re.sub(r'\b(?:const\s+)?[\w]+(?:\s+const)?\s*\*?\s*\w+\s*=\s*jass_check\w+\([^;]*\);', '', clean)
    clean = re.sub(r'\s+', '', clean)
    return bool(re.fullmatch(r'return(?:0|jass_push(?:integer|number|boolean)\(j,(?:0|false)\)|jass_pushnullhandle\(j,\))\s*;', clean))


def calls(source):
    clean = mask(source)
    return set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', clean)) | set(re.findall(r'\bfunction\s+(\w+)(?!\w)', clean))


def audit(repo, script, supports):
    src = script.read_text(encoding='utf-8-sig')
    sources = [(script.name, src)] + [(p.name, p.read_text(encoding='utf-8-sig')) for p in supports]
    sources.append(('common.txt', (repo / 'games/warcraft-3/game/common.txt').read_text(encoding='utf-8-sig')))
    declared, definitions = set(), {}
    for _, content in sources:
        declared.update(re.findall(r'\bnative\s+(\w+)', mask(content)))
        definitions.update(functions(content, 'jass'))
    registry_source = (repo / 'games/warcraft-3/game/api/api_module.c').read_text()
    registry = set(re.findall(r'\{\s*"(\w+)"\s*,', registry_source))
    implementations = {}
    for p in sorted((repo / 'games/warcraft-3/game/api').glob('*.h')):
        for name, (body, line) in functions(p.read_text()).items():
            implementations[name] = {'path': str(p.relative_to(repo)), 'line': line,
                                     'status': 'placeholder' if placeholder(body) else 'needs_review'}
    direct = calls(src)
    direct_counts = Counter(re.findall(r'\b([A-Za-z_]\w*)\s*\(', mask(src)))
    reachable, pending = set(), list(direct)
    while pending:
        name = pending.pop()
        if name in reachable:
            continue
        reachable.add(name)
        if name in definitions:
            pending.extend(calls(definitions[name]) - reachable)
    native_names = reachable & (registry | declared | implementations.keys())
    rows = []
    for name in sorted(native_names):
        rows.append({'name': name, 'registered': name in registry,
                     'direct_calls': direct_counts[name],
                     **implementations.get(name, {'status': 'needs_review' if name in registry else 'missing'})})
    keywords = {'if', 'elseif', 'loop', 'return', 'and', 'or', 'not'}
    return {'schema': 1, 'method': 'Static call closure; nonempty/registered does not mean implemented or correct.',
            'sources': [{'name': n, 'normalized_sha256': hashlib.sha256(c.encode()).hexdigest()} for n,c in sources],
            'registered': len(registry), 'direct_native_count': len(direct & native_names),
            'referenced_native_count': len(native_names),
            'placeholder_count': sum(r['status'] == 'placeholder' for r in rows),
            'missing_declared_natives': sorted((reachable & declared) - registry),
            'unresolved_calls': sorted(reachable - definitions.keys() - registry - implementations.keys() - declared - keywords),
            'natives': rows}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('script', type=Path)
    parser.add_argument('--support', action='append', type=Path, default=[])
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = json.dumps(audit(args.repo, args.script, args.support), ensure_ascii=False, indent=2) + '\n'
    if args.output:
        args.output.write_text(result)
    else:
        print(result, end='')

if __name__ == '__main__':
    main()
