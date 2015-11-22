#!/bin/bash
echo -n "Run Test 10 times" 
for i in `seq 1 10`; do sh ./test.sh $i > result.$i; echo -n " Test $i"; done;
#(sleep 0.5; for i in 'seq 1 10'; do echo "./test.sh"; sleep 0.1; done; sleep 1; echo "done")
