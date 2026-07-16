#!/usr/bin/env bash
#
# check-format.sh — Validate C/C++ files against clang-format.
#
# Designed to run identically in CI and locally. The CI workflow calls this
# script; developers can run it before pushing to catch formatting issues
# without waiting for GitHub Actions.
#
# Usage:
#   scripts/ci/check-format.sh              # check all files under src/
#   scripts/ci/check-format.sh --changed BASE_SHA  # check only files changed since BASE_SHA
#
# Environment:
#   CLANG_FORMAT  Path to clang-format binary (default: "clang-format")
#
# Exit codes:
#   0  All files are correctly formatted
#   1  One or more files need formatting
#   2  Usage error
#
set -uo pipefail

# ── Config ──────────────────────────────────────────────────────────────

readonly SRC_DIR="src"
readonly CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"
readonly GIT_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"

# Extensions to check
shopt -s extglob
readonly CPP_EXT='*.@(cpp|cxx|cc|c|hpp|h|hxx|hh)'

# ── Helpers ─────────────────────────────────────────────────────────────

print_usage() {
    cat <<EOF
Usage: $0 [--all | --changed BASE_SHA]

  --all             Check all C/C++ files under ${SRC_DIR}/ (default)
  --changed SHA     Check only files changed between SHA and HEAD

Environment:
  CLANG_FORMAT      Path to clang-format binary (default: clang-format)
EOF
}

# Collect the list of files to check, one per line on stdout.
collect_files() {
    local mode="${1:---all}"
    local base_sha="${2:-}"

    if [[ "$mode" == "--changed" ]]; then
        # Use three-dot diff to get the merge-base comparison (matches PR behavior)
        git diff --name-only --diff-filter=d "${base_sha}...HEAD" -- "$SRC_DIR" \
            | grep -iE '\.(cpp|cxx|cc|c|hpp|h|hxx|hh)$' \
            | while IFS= read -r f; do [[ -f "$f" ]] && echo "$f"; done
    else
        find "$SRC_DIR" -type f \( -name '*.cpp' -o -name '*.cxx' -o -name '*.cc' \
            -o -name '*.c' -o -name '*.hpp' -o -name '*.h' -o -name '*.hxx' \
            -o -name '*.hh' \) | sort
    fi
}

# ── Main ────────────────────────────────────────────────────────────────

main() {
    local mode="--all"
    local base_sha=""

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --all)    mode="--all"; shift ;;
            --changed) mode="--changed"; base_sha="${2:?--changed requires a SHA argument}"; shift 2 ;;
            -h|--help) print_usage; exit 0 ;;
            *) echo "Unknown option: $1" >&2; print_usage >&2; exit 2 ;;
        esac
    done

    cd "$GIT_ROOT" || exit 2

    # Verify clang-format is available
    if ! command -v "$CLANG_FORMAT" &>/dev/null; then
        echo "::error::clang-format not found. Set CLANG_FORMAT or install it." >&2
        exit 2
    fi

    local cf_version
    cf_version=$("$CLANG_FORMAT" --version 2>&1 | head -1)
    echo "clang-format: $cf_version"
    echo "Mode: $mode"
    echo ""

    # Collect files
    local files
    files=$(collect_files "$mode" "$base_sha")

    if [[ -z "$files" ]]; then
        echo "No C/C++ files to check."
        exit 0
    fi

    local total
    total=$(echo "$files" | wc -l | tr -d ' ')
    echo "Checking $total file(s)..."
    echo ""

    local failed=0
    local checked=0

    while IFS= read -r file; do
        [[ -z "$file" ]] && continue
        ((checked++)) || true

        # Run clang-format and capture the diff
        local diff_output
        diff_output=$("$CLANG_FORMAT" -style=file "$file" 2>/dev/null | diff -u "$file" - 2>&1) || true

        if [[ -n "$diff_output" ]]; then
            # Emit GitHub annotation (if running in CI)
            if [[ -n "${GITHUB_ACTIONS:-}" ]]; then
                echo "::error file=${file},title=clang-format::File is not formatted correctly"
            else
                echo "FAIL: $file"
            fi
            # Show the actual diff for developer context
            echo "$diff_output" >&2
            echo "" >&2
            ((failed++)) || true
        fi
    done <<< "$files"

    # Summary
    echo ""
    if [[ $failed -eq 0 ]]; then
        echo "All $checked file(s) are correctly formatted."
        exit 0
    else
        echo "$failed of $checked file(s) need formatting."
        echo ""
        echo "To fix:"
        echo "  # Single file:"
        echo "  clang-format -i <file>"
        echo ""
        echo "  # All src/ files:"
        echo "  find src -type f \\( -name '*.cpp' -o -name '*.h' \\) -exec clang-format -i {} +"
        echo ""
        echo "  # Using this script's --changed mode to preview:"
        echo "  scripts/ci/check-format.sh --changed HEAD~1"
        exit 1
    fi
}

main "$@"
