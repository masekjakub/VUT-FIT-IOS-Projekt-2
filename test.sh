#!/bin/bash

for i in {0..100}
do
    ./proj2 20 29 5 5
    sleep 0.2
    RES=$(cat proj2.out| bash kontrola-vystupu.sh)
    if [[ $RES != "" ]]
    then
        echo ""
        echo ""
        echo ""
        echo $RES
        cat proj2.out
    fi
  
done