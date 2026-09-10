"""
Read-only inspection helper for the user-supplied ballistic helmet Blender source.

Run with Blender auto-execution disabled:

    blender --background --disable-autoexec "<path>/Example.blend" \
      --python Tools/Blender/ballistic_helmet_inspect.py

Optional environment variable:
    TU_HELMET_REPORT_JSON   explicit output report path

The script does not save or modify the opened .blend file. It reports mesh topology,
materials, UVs, transforms, bounds, disconnected-island counts and loaded image metadata.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any

import bmesh
import bpy


def _blend_directory() -> Path:
    if bpy.data.filepath:
        return Path(bpy.data.filepath).resolve().parent
    return Path.cwd()


def _report_path() -> Path:
    return Path(
        os.environ.get(
            "TU_HELMET_REPORT_JSON",
            str(_blend_directory() / "ballistic_helmet_inspection.json"),
        )
    ).resolve()


def _connected_component_count(mesh: bpy.types.Mesh) -> int:
    bm = bmesh.new()
    try:
        bm.from_mesh(mesh)
        unseen = set(bm.verts)
        count = 0
        while unseen:
            count += 1
            seed = unseen.pop()
            stack = [seed]
            while stack:
                vertex = stack.pop()
                for edge in vertex.link_edges:
                    other = edge.other_vert(vertex)
                    if other in unseen:
                        unseen.remove(other)
                        stack.append(other)
        return count
    finally:
        bm.free()


def _world_bounds(obj: bpy.types.Object) -> dict[str, list[float]]:
    points = [obj.matrix_world @ corner for corner in obj.bound_box]
    return {
        "min": [round(min(point[axis] for point in points), 6) for axis in range(3)],
        "max": [round(max(point[axis] for point in points), 6) for axis in range(3)],
        "size": [
            round(max(point[axis] for point in points) - min(point[axis] for point in points), 6)
            for axis in range(3)
        ],
    }


def _mesh_record(obj: bpy.types.Object) -> dict[str, Any]:
    mesh = obj.data
    materials = [
        slot.material.name if slot.material else None
        for slot in obj.material_slots
    ]

    return {
        "object": obj.name,
        "vertex_count": len(mesh.vertices),
        "edge_count": len(mesh.edges),
        "polygon_count": len(mesh.polygons),
        "triangle_count_estimate": sum(max(0, len(poly.vertices) - 2) for poly in mesh.polygons),
        "disconnected_island_count": _connected_component_count(mesh),
        "material_slots": materials,
        "uv_layers": [layer.name for layer in mesh.uv_layers],
        "has_custom_normals": bool(getattr(mesh, "has_custom_normals", False)),
        "location": [round(value, 6) for value in obj.location],
        "rotation_euler": [round(value, 6) for value in obj.rotation_euler],
        "scale": [round(value, 6) for value in obj.scale],
        "world_bounds": _world_bounds(obj),
    }


def _image_record(image: bpy.types.Image) -> dict[str, Any]:
    filepath = bpy.path.abspath(image.filepath) if image.filepath else ""
    width, height = image.size[:] if len(image.size) >= 2 else (0, 0)
    return {
        "name": image.name,
        "filepath": filepath,
        "width": int(width),
        "height": int(height),
        "channels": int(image.channels),
        "colorspace": image.colorspace_settings.name if image.colorspace_settings else None,
        "packed": image.packed_file is not None,
    }


def main() -> None:
    mesh_objects = sorted(
        (obj for obj in bpy.data.objects if obj.type == "MESH"),
        key=lambda obj: obj.name.lower(),
    )

    report = {
        "source_blend": str(Path(bpy.data.filepath).resolve()) if bpy.data.filepath else None,
        "read_only_inspection": True,
        "mesh_object_count": len(mesh_objects),
        "meshes": [_mesh_record(obj) for obj in mesh_objects],
        "images": [_image_record(image) for image in bpy.data.images],
        "recommended_runtime_form": "rigid_static_mesh_headwear",
        "recommended_unreal_asset": "SM_BallisticHelmet",
        "recommended_equipment_definition": "EQ_BallisticHelmet",
        "inspection_notes": [
            "Prefer the supplied FBX for Unreal import if scale, pivot, normals and UVs validate.",
            "Use this Blender source as the editable master for pivot/geometry/material corrections.",
            "Do not assume 3_helmet_exp.tga channel semantics from its filename; inspect channels before wiring material inputs.",
            "Do not assign an NIJ/protection rating from appearance alone.",
            "If the mesh contains multiple disconnected islands, verify whether they are legitimate helmet hardware before joining or separating them.",
        ],
    }

    output_path = _report_path()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(f"[TheUnit] Helmet mesh objects: {len(mesh_objects)}")
    print(f"[TheUnit] Report: {output_path}")
    for record in report["meshes"]:
        print(
            f"[TheUnit] {record['object']}: "
            f"{record['vertex_count']} verts, "
            f"{record['polygon_count']} polys, "
            f"{record['disconnected_island_count']} islands"
        )


if __name__ == "__main__":
    main()
