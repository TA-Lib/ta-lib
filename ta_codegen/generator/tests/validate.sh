#!/bin/bash
set -e
cd "$(dirname "$0")/.."

PASS=0
FAIL=0

pass() { echo "  PASS: $1"; PASS=$((PASS + 1)); }
fail() { echo "  FAIL: $1"; FAIL=$((FAIL + 1)); }

echo "=== Building ta_codegen ==="
cargo build --release

echo ""
echo "=== Generating all backends ==="
cargo run --release -- generate

echo ""
echo "=== Generating servers ==="
cargo run --release -- generate-servers --backend=c

echo ""
echo "=== Building C server ==="
mkdir -p ../../bin
cargo run --release -- build --backend=c

C_SERVER="../../bin/ta_codegen_serve_c"
if [ ! -x "$C_SERVER" ]; then
    echo "FATAL: C server binary not found at $C_SERVER"
    exit 1
fi

echo ""
echo "=== Checking generated files exist and are non-empty ==="
# Auto-discover indicators from ta_codegen/input/ directories
for dir in ../../ta_codegen/input/*/; do
    name=$(basename "$dir")
    # Skip if not a valid indicator (must have both .yaml and .c)
    [ -f "$dir/${name}.yaml" ] && [ -f "$dir/${name}.c" ] || continue

    UPPER=$(echo "$name" | tr '[:lower:]' '[:upper:]')
    for f in \
        "../../src/ta_func/ta_${UPPER}.c" \
        "../../ta_codegen/output/rust/library/src/ta_func/${name}.rs" \
        "../../ta_codegen/output/java/library/fragments/Core_${UPPER}.java"; do
        if [ -s "$f" ]; then
            pass "Generated file exists: $(basename "$f")"
        else
            fail "Missing or empty: $f"
        fi
    done
done

echo ""
echo "=== Checking generated C files contain all function variants ==="
# Auto-discover and check C variants for all indicators
for dir in ../../ta_codegen/input/*/; do
    name=$(basename "$dir")
    [ -f "$dir/${name}.yaml" ] && [ -f "$dir/${name}.c" ] || continue

    UPPER=$(echo "$name" | tr '[:lower:]' '[:upper:]')
    c_file="../../src/ta_func/ta_${UPPER}.c"
    [ -s "$c_file" ] || continue

    for variant in "TA_${UPPER}_Unguarded" "TA_${UPPER}_Lookback" "TA_${UPPER}"; do
        if grep -q "$variant" "$c_file"; then
            pass "ta_${UPPER}.c contains $variant"
        else
            fail "ta_${UPPER}.c missing $variant"
        fi
    done
done

echo ""
echo "=== Testing JSON-RPC via C server ==="

# Test TA_MULT
REQUEST='{"method":"TA_MULT","params":{"startIdx":0,"endIdx":4,"inReal0":[1,2,3,4,5],"inReal1":[2,3,4,5,6]}}'
RESPONSE=$(echo "$REQUEST" | "$C_SERVER")
EXPECTED='2,6,12,20,30'
if echo "$RESPONSE" | grep -q "$EXPECTED"; then
    pass "JSON-RPC TA_MULT: correct output"
else
    fail "JSON-RPC TA_MULT: unexpected output"
    echo "    Request:  $REQUEST"
    echo "    Response: $RESPONSE"
fi

# Test MULT lookback
REQUEST_LB='{"method":"TA_MULT_Lookback","params":{}}'
RESPONSE_LB=$(echo "$REQUEST_LB" | "$C_SERVER")
if echo "$RESPONSE_LB" | grep -q '"lookback":0'; then
    pass "JSON-RPC TA_MULT_Lookback: correct output"
else
    fail "JSON-RPC TA_MULT_Lookback: unexpected output"
    echo "    Response: $RESPONSE_LB"
fi

# Test TA_SMA
REQUEST_SMA='{"method":"TA_SMA","params":{"startIdx":0,"endIdx":4,"inReal":[1,2,3,4,5],"optInTimePeriod":3}}'
RESPONSE_SMA=$(echo "$REQUEST_SMA" | "$C_SERVER")
EXPECTED_SMA='2,3,4'
if echo "$RESPONSE_SMA" | grep -q "$EXPECTED_SMA"; then
    pass "JSON-RPC TA_SMA: correct output"
else
    fail "JSON-RPC TA_SMA: unexpected output"
    echo "    Request:  $REQUEST_SMA"
    echo "    Response: $RESPONSE_SMA"
fi

# Test SMA lookback
REQUEST_SMA_LB='{"method":"TA_SMA_Lookback","params":{"optInTimePeriod":30}}'
RESPONSE_SMA_LB=$(echo "$REQUEST_SMA_LB" | "$C_SERVER")
if echo "$RESPONSE_SMA_LB" | grep -q '"lookback":29'; then
    pass "JSON-RPC TA_SMA_Lookback: correct output"
else
    fail "JSON-RPC TA_SMA_Lookback: unexpected output"
    echo "    Response: $RESPONSE_SMA_LB"
fi

# Test TA_EMA
REQUEST_EMA='{"method":"TA_EMA","params":{"startIdx":0,"endIdx":9,"inReal":[1,2,3,4,5,6,7,8,9,10],"optInTimePeriod":3}}'
RESPONSE_EMA=$(echo "$REQUEST_EMA" | "$C_SERVER")
if echo "$RESPONSE_EMA" | grep -q '"retCode":0'; then
    pass "JSON-RPC TA_EMA: returns success"
else
    fail "JSON-RPC TA_EMA: unexpected output"
    echo "    Response: $RESPONSE_EMA"
fi

# Test EMA lookback
REQUEST_EMA_LB='{"method":"TA_EMA_Lookback","params":{"optInTimePeriod":10}}'
RESPONSE_EMA_LB=$(echo "$REQUEST_EMA_LB" | "$C_SERVER")
if echo "$RESPONSE_EMA_LB" | grep -q '"lookback":9'; then
    pass "JSON-RPC TA_EMA_Lookback: correct output"
else
    fail "JSON-RPC TA_EMA_Lookback: unexpected output"
    echo "    Response: $RESPONSE_EMA_LB"
fi

# Test TA_WMA
REQUEST_WMA='{"method":"TA_WMA","params":{"startIdx":0,"endIdx":4,"inReal":[1,2,3,4,5],"optInTimePeriod":3}}'
RESPONSE_WMA=$(echo "$REQUEST_WMA" | "$C_SERVER")
if echo "$RESPONSE_WMA" | grep -q '"retCode":0'; then
    pass "JSON-RPC TA_WMA: returns success"
else
    fail "JSON-RPC TA_WMA: unexpected output"
    echo "    Response: $RESPONSE_WMA"
fi

# Test WMA lookback
REQUEST_WMA_LB='{"method":"TA_WMA_Lookback","params":{"optInTimePeriod":5}}'
RESPONSE_WMA_LB=$(echo "$REQUEST_WMA_LB" | "$C_SERVER")
if echo "$RESPONSE_WMA_LB" | grep -q '"lookback":4'; then
    pass "JSON-RPC TA_WMA_Lookback: correct output"
else
    fail "JSON-RPC TA_WMA_Lookback: unexpected output"
    echo "    Response: $RESPONSE_WMA_LB"
fi

# Test RSI
REQUEST_RSI='{"method":"TA_RSI","params":{"startIdx":0,"endIdx":29,"inReal":[44.0,44.34,44.09,43.61,44.33,44.83,44.32,44.55,43.93,44.05,43.80,43.64,43.82,44.29,44.09,44.15,43.61,44.33,44.83,44.32,44.55,43.93,44.05,43.80,43.64,43.82,44.29,44.09,44.15,43.61],"optInTimePeriod":14}}'
RESPONSE_RSI=$(echo "$REQUEST_RSI" | "$C_SERVER")
if echo "$RESPONSE_RSI" | grep -q '"outBegIdx":14'; then
    pass "JSON-RPC TA_RSI: correct output"
else
    fail "JSON-RPC TA_RSI: unexpected output"
    echo "    Response: $RESPONSE_RSI"
fi

# Test RSI lookback
REQUEST_RSI_LB='{"method":"TA_RSI_Lookback","params":{"optInTimePeriod":14}}'
RESPONSE_RSI_LB=$(echo "$REQUEST_RSI_LB" | "$C_SERVER")
if echo "$RESPONSE_RSI_LB" | grep -q '"lookback":14'; then
    pass "JSON-RPC TA_RSI_Lookback: correct output"
else
    fail "JSON-RPC TA_RSI_Lookback: unexpected output"
    echo "    Response: $RESPONSE_RSI_LB"
fi

echo ""
echo "=== Running cargo test ==="
cargo test 2>&1

echo ""
echo "=== Validation Summary ==="
echo "  Passed: $PASS"
echo "  Failed: $FAIL"
if [ "$FAIL" -gt 0 ]; then
    echo "  Some validations FAILED."
    exit 1
else
    echo "  All validations passed."
fi
