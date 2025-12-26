// server.h
#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <string>
#include <atomic> // Thread safety ke liye bohot zaroori hai
#include <chrono> // Time/Uptime track karne ke liye (Ye missing tha, add kar diya)
#include "ThreadPool.h" // Workers ki definition yahan hai

class WebServer {
public:
    // Constructor: Port aur threads ki ginti set karne ke liye
    WebServer(int port, int num_threads);

    // Ye function server ka main loop chalu karega
    void run();

private:
    int server_fd; // Server socket ka ID (File Descriptor)
    int port;
    ThreadPool thread_pool; // Humari workers ki team
    
    // --- Metrics (Stats) ---
    // 'atomic' isliye use kiya hai taaki jab 100 threads ek saath
    // in variables ko update karein, toh ginti gadbad na ho (Race Condition fix).
    std::atomic<int> total_requests;
    std::atomic<int> active_threads;
    
    // Server kab start hua, wo time yahan store hoga
    std::chrono::steady_clock::time_point start_time;

    // Optimization:
    // Baar-baar hard disk se index.html padhna slow hota hai.
    // Isliye file ko ek baar padh ke RAM (string) mein save kar lenge.
    std::string cached_index_html;
    
    // --- Helper Functions ---
    
    // Ek single client connection ko handle karne ka logic yahan hai
    void handle_client(int client_socket);
    
    // File read karke string return karta hai
    std::string load_file(const std::string& filename);
    
    // Check karta hai ki server abhi kitni RAM kha raha hai
    long get_memory_usage();
};

#endif