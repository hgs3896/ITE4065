cmake . -B build -DCMAKE_BUILD_TYPE=Debug
rm -f run && cd build && make && cd - && cp build/AtomicSnapshot run