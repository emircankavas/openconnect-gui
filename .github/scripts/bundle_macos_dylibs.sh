#!/usr/bin/env bash
#
# Make a macOS .app self-contained by embedding its non-system dynamic
# dependencies (standalone *.dylib AND *.framework binaries from Homebrew)
# and rewriting every install name / rpath so nothing resolves outside the
# bundle.
#
# Complements macdeployqt, which leaves some things behind on Homebrew Qt:
# QtQml*/QtDBus frameworks, and transitive C libs (e.g. brotli via gnutls).
#
# Usage: bundle_macos_dylibs.sh /path/to/App.app
#
set -euo pipefail

APP="${1:?usage: bundle_macos_dylibs.sh /path/to/App.app}"
if [ ! -d "$APP" ]; then
  echo "error: not a bundle: $APP" >&2
  exit 1
fi
# Absolute root: readlink -f returns absolute paths, so the internal-vs-external
# test below is only reliable against an absolute $APP.
APP="$(cd "$(dirname "$APP")" && pwd -P)/$(basename "$APP")"

FW="$APP/Contents/Frameworks"
FW_REL="@executable_path/../Frameworks"
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
# Falls back to well-known Homebrew prefixes when no LC_RPATH matches.
resolve_ref() {
  local file="$1" ref="$2" name rp
  name="${ref#@rpath/}"; name="${name#@loader_path/}"
  while IFS= read -r rp; do
    [ -n "$rp" ] || continue
    case "$rp" in
      @executable_path*) rp="${rp/@executable_path/$(dirname "$file")}" ;;
      @loader_path*)     rp="${rp/@loader_path/$(dirname "$file")}" ;;
    esac
    if [ -f "$rp/$name" ]; then printf '%s\n' "$rp/$name"; return 0; fi
  done < <(otool -l "$file" | awk '/LC_RPATH/{getline; getline; print $2}')

  for base in /opt/homebrew/lib /usr/local/lib; do
    if [ -f "$base/$name" ]; then printf '%s\n' "$base/$name"; return 0; fi
  done
  return 1
}

deps_of() { otool -L "$1" 2>/dev/null | awk 'NR > 1 { print $1 }'; }

# Copy destination + new install id for an absolute dependency path.
dest_for() {
  local src="$1" name
  case "$src" in
    *.framework/*)
      name="$(printf '%s\n' "$src" | sed -n 's|.*/\([^/]*\.framework\)/.*|\1|p')"
      printf '%s\t%s/%s\n' "$FW/$name" "$FW_REL" "$name"
      ;;
    *)
      name="$(basename "$src")"
      printf '%s\t%s\n' "$FW/$name" "$FW_REL/$name"
      ;;
  esac
}

# --- Phase 0: dereference symlinks that escape the bundle -------------------
# macdeployqt copies Homebrew Qt plugins/frameworks as symlinks; in Homebrew
# those point into ../Cellar/..., which does not exist on a user's machine
# (e.g. PlugIns/platforms/libqcocoa.dylib). Replace any link whose real target
# lives outside the bundle with the actual file/dir; keep internal links
# (framework Versions/Current -> A, which codesign requires). Runs BEFORE the
# embedding passes so the materialised Mach-O files are processed too.
echo "== dereferencing escaping symlinks =="
while IFS= read -r -d '' l; do
  link_target="$(readlink "$l")"
  cur="$(cd "$(dirname "$l")" && pwd -P)"
  # resolve relative to the link's own directory, without requiring existence
  real="$(cd "$cur" 2>/dev/null && readlink -f "$link_target" 2>/dev/null || true)"

  if [ -z "$real" ] || [ ! -e "$real" ]; then
    # dangling link whose target escapes the bundle -> re-resolve via Homebrew.
    # Cellar paths are versioned, but /opt/homebrew/opt/<formula> is the stable
    # symlink to the same tree, so rewrite Cellar/<f>/<ver>/<rest> -> opt/<f>/<rest>.
    case "$link_target" in
      *Cellar/*)
        formula="$(printf '%s\n' "$link_target" | sed -n 's|.*Cellar/\([^/]*\)/[^/]*/.*|\1|p')"
        rest="$(printf '%s\n' "$link_target" | sed -n 's|.*Cellar/[^/]*/[^/]*/\(.*\)|\1|p')"
        if [ -n "$formula" ] && [ -n "$rest" ]; then
          for root in /opt/homebrew/opt /usr/local/opt; do
            if [ -e "$root/$formula/$rest" ]; then
              real="$root/$formula/$rest"
              break
            fi
          done
        fi
        ;;
    esac
  fi

  if [ -z "$real" ] || [ ! -e "$real" ]; then
    # last resort: search the Homebrew trees for the basename
    base="$(basename "$link_target")"
    real="$(find /opt/homebrew /usr/local -name "$base" 2>/dev/null \
              -not -path '*/Cellar/*' | head -n1 || true)"
  fi

  if [ -z "$real" ] || [ ! -e "$real" ]; then
    echo "   WARN: unresolvable symlink ${l#$APP/} -> ${link_target}" >&2
    continue
  fi

  case "$real" in
    "$APP"/*) continue ;;   # internal: leave as-is
  esac
  echo "   * ${l#$APP/}  (was -> ${link_target})"
  rm -f "$l"
  if [ -d "$real" ]; then
    cp -RfL "$real" "$l"
  else
    cp -fL "$real" "$l"
  fi
done < <(find "$APP" -type l -print0)

echo "== embedding external dependencies into $FW =="

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
      is_system "$dep" && continue
      case "$dep" in "$FW_REL"/*) continue ;; esac

      abs="$dep"
      case "$dep" in
        @rpath/*|@loader_path/*)
          if ! abs="$(resolve_ref "$f" "$dep")"; then
            echo "   WARN: could not resolve $dep (referenced by ${f#$APP/})" >&2
            continue
          fi
          ;;
      esac
      is_system "$abs" && continue

      IFS=$'\t' read -r target newid <<<"$(dest_for "$abs")"

      if [ ! -e "$target" ]; then
        echo "   + ${target#$FW/}  <- $abs"
        mkdir -p "$(dirname "$target")"
        # Replace any pre-existing (possibly dangling) symlink with real content.
        rm -rf "$target" 2>/dev/null || true
        case "$abs" in
          *.framework/*)
            fw_src="$(printf '%s\n' "$abs" | sed -n 's|\(.*\.framework\)/.*|\1|p')"
            # -L: /opt/homebrew/lib/<X>.framework is itself a symlink into
            # Cellar; copying it without dereferencing ships a broken link.
            cp -RfL "$fw_src" "$FW/"
            chmod -R u+w "$target" 2>/dev/null || true
            ;;
          *)
            cp -fL "$abs" "$target"
            chmod u+w "$target"
            ;;
        esac
        install_name_tool -id "$newid" "$target" 2>/dev/null || true
        progress=1
      fi

      install_name_tool -change "$dep" "$newid" "$f" 2>/dev/null || true
    done < <(deps_of "$f")
  done < <(find "$APP" -type f -print0)
done

# --- normalise ids of everything we shipped ---------------------------------
echo "== normalising bundled identifiers =="
while IFS= read -r -d '' f; do
  is_macho "$f" || continue
  case "$f" in
    "$FW"/*)
      install_name_tool -id "$FW_REL/${f#$FW/}" "$f" 2>/dev/null || true
      ;;
  esac
done < <(find "$APP" -type f -print0)

# --- re-point any absolute Homebrew reference at its bundled copy -----------
echo "== rewriting remaining absolute references =="
while IFS= read -r -d '' f; do
  is_macho "$f" || continue
  while IFS= read -r dep; do
    case "$dep" in
      /opt/homebrew/*|/usr/local/opt/*|/usr/local/Cellar/*)
        base="$(basename "$dep")"
        if [ -e "$FW/$base" ]; then
          install_name_tool -change "$dep" "$FW_REL/$base" "$f" 2>/dev/null || true
        elif printf '%s' "$dep" | grep -q '\.framework/'; then
          fw="$(printf '%s\n' "$dep" | sed -n 's|.*/\([^/]*\.framework\)/.*|\1|p')"
          inner="$(printf '%s\n' "$dep" | sed 's|.*\.framework/||')"
          if [ -e "$FW/$fw" ]; then
            install_name_tool -change "$dep" "$FW_REL/$fw/$inner" "$f" 2>/dev/null || true
          fi
        fi
        ;;
    esac
  done < <(deps_of "$f")
done < <(find "$APP" -type f -print0)

# --- strip absolute rpaths from every Mach-O in the bundle ------------------
echo "== stripping absolute rpaths =="
while IFS= read -r -d '' f; do
  is_macho "$f" || continue
  while IFS= read -r rp; do
    case "$rp" in
      @*) ;;
      *) install_name_tool -delete_rpath "$rp" "$f" 2>/dev/null || true ;;
    esac
  done < <(otool -l "$f" | awk '/LC_RPATH/{getline; getline; print $2}')
done < <(find "$APP" -type f -print0)

# --- make sure every Mach-O can reach the bundled Frameworks dir ------------
while IFS= read -r -d '' f; do
  is_macho "$f" || continue
  if ! otool -l "$f" | awk '/LC_RPATH/{getline; getline; print $2}' | grep -qx "$FW_REL"; then
    install_name_tool -add_rpath "$FW_REL" "$f" 2>/dev/null || true
  fi
done < <(find "$APP" -type f -print0)

echo "== embedded: $(find "$FW" -maxdepth 1 -name '*.dylib' | wc -l | tr -d ' ') dylibs, $(find "$FW" -maxdepth 1 -name '*.framework' | wc -l | tr -d ' ') frameworks =="
