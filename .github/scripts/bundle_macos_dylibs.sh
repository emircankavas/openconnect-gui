#!/usr/bin/env bash
#
# Embed non-Qt (Homebrew) dynamic libraries into a macOS .app bundle and
# rewrite their install names so the app runs on machines without Homebrew.
#
# Qt frameworks and Qt plugins are handled separately by macdeployqt; this
# script only walks plain .dylib dependencies (openconnect, gnutls, spdlog,
# fmt, nettle, hogweed, gmp, p11-kit, libidn2, ...) recursively.
#
# Usage: bundle_macos_dylibs.sh /path/to/App.app
#
set -euo pipefail

APP="${1:?usage: bundle_macos_dylibs.sh /path/to/App.app}"
if [ ! -d "$APP" ]; then
  echo "error: not a bundle: $APP" >&2
  exit 1
fi

FW="$APP/Contents/Frameworks"
FRAMEWORKS_PREFIX="@executable_path/../Frameworks"
mkdir -p "$FW"

is_system() {
  case "$1" in
    /usr/lib/*|/System/*) return 0 ;;
    *) return 1 ;;
  esac
}

is_macho() {
  case "$(file -b "$1" 2>/dev/null)" in
    *Mach-O*) return 0 ;;
    *) return 1 ;;
  esac
}

# Resolve an @rpath/@loader_path reference using the loader commands of $1.
resolve_ref() {
  local file="$1" ref="$2" name="${2#@rpath/}"
  name="${name#@loader_path/}"
  local rp
  while IFS= read -r rp; do
    [ -n "$rp" ] || continue
    case "$rp" in
      @executable_path*) rp="${rp/@executable_path/$(dirname "$file")}" ;;
      @loader_path*)     rp="${rp/@loader_path/$(dirname "$file")}" ;;
    esac
    if [ -f "$rp/$name" ]; then
      printf '%s\n' "$rp/$name"
      return 0
    fi
  done < <(otool -l "$file" | awk '/LC_RPATH/{getline; getline; print $2}')
  return 1
}

echo "== bundling non-Qt dylibs into $FW =="

pass=0
progress=1
while [ "$progress" -eq 1 ]; do
  progress=0
  pass=$((pass + 1))
  echo "-- pass $pass --"

  while IFS= read -r -d '' f; do
    is_macho "$f" || continue

    while IFS= read -r dep; do
      [ -n "$dep" ] || continue

      # system / already-bundled dependencies need no action
      if is_system "$dep"; then continue; fi
      case "$dep" in
        "$FRAMEWORKS_PREFIX"/*) continue ;;
      esac

      # resolve relative loader references to an absolute path
      abs="$dep"
      case "$dep" in
        @rpath/*|@loader_path/*)
          if ! abs="$(resolve_ref "$f" "$dep")"; then
            echo "   WARN: unresolved $dep (in ${f#$APP/})" >&2
            continue
          fi
          ;;
      esac

      # leave .framework deps to macdeployqt
      case "$abs" in
        *.framework/*) continue ;;
      esac

      base="$(basename "$abs")"
      target="$FW/$base"

      if [ ! -f "$target" ]; then
        cp -f "$abs" "$target"
        chmod u+w "$target"
        install_name_tool -id "$FRAMEWORKS_PREFIX/$base" "$target" 2>/dev/null || true
        progress=1
      fi

      install_name_tool -change "$dep" "$FRAMEWORKS_PREFIX/$base" "$f" 2>/dev/null || true
    done < <(otool -L "$f" | awk 'NR > 1 { print $1 }')
  done < <(find "$APP" -type f -print0)
done

echo "== embedded $(find "$FW" -name '*.dylib' | wc -l | tr -d ' ') dylibs =="
ls -1 "$FW" || true
