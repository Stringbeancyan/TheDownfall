"""First interactive editor launch creates the shipped scene recipe once.

Unreal discovers this file on its project Python path. Defer work until the
editor has finished opening its startup world. Existing maps are never rebuilt.
"""
import os
from pathlib import Path
import runpy
import traceback

import unreal

MAP = "/Game/Downfall/Maps/L_Breakwater_District"
_handle = None
_elapsed = 0.0


def _create_missing_scene(delta_seconds):
    global _elapsed
    _elapsed += delta_seconds
    if _elapsed < 2.0:
        return
    unreal.unregister_slate_post_tick_callback(_handle)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        return
    try:
        root = Path(unreal.Paths.project_dir()).resolve()
        unreal.log("DOWNFALL: creating Breakwater. Please wait for shader compilation before playing.")
        runpy.run_path(str(root / "Scripts/build_vertical_slice.py"), run_name="__main__")
    except Exception:
        unreal.log_error(traceback.format_exc())
        unreal.EditorDialog.show_message(
            "Breakwater setup needs attention",
            "The scene could not be generated. See Window > Output Log for the Python error. "
            "Save your work, then run Scripts/build_vertical_slice.py using Tools > Execute Python Script.",
            unreal.AppMsgType.OK)


# Commandlets/cooks explicitly generate content before cooking; never modify it
# from a background build. An existing scene also avoids registering a callback.
command_line = unreal.SystemLibrary.get_command_line().lower()
if (os.environ.get("DOWNFALL_SKIP_AUTOBUILD") != "1"
        and not any(flag in command_line for flag in ("-run=", "-game", "-server", "-executepythonscript", "-unattended"))
        and not unreal.EditorAssetLibrary.does_asset_exist(MAP)):
    _handle = unreal.register_slate_post_tick_callback(_create_missing_scene)
