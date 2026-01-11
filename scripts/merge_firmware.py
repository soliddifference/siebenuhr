"""
PlatformIO post-build script to create merged firmware binaries.

Merged binaries combine bootloader, partition table, boot_app0, and application
into a single file that can be flashed to offset 0x0. This is required for
web-based ESP flasher tools that expect a complete flash image.

Output: .pio/build/{env}/firmware-{env}-merged.bin
"""

Import("env")


def merge_bin(source, target, env):
    """Create a merged firmware binary for web flasher tools."""
    from os.path import join, exists
    import subprocess

    build_dir = env.subst("$BUILD_DIR")
    board = env.BoardConfig()
    mcu = board.get("build.mcu", "esp32")

    # PlatformIO generates bootloader.bin and partitions.bin in the build directory
    bootloader = join(build_dir, "bootloader.bin")
    partitions = join(build_dir, "partitions.bin")
    firmware = join(build_dir, "firmware.bin")

    # boot_app0.bin is in the framework's partitions directory
    platform = env.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif32")
    boot_app0 = join(framework_dir, "tools", "partitions", "boot_app0.bin")

    # Get esptool from PlatformIO's tool-esptoolpy package
    esptool_dir = platform.get_package_dir("tool-esptoolpy")
    esptool_py = join(esptool_dir, "esptool.py")

    # Output file includes environment name
    env_name = env.subst("$PIOENV")
    merged = join(build_dir, f"firmware-{env_name}-merged.bin")

    # Verify all input files exist
    missing = []
    for filepath, name in [
        (bootloader, "bootloader"),
        (boot_app0, "boot_app0"),
        (partitions, "partitions"),
        (firmware, "firmware"),
        (esptool_py, "esptool.py"),
    ]:
        if not exists(filepath):
            missing.append(f"{name}: {filepath}")

    if missing:
        print("\nERROR: Cannot create merged binary, missing files:")
        for m in missing:
            print(f"  - {m}")
        return

    # ESP32 flash layout offsets
    # 0x1000  - bootloader
    # 0x8000  - partition table
    # 0xe000  - boot_app0 (OTA data)
    # 0x10000 - application
    # Use PlatformIO's esptool.py from tool-esptoolpy package
    cmd = [
        env.subst("$PYTHONEXE"), esptool_py,
        "--chip", mcu,
        "merge_bin",
        "-o", merged,
        "--flash_mode", "dio",
        "--flash_freq", "40m",
        "--flash_size", "4MB",
        "0x1000", bootloader,
        "0x8000", partitions,
        "0xe000", boot_app0,
        "0x10000", firmware,
    ]

    print(f"\nCreating merged firmware...")
    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode == 0:
        print(f"Created: {merged}")
    else:
        print(f"ERROR creating merged binary:")
        print(result.stderr)


env.AddPostAction("$BUILD_DIR/firmware.bin", merge_bin)
