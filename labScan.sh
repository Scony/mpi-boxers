#!/bin/bash

for ((i = 1; i <= 16; i++))
do
    printf "lab-os-$i:"
    ping "lab-os-$i" -c 1 -W 1 | grep "1 received" -c
done
