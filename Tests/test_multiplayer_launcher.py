"""Launcher argument/log handling tests; these do not run Unreal Engine."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location("run_multiplayer", Path(__file__).resolve().parents[1] / "Scripts/run_multiplayer.py")
LAUNCHER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(LAUNCHER)


class MultiplayerLauncherTests(unittest.TestCase):
    def test_separate_server_and_client_logs_with_space_paths(self):
        commands = LAUNCHER.build_commands(Path("/engine path/UnrealEditor"), Path("/project path/Downfall.uproject"), Path("/test logs"), 4, 7788)
        self.assertEqual(len(commands), 5)
        self.assertEqual(commands[0][0], "/engine path/UnrealEditor")
        self.assertIn("-nullrhi", commands[0])
        self.assertIn("-server", commands[0])
        logs = []
        for index, command in enumerate(commands):
            logs.extend(argument for argument in command if argument.startswith("-abslog="))
            if index:
                self.assertIn("127.0.0.1:7788", command)
                self.assertNotIn("-nullrhi", command)
                self.assertIn("-game", command)
        self.assertEqual(len(set(logs)), 5)

    def test_player_count_and_ports_outside_slice_contract_rejected(self):
        for count, port in [(1, 7777), (5, 7777), (2, 0), (2, 65536)]:
            with self.subTest(count=count, port=port), self.assertRaises(ValueError):
                LAUNCHER.build_commands(Path("editor"), Path("project"), Path("logs"), count, port)

    def test_readiness_matches_requested_driver_port_not_partial_numbers(self):
        with tempfile.TemporaryDirectory() as folder:
            log = Path(folder) / "server.log"
            self.assertFalse(LAUNCHER.is_server_listening(log, 7777))
            log.write_text("LogNet: IpNetDriver listening on port 77770", encoding="utf-8")
            self.assertFalse(LAUNCHER.is_server_listening(log, 7777))
            log.write_text("LogNet: IpNetDriver listening on port 7777", encoding="utf-8")
            self.assertTrue(LAUNCHER.is_server_listening(log, 7777))
            self.assertFalse(LAUNCHER.is_server_listening(log, 7788))

    def test_editor_path_per_host(self):
        self.assertEqual(LAUNCHER.editor_executable(Path("/ue"), "Linux"), Path("/ue/Engine/Binaries/Linux/UnrealEditor"))
        self.assertEqual(LAUNCHER.editor_executable(Path("/ue"), "Windows").name, "UnrealEditor.exe")
        with self.assertRaises(ValueError):
            LAUNCHER.editor_executable(Path("/ue"), "Plan9")


if __name__ == "__main__":
    unittest.main()
