"""Create the two minimal UE maps required by The Unit's end-to-end prototype loop.

Run through Tools/Unreal/create_end_to_end_maps.ps1 after the C++ module builds.
The maps intentionally stay nearly empty: their GameModes bootstrap the hideout,
kill house, extraction zone and player starts at runtime.
"""

from __future__ import annotations

from pathlib import Path
import configparser
import unreal

MAP_DIR = "/Game/TheUnit/Maps"
COMMAND_CENTER = f"{MAP_DIR}/CommandCenter"
KILLHOUSE = f"{MAP_DIR}/Killhouse"

HIDEOUT_GAME_MODE = "/Script/TheUnit.TU_HideoutGameMode"
MISSION_GAME_MODE = "/Script/TheUnit.TU_TrainingMissionGameMode"


def _editor_world():
    subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = subsystem.get_editor_world() if subsystem else None
    if world is None:
        raise RuntimeError("Unable to resolve the current editor world.")
    return world


def _level_subsystem():
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if subsystem is None:
        raise RuntimeError("LevelEditorSubsystem is unavailable.")
    return subsystem


def _open_or_create_map(package_path: str) -> None:
    levels = _level_subsystem()
    if unreal.EditorAssetLibrary.does_asset_exist(package_path):
        if not levels.load_level(package_path):
            raise RuntimeError(f"Failed to load existing level: {package_path}")
    else:
        if not levels.new_level(package_path):
            raise RuntimeError(f"Failed to create level: {package_path}")


def _assign_game_mode(game_mode_path: str) -> None:
    game_mode_class = unreal.load_class(None, game_mode_path)
    if game_mode_class is None:
        raise RuntimeError(
            f"Could not load {game_mode_path}. Build the TheUnit C++ module before running this script."
        )

    world_settings = _editor_world().get_world_settings()
    if world_settings is None:
        raise RuntimeError("Current level has no WorldSettings actor.")
    world_settings.set_editor_property("default_game_mode", game_mode_class)


def _save_current_level() -> None:
    if not _level_subsystem().save_current_level():
        raise RuntimeError("Failed to save current level.")


def _write_startup_map_config() -> None:
    """Persist startup/default map in DefaultEngine.ini without overwriting unrelated settings."""
    config_path = Path(unreal.Paths.project_config_dir()) / "DefaultEngine.ini"
    config = configparser.ConfigParser(strict=False)
    config.optionxform = str
    if config_path.exists():
        config.read(config_path, encoding="utf-8")

    section = "/Script/EngineSettings.GameMapsSettings"
    if section not in config:
        config.add_section(section)

    command_center_object = f"{COMMAND_CENTER}.CommandCenter"
    config[section]["EditorStartupMap"] = command_center_object
    config[section]["GameDefaultMap"] = command_center_object
    config[section]["GlobalDefaultGameMode"] = HIDEOUT_GAME_MODE

    config_path.parent.mkdir(parents=True, exist_ok=True)
    with config_path.open("w", encoding="utf-8") as stream:
        config.write(stream, space_around_delimiters=False)

    unreal.log(f"[TheUnit] Updated startup map config: {config_path}")


def main() -> None:
    unreal.log("[TheUnit] Creating end-to-end prototype maps...")

    _open_or_create_map(COMMAND_CENTER)
    _assign_game_mode(HIDEOUT_GAME_MODE)
    _save_current_level()
    unreal.log(f"[TheUnit] Ready: {COMMAND_CENTER}")

    _open_or_create_map(KILLHOUSE)
    _assign_game_mode(MISSION_GAME_MODE)
    _save_current_level()
    unreal.log(f"[TheUnit] Ready: {KILLHOUSE}")

    # Return the editor to HQ and make HQ the next startup/default map.
    _level_subsystem().load_level(COMMAND_CENTER)
    _write_startup_map_config()

    unreal.log("[TheUnit] Prototype map bootstrap complete.")


if __name__ == "__main__":
    main()
