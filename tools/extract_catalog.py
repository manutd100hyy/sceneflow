#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Extract sketch/aerial catalog data from the legacy SketchRoad2dx resources.

World units in the new app are metres. Legacy geometry is stored in centimetres,
so every coordinate is divided by 100.

Outputs (under ./data):
  symbols.json, templates.json, menu.json, dropdowns.json, forms.json
"""
import json
import os
import plistlib
import shutil
import xml.etree.ElementTree as ET
from collections import OrderedDict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
LEGACY = os.path.join(ROOT, "upstream-src", "SketchRoad2dx")
RES = os.path.join(LEGACY, "Resources")
CSD_DIR = os.path.join(LEGACY, "CocosStudio", "NoteScene", "cocosstudio")
OUT = os.path.join(ROOT, "data")
FORM_OUT = os.path.join(OUT, "forms")


def num(v, default=0.0):
    try:
        return float(v)
    except (TypeError, ValueError):
        return default


def xy_m(p):
    if not isinstance(p, dict):
        return None
    if "X" not in p or "Y" not in p:
        return None
    return [round(num(p["X"]) / 100.0, 4), round(num(p["Y"]) / 100.0, 4)]


def apply_affine(pt, tr):
    if not pt:
        return None
    if not isinstance(tr, dict):
        return pt
    a = num(tr.get("a", 1), 1)
    b = num(tr.get("b", 0), 0)
    c = num(tr.get("c", 0), 0)
    d = num(tr.get("d", 1), 1)
    tx = num(tr.get("tx", 0), 0) / 100.0
    ty = num(tr.get("ty", 0), 0) / 100.0
    x, y = pt
    return [round(a * x + c * y + tx, 4), round(b * x + d * y + ty, 4)]


def as_int(v, default=0):
    try:
        return int(float(v))
    except (TypeError, ValueError):
        return default


def prim_from(pr):
    if not isinstance(pr, dict):
        return None
    k = as_int(pr.get("kind"), -1)
    out = {"k": k}
    if k == 0:
        a, b = xy_m(pr.get("start")), xy_m(pr.get("end"))
        if not a or not b:
            return None
        out["a"], out["b"] = a, b
    elif k == 1:
        r = pr.get("rect") or {}
        out["x"] = round(num(r.get("X")) / 100.0, 4)
        out["y"] = round(num(r.get("Y")) / 100.0, 4)
        out["w"] = round(num(r.get("Width")) / 100.0, 4)
        out["h"] = round(num(r.get("Height")) / 100.0, 4)
        out["cr"] = round(num(pr.get("cornerRadius")) / 100.0, 4)
    elif k in (2, 9):
        c = xy_m(pr.get("center"))
        if not c:
            return None
        out["c"] = c
        out["r"] = round(num(pr.get("radius")) / 100.0, 4)
        if k == 2:
            out["f"] = bool(pr.get("filled"))
    elif k == 3:
        r = pr.get("rect") or {}
        out["x"] = round(num(r.get("X")) / 100.0, 4)
        out["y"] = round(num(r.get("Y")) / 100.0, 4)
        out["w"] = round(num(r.get("Width")) / 100.0, 4)
        out["h"] = round(num(r.get("Height")) / 100.0, 4)
    elif k == 4:
        nodes = []
        for n in pr.get("nodes") or []:
            p = xy_m(n)
            if p:
                nodes.append(p)
        if len(nodes) < 2:
            return None
        out["n"] = nodes
    elif k == 5:
        c = xy_m(pr.get("center"))
        if not c:
            return None
        out["c"] = c
        out["r"] = round(num(pr.get("radius")) / 100.0, 4)
        # Legacy stores the angle in units of pi radians (see Primitive.cpp).
        out["a0"] = num(pr.get("startAngle"))
        out["a1"] = num(pr.get("endAngle"))
        out["cw"] = bool(pr.get("clockwised"))
        out["f"] = bool(pr.get("filled"))
    elif k == 7:
        pts = []
        for key in ("first", "second", "third"):
            p = xy_m(pr.get(key))
            if p:
                pts.append(p)
        if len(pts) < 3:
            return None
        out["p"] = pts
        out["f"] = bool(pr.get("fill", True))
    elif k == 8:
        pts = []
        for n in pr.get("points") or []:
            p = xy_m(n)
            if p:
                pts.append(p)
        if len(pts) < 3:
            return None
        out["p"] = pts
        out["f"] = True
    else:
        return None
    return out


def control_points(sc, tr):
    pts = []
    if not isinstance(sc, dict):
        return pts
    for cp in sc.get("controlPoints") or []:
        pos = cp.get("position") if isinstance(cp, dict) else None
        p = xy_m(pos)
        if p:
            pts.append(apply_affine(p, tr))
    return pts


def style_from(sc, driving):
    if not isinstance(sc, dict):
        return None
    prims = []
    for pr in sc.get("primitives") or []:
        item = prim_from(pr)
        if item:
            prims.append(item)
    skel = control_points(sc, None)
    if not prims and len(skel) < 2:
        return None
    return {
        "name": sc.get("name") or "正常",
        "prims": prims,
        "skel": skel,
        "points": driving or [],
    }


def driving_for_style(raw, index):
    if not isinstance(raw, list) or not raw:
        return []
    chosen = None
    if isinstance(raw[0], dict):
        chosen = raw if index == 0 else []
    elif index < len(raw) and isinstance(raw[index], list):
        chosen = raw[index]
    elif raw and isinstance(raw[0], list):
        chosen = raw[0]
    pts = []
    for item in chosen or []:
        if not isinstance(item, dict):
            continue
        p = xy_m(item.get("position"))
        if not p:
            continue
        pts.append({"name": item.get("name") or "", "x": p[0], "y": p[1]})
    return pts


def extract_symbols(db):
    out = []
    for name, sym in db["symbols"].items():
        if not isinstance(sym, dict):
            continue
        styles = []
        raw_dp = sym.get("drivingPoints")
        own = style_from(sym.get("shapeController"), driving_for_style(raw_dp, 0))
        if own:
            styles.append(own)
        for i, st in enumerate(sym.get("shapeStyles") or []):
            block = style_from(st, driving_for_style(raw_dp, i))
            if not block:
                continue
            # Skip a duplicate of the base style.
            if own and block["name"] == own["name"] and block["prims"] == own["prims"]:
                continue
            styles.append(block)
        if not styles:
            continue
        nt = sym.get("numberType")
        pri = sym.get("dimensionPriority")
        out.append({
            "name": name,
            "uuid": sym.get("uuid") or "",
            "legacyKind": as_int(sym.get("kind"), 0),
            "numberType": as_int(nt, 0) if nt is not None else 0,
            "priority": as_int(pri, 1) if pri is not None else 1,
            "fill": sym.get("fillColor") or "",
            "line": sym.get("lineColor") or "黑色",
            "styles": styles,
        })
    out.sort(key=lambda s: s["name"])
    return out


def rope_line(rope):
    sc = rope.get("shapeController") or {}
    tr = rope.get("transform") or {}
    pts = control_points(sc, tr)
    if len(pts) < 2:
        return None
    curve = as_int(sc.get("kind"), 0) == 1 and len(pts) >= 3
    return {"style": as_int(rope.get("style"), 1), "pts": pts, "curve": curve}


def mark_from(el):
    if not isinstance(el, dict):
        return None
    kind = as_int(el.get("kind"), -1)
    tr = el.get("transform") or {}
    if kind == 8:
        a = apply_affine(xy_m(el.get("begin")), tr) if xy_m(el.get("begin")) else None
        b = apply_affine(xy_m(el.get("end")), tr) if xy_m(el.get("end")) else None
        if not a or not b:
            pts = control_points(el.get("shapeController") or {}, tr)
            if len(pts) >= 2:
                a, b = pts[0], pts[-1]
        if not a or not b:
            return None
        return {"type": "guide", "pts": [a, b]}
    if kind == 12:
        a = xy_m(el.get("begin"))
        b = xy_m(el.get("end"))
        if a:
            a = apply_affine(a, tr)
        if b:
            b = apply_affine(b, tr)
        if not a or not b:
            return None
        return {
            "type": "crosswalk",
            "pts": [a, b],
            "width": round(num(el.get("width"), 300) / 100.0, 3),
            "interval": round(num(el.get("interval"), 50) / 100.0, 3),
        }
    if kind == 35:
        return {
            "type": "parking",
            "x": round(num(tr.get("tx")) / 100.0, 3),
            "y": round(num(tr.get("ty")) / 100.0, 3),
            "w": round(num(el.get("w"), 250) / 100.0, 3),
            "h": round(num(el.get("h"), 500) / 100.0, 3),
            "count": as_int(el.get("num"), 1),
            "ang": num(el.get("ang")),
        }
    return None


def extract_templates(path):
    with open(path, "rb") as f:
        scenes = plistlib.load(f)["scenes"]
    templates = []
    seen_ids = {}
    for scene in scenes:
        uid = scene.get("uuid") or ""
        name = (scene.get("name") or "").strip() or "未命名模板"
        if uid in seen_ids:
            # Some lane-count presets share a uuid. Keep the first geometry.
            continue
        seen_ids[uid] = name
        lines = []
        marks = []
        seen_rope = set()
        for tufu in scene.get("tufus") or []:
            kind = as_int(tufu.get("kind"), -1)
            if kind in (1, 36):
                for lane in tufu.get("lanes") or []:
                    for rope in lane.get("ropes") or []:
                        rid = rope.get("uuid")
                        if rid and rid in seen_rope:
                            continue
                        if rid:
                            seen_rope.add(rid)
                        line = rope_line(rope)
                        if line:
                            lines.append(line)
                    for el in lane.get("Elements") or []:
                        mk = mark_from(el)
                        if mk:
                            marks.append(mk)
                for el in tufu.get("elements") or []:
                    mk = mark_from(el)
                    if mk:
                        marks.append(mk)
            elif kind == 3:
                rid = tufu.get("uuid")
                if rid and rid in seen_rope:
                    continue
                if rid:
                    seen_rope.add(rid)
                line = rope_line(tufu)
                if line:
                    lines.append(line)
            else:
                mk = mark_from(tufu)
                if mk:
                    marks.append(mk)
        templates.append({"id": uid, "name": name, "lines": lines, "marks": marks})
    return templates


def menu_leaves(node):
    kids = []
    for arr in node.get("itemGroups") or []:
        if isinstance(arr, list):
            kids.extend(arr)
    return kids


def extract_menu(menu, templates):
    by_id = {t["id"]: t for t in templates}
    top = menu_leaves(menu)
    template_items = []
    symbol_groups = []
    for group in top:
        gname = group.get("name") or group.get("iconPath") or ""
        leaves = menu_leaves(group)
        if not leaves and group.get("notification") == "AddSceneNotification":
            leaves = [group]
        if gname == "道路模板" or (leaves and leaves[0].get("notification") == "AddSceneNotification"):
            for leaf in leaves:
                uid = leaf.get("targetUuid") or ""
                name = leaf.get("name") or leaf.get("iconPath") or ""
                if uid not in by_id:
                    # Match by name when the menu uuid is a shared preset id.
                    for t in templates:
                        if t["name"] == name or name and name in t["name"]:
                            uid = t["id"]
                            break
                template_items.append({"name": name, "id": uid})
            continue
        if gname in ("预制道路",):
            continue
        items = []
        for leaf in leaves:
            items.append({
                "name": leaf.get("name") or "",
                "notification": leaf.get("notification") or "",
                "flag": as_int(leaf.get("commandFlag"), 0),
                "uuid": leaf.get("targetUuid") or "",
            })
        if items:
            symbol_groups.append({"name": gname, "items": items})
    return {"templates": template_items, "groups": symbol_groups}


def extract_dropdowns(db):
    def conv(d):
        out = {}
        if not isinstance(d, dict):
            return out
        for k, v in d.items():
            if isinstance(v, list):
                out[k] = [str(x) for x in v]
        return out
    return {
        "survey": conv(db.get("surveyNotesData")),
        "inquiry": conv(db.get("inquiryRecordData")),
        "interrogation": conv(db.get("interrogationRecordData")),
        "certificate": conv(db.get("accidentCertificationNotesData")),
    }


WORD = {
    "accident": "事故", "time": "时间", "local": "地点", "location": "地点",
    "weather": "天气", "party": "当事人", "driver": "驾驶人", "telephone": "电话",
    "identity": "证件", "number": "号码", "license": "证件", "tag": "号牌",
    "insurance": "保险", "road": "道路", "name": "名称", "car": "车辆",
    "death": "死亡", "injuries": "受伤", "survey": "勘察", "company": "单位",
    "people": "人员", "begin": "开始", "end": "结束", "date": "日期",
    "record": "记录", "person": "人", "sign": "签名", "other": "其他",
    "information": "情况", "description": "描述", "longitude": "经度",
    "latitude": "纬度", "question": "问", "answer": "答", "inquiry": "询问",
    "sex": "性别", "age": "年龄", "address": "住址", "nation": "民族",
    "notes": "编号", "num": "次数", "man": "人", "office": "单位",
    "household": "户籍", "be": "被", "ask": "询问", "inquirer": "询问人",
    "traffic": "交通", "style": "方式", "certificateion": "认定书",
    "certificate": "认定书", "damage": "损坏", "duty": "责任", "reason": "原因",
    "behaviour": "行为", "behavior": "行为", "explain": "说明", "monitor": "监控",
    "equipment": "设备", "danger": "危险品", "surface": "路面", "direction": "方向",
    "speed": "速度", "light": "灯光", "stall": "档位", "steer": "转向",
    "detain": "暂扣", "color": "颜色", "model": "车型", "towards": "方向",
    "away": "逃逸", "count": "数量", "site": "地点", "documents": "证件",
    "used": "曾用", "finger": "指纹", "picture": "图片", "width": "宽度",
    "length": "长度", "phone": "电话", "plate": "号牌", "org": "机关",
    "unit": "单位", "start": "开始", "stop": "结束", "witness": "见证人",
    "statement": "陈述", "fact": "事实", "law": "法律", "item": "条款",
    "result": "结论", "opinion": "意见", "signature": "签名", "remark": "备注",
    "note": "说明", "page": "页", "next": "续", "blood": "血迹", "trace": "痕迹",
    "vehicle": "机动车", "bike": "非机动车", "human": "人体", "body": "尸体",
    "contact": "接触点", "final": "最终", "park": "停车", "point": "点",
}
PARTY = {"A": "甲", "B": "乙", "C": "丙", "D": "丁", "E": "戊"}
OVERRIDE = {
    "surveyCompany": "勘察单位",
    "surveyBeginDate": "勘察开始时间",
    "surveyEndDate": "勘察结束时间",
    "surveyPeople": "勘察人",
    "drawingPeople": "绘图人",
    "accidentTime": "事故时间",
    "accidentLocation": "事故地点",
    "accidentlocal": "事故地点",
    "roadName": "道路名称",
    "roadNumber": "道路编号",
    "locationDescription": "地点描述",
    "weather": "天气",
    "certificateionNum": "认定书编号",
    "notesNum": "笔录编号",
    "inquiryLocation": "询问地点",
    "inquiryman": "询问人",
    "inquirymanOffice": "询问人单位",
    "recordman": "记录人",
    "recordmanOffice": "记录人单位",
    "beAskman": "被询问人",
    "beInquirymanSex": "性别",
    "householdLocation": "户籍地",
    "identity": "身份证件号",
    "telephone": "联系电话",
    "inquiryBginDate": "开始时间",
    "inquiryEndDate": "结束时间",
    "death": "死亡人数",
    "injuries": "受伤人数",
    "deathOfCasualties": "伤亡情况",
    "otherExplain": "其他说明",
    "dangerName": "危险品名称",
    "monitorEquipment": "监控设备",
    "adOtherRoad": "其他道路",
}


def split_camel(key):
    out = []
    buf = ""
    for ch in key:
        if ch.isupper() and buf:
            out.append(buf)
            buf = ch
        else:
            buf += ch
    if buf:
        out.append(buf)
    return out


def label_of(key):
    if key in OVERRIDE:
        return OVERRIDE[key]
    party = ""
    base = key
    if len(key) > 1 and key[-1] in PARTY and key[-2].islower():
        party = PARTY[key[-1]]
        base = key[:-1]
    if base in OVERRIDE:
        text = OVERRIDE[base]
        return text + party if party else text
    words = split_camel(base)
    translated = []
    for w in words:
        low = w.lower()
        if low in WORD:
            translated.append(WORD[low])
        elif w in PARTY:
            translated.append(PARTY[w])
        else:
            translated.append(w)
    text = "".join(translated)
    if party:
        text += party
    if text == key or any(ch.isascii() and ch.isalpha() for ch in text):
        # Keep a readable fallback: Chinese prefix when we translated part of it.
        return text
    return text


def field_kind(node):
    ctype = node.attrib.get("ctype", "")
    name = node.attrib.get("Name", "")
    tag = as_int(node.attrib.get("Tag"), 0)
    if ctype == "TextFieldObjectData":
        return "text"
    if ctype != "ButtonObjectData":
        return None
    lname = name.lower()
    if "sign" in lname or "finger" in lname:
        return "sign"
    if 150 <= tag < 200:
        return "date"
    if tag >= 300:
        return "check"
    return "text"


def extract_forms(dropdowns):
    os.makedirs(FORM_OUT, exist_ok=True)
    specs = [
        ("survey", "道路交通事故现场勘察笔录",
         ["surveyNote1.csd", "surveyNote2.csd", "surveyNote3.csd", "surveyNote4.csd", "surveyNote5.csd"],
         "survey"),
        ("inquiry", "询问笔录", ["InquiryRecord.csd", "InquiryRecordNext.csd"], "inquiry"),
        ("interrogation", "讯问笔录", ["InterrogationRecord.csd", "InterrogationRecordNext.csd"], "interrogation"),
        ("certificate", "道路交通事故认定书（简易程序）", ["RoadTrafficAccidentCertificate.csd"], "certificate"),
    ]
    forms = []
    for fid, title, files, drop_key in specs:
        pages = []
        options_src = dropdowns.get(drop_key, {})
        for idx, fn in enumerate(files, start=1):
            path = os.path.join(CSD_DIR, fn)
            root = ET.parse(path).getroot()
            obj = root.find(".//ObjectData")
            size = obj.find("Size") if obj is not None else None
            pw = num(size.attrib.get("X") if size is not None else 1536, 1536)
            ph = num(size.attrib.get("Y") if size is not None else 2048, 2048)
            image_name = ""
            fields = []
            for node in root.iter("AbstractNodeData"):
                ctype = node.attrib.get("ctype", "")
                if ctype == "ImageViewObjectData" and not image_name:
                    fd = node.find("FileData")
                    if fd is not None:
                        image_name = fd.attrib.get("Path", "")
                    continue
                kind = field_kind(node)
                if not kind:
                    continue
                name = node.attrib.get("Name") or ""
                if not name or name.startswith("Image"):
                    continue
                pos = node.find("Position")
                sz = node.find("Size")
                if pos is None or sz is None:
                    continue
                fields.append({
                    "key": name,
                    "label": label_of(name),
                    "kind": kind,
                    "x": round(num(pos.attrib.get("X")), 2),
                    "y": round(num(pos.attrib.get("Y")), 2),
                    "w": round(num(sz.attrib.get("X")), 2),
                    "h": round(num(sz.attrib.get("Y")), 2),
                    "size": as_int(node.attrib.get("FontSize"), 22),
                    "options": options_src.get(name, []),
                })
            rel = ""
            if image_name:
                src = os.path.join(CSD_DIR, image_name)
                if not os.path.exists(src):
                    src = os.path.join(RES, "res", "NoteScene", image_name)
                ext = os.path.splitext(image_name)[1] or ".png"
                rel = "forms/%s-%d%s" % (fid, idx, ext)
                dest = os.path.join(OUT, rel)
                if os.path.exists(src):
                    shutil.copyfile(src, dest)
                else:
                    rel = ""
            pages.append({
                "title": "第%d页" % idx,
                "image": rel,
                "width": pw,
                "height": ph,
                "fields": fields,
            })
        forms.append({"id": fid, "title": title, "pages": pages})
    return {"forms": forms}


def main():
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(RES, "DataSource.plist"), "rb") as f:
        db = plistlib.load(f)["database"]
    symbols = extract_symbols(db)
    templates = extract_templates(os.path.join(RES, "Templates.plist"))
    menu = extract_menu(db["menu"], templates)
    ebike = next((s for s in symbols if s.get("name") == "电动自行车"), None)
    if ebike:
        for group in menu["groups"]:
            if group.get("name") != "交通事故元素":
                continue
            names = [it.get("name") for it in group["items"]]
            if "电动自行车" in names:
                break
            item = {
                "name": "电动自行车",
                "notification": "AddTufuNotification",
                "flag": 0,
                "uuid": ebike.get("uuid") or "",
            }
            insert_at = names.index("电瓶车") + 1 if "电瓶车" in names else len(group["items"])
            group["items"].insert(insert_at, item)
            break
    dropdowns = extract_dropdowns(db)
    forms = extract_forms(dropdowns)
    with open(os.path.join(OUT, "symbols.json"), "w", encoding="utf-8") as f:
        json.dump({"units": "m", "symbols": symbols}, f, ensure_ascii=False, separators=(",", ":"))
    with open(os.path.join(OUT, "templates.json"), "w", encoding="utf-8") as f:
        json.dump({"units": "m", "templates": templates}, f, ensure_ascii=False, separators=(",", ":"))
    with open(os.path.join(OUT, "menu.json"), "w", encoding="utf-8") as f:
        json.dump(menu, f, ensure_ascii=False, indent=2)
    with open(os.path.join(OUT, "dropdowns.json"), "w", encoding="utf-8") as f:
        json.dump(dropdowns, f, ensure_ascii=False, indent=2)
    with open(os.path.join(OUT, "forms.json"), "w", encoding="utf-8") as f:
        json.dump(forms, f, ensure_ascii=False, indent=2)
    stats = {
        "symbols": len(symbols),
        "templates": len(templates),
        "templateLines": sum(len(t["lines"]) for t in templates),
        "templateMarks": sum(len(t["marks"]) for t in templates),
        "menuTemplates": len(menu["templates"]),
        "menuGroups": [(g["name"], len(g["items"])) for g in menu["groups"]],
        "dropdowns": {k: len(v) for k, v in dropdowns.items()},
        "forms": [(fm["id"], len(fm["pages"]), sum(len(p["fields"]) for p in fm["pages"])) for fm in forms["forms"]],
        "emptyTemplates": [t["name"] for t in templates if not t["lines"]],
    }
    with open(os.path.join(OUT, "catalog_stats.json"), "w", encoding="utf-8") as f:
        json.dump(stats, f, ensure_ascii=False, indent=2)
    print(json.dumps(stats, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
