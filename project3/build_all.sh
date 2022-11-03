#!/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
echo "Create debug project" && \
    cmake -B $DIR/debug -DCMAKE_BUILD_TYPE=DEBUG $DIR && \
    cd $DIR/debug && \
    (make -j; cd -) && \
    echo "Successfully created debug project"

echo "Create release project" && \
    cmake -B $DIR/release -DCMAKE_BUILD_TYPE=RELEASE $DIR && \
    cd $DIR/release && \
    (make -j; cd -) && \
    echo "Successfully created release project"