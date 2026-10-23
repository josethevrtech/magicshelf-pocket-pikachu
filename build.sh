#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
sdk="${MAGICSHELF_SDK_DIR:-$PWD/vendor/magicshelf}"
case "$(uname -m)" in aarch64) platform=linuxarm64;; x86_64) platform=linux64;; *) exit 1;; esac
vr="${OPENVR_LIB_DIR:-/opt/steamvr/bin/$platform}"
mkdir -p build/tests build/package/{bin,lib,runtime,emulator,assets}
python3 tools/prepare_emulator.py
cp runtime/host.js runtime/alarm.js runtime/device.js build/package/runtime/
flags=(-O2 -std=c++17 -fPIC -pthread -DQT_NO_KEYWORDS -Isrc -I"$sdk/native/compat" -I"$sdk/native" '-DMAGICSHELF_CREATION_PROFILE="creation_profile.h"')
c++ "${flags[@]}" tests/engine_test.cpp -o build/tests/engine_test $(pkg-config --cflags --libs Qt6Qml egl glesv2 cairo libpulse-simple)
build/tests/engine_test "$PWD/build/package"
c++ "${flags[@]}" tests/alarm_test.cpp -o build/tests/alarm_test $(pkg-config --cflags --libs Qt6Qml)
build/tests/alarm_test "$PWD/build/package"
c++ "${flags[@]}" tests/navigation_test.cpp -o build/tests/navigation_test $(pkg-config --cflags --libs Qt6Qml)
build/tests/navigation_test "$PWD/build/package"
c++ "${flags[@]}" tests/progression_test.cpp -o build/tests/progression_test $(pkg-config --cflags --libs Qt6Qml)
build/tests/progression_test "$PWD/build/package"
c++ "${flags[@]}" tests/controller_test.cpp -o build/tests/controller_test $(pkg-config --cflags --libs egl glesv2 cairo)
build/tests/controller_test
c++ "${flags[@]}" tests/motion_test.cpp -o build/tests/motion_test $(pkg-config --cflags --libs egl glesv2 cairo)
build/tests/motion_test
if [[ "${1:-}" == "--tests-only" ]]; then exit 0; fi
if [[ ! -f assets/device.mesh ]]; then echo 'Import the handheld with tools/import_model.py first.' >&2; exit 1; fi
c++ "${flags[@]}" tools/texture.cpp -o build/tests/texture $(pkg-config --cflags --libs Qt6Gui)
build/tests/texture assets/body.png assets/body-0.rgba
cp assets/device.mesh assets/body-0.rgba assets/model-info.json build/package/assets/
cp -r assets/input build/package/assets/
cp "$vr/libopenvr_api.so" build/package/lib/
c++ "${flags[@]}" src/main.cpp -o build/package/bin/pocket-pikachu -L"$vr" -lopenvr_api \
  '-Wl,-rpath,$ORIGIN/../lib' $(pkg-config --cflags --libs egl glesv2 cairo json-c Qt6Qml Qt6Widgets libpulse-simple)
cp runtime/control build/package/bin/control
chmod +x build/package/bin/control
cp LICENSE OPENVR-LICENSE.txt ASSET_CREDITS.md README.md FIDELITY.md build/package/
python3 - <<'PY'
import json,platform
from pathlib import Path
meta=json.loads(Path('module.json').read_text());meta['architecture']=platform.machine()
Path('build/package/module.json').write_text(json.dumps(meta,indent=2)+'\n')
PY
mkdir -p build/package/media
build/package/bin/pocket-pikachu --preview "$PWD/build/package/media/pocket-pikachu.png"
echo 'Native creation built. Preview: build/package/bin/pocket-pikachu --desktop'
