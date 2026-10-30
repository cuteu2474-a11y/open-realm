import importlib.util
import tempfile
import unittest
from pathlib import Path
spec = importlib.util.spec_from_file_location('audit', Path(__file__).resolve().parents[1] / 'tools/ordr/audit_natives.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

class NativeAuditTest(unittest.TestCase):
    def test_placeholders(self):
        for body in ['// pending\n return 0;', 'return jass_pushinteger(j, 0);', 'return jass_pushnullhandle(j, "unit");', 'float x = jass_checknumber(j, 1); (void)x; return 0;']:
            self.assertTrue(audit.placeholder(body), body)
        for body in ['(void)jass_startcoroutinebyname(j, name); return 0;', 'unit->value = 0; return 0;', 'return jass_pushinteger(j, level.value);']:
            self.assertFalse(audit.placeholder(body), body)

    def test_balanced_body(self):
        src = 'uint32_t Foo(jass_t *j) { if (x) { log("}"); } return 0; }\nuint32_t Bar(jass_t *j) { return 0; }'
        found = audit.functions(src)
        self.assertEqual(set(found), {'Foo', 'Bar'})
        self.assertFalse(audit.placeholder(found['Foo'][0]))
        self.assertTrue(audit.placeholder(found['Bar'][0]))

    def test_calls_ignore_literals_and_follow_callbacks(self):
        src = '// Fake()\ncall Foo("Bogus()")\ncall Condition(function Callback)'
        self.assertEqual(audit.calls(src), {'Foo', 'Condition', 'Callback'})

    def test_support_wrapper_reaches_unregistered_standard_native(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            api = root / 'games/warcraft-3/game/api'
            api.mkdir(parents=True)
            (api / 'api_module.c').write_text('{ "Known", Known },')
            (api / 'api_test.h').write_text('uint32_t Known(jass_t *j) { return 0; }')
            (api.parent / 'common.txt').write_text('native Missing takes nothing returns nothing')
            script = root / 'war3map.j'
            script.write_text('function main takes nothing returns nothing\ncall Wrapper()\nendfunction')
            support = root / 'Blizzard.j'
            support.write_text('function Wrapper takes nothing returns nothing\ncall Missing()\nendfunction')
            result = audit.audit(root, script, [support])
            self.assertEqual(result['missing_declared_natives'], ['Missing'])
            self.assertEqual(result['unresolved_calls'], [])
            self.assertEqual(result['natives'][0]['status'], 'missing')

if __name__ == '__main__':
    unittest.main()
