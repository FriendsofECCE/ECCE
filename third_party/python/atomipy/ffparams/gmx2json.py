import argparse
import os
import json
from collections import defaultdict

"""
Parse MINFF .itp files (ffnonbonded*.itp and ffbonded*.itp) and export them to JSON.

What it does
- Reads atomtypes from ffnonbonded*.itp, keeping each #ifdef block separate.
- Stores unconditional entries (e.g., common water parameters) once under common_atomtypes.
- Reads bondtypes/angletypes from ffbonded*.itp (using a fallback ffbonded.itp if present).
- Outputs JSON with per-block atomtypes and metadata about files/units/selected blocks.

Quick examples
- GMINFF blocks:
    python gmx2json.py -nb ../min.ff/ffnonbonded_gminff.itp -b ../min.ff/ffbonded.itp -blocks GMINFF_k500 OPC3_HFE_LM -o GMINFF/gminff_opc3_hfe_lm_k500.json
- TMINFF blocks:
    python gmx2json.py -nb ../min.ff/ffnonbonded_tminff_k500.itp -b ../min.ff/ffbonded.itp -blocks Kaolinite_k500 OPC3_HFE_LM -o TMINFF/Kaolinite_opc3_hfe_lm_k500.json
- Discover available blocks:
    python gmx2json.py -nb ../min.ff/ffnonbonded_gminff.itp --list-blocks
"""

DEFAULT_BLOCK = "__unconditional__"


def _parse_atomtype_line(line):
    """Parse a single atomtypes line, returning name and properties or None if it cannot be parsed."""
    fields = line.split()
    if len(fields) < 7:
        return None
    name = fields[0]
    try:
        mass = float(fields[-5])
        charge = float(fields[-4])
        sigma = float(fields[-2])
        epsilon = float(fields[-1])
    except ValueError:
        return None
    ptype = fields[-3]
    return name, {
        "mass": mass,
        "charge": charge,
        "ptype": ptype,
        "sigma": sigma,
        "epsilon": epsilon
    }


def parse_nonbonded_blocks(nonbonded_path):
    """
    Parse ffnonbonded*.itp, collecting atomtypes per #ifdef block plus unconditional entries.
    Returns:
    - blocks: mapping of block_name -> {"atomtypes": {...}}
    - block_order: appearance order (helps stable listing)
    """
    blocks = defaultdict(lambda: {"atomtypes": {}})
    block_order = []
    current_section = None
    block_stack = []

    def ensure_block(name):
        if name not in blocks:
            blocks[name] = {"atomtypes": {}}
            block_order.append(name)

    ensure_block(DEFAULT_BLOCK)

    with open(nonbonded_path, "r") as handle:
        for raw_line in handle:
            line = raw_line.split(";", 1)[0].strip()
            if not line:
                continue

            if line.startswith("#"):
                directive = line.split(None, 1)
                keyword = directive[0]
                if keyword == "#ifdef" and len(directive) == 2:
                    block_name = directive[1].strip()
                    block_stack.append(block_name)
                    ensure_block(block_name)
                elif keyword == "#endif" and block_stack:
                    block_stack.pop()
                continue

            if line.startswith("["):
                current_section = line.strip("[]").strip().lower()
                continue

            if current_section != "atomtypes":
                continue

            active_block = block_stack[-1] if block_stack else DEFAULT_BLOCK
            parsed = _parse_atomtype_line(line)
            if not parsed:
                continue
            name, props = parsed
            blocks[active_block]["atomtypes"][name] = props

    return blocks, block_order


def parse_bonded_file(bonded_path):
    """Parse ffbonded*.itp for bondtypes and angletypes."""
    data = {"bondtypes": [], "angletypes": []}
    current_section = None

    with open(bonded_path, "r") as handle:
        for raw_line in handle:
            line = raw_line.split(";", 1)[0].strip()
            if not line:
                continue
            if line.startswith("["):
                current_section = line.strip("[]").strip().lower()
                continue
            fields = line.split()
            if current_section == "bondtypes" and len(fields) >= 5:
                try:
                    func = int(fields[2])
                    length = float(fields[3])
                    k = float(fields[4])
                except ValueError:
                    continue
                data["bondtypes"].append({
                    "atoms": [fields[0], fields[1]],
                    "func": func,
                    "length": length,
                    "k": k
                })
            elif current_section == "angletypes" and len(fields) >= 6:
                try:
                    func = int(fields[3])
                    theta0 = float(fields[4])
                    k = float(fields[5])
                except ValueError:
                    continue
                data["angletypes"].append({
                    "atoms": [fields[0], fields[1], fields[2]],
                    "func": func,
                    "theta0": theta0,
                    "k": k
                })

    return data


def build_output(nonbonded_blocks, block_order, bonded_data, selected_blocks, variant, nonbonded_path, bonded_path):
    """Assemble final JSON structure with selected blocks, shared atomtypes, and metadata."""
    conditional_blocks = [b for b in block_order if b != DEFAULT_BLOCK]
    unconditional_atomtypes = nonbonded_blocks.get(DEFAULT_BLOCK, {}).get("atomtypes", {})

    if selected_blocks:
        chosen_blocks = [b for b in selected_blocks if b in nonbonded_blocks]
        missing = [b for b in selected_blocks if b not in nonbonded_blocks]
        if missing:
            print(f"Warning: requested blocks not found in {nonbonded_path}: {', '.join(missing)}")
    else:
        chosen_blocks = conditional_blocks if conditional_blocks else [DEFAULT_BLOCK]

    if not chosen_blocks:
        chosen_blocks = [DEFAULT_BLOCK]

    nonbonded_output = {}
    for block_name in chosen_blocks:
        block_atomtypes = nonbonded_blocks.get(block_name, {}).get("atomtypes", {})
        nonbonded_output[block_name] = {"atomtypes": dict(block_atomtypes)}

    data = {
        "nonbonded_blocks": nonbonded_output,
        "common_atomtypes": unconditional_atomtypes,
        "bondtypes": bonded_data["bondtypes"],
        "angletypes": bonded_data["angletypes"],
        "metadata": {
            "variant": variant,
            "source": "MINFF",
            "nonbonded_file": nonbonded_path,
            "bonded_file": bonded_path,
            "available_blocks": conditional_blocks,
            "selected_blocks": chosen_blocks,
            "units": {
                "mass": "u",
                "charge": "e",
                "sigma": "nm",
                "epsilon": "kJ/mol",
                "bond_length": "nm",
                "bond_force_constant": "kJ/(mol*nm^2)",
                "angle": "deg",
                "angle_force_constant": "kJ/(mol*rad^2)"
            }
        }
    }

    if len(chosen_blocks) == 1:
        only_key = chosen_blocks[0]
        merged = dict(unconditional_atomtypes)
        merged.update(nonbonded_output[only_key]["atomtypes"])
        data["atomtypes"] = merged

    return data


def main():
    parser = argparse.ArgumentParser(description="Parse MINFF .itp files and export to JSON.")
    parser.add_argument("--variant", help="Base name used in ffnonbonded_*.itp (e.g. gminff or tminff_k0)")
    parser.add_argument("--input_dir", default=".", help="Directory containing the .itp files")
    parser.add_argument("-blocks", "--blocks", nargs="+", help="Specific #ifdef block names to extract (mineral, ions, water). If omitted, all blocks are exported.")
    parser.add_argument("--list-blocks", action="store_true", help="List available #ifdef blocks in the non-bonded file and exit.")
    parser.add_argument("-nb", "--nonbonded-file", help="Explicit path to ffnonbonded*.itp. Overrides --variant.")
    parser.add_argument("-b", "--bonded-file", help="Explicit path to ffbonded*.itp. Overrides --variant.")
    parser.add_argument("-o", "--output", help="Output .json filename (optional)")

    args = parser.parse_args()
    variant = args.variant

    if args.nonbonded_file:
        nonbonded_file = args.nonbonded_file
    else:
        if not variant:
            print("Error: provide --variant or --nonbonded-file")
            return
        nonbonded_file = os.path.join(args.input_dir, f"ffnonbonded_{variant}.itp")

    if args.bonded_file:
        bonded_file = args.bonded_file
    else:
        bonded_file = os.path.join(args.input_dir, f"ffbonded_{variant}.itp") if variant else None

    if not os.path.exists(nonbonded_file):
        print(f"Missing nonbonded file: {nonbonded_file}")
        return

    if not bonded_file or not os.path.exists(bonded_file):
        fallback_bonded_file = os.path.join(args.input_dir, "ffbonded.itp")
        if os.path.exists(fallback_bonded_file):
            print(f"Using fallback bonded file: {fallback_bonded_file}")
            bonded_file = fallback_bonded_file
        else:
            print("Missing bonded file and fallback ffbonded.itp not found.")
            return

    nonbonded_blocks, block_order = parse_nonbonded_blocks(nonbonded_file)

    if args.list_blocks:
        conditional_blocks = [b for b in block_order if b != DEFAULT_BLOCK]
        if conditional_blocks:
            print("Detected #ifdef blocks:")
            for block_name in conditional_blocks:
                print(f" - {block_name}")
        else:
            print("No #ifdef blocks found; only unconditional atomtypes are present.")
        return

    bonded_data = parse_bonded_file(bonded_file)
    parsed_data = build_output(
        nonbonded_blocks=nonbonded_blocks,
        block_order=block_order,
        bonded_data=bonded_data,
        selected_blocks=args.blocks,
        variant=variant,
        nonbonded_path=nonbonded_file,
        bonded_path=bonded_file
    )

    output_file = args.output or f"minff_{variant}.json"
    with open(output_file, "w") as handle:
        json.dump(parsed_data, handle, indent=2)
    print(f"Written output to {output_file}")


if __name__ == "__main__":
    main()
