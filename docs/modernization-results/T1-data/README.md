# T1 local validation artifacts

`clang-tidy22-final.txt` records the worker's final host gate. The coordinator
ran the final 18.1.3 gate and recorded its stdout/stderr in
`clang-tidy18-final.txt`; the JSON timing records include exit status and wall
time. These are local runs, not GitHub CI results or comparable performance
benchmarks. Both use four analysis jobs and the current Clang Debug database.

## Isolated LLVM 18 environment

The host has LLVM 22 and GCC 16. LLVM 18 cannot parse its GCC 16 C++ headers
(`__builtin_popcountg` and newer iterator definitions). The initial attempt
failed in fixture compilation and was not accepted. For the final run only,
Ubuntu's GCC 13 headers replace the C++ include search; Qt/KF, the compile
database, generated files, and C/system headers still come from the host.
This is version coverage, not an equivalent Ubuntu/Neon CI environment.

Packages were downloaded from `https://mirrors.ustc.edu.cn/ubuntu/pool/`:

| Extracted package | Path below the mirror's pool |
| --- | --- |
| clang-tidy | `universe/l/llvm-toolchain-18/clang-tidy-18_18.1.3-1ubuntu1_amd64.deb` |
| Clang library | `main/l/llvm-toolchain-18/libclang-cpp18_18.1.3-1ubuntu1_amd64.deb` |
| LLVM library | `main/l/llvm-toolchain-18/libllvm18_18.1.3-1ubuntu1_amd64.deb` |
| Resource headers | `universe/l/llvm-toolchain-18/libclang-common-18-dev_18.1.3-1ubuntu1_amd64.deb` |
| C++ headers | `main/g/gcc-13/libstdc%2B%2B-13-dev_13.3.0-6ubuntu2~24.04.1_amd64.deb` |
| libedit | `main/libe/libedit/libedit2_3.1-20230828-1build1_amd64.deb` |
| libtinfo | `main/n/ncurses/libtinfo6_6.4%2B20240113-1ubuntu2_amd64.deb` |

`llvm18-packages.json` records local filenames, sizes and SHA-256 hashes of
the downloaded archives. Binaries are not committed or installed. Each
archive's `data.tar.zst` was extracted with `ar p` and `tar --zstd -xf` into
`/tmp/latte-llvm18/root`. A PATH wrapper named `clang-tidy` contains:

```sh
#!/bin/sh
export LD_LIBRARY_PATH=/tmp/latte-llvm18/root/usr/lib/x86_64-linux-gnu
exec /tmp/latte-llvm18/root/usr/bin/clang-tidy-18 \
    --extra-arg=-nostdinc++ \
    --extra-arg=-isystem/tmp/latte-llvm18/root/usr/include/c++/13 \
    --extra-arg=-isystem/tmp/latte-llvm18/root/usr/include/x86_64-linux-gnu/c++/13 \
    "$@"
```

The final invocation prepends `/tmp/latte-llvm18/bin` to PATH and runs:

```sh
python3 scripts/test-clang-tidy.py \
    --build-dir build/modernization/clang-debug --jobs 4
```

The first isolated run emitted loader version warnings from the host's
libtinfo. Extracting the matching Ubuntu libtinfo removed those warnings before
the final run. Neither system packages nor the application's dependencies,
build settings or user configuration were changed.
