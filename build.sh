#!/bin/bash
if [ ! -d "bin" ]; then
    mkdir bin
fi
cd bin
cmake ..
make