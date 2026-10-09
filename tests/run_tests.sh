#!/usr/bin/env bash
# ==============================================================================
# ConfigLang Compiler - Student Test Suite
# Course: Principles of Compiler Design (4CS501CC25) - Nirma University
# Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
# ==============================================================================

set -e

BIN="./build/configlang"
PASS=0
FAIL=0

if [ ! -f "$BIN" ]; then
    echo "Error: Binary $BIN not found. Please run 'make' first."
    exit 1
fi

echo "=========================================================="
echo "         ConfigLang Compiler Test Suite"
echo "=========================================================="

assert_exit() {
    local cmd="$1"
    local expected="$2"
    local desc="$3"
    set +e
    eval "$cmd" > /dev/null 2>&1
    local actual=$?
    set -e
    if [ "$actual" -eq "$expected" ]; then
        echo "  [PASS] $desc"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $desc (expected $expected, got $actual)"
        FAIL=$((FAIL + 1))
    fi
}

echo ""
echo "=== Step 1: CLI and Driver Tests ==="
assert_exit "$BIN --version" 0 "Check --version flag"
assert_exit "$BIN --help" 0 "Check --help flag"
assert_exit "$BIN non_existent_file.cfg" 1 "Reject non-existent file"

echo ""
echo "=== Step 2: Golden Verification (examples/server.cfg) ==="
assert_exit "$BIN -o build/server_test.json examples/server.cfg && diff -u examples/server.json build/server_test.json" 0 "Compile examples/server.cfg and match golden JSON"
rm -f build/server_test.json

echo ""
echo "=== Step 3: Compiler Pipeline Phase Tracing (--phases) ==="
assert_exit "$BIN --phases examples/server.cfg" 0 "Trace all 8 compiler pipeline phases"

echo ""
echo "=== Step 4: Valid Configuration Test Suite ==="
for cfg in tests/valid/*.cfg; do
    [ -f "$cfg" ] || continue
    tname=$(basename "$cfg")
    assert_exit "$BIN $cfg" 0 "Valid config: $tname"
done

echo ""
echo "=== Step 5: Negative Tests (Error Handling) ==="
assert_exit "$BIN tests/invalid/syn_missing_semi.cfg" 1 "Catch syntax error (missing semicolon)"
assert_exit "$BIN tests/invalid/sem_undefined_ref.cfg" 1 "Catch semantic error (undefined reference)"
assert_exit "$BIN tests/invalid/sem_type_mismatch.cfg" 1 "Catch semantic error (type mismatch)"
assert_exit "$BIN tests/invalid/sem_duplicate.cfg" 1 "Catch semantic error (duplicate variable)"
assert_exit "$BIN tests/invalid/sem_cycle_self.cfg" 1 "Catch semantic error (circular reference)"
assert_exit "$BIN tests/invalid/opt_div_zero.cfg" 1 "Catch division by zero (integer 1 / 0)"
assert_exit "$BIN tests/invalid/opt_div_zero_float.cfg" 1 "Catch division by zero (float 1.0 / 0.0)"
assert_exit "$BIN tests/invalid/opt_div_zero_complex.cfg" 1 "Catch complex division by zero (@val / (5 - 5))"
assert_exit "$BIN tests/invalid/opt_div_zero_nested.cfg" 1 "Catch nested block division by zero (8 / (@workers - 4))"
assert_exit "$BIN tests/invalid/opt_div_zero_expr.cfg" 1 "Catch arithmetic expression division by zero (8 / (10 - 2 * 5))"

echo ""
echo "=========================================================="
echo "  Test Summary: $PASS passed, $FAIL failed"
echo "=========================================================="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi

echo "All tests passed successfully!"
