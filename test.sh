#!/bin/bash
echo "TESTING..."
PARAM=$1
PROCCOUNT=$2
for i in $(seq "$PARAM")
do
    ./proj2 3 5 0 0
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
        if(NR != 30){
            print "Some rows are missing! Expected 30, got: "NR
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
    no=$(($RANDOM%$PROCCOUNT))
    nh=$(($RANDOM%$PROCCOUNT))
    ./proj2 $no $nh $ti $tb
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
    }')

    if [[ $RES != "" ]]
    then
        echo "----------------------------------------------------------------------"
        echo "./proj2 "$no $nh $ti $tb
        echo $RES
        cat proj2.out
    fi
done
echo "TESTING DONE"