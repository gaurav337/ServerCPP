# Compiler set kar rahe hain (g++ use karenge)
CC = g++

# Flags define kar rahe hain:
# -O3       : Code ko full optimize kar dega taaki speed badhe.
# -pthread  : Multithreading enable karne ke liye zaroori hai.
# -std=c++17: Modern C++ features use karne ke liye.
CFLAGS = -O3 -pthread -std=c++17

# Final jo executable file banegi, uska naam 'server' hoga
TARGET = server

# Default target: Jab sirf 'make' likhoge terminal mein, toh ye chalega
all: $(TARGET)

# Main Compilation Rule:
# Ye check karega ki main.cpp ya server.cpp mein kuch change hua hai kya.
# Agar hua hai, toh dobara compile karega.
$(TARGET): main.cpp server.cpp
	$(CC) $(CFLAGS) main.cpp server.cpp -o $(TARGET)

# Safai Abhiyan:
# 'make clean' chalane par purana executable delete ho jayega.
# Taaki fresh build bana sako.
clean:
	rm -f $(TARGET)