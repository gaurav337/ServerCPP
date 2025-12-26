// ThreadPool.h
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    // --- Constructor ---
    // Jitne threads bole (num_threads), utne workers create karega.
    ThreadPool(size_t num_threads) : stop(false) {
        
        for(size_t i = 0; i < num_threads; ++i) {
            // Har thread ke liye ek lambda function bana rahe hain
            // Jo infinite loop mein chalega aur kaam ka wait karega.
            workers.emplace_back([this] {
                while(true) {
                    std::function<void()> task;

                    {
                        // LOCK LAGA RAHE HAIN:
                        // Queue share ho rahi hai sab threads ke beech, 
                        // isliye race condition na ho, lock zaroori hai.
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        
                        // WAIT KARO:
                        // Agar queue khali hai aur stop nahi hua, toh yahan wait karo (so jao).
                        // CPU waste nahi hoga.
                        condition.wait(lock, [this]{ 
                            return stop || !tasks.empty(); 
                        });
                        
                        // Agar stop signal mil gaya aur kaam bhi nahi bacha, toh return (exit thread).
                        if(stop && tasks.empty()) return;
                        
                        // Queue se sabse pehla task uthao
                        task = std::move(tasks.front());
                        tasks.pop();
                    } 
                    // LOCK KHUL GAYA YAHAN (Scope end)

                    // Ab task execute karo (Lock ki zarurat nahi hai execution mein)
                    task();
                }
            });
        }
    }

    // --- Enqueue (Kaam add karne ke liye) ---
    // Template use kiya hai taaki kisi bhi type ka function/lambda pass kar sakein.
    template<class F>
    void enqueue(F&& f) {
        {
            // Queue safe rakhni hai, isliye lock lagaya
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            // Task ko queue mein daal diya
            // std::forward use kiya taaki perfect forwarding ho sake
            tasks.emplace(std::forward<F>(f));
        }
        
        // Ek soye hue thread ko jaga do ki "Uth ja bhai, kaam aaya hai"
        condition.notify_one();
    }

    // --- Destructor ---
    // Jab server band hoga, toh threads ko bhi gracefully band karna padega.
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true; // Dukaan band karne ka signal
        }
        
        // Sab soye hue threads ko utha do taaki wo check karein ki stop == true hai
        condition.notify_all();
        
        // Wait karo jab tak saare threads apna current kaam khatam karke band na ho jayein
        for(std::thread &worker : workers)
            worker.join();
    }

private:
    // Humare workers (Threads)
    std::vector<std::thread> workers;
    
    // Kaam ki list (Queue of functions)
    std::queue<std::function<void()>> tasks;
    
    // Safety tools for threading
    std::mutex queue_mutex;
    std::condition_variable condition;
    
    // Flag ki ab rukna hai ya nahi
    bool stop;
};

#endif