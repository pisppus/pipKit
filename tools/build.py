"""
Build PSDF assets (fonts and icons).

Usage:
    python tools/build.py         # Run both fonts and icons
    python tools/build.py --fonts  # Run only fonts
    python tools/build.py --icons  # Run only icons

Compatible with PlatformIO's SCons environment.
"""
import sys

sys.dont_write_bytecode = True

import argparse
import os

try:
    Import("env")
    _PLATFORMIO_ENV = env
except Exception:
    _PLATFORMIO_ENV = None


def detect_project_dir():
    """Detect project directory from PlatformIO env or relative path."""
    if _PLATFORMIO_ENV:
        return _PLATFORMIO_ENV["PROJECT_DIR"]
    return os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))


def build_assets(targets=None):
    """Core build logic - can be called from main() or SCons."""
    project_dir = detect_project_dir()
    
    if not os.path.isdir(project_dir):
        print(f"[build] Error: Project directory not found: {project_dir}")
        return False

    tools_dir = os.path.join(project_dir, "tools")
    if tools_dir not in sys.path:
        sys.path.insert(0, tools_dir)

    run_fonts = not targets or "fonts" in targets
    run_icons = not targets or "icons" in targets
    
    success = True

    if run_fonts:
        print("[build] === Generating fonts ===")
        try:
            from fonts.generate import generate_all as generate_fonts
            if not generate_fonts(project_dir):
                success = False
        except Exception as e:
            print(f"[build] Font generation failed: {e}")
            success = False

    if run_icons:
        print("[build] === Generating icons ===")
        try:
            from icons.generate import generate_all as generate_icons
            if not generate_icons(project_dir):
                success = False
        except Exception as e:
            print(f"[build] Icon generation failed: {e}")
            success = False

    if success:
        print("[build] === Done ===")
    else:
        print("[build] === Completed with errors ===")
    
    return success


def main():
    parser = argparse.ArgumentParser(description="Build PSDF assets (fonts and icons)")
    parser.add_argument("--fonts", "--fonts-only", action="store_true", help="Generate only fonts")
    parser.add_argument("--icons", "--icons-only", action="store_true", help="Generate only icons")
    parser.add_argument("--project-dir", type=str, default=None, help="Override project directory")
    args = parser.parse_args()

    targets = []
    if args.fonts:
        targets.append("fonts")
    if args.icons:
        targets.append("icons")
    
    if args.project_dir:
        global _PROJECT_DIR_OVERRIDE
        _PROJECT_DIR_OVERRIDE = args.project_dir

    success = build_assets(targets if targets else None)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()

if _PLATFORMIO_ENV:
    print("[build] Running from PlatformIO SCons environment")
    build_assets()
