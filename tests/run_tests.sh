#!/usr/bin/env bash
# ConfigLang Test Runner
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

echo "Running M0 Scaffold Tests:"
assert_exit "$BIN --version" 0 "Check --version exit code"
assert_exit "$BIN --help" 0 "Check --help exit code"
assert_exit "$BIN --invalid-option-flag" 2 "Check unrecognized option rejection"
assert_exit "$BIN tests/non_existent_file.cfg" 2 "Check non-existent file E901 exit code"

echo "----------------------------------------"
echo "Total passed: $PASS, failed: $FAIL"
if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
