# Source this before tools/rom/assemble.sh and run-capture.sh:
#   . tools/rom/env.sh
# Tools and firmware live outside the repository (CLAUDE.md evidence rules).
# Override DOD_TOOLS or DOD_FIRMWARE to use other locations.
DOD_TOOLS=${DOD_TOOLS:-$HOME/coco-tools}
export DOD_FIRMWARE=${DOD_FIRMWARE:-$HOME/coco-firmware}
export DOD_MAME=$DOD_TOOLS/mame/usr/games/mame
export DOD_HASHPATH=$DOD_TOOLS/mame/usr/share/games/mame/hash
export LD_LIBRARY_PATH=$DOD_TOOLS/mame/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export PATH=$DOD_TOOLS/lwtools-4.25/lwasm:$PATH
