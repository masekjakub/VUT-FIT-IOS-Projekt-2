#!/bin/bash

PARAM=$1
for i in $(seq "$PARAM")
do
    $(./proj2 30 17 0 0)
    wait $!

   RES=$(cat proj2.out | tr -d : | awk -F' ' -v creating=0 -v created=0 -v row=0 '{
        row++
        if($1 != row){
            print "Wrong A: at line "NR
        }
        if($4 == "creating")
        {
            creating++
            if(creating == 3){
                creating=0
            }
        }else if ($6 == "created" && creating != 0){
            print "Earlier created then creating on line "NR+1
        }
    }END{
        if(creating != 0){
            print "Unused atom printed creating"
        }
        if(NR != 165){
            print "Some rows are missing! Expected 165, got: "NR
        }
    }')

    RES=$RES$(cat proj2.out | bash kontrola-vystupu.sh)
    if [[ $RES != "" ]]
    then
        echo "----------------------------------------------------------------------"
        echo $RES
        cat proj2.out
    fi
done

for i in $(seq "$PARAM")
do
    ti=$(($RANDOM%1001))
    tb=$(($RANDOM%1001))
    $(./proj2 5 9 $ti $tb)
    wait $!

   RES=$(cat proj2.out | tr -d : | awk -F' ' -v creating=0 -v created=0 -v row=0 '{
        row++
        if($1 != row){
            print "Wrong A: at line "NR
        }
        if($4 == "creating")
        {
            creating++
            if(creating == 3){
                creating=0
            }
        }else if ($6 == "created" && creating != 0){
            print "Earlier created then creating on line "NR+1
        }
    }END{
        if(creating != 0){
            print "Unused atom printed creating"
        }
        if(NR != 54){
            print "Some rows are missing! Expected 54, got: "NR
        }
    }')

    RES=$RES$(cat proj2.out | bash kontrola-vystupu.sh)
    if [[ $RES != "" ]]
    then
        echo "----------------------------------------------------------------------"
        echo $RES
        cat proj2.out
    fi
done