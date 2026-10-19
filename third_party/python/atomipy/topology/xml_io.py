"""
Optional XML serializer for the Topology dict (decision: JSON-first; XML is an
optional OpenFF-flavoured artifact). This is a *generic, type-tagged* dict<->XML
mapping so it round-trips identically — it is an explicit per-instance listing,
NOT SMIRNOFF and not SMARTS-based.
"""
from __future__ import annotations

from typing import Any
import xml.etree.ElementTree as ET


def _to_el(tag: str, val: Any) -> ET.Element:
    e = ET.Element(tag)
    if val is None:
        e.set("t", "null")
    elif isinstance(val, bool):
        e.set("t", "bool"); e.text = "true" if val else "false"
    elif isinstance(val, int):
        e.set("t", "int"); e.text = str(val)
    elif isinstance(val, float):
        e.set("t", "float"); e.text = repr(val)
    elif isinstance(val, str):
        e.set("t", "str"); e.text = val
    elif isinstance(val, dict):
        e.set("t", "dict")
        for k, v in val.items():
            e.append(_to_el(str(k), v))
    elif isinstance(val, (list, tuple)):
        e.set("t", "list")
        for it in val:
            e.append(_to_el("item", it))
    else:
        e.set("t", "str"); e.text = str(val)
    return e


def _from_el(e: ET.Element) -> Any:
    t = e.get("t")
    if t == "null":
        return None
    if t == "bool":
        return (e.text or "").strip() == "true"
    if t == "int":
        return int(e.text)
    if t == "float":
        return float(e.text)
    if t == "str":
        return e.text if e.text is not None else ""
    if t == "dict":
        return {c.tag: _from_el(c) for c in e}
    if t == "list":
        return [_from_el(c) for c in e]
    return e.text


def to_xml_string(d: dict, root_tag: str = "Topology") -> str:
    el = _to_el(root_tag, d)
    return ET.tostring(el, encoding="unicode")


def from_xml_string(s: str) -> dict:
    return _from_el(ET.fromstring(s))
