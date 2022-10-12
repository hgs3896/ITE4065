cmake . -B build -DCMAKE_BUILD_TYPE=Release
rm -f run && cd build && make && cd - && cp build/AtomicSnapshot run