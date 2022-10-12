cmake . -B build -DCMAKE_BUILD_TYPE=Release -DFORCE_TESTS=ON
rm -f run && cd build && make && cd - && cp build/AtomicSnapshot run