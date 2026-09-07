"""
Prepare the user-supplied SWAT Operator Blender source for manual modular-gear review.

This script is intentionally conservative:
- it never deletes or renames the original source object;
- it duplicates `sol_8_low` and works only on the duplicate;
- it separates disconnected geometry islands into candidate objects;
- it preserves copied vertex groups/modifiers/material assignments where Blender supports it;
- it writes a JSON report for classification;
- it saves to a NEW working .blend path, never over the opened source file.

Recommended invocation from the repository root:

    blender --background --disable-autoexec "<path>/SWAT+Operator+CGTrader.blend" \
      --python Tools/Blender/swat_operator_prepare.py

Optional environment variables:
    TU_SWAT_SOURCE_OBJECT   source mesh object name (default: sol_8_low)
    TU_SWAT_WORKING_BLEND  explicit output .blend path
    TU_SWAT_REPORT_JSON    explicit report JSON path
"""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any

import bpy

SOURCE_OBJECT_NAME = os.environ.get("TU_SWAT_SOURCE_OBJECT", "sol_8_low")
CANDIDATE_COLLECTION_NAME = "TU_SWAT_SeparationCandidates"
WORKING_PREFIX = "TU_SWAT_Candidate"


def _blend_directory() -> Path:
    source_path = Path(bpy.data.filepath) if bpy.data.filepath else Path.cwd() / "SWAT_source.blend"
    return source_path.resolve().parent


def _default_working_path() -> Path:
    source_path = Path(bpy.data.filepath) if bpy.data.filepath else (_blend_directory() / "SWAT_source.blend")
    return source_path.with_name(f"{source_path.stem}_TU_working.blend")


def _default_report_path() -> Path:
    return _blend_directory() / "swat_loose_parts_report.json"


def _ensure_object_mode() -> None:
    active = bpy.context.view_layer.objects.active
    if active and active.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")


def _clear_selection() -> None:
    for obj in bpy.context.selected_objects:
        obj.select_set(False)


def _ensure_candidate_collection() -> bpy.types.Collection:
    existing = bpy.data.collections.get(CANDIDATE_COLLECTION_NAME)
    if existing:
        return existing

    collection = bpy.data.collections.new(CANDIDATE_COLLECTION_NAME)
    bpy.context.scene.collection.children.link(collection)
    return collection


def _link_only_to_collection(obj: bpy.types.Object, collection: bpy.types.Collection) -> None:
    for current_collection in list(obj.users_collection):
        current_collection.objects.unlink(obj)
    collection.objects.link(obj)


def _duplicate_source(source: bpy.types.Object, collection: bpy.types.Collection) -> bpy.types.Object:
    duplicate = source.copy()
    duplicate.data = source.data.copy()
    duplicate.name = f"{WORKING_PREFIX}_SourceDuplicate"
    collection.objects.link(duplicate)
    return duplicate


def _separate_loose_parts(duplicate: bpy.types.Object) -> list[bpy.types.Object]:
    _ensure_object_mode()
    _clear_selection()
    duplicate.select_set(True)
    bpy.context.view_layer.objects.active = duplicate

    before = set(bpy.data.objects)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.separate(type="LOOSE")
    bpy.ops.object.mode_set(mode="OBJECT")

    after = set(bpy.data.objects)
    created = list(after - before)

    # Blender keeps one loose part in the original duplicate object and creates
    # new objects for the others, so include the duplicate itself as a candidate.
    candidates = [duplicate] + created
    mesh_candidates = [obj for obj in candidates if obj.type == "MESH"]
    mesh_candidates.sort(key=lambda obj: obj.name)
    return mesh_candidates


def _world_bbox(obj: bpy.types.Object) -> dict[str, list[float]]:
    points = [obj.matrix_world @ obj.bound_box[i] for i in range(8)]
    return {
        "min": [min(point[axis] for point in points) for axis in range(3)],
        "max": [max(point[axis] for point in points) for axis in range(3)],
    }


def _bone_influence_summary(obj: bpy.types.Object, limit: int = 12) -> list[dict[str, Any]]:
    if obj.type != "MESH" or not obj.vertex_groups:
        return []

    weights: dict[int, float] = {}
    for vertex in obj.data.vertices:
        for assignment in vertex.groups:
            weights[assignment.group] = weights.get(assignment.group, 0.0) + float(assignment.weight)

    ranked = sorted(weights.items(), key=lambda pair: pair[1], reverse=True)[:limit]
    result: list[dict[str, Any]] = []
    for group_index, weight_sum in ranked:
        if group_index >= len(obj.vertex_groups):
            continue
        result.append({
            "group": obj.vertex_groups[group_index].name,
            "weight_sum": round(weight_sum, 4),
        })
    return result


def _candidate_record(obj: bpy.types.Object) -> dict[str, Any]:
    materials = []
    for slot in obj.material_slots:
        materials.append(slot.material.name if slot.material else None)

    armature_modifiers = [
        modifier.object.name if modifier.object else None
        for modifier in obj.modifiers
        if modifier.type == "ARMATURE"
    ]

    return {
        "object": obj.name,
        "vertex_count": len(obj.data.vertices),
        "polygon_count": len(obj.data.polygons),
        "material_slots": materials,
        "armature_modifiers": armature_modifiers,
        "vertex_group_count": len(obj.vertex_groups),
        "top_bone_influences": _bone_influence_summary(obj),
        "world_bbox": _world_bbox(obj),
        "recommended_classification": None,
        "notes": "Classify visually before export; do not infer gear slot from object index alone.",
    }


def main() -> None:
    source = bpy.data.objects.get(SOURCE_OBJECT_NAME)
    if source is None:
        raise RuntimeError(f"Source object '{SOURCE_OBJECT_NAME}' was not found in the opened .blend file.")
    if source.type != "MESH":
        raise RuntimeError(f"Source object '{SOURCE_OBJECT_NAME}' is type {source.type}, expected MESH.")

    collection = _ensure_candidate_collection()
    duplicate = _duplicate_source(source, collection)
    candidates = _separate_loose_parts(duplicate)

    # Ensure every created candidate lives in the dedicated collection and has a
    # deterministic name without modifying the original `sol_8_low` object.
    for index, candidate in enumerate(candidates):
        candidate.name = f"{WORKING_PREFIX}_{index:03d}"
        _link_only_to_collection(candidate, collection)

    report_path = Path(os.environ.get("TU_SWAT_REPORT_JSON", str(_default_report_path()))).resolve()
    working_path = Path(os.environ.get("TU_SWAT_WORKING_BLEND", str(_default_working_path()))).resolve()
    report_path.parent.mkdir(parents=True, exist_ok=True)
    working_path.parent.mkdir(parents=True, exist_ok=True)

    report = {
        "source_blend": str(Path(bpy.data.filepath).resolve()) if bpy.data.filepath else None,
        "source_object": SOURCE_OBJECT_NAME,
        "source_object_preserved": True,
        "candidate_collection": CANDIDATE_COLLECTION_NAME,
        "candidate_count": len(candidates),
        "candidates": [_candidate_record(candidate) for candidate in candidates],
        "next_step": (
            "Open the working blend, visually classify candidates, verify hidden body coverage, "
            "then merge/split candidates as needed before exporting final equipment pieces."
        ),
    }

    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    bpy.ops.wm.save_as_mainfile(filepath=str(working_path), copy=False)

    print(f"[TheUnit] Source preserved: {SOURCE_OBJECT_NAME}")
    print(f"[TheUnit] Candidate pieces: {len(candidates)}")
    print(f"[TheUnit] Report: {report_path}")
    print(f"[TheUnit] Working blend: {working_path}")


if __name__ == "__main__":
    main()
