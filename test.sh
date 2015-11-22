#!/bin/sh

check_output() {
    cmp .output.$1.$2 .expected_output.$1.$2 2>/dev/null >/dev/null
    if [ $? -eq 0 ]
    then
        echo "PASSED"
        #rm $1 $2
    else
        echo "FAILED"
        echo "Output from tritontalk binary is in file: .output.$1.$2"
        echo "Expected output is in file: .expected_output.$1.$2"
        echo "Debug output from tritontalk binary is in file: .debug_output.$1.$2"
    fi
    echo
}

echo "###########################################################################"
echo "NOTE: Guidelines for using this script:"
echo "      - Tests check output written to stdout only."
echo "        If you have debug output in stdout, it may cause test failures."
echo "      - This is not a comprehensive set of test cases."
echo "        It is intended to help you do basic checks on your project."
echo "      - If your program fails any test, it will print the output files"
echo "        generated by your program and the output expected for the test case."
echo "      - The script should take about 20 seconds to complete."
echo "###########################################################################"
echo
echo
echo "Rebuilding tritontalk..."
make clean; make -j4 tritontalk
echo "Done rebuilding tritontalk"
echo


## Test Case 1
echo -n "Test case 1: Sending 300 packet and expecting receiver to print it out: "
(sleep 0.5; for i in `seq 1 300`; do echo "msg 0 0 Packet: $i"; sleep 0.1; done; sleep 5; echo "exit") | ./tritontalk -r 1 -s 1 > .output.1.$1 2> .debug_output.1.$1

(for i in `seq 1 300`; do echo "<RECV_0>:[Packet: $i]"; done) > .expected_output.1.$1

check_output 1 $1


## Test Case 2
echo -n "Test case 2: Sending 100 packets with 60% drop rate and expecting receiver to print them out in order: "
(sleep 0.5; for i in `seq 1 100`; do echo "msg 0 0 Packet: $i"; sleep 0.1; done; sleep 5; echo "exit") | ./tritontalk -r 1 -s 1 -d 0.6 > .output.2.$1 2> .debug_output.2.$1

(for i in `seq 1 100`; do echo "<RECV_0>:[Packet: $i]"; done) > .expected_output.2.$1

check_output 2 $1


## Test Case 3
echo -n "Test case 3: Sending 100 packets (with corrupt probability of 60%) and expecting receiver to print them out in order: "
(sleep 0.5; for i in `seq 1 100`; do echo "msg 0 0 Packet: $i"; sleep 0.1; done; sleep 5; echo "exit") | ./tritontalk -c 0.6 -r 1 -s 1 > .output.3.$1 2> .debug_output.3.$1

(for i in `seq 1 100`; do echo "<RECV_0>:[Packet: $i]"; done) > .expected_output.3.$1

check_output 3 $1


## Test Case 4
echo -n "Test case 4: Sending 100 packets (with corrupt probability of 25% and drop probability of 25%) and expecting receiver to print them out in order: "
(sleep 0.5; for i in `seq 1 100`; do echo "msg 0 0 Packet: $i"; sleep 0.1; done; sleep 5; echo "exit") | ./tritontalk -d 0.25 -c 0.25 -r 1 -s 1 > .output.4.$1 2> .debug_output.4.$1

(for i in `seq 1 100`; do echo "<RECV_0>:[Packet: $i]"; done) > .expected_output.4.$1

check_output 4 $1

echo
echo "Completed test cases"
