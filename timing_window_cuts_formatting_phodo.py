#!/usr/bin/env python3
"""Generate phodo_cuts.param from timing_window_setup output."""
import argparse
import re

# The database expects 21 values per plane even though the timing window
# script only prints 13 values for 1x and 1y, 14 for 2x and 21 for 2y.
# Pad each list with zeros so the final arrays contain 21 entries for every
# plane.
PLANE_SIZE = 21


def parse_values(text):
    values = [float(v.strip()) for v in text.split(',') if v.strip()]
    return values

PATTERNS = {
    'pos_min': re.compile(r'^phodo_(1x|1y|2x|2y)_PosAdcTimeWindowMin=\s*(.*)$'),
    'pos_max': re.compile(r'^phodo_(1x|1y|2x|2y)_PosAdcTimeWindowMax=\s*(.*)$'),
    'neg_min': re.compile(r'^phodo_(1x|1y|2x|2y)_NegAdcTimeWindowMin=\s*(.*)$'),
    'neg_max': re.compile(r'^phodo_(1x|1y|2x|2y)_NegAdcTimeWindowMax=\s*(.*)$'),
}

def parse_input(lines):
    data = {k: {} for k in PATTERNS}
    for line in lines:
        line = line.strip()
        for key, pat in PATTERNS.items():
            m = pat.match(line)
            if m:
                plane, nums = m.groups()
                data[key][plane] = parse_values(nums)
                break
    return data

def pad_plane(values):
    """Pad the list of values so each plane has ``PLANE_SIZE`` entries."""
    padded = values + [0.0] * (PLANE_SIZE - len(values))
    return padded[:PLANE_SIZE]


def build_arrays(parsed):
    arrays = {
        'PosAdcTimeWindowMin': [],
        'PosAdcTimeWindowMax': [],
        'NegAdcTimeWindowMin': [],
        'NegAdcTimeWindowMax': [],}

    order = ['1x', '1y', '2x', '2y']
    # Build padded lists for each plane first
    padded = {key: [pad_plane(parsed[key].get(p, [])) for p in order]
              for key in ('pos_min', 'pos_max', 'neg_min', 'neg_max')}

    for i in range(PLANE_SIZE):
        for idx, plane in enumerate(order):
            arrays['PosAdcTimeWindowMin'].append(padded['pos_min'][idx][i])
            arrays['PosAdcTimeWindowMax'].append(padded['pos_max'][idx][i])
            arrays['NegAdcTimeWindowMin'].append(padded['neg_min'][idx][i])
            arrays['NegAdcTimeWindowMax'].append(padded['neg_max'][idx][i])
    return arrays

def fmt_num(v):
    return f'{v:.2f}'.rstrip('0').rstrip('.')

def format_array(name, values):
    lines = []
    line = f'{name} = '
    for idx, v in enumerate(values):
        if idx and idx % 4 == 0:
            lines.append(line.rstrip(', '))
            line = ' ' * 28
        line += fmt_num(v)
        if idx < len(values)-1:
            line += ', '
    lines.append(line)
    return '\n'.join(lines)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('input', help='File with timing_window_setup output')
    ap.add_argument('output', help='Destination param file')
    ap.add_argument('--template', default='PARAM/SHMS/HODO/phodo_cuts.param',
                    help='Existing param file to copy header from')
    args = ap.parse_args()

    with open(args.input) as f:
        parsed = parse_input(f.readlines())
    arrays = build_arrays(parsed)

    header_lines = []
    with open(args.template) as f:
        for line in f:
            if line.startswith('phodo_PosAdcTimeWindowMin'):
                break
            header_lines.append(line.rstrip())

    with open(args.output, 'w') as out:
        for h in header_lines:
            out.write(h + '\n')
        out.write('\n')
        out.write(format_array('phodo_PosAdcTimeWindowMin', arrays['PosAdcTimeWindowMin']))
        out.write('\n\n')
        out.write(format_array('phodo_PosAdcTimeWindowMax', arrays['PosAdcTimeWindowMax']))
        out.write('\n\n')
        out.write(format_array('phodo_NegAdcTimeWindowMin', arrays['NegAdcTimeWindowMin']))
        out.write('\n\n')
        out.write(format_array('phodo_NegAdcTimeWindowMax', arrays['NegAdcTimeWindowMax']))
        out.write('\n')

if __name__ == '__main__':
    main()
