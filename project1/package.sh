#!/bin/bash

tar --dereference -zcf \
    submission.tar.gz \
    *.sh \
    *.txt \
    *.cpp \
    include \
    README