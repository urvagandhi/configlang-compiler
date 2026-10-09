#!/usr/bin/env bash
# ==============================================================================
# ConfigLang Final 6 Showcase Test Cases Runner
# Course: Principles of Compiler Design (4CS501CC25) - Nirma University
# Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
# ==============================================================================

set -e

BIN="./build/configlang"
DIR="tests/showcase"

if [ ! -f "$BIN" ]; then
    echo "Compiler binary not found. Running 'make'..."
    make
fi

echo "========================================================================"
echo "      ConfigLang Final Showcase: 6 Comprehensive Test Cases"
echo "      (Covering: Simple -> Feature-Rich -> Complex -> Edge -> Errors)"
echo "========================================================================"
echo ""

run_test() {
    local num="$1"
    local title="$2"
    local file="$3"
    local expected_exit="$4"
    local desc="$5"

    echo "------------------------------------------------------------------------"
    echo ">> Test Case $num: $title"
    echo "   File: $file"
    echo "   Description: $desc"
    echo "------------------------------------------------------------------------"
    
    set +e
    $BIN "$file" > /tmp/showcase_out.txt 2> /tmp/showcase_err.txt
    local status=$?
    set -e

    if [ "$status" -eq "$expected_exit" ]; then
        echo "   Result: [PASS] (Exit Code: $status, as expected)"
        if [ "$expected_exit" -eq 0 ]; then
            echo "   Generated JSON Output:"
            cat /tmp/showcase_out.txt | sed 's/^/     /'
        else
            echo "   Caught Expected Compiler Error:"
            cat /tmp/showcase_err.txt | sed 's/^/     /'
        fi
    else
        echo "   Result: [FAIL] (Expected exit $expected_exit, but got $status)"
        cat /tmp/showcase_err.txt
        exit 1
    fi
    echo ""
}

run_test "1" "Simple / Baseline Configuration" \
    "$DIR/tc1_simple_baseline.cfg" 0 \
    "Validates all 4 primitive types (int, float, string, bool), basic blocks, comments, and simple TAC addition."

run_test "2" "Medium / Feature-Rich (References & String Concat)" \
    "$DIR/tc2_feature_rich.cfg" 0 \
    "Demonstrates type inference, cross-block references (@), and compile-time string concatenation (+)."

run_test "3" "Complex / Enterprise Cluster Hierarchy" \
    "$DIR/tc3_complex_hierarchy.cfg" 0 \
    "Demonstrates 3-level deep nesting, multi-hop reference chains (A->B->C->D), mixed arithmetic, and multi-pass constant folding."

run_test "4" "Edge Cases & Boundary Matrix" \
    "$DIR/tc4_edge_cases.cfg" 0 \
    "Tests unary minus, type widening, zero numerator, empty block handling, string escape sequences, and multi-line comments."

run_test "5" "Semantic Analysis Diagnostics (Negative Test)" \
    "$DIR/tc5_semantic_errors.cfg" 1 \
    "Validates Phase 4 & 5 detecting narrowing type mismatch (int = 30 * 100.00), duplicate symbols, string-int mismatch, undefined refs, and self cycles."

run_test "6" "Optimizer Constant Propagation Error (Negative Test)" \
    "$DIR/tc6_opt_div_zero.cfg" 1 \
    "Demonstrates dynamic division by zero (5000 / 0) caught during Phase 6 TAC constant propagation on structurally valid code."

echo "========================================================================"
echo "  All 6 Final Showcase Test Cases Executed Successfully!"
echo "========================================================================"
