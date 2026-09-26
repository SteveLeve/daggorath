#!/usr/bin/env bash
# PostToolUse: clang-format edited C++ files. No-op if clang-format is absent.
command -v clang-format >/dev/null || exit 0
path=$(jq -r '.tool_input.file_path // empty')
case "$path" in
  *.cpp|*.hpp|*.h|*.cc) [ -f "$path" ] && clang-format -i "$path" ;;
esac
exit 0
