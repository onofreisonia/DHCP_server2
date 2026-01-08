

# Numarul de clienti de simulat
NUM_CLIENTS=3

echo "=== [SIMULATION] Building Client ==="

echo "=== [SIMULATION] Building Client ==="
make -C .

if [ $? -ne 0 ]; then
    echo "Eroare la compilare!"
    exit 1
fi

echo "=== [SIMULATION] Initializare... ==="

echo "=== [SIMULATION] Pornire $NUM_CLIENTS Clienti... ==="

pids=""

for ((i=1; i<=NUM_CLIENTS; i++))
do
    echo "  -> Pornire Client #$i"
    # Pornim clientul in background si ii dam input "exit" 
    # Pentru test, le lasam sa ruleze.
    ./client_app > "client_$i.log" 2>&1 &
    pids="$pids $!"
    sleep 1 
done

echo "=== [SIMULATION] Clientii ruleaza in background. ==="
echo "PID-uri: $pids"
echo "Log-urile sunt salvate in client_X.log"
echo "Apasa ENTER pentru a opri toti clientii..."

read

echo "=== [SIMULATION] Oprire Clienti... ==="
for pid in $pids; do
    kill -SIGINT $pid
done

wait
echo "=== [SIMULATION] Gata. ==="
