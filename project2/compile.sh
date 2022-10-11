cmake . -B build -DCMAKE_BUILD_TYPE=Release
cd build && make && cd - && cp build/AtomicSnapshot run