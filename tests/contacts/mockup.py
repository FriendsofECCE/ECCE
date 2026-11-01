#!/usr/bin/env python3
"""Layout mock-up of the ensemble table panel (#708); not the panel itself."""
import math, sys
import matplotlib; matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle
out = sys.argv[1] if len(sys.argv) > 1 else 'mockup2.png'
W, H = 1000, 640
fig, ax = plt.subplots(figsize=(10, 6.4), dpi=100)
ax.set_xlim(0, W); ax.set_ylim(H, 0); ax.axis('off')
fig.subplots_adjust(0, 0, 1, 1)
ax.add_patch(Rectangle((0, 0), W, H, fc='#f2f2f2', ec='#888'))
ax.text(12, 22, 'Ensemble: Na+ (H2O)6, CREST GFN-FF  -  mock-up', fontsize=11, weight='bold')
ax.text(12, 48, 'Group by:  [x] coordination pattern     Sort by: energy v       12 structures, 3 patterns', fontsize=9)
X = {'sel': 14, 'e': 48, 'hb': 300, 'th': 700}
ax.add_patch(Rectangle((8, 62), W - 16, 24, fc='#d8d8d8', ec='#999'))
ax.text(X['e'], 78, 'Rel. energy (kcal/mol)', fontsize=9, weight='bold')
ax.text(X['hb'], 78, 'H-bonds', fontsize=9, weight='bold')
ax.text(X['th'], 78, 'Thumbnail', fontsize=9, weight='bold')
groups = [('Na: 6×O(H2O)', [0.0, 0.3, 0.6, 1.1, 1.4], ['0', '0', '1  H2O>H2O', '0', '2  H2O>H2O']),
          ('Na: 5×O(H2O)', [0.4, 0.9, 1.2, 2.0], ['1  H2O>H2O', '1  H2O>H2O', '2  H2O>H2O', '0']),
          ('Na: 4×O(H2O)', [1.8, 2.2, 2.4], ['2  H2O>H2O', '3  H2O>H2O', '2  H2O>H2O'])]
y = 92
for name, es, hbs in groups:
    ax.add_patch(Rectangle((8, y), W - 16, 22, fc='#e6edf5', ec='#aab'))
    ax.text(14, y + 15, 'v  %s     (%d structures, representative: lowest energy)' % (name, len(es)),
            fontsize=9, weight='bold')
    y += 22
    for i, (e, hb) in enumerate(zip(es, hbs)):
        sel = (name.startswith('Na: 6') and i in (0, 2)) or (name.startswith('Na: 5') and i == 0)
        ax.add_patch(Rectangle((8, y), W - 16, 34, fc='#9cc3ee' if sel else 'white', ec='#ccc'))
        ax.add_patch(Rectangle((X['sel'], y + 11), 12, 12, fc='#336' if sel else 'white', ec='#444'))
        ax.text(X['e'], y + 21, '%.2f' % e, fontsize=9)
        ax.text(X['hb'], y + 21, hb, fontsize=9)
        ax.add_patch(Rectangle((X['th'], y + 2), 48, 30, fc='#fafafa', ec='#999'))
        ax.add_patch(Circle((X['th'] + 24, y + 17), 5, fc='#8a2be2'))
        for k in range(6):
            a = k * math.pi / 3
            ax.add_patch(Circle((X['th'] + 24 + 11 * math.cos(a), y + 17 + 11 * math.sin(a)), 2.5, fc='#d22'))
        y += 34
    y += 2
ax.text(14, 548, 'Right-click: Copy as CSV / JSON      Double-click a row: show in the 3-D view',
        fontsize=8, color='#555')
x = 12
for t, w in [('Superpose selected', 190), ('New calculation from selected', 270),
             ('Select representatives', 210), ('Export CSV...', 150)]:
    ax.add_patch(Rectangle((x, 580), w, 28, fc='#e8e8e8', ec='#666'))
    ax.text(x + 10, 598, t, fontsize=9); x += w + 12
fig.savefig(out)
