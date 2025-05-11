# NUMATyping_umf

This repository contains experiments to improve or investigate the NUMA allocator from the [Unified Memory Framework (UMF)](https://github.com/oneapi-src/unified-memory-framework), comparing it to the default memory allocator `malloc()`. It uses `umf_test.cpp` from the [NumaTyping](https://github.com/CU-NVM/NUMATyping) project, which allocates memory with multiple threads using either `malloc()` or a NUMA-aware allocator (primarily `mallocx()` or `mallocv()` from a modified version of [jemalloc](https://github.com/jemalloc/jemalloc/releases/tag/5.3.0).)

---

## 🔀 Branch Overview

### ✅ Important Branches:

1. **`Base_UMF`**  
   Mirrors `main` with the UMF used in NumaTyping. Updates to UMF include adding tcaches and arenas. Test file outdated.

2. **`old_old_umf`**  
   Original UMF fork without tcache logic.

3. **`Inline_UMF`**  
   Same as Base_UMF, but with an updated test file. 

4. **`Dynamic_UMF`**  
   Makes tcaches dynamically resizable. Uses `pthread` read/write locks.

5. **`locks_UMF`**  
   Like `Dynamic_UMF`, but replaces `pthread` locks with multi-reader locks from the [qd-locks library](https://github.com/kjellwinblad/qd_lock_lib). Requires compatibility changes to `qd`.
   Note: changes were lost due to push errors, but are mainly using the system `stdatomic.h`, and re-writing a couple functions to resolve errors. 

7. **`local_thread`**  
   Uses thread-local tcaches, eliminating the need for locks.

8. **`libjemallocv`**  
   Uses a modified version of [jemalloc](https://github.com/jemalloc/jemalloc/releases/tag/5.3.0) to introduce `mallocv(size, flags)` for faster tcache/arena-aware allocations. Slightly faster than `mallocx`, but still slower than `malloc()`. Also updates test to touch the allocated memory.

### Other Branches:
The branches `main`, `updated_UMF`, and `mallocv` are drafts or backups of the above.

---

## 📂 Key Files

| File | Description |
|------|-------------|
| `numa-test/umf_test.cpp` | Main test file. Takes allocator ID (0 = `malloc`, 1 = `mallocx`/`mallocv`) and thread count. |
| `numa-test/meta.py` | Automates test runs and CSV logging. |
| `numa-test/Makefile` | Build script for `umf_test.cpp`. Update when changing libraries to umf_test.cpp. |
| `umf/pools/pool_jemalloc.h` | Contains jemalloc pool structure and `umfFastJemallocMalloc` using `mallocv()`. |
| `src/pool/pool_jemalloc.c` | Initializes tcache size and arena count. |
| `unified-memory-framework/src/pool/CMakeLists.txt' and 'unified-memory-framework/CMakeLists.txt`| Update these to link new libraries to UMF. |
| `jemalloc/src/jemalloc.c` | Where `imallocv_fastpath()` is implemented. |

---

## 🛠 Compilation & Linking Notes

1. `umf_test.cpp` is linked to `libjemalloc.so`, so it **does not need to be rebuilt** when changes are made to `libjemalloc`.

2. To run using the local version of `libjemalloc` instead of the system one, **export the preload first**:
   ```bash
   export LD_PRELOAD=<path_to_NUMATyping_umf>/jemallocv/lib/libjemalloc.so
   ```

3. `jemalloc` header files are **generated automatically**. If any function signatures or prototypes in the library are changed or added, follow these steps to avoid linking errors:
   1. Clean existing generated headers:
      ```bash
      # Inside jemalloc directory
      make distclean
      ```
   2. Update `jemalloc/configure` and other files as needed.
   3. Reconfigure and rebuild jemalloc:
      ```bash
      autoconf;
      ./configure;
      make
      ```

4. To improve accuracy and ensure reproducibility of experimental results, consider cleaning the project and rerunning the experiments with the following updates:
   1. **Remove print statements** used for debugging.
   2. **Update all branches** to use the latest version of `umf_test.cpp`, which actively uses allocated memory (currently only updated in `libjemallocv`; note that prior results were obtained before this update).
   3. Ensure **no discrepancies** exist in experimental setups, including:
      - Test buffer sizes
      - Number of arenas
      - Initial tcache sizes
      - Number of nodes
   4. Ensure **all components are compiled with `-O3`** optimization.

5. ⚠️ Some experimental results (`old_old_umf`, `Dynamic_UMF`, and `locks_UMF`) are omitted from the final performance graph below due to lack of significance. This includes the original `old_umf`, and the thread global `Dynamic_UMF` and `locks_UMF`.
6. 

---

## 🚀 Usage Instructions

### 1. Clone the Project

```bash
git clone git@github.com:RazanAl/NUMATyping_umf.git
```

### 2. Build Required Components

```bash
# Build jemalloc
cd NUMATyping_umf/jemalloc;
make

# Build unified-memory-framework
cd ../unified-memory-framework;
mkdir build;
cd build;
cmake ..;
make

# Build numa-test (also re-builds unified-memory-framework, but building both individually is recommended)
cd ../../numa-test;
make;
```

### 3. Preload Custom jemalloc Library (If Applicable)

```bash
export LD_PRELOAD=<path_to_NUMATyping_umf>/jemalloc/lib/libjemalloc.so
```

### 4. Run the Benchmark

Inside `NUMATyping_umf/numa-test`, execute the meta command script:

```bash
python meta.py ./bin/umf_test   --meta a:0:1   --meta t:1...37   --repeat 5   --csv_file fastjemallocv_umf.csv
```

This command runs `./bin/umf_test`:
- With allocators 0 (malloc) and 1 (mallocv)
- For thread counts from 1 to 37
- Repeats each configuration 5 times
- Outputs results, including each configuration average, into `fastjemallocv_umf.csv`

