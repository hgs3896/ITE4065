cmake . -B build -DCMAKE_BUILD_TYPE=Debug
cd build && make && cd - && cp build/AtomicSnapshot run