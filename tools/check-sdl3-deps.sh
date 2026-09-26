#!/usr/bin/env bash
# Report which packages needed to build SDL3 from source are missing (Debian/Ubuntu).
# Usage: tools/check-sdl3-deps.sh [--install]
set -euo pipefail

pkgs=(
  build-essential cmake pkg-config git
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev
  libwayland-dev libxkbcommon-dev libdecor-0-dev
  libasound2-dev libpulse-dev
  libdrm-dev libgbm-dev libgl1-mesa-dev libegl1-mesa-dev
  libudev-dev libdbus-1-dev
)

missing=()
for p in "${pkgs[@]}"; do
  if dpkg-query -W -f='${Status}' "$p" 2>/dev/null | grep -q "install ok installed"; then
    printf '  ok       %s\n' "$p"
  else
    printf '  MISSING  %s\n' "$p"
    missing+=("$p")
  fi
done

echo
if pkg-config --exists sdl3 2>/dev/null; then
  echo "SDL3 found: $(pkg-config --modversion sdl3)"
else
  echo "SDL3 not found by pkg-config"
fi

if [ ${#missing[@]} -eq 0 ]; then
  echo "All build dependencies present."
  exit 0
fi

echo "${#missing[@]} missing. Install with:"
echo "  sudo apt install ${missing[*]}"
if [ "${1:-}" = "--install" ]; then
  sudo apt install -y "${missing[@]}"
fi
exit 1
