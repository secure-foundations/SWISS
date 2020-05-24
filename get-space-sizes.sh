set -x

./bench-simple.sh sdl --get-space-size | grep 'symm counts'

./bench-simple.sh breadth-leader-election --get-space-size | grep 'symm counts'
./bench-simple.sh leader-election-depth2 --get-space-size | grep 'symm counts'
./bench-simple.sh breadth-2pc --get-space-size | grep 'symm counts'
./bench-simple.sh breadth-lock-server --get-space-size | grep 'symm counts'
./bench-simple.sh breadth-learning-switch --get-space-size | grep 'symm counts'
./bench-simple.sh better-template-paxos --get-space-size | grep 'symm counts'
./bench-simple.sh better-template-flexible-paxos --get-space-size | grep 'symm counts'
./bench-simple.sh chord-gimme-1 --get-space-size | grep 'symm counts'

./bench-simple.sh decentralized-lock-gimme-1 --get-space-size | grep 'symm counts'
