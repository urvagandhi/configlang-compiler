#!/usr/bin/env bash
# ConfigLang Automated Test Suite
# Course: PCD (Principles of Compiler Design), 4CS501CC25
# Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)

set -e

BIN="./build/configlang"
PASS=0
FAIL=0

if [ ! -f "$BIN" ]; then
    echo "Binary $BIN not found! Run make first."
    exit 1
fi

echo "=========================================================="
echo "         ConfigLang Compiler Comprehensive Test Suite     "
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
        echo "  [PASS] $desc (exit code $actual)"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $desc (expected $expected, got $actual)"
        FAIL=$((FAIL + 1))
    fi
}

echo ""
echo "=== Phase 0: Driver, CLI & File Handling Tests ==="
assert_exit "$BIN --version" 0 "Check --version exit code"
assert_exit "$BIN --help" 0 "Check --help exit code"
assert_exit "$BIN --invalid-flag" 2 "Check unrecognized option rejection"
assert_exit "$BIN tests/non_existent.cfg" 2 "Check non-existent file E901 exit code"

echo ""
echo "=== Phase 1: Valid Configuration Compilation Tests ==="
for cfg in tests/valid/*.cfg examples/server.cfg; do
    [ -f "$cfg" ] || continue
    tname=$(basename "$cfg")
    assert_exit "$BIN $cfg" 0 "Compile $tname to JSON"
    assert_exit "$BIN -f yaml $cfg" 0 "Compile $tname to YAML"
done

echo ""
echo "=== Phase 2: Golden Output Verification ==="
assert_exit "$BIN -o build/server_out.json examples/server.cfg && diff -u examples/server.json build/server_out.json" 0 "Golden JSON match for examples/server.cfg"

echo ""
echo "=== Phase 3: Intermediate Representation Phase Dumps ==="
assert_exit "$BIN --tokens examples/server.cfg" 0 "Dump token stream (--tokens)"
assert_exit "$BIN --ast examples/server.cfg" 0 "Dump abstract syntax tree (--ast)"
assert_exit "$BIN --symtab examples/server.cfg" 0 "Dump symbol table (--symtab)"
assert_exit "$BIN --deps examples/server.cfg" 0 "Dump dependency graph (--deps)"
assert_exit "$BIN --tac examples/server.cfg" 0 "Dump 3-address code (--tac)"
assert_exit "$BIN --tac-opt examples/server.cfg" 0 "Dump optimized TAC (--tac-opt)"
assert_exit "$BIN --phases examples/server.cfg" 0 "Dump all phases sequentially (--phases)"

echo ""
echo "=== Phase 4: Negative Diagnostic Tests (Lexical, Syntax, Semantic, Optimizer, Validator) ==="
for exp in tests/invalid/*.expected; do
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

echo ""
echo "=== Phase 5: Robustness & Stress Tests ==="
# Deep nesting
assert_exit "$BIN tests/valid/deep_nesting.cfg" 0 "100-level block nesting test"

# Fuzz testing with random garbage bytes
FUZZ_PASS=1
for i in $(seq 1 30); do
    head -c 128 /dev/urandom > build/fuzz_tmp.cfg
    set +e
    "$BIN" build/fuzz_tmp.cfg > /dev/null 2>&1
    fexit=$?
    set -e
    # Must exit with standard compiler errors (1 or 2), never crash (signal > 128)
    if [ "$fexit" -gt 128 ]; then
        echo "  [FAIL] Compiler crashed on random input $i with exit code $fexit"
        FUZZ_PASS=0
        FAIL=$((FAIL + 1))
        break
    fi
done
rm -f build/fuzz_tmp.cfg
if [ "$FUZZ_PASS" -eq 1 ]; then
    echo "  [PASS] 30 random byte fuzz tests survived without crash"
    PASS=$((PASS + 1))
fi

echo ""
echo "=========================================================="
echo "  Test Summary: $PASS passed, $FAIL failed"
echo "=========================================================="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
