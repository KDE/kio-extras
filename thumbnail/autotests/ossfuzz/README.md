<!-- SPDX-FileCopyrightText: 2025 Azhar Momin <azhar.momin@kdemail.net> -->
<!-- SPDX-License-Identifier: CC-BY-SA-4.0 -->

# KDE-Thumbnailers OSS-Fuzz Integration

## Fuzzing Locally
Make sure you're using Clang (by setting `CC` and `CXX`), since fuzzing requires it. Then build KIO-Extras with `BUILD_FUZZERS=ON` to generate the fuzzer binaries:
```sh
cmake -B build -DBUILD_FUZZERS=ON
cmake --build build
# Then run one of the fuzzer binaries:
./build/bin/fuzzers/textthumbnail_fuzzer
```

## Testing OSS-Fuzz Integration
Testing OSS-Fuzz integration requires: Python & Docker

First clone the OSS-Fuzz repository:
```sh
git clone https://github.com/google/oss-fuzz.git
```

After navigating to the cloned repository, run the following command to build the fuzzers:
```sh
python3 infra/helper.py build_image kde-thumbnailers
python3 infra/helper.py build_fuzzers --sanitizer address kde-thumbnailers
```

This may take a while since it builds the qtbase, all thumbnailer dependencies, and KIO-Extras itself. Once the build is completed, you can run the fuzzers using the following command:
```sh
python3 infra/helper.py run_fuzzer kde-thumbnailers textthumbnail_fuzzer
```

The code for preparing the build lives in the `prepare_build.sh` script and the code for building the fuzzers lives in the `build_fuzzers.sh` script (which is also responsible for building the dependencies, creating the seed corpus and copying the dict file).

For more information on OSS-Fuzz, visit the [official website](https://google.github.io/oss-fuzz/).

## Integrating New Fuzzers

When you add a new thumbnailer, you need to add a corresponding fuzzer for it using the following steps:

- Update `CMakeLists.txt` to include the new fuzzer, following the pattern of existing ones
- If the thumbnailer needs extra dependencies, update `prepare_build.sh` accordingly
- Update `build_fuzzers.sh` to build those dependencies and include the new fuzzer
