#!/bin/bash

rm coucou

make clean
make receiver
make sender

./receiver --file coucou ::1 10000
