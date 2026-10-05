#!/usr/bin/env bash
# ConfigLang Automated Test Suite
# Course: PCD (Compiler Design), 4CS501CC25
# Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)

set -e

BIN="./build/configlang"
PASS=0
FAIL=0

if [ ! -f "$BIN" ]; then
    echo "Binary $BIN not found! Run make first."
    exit 1
fi

assert_exit() {
    local cmd="$1"
    local expected="$2"
    local desc="$3"
    set +e
    eval "$cmd" > /dev/null 2>&1
    local actual=$?
    set -e
    if [ "$actual" -eq "$expected" ]; then
        echo "  [PASS] $desc (exit code $actual)"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $desc (expected $expected, got $actual)"
        FAIL=$((FAIL + 1))
    fi
}

echo "=== M0: Driver and CLI Tests ==="
assert_exit "$BIN --version" 0 "Check --version exit code"
assert_exit "$BIN --help" 0 "Check --help exit code"
assert_exit "$BIN --invalid-flag" 2 "Check unrecognized option rejection"
assert_exit "$BIN tests/non_existent.cfg" 2 "Check non-existent file E901 exit code"

echo "=== M1: Lexical Analysis Valid Tests ==="
assert_exit "$BIN --tokens examples/server.cfg" 0 "Tokenize examples/server.cfg"

echo "=== M1: Lexical Analysis Negative Tests (E101-E105) ==="
for exp in tests/invalid/lex_*.expected; do
    [ -f "$exp" ] || continue
    cfg="${exp%.expected}.cfg"
    tname=$(basename "$cfg" .cfg)
    
    # Run compiler and extract "LINE:COL CODE" from error lines
    actual=$(set +e; "$BIN" "$cfg" 2>&1 | sed -nE 's/^[^:]+:([0-9]+:[0-9]+): (error|warning)\[([EW][0-9]{3})\].*$/\1 \3/p'; set -e)
    expected=$(cat "$exp" | tr -d '\r')
    
    if [ "$actual" = "$expected" ]; then
        echo "  [PASS] $tname -> matched diagnostic: $actual"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $tname:"
        echo "         Expected: '$expected'"
        echo "         Actual:   '$actual'"
        FAIL=$((FAIL + 1))
    fi
done

echo "----------------------------------------"
echo "Summary: Total passed: $PASS, failed: $FAIL"
if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
