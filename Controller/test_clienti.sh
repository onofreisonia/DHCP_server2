

cd "$(dirname "$0")/.." || exit

echo "--- Re-compilare proiect ---"
make clean && make CFLAGS="-D_POSIX_C_SOURCE=200809L -Wall -g -pthread -I./Model -I./Service -I./View -I./Common -DSIMULATE_ERRORS=0"

echo "--- Pornire simulare 3 clienti simultani ---"
# Pornim 3 clienti in background
./client_app &
PID1=$!
echo "[SCRIPT] Client 1 pornit (PID $PID1)"

./client_app &
PID2=$!
echo "[SCRIPT] Client 2 pornit (PID $PID2)"

./client_app &
PID3=$!
echo "[SCRIPT] Client 3 pornit (PID $PID3)"

# Asteptam ca toti sa termine
echo "[SCRIPT] Se asteapta finalizarea clientilor..."
wait $PID1 $PID2 $PID3

echo "--- Simulare finalizata ---"
