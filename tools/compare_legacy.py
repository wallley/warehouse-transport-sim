#!/usr/bin/env python3
"""以固定种子比较原单文件与模块化核心；生成文件均写入忽略的构建目录。"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import sys
from typing import Any

PROJECT_ROOT = Path(__file__).resolve().parents[1]
CHECKPOINTS = "0,1,30,60,120,300,500,600,900,1200,1800,3600,7200,10800"
TOLERANCE = 1e-8

LEGACY_MAIN = r"""
#define main legacy_gui_main
#include "@LEGACY_SOURCE@"
#undef main
#include "parity_snapshot.hpp"

// 原版只在业务函数中打印日志；关闭输出，不打开任何日志文件或图形窗口。
int main(int argc, char** argv) {
    try {
        const parity::ProbeOptions options = parity::parseOptions(argc, argv);
        if (options.help) {
            parity::printHelp();
            return 0;
        }
        std::cout.setstate(std::ios::failbit);
        PATROL_MODE_ACTIVE = options.mode == "patrol";
        SYSTEM_TIME_STEP = options.time_step;
        SYSTEM_ELAPSED_TIME = 0.0;
        SYSTEM_PAUSED = false;
        ENABLE_PARTICLES = false;
        ENABLE_MATRIX_RAIN = false;
        ENABLE_ANIMATIONS = false;
        ENABLE_GLOW_EFFECTS = false;
        srand(options.seed);
        initializeWorkStations();
        initializeTransportUnits(options.units);
        if (PATROL_MODE_ACTIVE) {
            system_cargo_orders.clear();
            station_order_queues.clear();
        } else {
            loadCargoOrders();
            initializeScheduleMode();
        }
        parity::runProbe(options, system_cargo_orders, system_transport_units,
                         system_work_stations, station_order_queues, SYSTEM_ELAPSED_TIME,
                         []() {
            if (!PATROL_MODE_ACTIVE) {
                for (auto& entry : system_work_stations) {
                    refresh_station_status(entry.second, system_cargo_orders, station_order_queues);
                }
                allocate_cargo_orders(system_transport_units, system_cargo_orders,
                                      system_work_stations);
            }
            for (int index = 0; index < static_cast<int>(system_transport_units.size()); ++index) {
                process_transport_dynamics(system_transport_units[index], index,
                                           system_transport_units, system_work_stations,
                                           system_cargo_orders);
            }
            SYSTEM_ELAPSED_TIME += SYSTEM_TIME_STEP;
            return !PATROL_MODE_ACTIVE && allOrdersCompleted();
        });
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Legacy parity probe: " << error.what() << '\n';
        return 2;
    }
}
"""

LEGACY_CMAKE = r"""
cmake_minimum_required(VERSION 3.20)
project(LegacyParityProbe LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "原版对照需要 Windows、MSVC 和 EasyX ANSI 库。")
endif()
set(EASYX_ROOT "" CACHE PATH "EasyX 根目录，含 include 和 lib 子目录")
file(GLOB _easyx_vs_roots
    "C:/Program Files/Microsoft Visual Studio/*/*/VC/Auxiliary/VS"
    "C:/Program Files (x86)/Microsoft Visual Studio/*/*/VC/Auxiliary/VS")
set(_roots "${EASYX_ROOT}" "$ENV{EASYX_ROOT}" "$ENV{VCINSTALLDIR}/Auxiliary/VS" ${_easyx_vs_roots})
set(_includes)
set(_libraries)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_arch x64)
else()
    set(_arch x86)
endif()
foreach(_root IN LISTS _roots)
    if(_root)
        list(APPEND _includes "${_root}/include" "${_root}")
        list(APPEND _libraries "${_root}/lib/${_arch}" "${_root}/lib" "${_root}")
    endif()
endforeach()
find_path(EASYX_INCLUDE_DIR graphics.h HINTS ${_includes})
find_library(EASYX_LIBRARY NAMES EasyXa HINTS ${_libraries})
if(NOT EASYX_INCLUDE_DIR OR NOT EASYX_LIBRARY)
    message(FATAL_ERROR "未找到 EasyX；使用 --easyx-root 指定其根目录。")
endif()
add_executable(legacy_parity_probe legacy_main.cpp)
target_compile_features(legacy_parity_probe PRIVATE cxx_std_17)
target_compile_options(legacy_parity_probe PRIVATE /utf-8)
target_compile_definitions(legacy_parity_probe PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
target_include_directories(legacy_parity_probe PRIVATE "${EASYX_INCLUDE_DIR}" "@SNAPSHOT_INCLUDE@")
get_filename_component(_easyx_lib_dir "${EASYX_LIBRARY}" DIRECTORY)
target_link_directories(legacy_parity_probe PRIVATE "${_easyx_lib_dir}")
target_link_libraries(legacy_parity_probe PRIVATE "${EASYX_LIBRARY}" winmm msimg32 gdi32 user32)
"""


def run(command: list[str], work_dir: Path, log_name: str) -> None:
    """将完整工具输出保存到构建目录，控制台仅报告失败。"""
    result = subprocess.run(
        command, cwd=work_dir, capture_output=True, text=True,
        encoding="utf-8", errors="replace", check=False,
    )
    log = result.stdout + result.stderr
    (work_dir / log_name).write_text(log, encoding="utf-8")
    if result.returncode != 0:
        raise RuntimeError(
            f"Command failed with exit code {result.returncode}: {command[0]}\n"
            f"Details: {work_dir / log_name}\n{log[-8000:]}"
        )


def cmake_quote(path: Path) -> str:
    text = path.as_posix()
    if any(character in text for character in ['"', "\n", "\r", ";"]):
        raise ValueError("路径含有无法安全传入 CMake 的字符")
    return text


def build_legacy(args: argparse.Namespace, work_dir: Path) -> Path:
    legacy_source = Path(args.legacy_source).resolve()
    if not legacy_source.is_file():
        raise ValueError(f"找不到原版源码：{legacy_source}")
    include_path = cmake_quote(legacy_source)
    (work_dir / "legacy_main.cpp").write_text(
        LEGACY_MAIN.replace("@LEGACY_SOURCE@", include_path), encoding="utf-8"
    )
    (work_dir / "CMakeLists.txt").write_text(
        LEGACY_CMAKE.replace("@SNAPSHOT_INCLUDE@", cmake_quote(PROJECT_ROOT / "tests")),
        encoding="utf-8",
    )
    build_dir = work_dir / "build"
    configure = [args.cmake, "-S", str(work_dir), "-B", str(build_dir)]
    if args.generator:
        configure.extend(["-G", args.generator])
        if args.generator.startswith("Visual Studio"):
            configure.extend(["-A", "x64"])
    if args.easyx_root:
        configure.append(f"-DEASYX_ROOT={Path(args.easyx_root).resolve().as_posix()}")
    run(configure, work_dir, "configure.log")
    run(
        [args.cmake, "--build", str(build_dir), "--config", args.configuration,
         "--target", "legacy_parity_probe"],
        work_dir, "build.log",
    )
    for executable in (
        build_dir / args.configuration / "legacy_parity_probe.exe",
        build_dir / "legacy_parity_probe.exe",
    ):
        if executable.is_file():
            return executable
    raise RuntimeError("原版编译成功，但未找到 legacy_parity_probe.exe")


def differences(reference: Any, candidate: Any, path: str = "$", limit: int = 12) -> list[str]:
    """整数、布尔、字符串和结构精确比较；浮点采用绝对误差上限。"""
    found: list[str] = []

    def compare(expected: Any, actual: Any, location: str) -> None:
        if len(found) >= limit:
            return
        numeric = (
            isinstance(expected, (int, float)) and not isinstance(expected, bool)
            and isinstance(actual, (int, float)) and not isinstance(actual, bool)
        )
        if numeric and (isinstance(expected, float) or isinstance(actual, float)):
            if not math.isfinite(expected) or not math.isfinite(actual):
                found.append(f"{location}: 非有限浮点值")
            elif abs(expected - actual) > TOLERANCE:
                found.append(f"{location}: {expected:.17g} != {actual:.17g}")
        elif type(expected) is not type(actual):
            found.append(f"{location}: 类型 {type(expected).__name__} != {type(actual).__name__}")
        elif isinstance(expected, dict):
            if expected.keys() != actual.keys():
                found.append(f"{location}: JSON 对象字段不一致")
                return
            for key in expected:
                compare(expected[key], actual[key], f"{location}.{key}")
        elif isinstance(expected, list):
            if len(expected) != len(actual):
                found.append(f"{location}: 长度 {len(expected)} != {len(actual)}")
                return
            for index, (left, right) in enumerate(zip(expected, actual)):
                compare(left, right, f"{location}[{index}]")
        elif expected != actual:
            found.append(f"{location}: {expected!r} != {actual!r}")

    compare(reference, candidate, path)
    return found


def check_coverage(document: dict[str, Any]) -> list[str]:
    issues: list[str] = []
    final = document["final"]["state"]
    if document["mode"] == "schedule":
        if len(final["orders"]) != 108:
            issues.append("未覆盖完整的 108 个嵌入任务")
        if not document["observed_idle_reposition"]:
            issues.append("未观察到随机待命移动")
        if document["termination"] == "time-limit":
            if final["elapsed_time"] < 10800.0 - 1e-6:
                issues.append("未运行到调度的 3 小时上限")
        elif document["termination"] != "schedule-complete":
            issues.append("调度终止原因无效")
        if document["termination"] == "schedule-complete" and document["finished_orders"] != 108:
            issues.append("标记完成但完成任务数不是 108")
    else:
        if document["termination"] != "patrol-halted" and final["elapsed_time"] <= 500.0:
            issues.append("巡游既未最终停车，也未运行超过 500 秒")
        if len(final["units"]) != 3:
            issues.append("巡游车辆数不是 3")
    return issues


def compare_scenario(args: argparse.Namespace, work_dir: Path, legacy: Path,
                     probe: Path, mode: str, units: int) -> dict[str, Any]:
    label = f"{mode}-{units}"
    maximum = 10800 if mode == "schedule" else 1200
    common = [
        "--mode", mode, "--units", str(units), "--seed", str(args.seed),
        "--max-time", str(maximum), "--checkpoints", CHECKPOINTS,
    ]
    legacy_output = work_dir / f"{label}-legacy.json"
    new_output = work_dir / f"{label}-modular.json"
    run([str(legacy), *common, "--output", str(legacy_output)], work_dir, f"{label}-legacy.log")
    run([str(probe), *common, "--output", str(new_output)], work_dir, f"{label}-modular.log")
    reference = json.loads(legacy_output.read_text(encoding="utf-8"))
    candidate = json.loads(new_output.read_text(encoding="utf-8"))
    diff = differences(reference, candidate)
    coverage = check_coverage(reference) + check_coverage(candidate)
    passed = not diff and not coverage
    termination = reference["termination"]
    finished = reference["finished_orders"]
    elapsed = reference["final"]["state"]["elapsed_time"]
    task_summary = f"finished={finished}/108, " if mode == "schedule" else ""
    print(
        f"{'PASS' if passed else 'FAIL'} {label}: {termination}, "
        f"{task_summary}elapsed={elapsed:.2f}s, "
        f"checkpoints={len(reference['snapshots'])}"
    )
    if (mode == "schedule" and termination == "time-limit"
            and candidate["termination"] == "time-limit"
            and candidate["final"]["state"]["elapsed_time"] >= 10800.0 - 1e-6):
        print(f"  原版与新版均到达 3 小时上限；如实保留 {finished}/108 的完成状态。")
    for item in [*diff, *coverage]:
        print(f"  {item}")
    return {
        "scenario": label, "passed": passed, "termination": termination,
        "finished_orders": finished, "elapsed_time": elapsed,
        "checkpoint_count": len(reference["snapshots"]),
        "observed_idle_reposition": reference["observed_idle_reposition"],
        "differences": diff, "coverage_issues": coverage,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--legacy-source", required=True, help="原始单文件源码，保持只读")
    parser.add_argument("--probe", required=True, help="模块化工程 warehouse_parity_probe.exe")
    parser.add_argument("--work-dir", default=str(PROJECT_ROOT / "build" / "legacy-parity"))
    parser.add_argument("--easyx-root", default=os.environ.get("EASYX_ROOT"))
    parser.add_argument("--cmake", default="cmake")
    parser.add_argument("--generator", default="Visual Studio 17 2022" if os.name == "nt" else None)
    parser.add_argument("--configuration", default="Release")
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()
    if not 0 <= args.seed <= 0xFFFFFFFF:
        parser.error("--seed 必须在 uint32 范围内")
    work_dir = Path(args.work_dir).resolve()
    probe = Path(args.probe).resolve()
    try:
        if os.name != "nt":
            raise ValueError("原版包含 Windows/EasyX，算法对照须在 Windows 上运行")
        if not probe.is_file():
            raise ValueError(f"找不到模块化探针：{probe}")
        legacy_source = Path(args.legacy_source).resolve()
        work_dir.mkdir(parents=True, exist_ok=True)
        before_hash = hashlib.sha256(legacy_source.read_bytes()).hexdigest()
        legacy = build_legacy(args, work_dir)
        results = [
            compare_scenario(args, work_dir, legacy, probe, "schedule", units)
            for units in (3, 5, 7)
        ]
        results.append(compare_scenario(args, work_dir, legacy, probe, "patrol", 3))
        after_hash = hashlib.sha256(legacy_source.read_bytes()).hexdigest()
        if before_hash != after_hash:
            raise RuntimeError("验证过程中原始源码发生变化，请重新运行")
        report = {
            "seed": args.seed, "float_absolute_tolerance": TOLERANCE,
            "legacy_source_sha256": before_hash, "scenarios": results,
            "passed": all(result["passed"] for result in results),
        }
        report_path = work_dir / "comparison-report.json"
        report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print(f"对照报告：{report_path}")
        return 0 if report["passed"] else 1
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print(f"算法对照失败：{error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
