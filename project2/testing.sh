#!/bin/bash

# Project 2 Testing Script - testing.sh
# Usage: chmod +x testing.sh && ./testing.sh

PORT=5010
SERVER=./dbserver
DBTEST=./dbtest

echo "Starting server on port $PORT..."
($SERVER $PORT &)
SERVER_PID=$!

sleep 2  

echo "Running SET test..."
$DBTEST --port=$PORT -S key1 value1

echo "Running GET test..."
$DBTEST --port=$PORT -G key1

echo "Running DELETE test..."
$DBTEST --port=$PORT -D key1

echo "Running GET test...(should fail)"
$DBTEST --port=$PORT -G key1

echo "Running GET test...(non-existent)"
$DBTEST --port=$PORT -G key2

echo "Running DELETE test...(non-existent)"
$DBTEST --port=$PORT -D key3

echo "Running Value rewrite test..."
$DBTEST --port=$PORT -S key4 value4
$DBTEST --port=$PORT -G key4 
$DBTEST --port=$PORT -S key4 value3
$DBTEST --port=$PORT -G key4 

echo "Running load test with 50 requests and 4 threads..."
$DBTEST --port=$PORT --count=50 --threads=4

echo "Running random test mix (10 concurrent random requests)..."
$DBTEST --port=$PORT --test

echo "Invalid command..."
echo "stats" | nc localhost $PORT

echo "stats test..."
echo "stats" | nc localhost $PORT

echo "quit command test..."
echo "quit" | nc localhost $PORT

sleep 2

wait $SERVER_PID
if [ $? -eq 0 ]; then
    echo "Server exited cleanly."
else
    echo "FAILED: Server did not exit cleanly."
fi

echo "Restarting server for quit test with -q"
($SERVER_EXEC $PORT &)  
SERVER_PID=$!

sleep 2

wait $SERVER_PID
if [ $? -eq 0 ]; then
    echo "Server exited cleanly."
else
    echo "FAILED: Server did not exit cleanly."
fi

echo "Testing qutit via -q flag..."
$DBTEST_EXEC --port=$PORT -q

echo "Test run complete."
