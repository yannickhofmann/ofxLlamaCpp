#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
ADDON_DIR = SCRIPT_DIR.parent
ADDONS_DIR = ADDON_DIR.parent


def run(cmd, cwd=None) -> None:
    subprocess.run(cmd, cwd=cwd, check=True)


def parse_args():
    parser = argparse.ArgumentParser(description="Run setup_libs first and then build_llama_static.")
    parser.add_argument("--enable-cuda", action="store_true")
    parser.add_argument("--enable-vulkan", action="store_true")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--purge-build", action="store_true")
    parser.add_argument("--purge-all", action="store_true")
    parser.add_argument("--generator", default="Visual Studio 17 2022")
    parser.add_argument("--platform", default="x64")
    parser.add_argument("--configurations", nargs="+", default=None)
    return parser.parse_args()


def find_powershell():
    for candidate in ("pwsh", "powershell"):
        resolved = shutil.which(candidate)
        if resolved:
            return resolved
    raise SystemExit("pwsh or powershell was not found in PATH.")


def default_external_build_dir() -> Path:
    if sys.platform.startswith("win"):
        return ADDONS_DIR / "ofxLlamaCpp_build" / "windows" / "build"
    if sys.platform == "darwin":
        return ADDONS_DIR / "ofxLlamaCpp_build" / "macos" / "build"
    return ADDONS_DIR / "ofxLlamaCpp_build" / "linux" / "build"


def main():
    args = parse_args()
    build_dir = default_external_build_dir()

    print("=========================================", flush=True)
    print(" ofxLlamaCpp Install Script", flush=True)
    print("=========================================", flush=True)
    print("Step 1/2: setup_libs", flush=True)

    if sys.platform.startswith("win"):
        powershell = find_powershell()
        run(
            [
                powershell,
                "-NoProfile",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(SCRIPT_DIR / "setup_libs.ps1"),
            ],
            cwd=SCRIPT_DIR,
        )

        print("Step 2/2: build_llama_static", flush=True)

        build_cmd = [
            powershell,
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(SCRIPT_DIR / "build_llama_static.ps1"),
            "-BuildDir",
            str(build_dir),
            "-Generator",
            args.generator,
            "-Platform",
            args.platform,
        ]
        if args.enable_cuda:
            build_cmd.append("-EnableCuda")
        if args.enable_vulkan:
            build_cmd.append("-EnableVulkan")
        if args.clean:
            build_cmd.append("-Clean")
        if args.purge_build:
            build_cmd.append("-PurgeBuild")
        if args.purge_all:
            build_cmd.append("-PurgeAll")
        if args.configurations:
            build_cmd += ["-Configurations", *args.configurations]

        run(build_cmd, cwd=SCRIPT_DIR)
    else:
        run(["bash", str(SCRIPT_DIR / "setup_libs.sh")], cwd=SCRIPT_DIR)

        print("Step 2/2: build_llama_static", flush=True)

        build_cmd = [
            "bash",
            str(SCRIPT_DIR / "build_llama_static.sh"),
            "--build-dir",
            str(build_dir),
        ]
        if args.clean:
            build_cmd.append("--clean")
        if args.purge_build:
            build_cmd.append("--purge-build")
        if args.purge_all:
            build_cmd.append("--purge-all")
        if args.enable_cuda:
            print("Warning: --enable-cuda is not required on Linux/macOS because build_llama_static.sh auto-detects CUDA.", flush=True)
        if args.enable_vulkan:
            print("Info: enabling Vulkan via OFX_LLAMACPP_ENABLE_VULKAN=1 for the build script.", flush=True)
            env = dict(os.environ)
            env["OFX_LLAMACPP_ENABLE_VULKAN"] = "1"
            subprocess.run(build_cmd, cwd=SCRIPT_DIR, env=env, check=True)
        else:
            run(build_cmd, cwd=SCRIPT_DIR)

    print("=========================================", flush=True)
    print("ofxLlamaCpp setup and build complete", flush=True)
    print("=========================================", flush=True)


if __name__ == "__main__":
    main()
