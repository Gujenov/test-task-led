#!/usr/bin/env python3
import datetime
import os
import re
import sys


VERSION_DEFINE_PATTERN = r'#define\s+FIRMWARE_VERSION\s+"([^"]+)"'
VERSION_VALUE_PATTERN = r'^(\d+)\.([A-Z]\d)\.(\d+)\.(\d{6})(?:-(\d{2}))?$'


def compute_next_version(current_version, today_yy_mm_dd):
    match = re.match(VERSION_VALUE_PATTERN, current_version)
    if not match:
        raise ValueError(
            "Unsupported firmware version format. Expected MCU.HW_VARIANT.RELEASE_TYPE.YYMMDD-XX"
        )

    mcu, hw_variant, release_type, build_date, build_index = match.groups()

    if build_date == today_yy_mm_dd:
        current_index = int(build_index) if build_index is not None else 0
        if current_index >= 99:
            raise ValueError("Build index overflow: version index already reached 99 for today")
        next_index = current_index + 1
    else:
        next_index = 0

    return f"{mcu}.{hw_variant}.{release_type}.{today_yy_mm_dd}-{next_index:02d}"


def update_version(project_dir=None):
    if project_dir is None:
        project_dir = os.getcwd()

    source_path = os.path.join(project_dir, "main", "test-task-led.c")
    today = datetime.datetime.now().strftime("%y%m%d")

    if not os.path.exists(source_path):
        print(f"[ERROR] File not found: {source_path}")
        return False

    try:
        with open(source_path, "r", encoding="utf-8") as source_file:
            content = source_file.read()

        define_match = re.search(VERSION_DEFINE_PATTERN, content)
        if not define_match:
            print("[ERROR] FIRMWARE_VERSION define not found in main/test-task-led.c")
            return False

        current_version = define_match.group(1)
        new_version = compute_next_version(current_version, today)

        updated_content = re.sub(
            VERSION_DEFINE_PATTERN,
            f'#define FIRMWARE_VERSION            "{new_version}"',
            content,
            count=1,
        )

        with open(source_path, "w", encoding="utf-8") as source_file:
            source_file.write(updated_content)

        print(f"[OK] Firmware version updated: {current_version} -> {new_version}")
        return True
    except Exception as error:
        print(f"[ERROR] Failed to update firmware version: {error}")
        return False


def main():
    project_dir = os.getcwd() if len(sys.argv) < 2 else sys.argv[1]
    return 0 if update_version(project_dir) else 1


if __name__ == "__main__":
    raise SystemExit(main())