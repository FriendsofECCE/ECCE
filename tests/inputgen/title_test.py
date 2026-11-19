#!/usr/bin/env python3
"""Generate real decks with titles containing spaces and punctuation."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
PARSERS = ROOT / "scripts" / "parsers"
TEMPLATES = {"ecceqm": "ecceqm.tpl", "orca": "orca.tpl", "nwchem": "nwch.tpl"}
BASES = {
    "ecceqm": "basis 6-31G*\nend\n",
    "orca": "useRouteCard def2-SVP\n",
    "nwchem": 'basis "ao basis" spherical print\nO library "sto-3g"\n'
              'H library "sto-3g"\nend\n',
}


def generate(code, title, work, final_title=False):
    template = work / "deck.tpl"
    shutil.copyfile(PARSERS / TEMPLATES[code], template)
    header = "# frag\nnum_atoms: 3\n"
    geometry = ("atom_info: symbol\natom_list:\n"
                "O 0.0 0.0 0.0\nH 0.0 0.76 0.59\nH 0.0 -0.76 0.59\n")
    title_line = "title:\t" + title
    fragment = (header + geometry + title_line if final_title else
                header + title_line + "\n" + geometry)
    (work / "w.frag").write_text(fragment)
    (work / "w.basis").write_text(BASES[code])
    (work / "w.param").write_text(
        "title: water\nannotation: " + title + "\nparseFile: w.ep\n"
        "Category: SCF\nTheory: " + ("HF" if code == "ecceqm" else "RHF") +
        "\nRunType: Energy\nCharge: 0\nChemSys.Multiplicity: 1\n")
    subprocess.run(
        ["perl", str(PARSERS / ("ai." + code)), "-p", "-f", "-b",
         "-n", "w", "-t", str(template)], cwd=work,
        env=dict(os.environ, ECCE_HOME=str(ROOT)),
        capture_output=True, text=True, check=True)
    return template.read_text().splitlines()


class TitleTest(unittest.TestCase):
    def test_titles(self):
        titles = ["Water", "Water minimum",
                  "Water, B3LYP-D3(BJ)/def2-SVP, minimum",
                  "Acetone to propen-2-ol direct 1,3-H shift, minimum",
                  "Water  minimum"]
        with tempfile.TemporaryDirectory(dir=ROOT / ".test-cache",
                                         prefix="inputgen-title-") as name:
            work = Path(name)
            for code in TEMPLATES:
                for title in titles:
                    for final_title in (False, True):
                        with self.subTest(code=code, title=title, final_title=final_title):
                            lines = generate(code, title, work, final_title)
                            if code == "ecceqm":
                                self.assertIn("title " + title, lines)
                            elif code == "orca":
                                self.assertIn("# " + title, lines)
                            else:
                                self.assertIn('Title "' + title + '"', lines)
                            self.assertFalse(any("##" in line for line in lines))

            lines = generate("orca", "Water # minimum # final", work)
            self.assertIn("# Water   minimum   final", lines)


if __name__ == "__main__":
    (ROOT / ".test-cache").mkdir(exist_ok=True)
    unittest.main()
