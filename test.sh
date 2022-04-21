#!/bin/bash

for i in {0..100}
do
    #120 rows
    $(./proj2 15 17 0 0)
    wait $!

    RES=$(cat proj2.out | awk -F' ' -v creating=0 -v created=0 '{
        if($4 == "creating")
        {
            creating++
            if(creating == 3){
                creating=0
            }
        }else if ($6 == "created" && creating != 0){
            print "Earlier created then creating!"
        }
    }END{if(creating != 0){print "Unused atom printed creating"}}')
    
    if [[ $RES != "" ]]
    then
        echo ""
        echo ""
        echo ""
        echo $RES
        cat proj2.out
    fi

    RES=$(cat proj2.out | bash kontrola-vystupu.sh)
    if [[ $RES != "" ]]
    then
        echo ""
        echo ""
        echo ""
        echo $RES
        cat proj2.out
    fi
done

for i in {0..50}
do
    ti=$(($RANDOM%1001))
    tb=$(($RANDOM%1001))
    #30 rows
    $(./proj2 5 9 $ti $tb)
    wait $!

    RES=$(cat proj2.out | awk -F' ' -v creating=0 -v created=0 '{
        if($4 == "creating")
        {
            creating++
            if(creating == 3){
                creating=0
            }
        }else if ($6 == "created" && creating != 0){
            print "Earlier created then creating!"
        }
    }END{if(creating != 0){print "Unused atom printed creating"}}')
    if [[ $RES != "" ]]
    then
        echo ""
        echo ""
        echo ""
        echo $RES
        cat proj2.out
    fi

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