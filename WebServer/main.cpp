// main.cpp
#include "server.h" // Apni server class yahan define hai
#include <thread>   // Threading handle karne ke liye standard library

int main() {
    // Server ka object bana rahe hain (Instantiation).
    // Pehla parameter (8080) -> Port number hai jahan server sunega.
    // Dusra parameter (100) -> Thread Pool size hai (matlab ek saath 100 requests handle hongi).
    WebServer server(8080, 100);

    // Ab server ko start kar rahe hain.
    // Ye function ek loop mein chala jayega aur requests ka wait karega.
    server.run();

    // Waise toh code yahan tak tabhi aayega jab server manually band karoge.
    // Par formality ke liye return 0 likhna zaroori hai.
    return 0;
}