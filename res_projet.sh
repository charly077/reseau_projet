#!/bin/bash

rm coucou

make clean
make receiver
make sender

./receiver --file coucou localhost 30000
