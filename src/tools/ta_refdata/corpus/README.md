# Reference corpus

Daily OHLCV series the function pages on ta-lib.org are computed over
(`scripts/build.py refdata`); not part of the released source tarball. Columns
`date,open,high,low,close,volume`, dates `YYYY-MM-DD` ascending. An optional
first line `# label: <text>` names the series the way a chart caption does
(`# label: IBM, daily, 1999`); without one the file name is the label. A
`# source: <text>` line says where the bars come from, and a
`# synthetic: <how it was made>` line marks a generated series instead.

`ibm_1999.csv`: IBM, 252 trading days of 1999, and every page's series unless
the page's pattern function never fires on it. The prices are split-adjusted
and the volume is unadjusted, exactly as in ta_regtest's
`TA_SREF_*_daily_ref_0_PRIV` arrays (`src/tools/ta_regtest/test_data.c`), which
is why a freshly downloaded series matches neither; the dates come from the
`daily_ref_0.csv` that produced those arrays. `ta_refdata` refuses to run when
the file and the arrays differ.

Every other file is a 40-bar excerpt, the pattern's lookback among them,
charted whole, for a pattern IBM 1999 never shows. Such a pattern takes the
excerpt where it fires most, a market one before a synthetic one.

- `simulated_*`: generated rather than taken from a market, for the patterns
  that need an opening gap no on-chain market prints and that no open source
  of stock prices shows.
- `wbtc_usdc_*`: daily UTC bars of Uniswap v3's WBTC/USDC pools on Ethereum,
  built from the pools' Swap event logs merged in block order, each block's
  last price; the volume is in WBTC. The `# source:` line names the pools, so
  anyone can rebuild them from the chain.
