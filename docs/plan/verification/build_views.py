#!/usr/bin/env python3
"""Build focused views from the canonical report; --check detects drift."""
from pathlib import Path
import argparse
import re

ROOT = Path(__file__).resolve().parents[1]
VIEWS = {
    'literature-survey.md': ('Literature and software survey', [3, 10]),
    'engine-specification.md': ('Mathematical specification and contracts', [4, 5, 6]),
    'implementation-plan.md': ('Implementation plan and decisions', [1, 2, 9]),
    'phasing-plan.md': ('Phases, validation and experiments', [7, 8, 11]),
}


def build(check=False):
    source = (ROOT/'research-and-plans.md').read_text()
    matches = list(re.finditer(r'<a id="(\d+)-[^\"]+"></a>\n\n## ', source))
    sections = {int(m.group(1)): source[m.start():matches[j+1].start() if j+1 < len(matches) else len(source)].rstrip()
                for j, m in enumerate(matches)}
    for filename, (title, numbers) in VIEWS.items():
        content = (f'# {title}\n\n'
                   '> Generated view of [research-and-plans.md](research-and-plans.md), the canonical source. '
                   'Edit the report and run `python docs/plan/verification/build_views.py`. '
                   'R/S IDs refer to the complete report’s bibliography.\n\n'
                   + '\n\n'.join(sections[n] for n in numbers) + '\n')
        path = ROOT/filename
        if check:
            if not path.exists() or path.read_text() != content:
                raise ValueError(f'Generated view out of date: {filename}')
        else:
            path.write_text(content)
    return len(VIEWS)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    print(f'{build(args.check)} focused views verified' if args.check else f'{build()} focused views generated')
