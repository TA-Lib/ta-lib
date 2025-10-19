# TA-Lib PostgreSQL Extension

This document describes how to build and install the `ta_pg` PostgreSQL extension.

## Building the Extension

### 1. Compile TA-Lib

First, you need to compile the `ta-lib` C library. The following commands will do this:

```bash
sh autogen.sh
./configure
make
```

### 2. Generate the PostgreSQL Wrappers

The C and SQL code for the PostgreSQL extension is generated from the `ta_func_api.xml` file using a Python script. To generate the wrappers, run the following command:

```bash
python3 scripts/generate_pg_wrappers.py
```

This will generate the `ta_pg.c` and `ta_pg--1.0.sql` files in the `src/ta_pg` directory.

### 3. Compile the PostgreSQL Extension

To compile the PostgreSQL extension, run the following command in the `src/ta_pg` directory:

```bash
make ta_pg.so
```

This will create the `ta_pg.so` shared library in the `src/ta_pg` directory.

## Installing the Extension

To install the extension, you need to copy the following files to your PostgreSQL installation directory (where ${PG_VERSION} is the postgresql version):

*   `src/ta_pg/ta_pg.so` to the PostgreSQL library directory (e.g., `/usr/lib/postgresql/${PG_VERSION}/lib/`)
*   `src/ta_pg/ta_pg.control` to the PostgreSQL extension directory (e.g., `/usr/share/postgresql/${PG_VERSION}/extension/`)
*   `src/ta_pg/ta_pg--1.0.sql` to the PostgreSQL extension directory (e.g., `/usr/share/postgresql/${PG_VERSION}/extension/`)

Once the files are copied, you can install the extension in your database by running the following SQL command:

```sql
CREATE EXTENSION ta_pg;
```
