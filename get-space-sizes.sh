set -x

./bench-simple.sh breadth-leader-election --get-space-size | grep 'space size'
./bench-simple.sh leader-election-depth2 --get-space-size | grep 'space size'
./bench-simple.sh breadth-2pc --get-space-size | grep 'space size'
./bench-simple.sh lock-server --get-space-size | grep 'space size'
./bench-simple.sh breadth-learning-switch --get-space-size | grep 'space size'
./bench-simple.sh breadth-paxos-4-r3 --get-space-size | grep 'space size'
./bench-simple.sh breadth-paxos-exist-2 --get-space-size | grep 'space size'
./bench-simple.sh finisher-paxos-exist-1-depth2 --get-space-size | grep 'space size'

./bench-simple.sh breadth-flexible-paxos-4-r3 --get-space-size | grep 'space size'
./bench-simple.sh breadth-flexible-paxos-exist-2 --get-space-size | grep 'space size'
./bench-simple.sh finisher-flexible-paxos-exist-1-depth2 --get-space-size | grep 'space size'

./bench-simple.sh breadth-multi-paxos-4-r3 --get-space-size | grep 'space size'
./bench-simple.sh breadth-multi-paxos-exist-2 --get-space-size | grep 'space size'
./bench-simple.sh finisher-multi-paxos-exist-1-depth2 --get-space-size | grep 'space size'
