#!/bin/bash

tar --dereference -zcf \
    submission.tar.gz \
    *.sh \
    *.txt \
    *.cpp \
    include \
    test \
    README

# tar --dereference \
#     --exclude='build' \
#     --exclude='submission.tar.gz' \
#     --exclude='workloads' \
#     --exclude='.ipynb_checkpoints' \
#     --exclude='.vscode' \
#     --exclude='html' \
#     --exclude='latex' \
#     --exclude='*.ipynb' \
#     -czf submission.tar.gz *