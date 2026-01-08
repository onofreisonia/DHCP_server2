
# Numarul de clienti de simulat
NUM_CLIENTS=3

# Mergem in root pentru compilare
cd .. # Asumand ca scriptul e in Client/
echo "=== [SIMULATION] Building Project (from root) ==="
make client_app

if [ $? -ne 0 ]; then
    echo "Eroare la compilare!"
    exit 1
fi

echo "=== [SIMULATION] Curatare socket-uri vechi ==="
rm -f client_sock_*

echo "=== [SIMULATION] Pornire $NUM_CLIENTS Clienti... ==="

pids=""

for ((i=1; i<=NUM_CLIENTS; i++))
do
    echo "  -> Pornire Client #$i"
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