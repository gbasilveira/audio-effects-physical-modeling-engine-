#!/usr/bin/env python3
"""Validate phase DAG, component coverage, citations, local links and generated views."""
from pathlib import Path
from collections import Counter
import json
import re
import yaml
from build_views import build

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
source = (ROOT/'research-and-plans.md').read_text()
expected = {f'FX{i:02}' for i in range(1, 33)}
components = re.findall(r'^### (FX\d\d) —', source, re.M)
assert set(components) == expected and len(components) == 32
refs = re.findall(r'^\| ([RS]\d\d) \|', source, re.M)
assert len(refs) == len(set(refs)), 'Duplicate bibliography IDs'
used = set(re.findall(r'\b[RS]\d\d\b', source))
assert used <= set(refs), f'Missing references: {used-set(refs)}'
for prefix, start, end in re.findall(r'\b([RS])(\d\d)–[RS](\d\d)\b', source):
    assert {f'{prefix}{i:02}' for i in range(int(start), int(end)+1)} <= set(refs)
plan = yaml.safe_load((ROOT/'phases.yaml').read_text())
nodes = {p['id']:p for p in plan['phases']}
assert len(nodes) == len(plan['phases']) == 17
owners = Counter(c for p in nodes.values() for c in p['components'])
assert set(owners) == expected and all(v == 1 for v in owners.values())
visited, active = set(), set()
def visit(node):
    assert node in nodes, f'Unknown dependency {node}'
    assert node not in active, f'Cycle at {node}'
    if node in visited:
        return
    active.add(node)
    p = nodes[node]
    assert p['status'] in {'pending','in_progress','completed','blocked'}
    assert p['deliverables'] and p['exit_gate']
    for dep in p['depends_on']:
        visit(dep)
    active.remove(node)
    visited.add(node)
for node in nodes:
    visit(node)
for variant, deps in plan['variant_dependencies'].items():
    assert variant.split('.')[0] in expected
    assert set(deps) <= set(nodes)
# Local markdown links only; external URLs were checked during research, not fetched here.
link_count = 0
for path in [REPO/'README.md', *ROOT.rglob('*.md')]:
    text = path.read_text()
    # Ignore illustrative YAML/code blocks, whose future paths are not links.
    text = re.sub(r'```.*?```', '', text, flags=re.S)
    for link in re.findall(r'!?\[[^\]]*\]\(([^\s)]+)\)', text):
        if '://' in link or link.startswith('mailto:'):
            continue
        target, _, fragment = link.partition('#')
        destination = (path.parent/target).resolve() if target else path
        assert destination.is_file(), f'{path}: missing {link}'
        if fragment:
            assert f'id="{fragment}"' in destination.read_text(), f'{path}: missing anchor {link}'
        link_count += 1
assert source.count('```') % 2 == 0
assert source.count('$$') % 2 == 0
build(check=True)
print(json.dumps({'components':len(components), 'phases':len(nodes), 'source_entries':len(refs),
                  'local_links_checked':link_count, 'dag':'acyclic', 'focused_views':'current'}, ensure_ascii=False))
