#include "server.h"
#include <iostream>
#include <unistd.h> // read, write, close ke liye
#include <cstring>  // memset ke liye
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <sys/resource.h> // RAM usage check karne ke liye
#include <netinet/in.h>   // sockaddr_in structures ke liye

// --- Naye Headers (Benchmark feature ke liye) ---
#include <array>  // Buffer array ke liye
#include <memory> // Unique_ptr ke liye
#include <cstdio> // Important: popen() aur pclose() isi mein hote hain

// --- Helper: Command Runner ---
// Ye function terminal command chalata hai aur jo bhi output aata hai,
// usse string banakar return karta hai.
std::string exec_command(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;

    // popen process open karta hai (read mode 'r' mein)
    // // unique_ptr use kiya taaki memory leak na ho, ye khud close kar dega
    // std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

    // Humne 'decltype' hata ke seedha bata diya ki ye kaisa function hai
    std::unique_ptr<FILE, int (*)(FILE *)> pipe(popen(cmd, "r"), pclose);

    if (!pipe)
    {
        return "Bhai command start hi nahi hui! (popen failed)";
    }

    // Output ko thoda-thoda karke buffer mein padh rahe hain
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

// --- Constructor: Server setup yahan hota hai ---
WebServer::WebServer(int port, int num_threads)
    : port(port), thread_pool(num_threads), total_requests(0), active_threads(0)
{

    // Server kab start hua, wo time note kar rahe hain (Uptime ke liye)
    start_time = std::chrono::steady_clock::now();

    // Performance Hack:
    // HTML file ko server start hote hi RAM mein load kar rahe hain.
    // Har request pe disk read karenge toh slow ho jayega.
    cached_index_html = load_file("public/index.html");

    if (cached_index_html.empty())
    {
        std::cerr << "Warning: Bhai 'public/index.html' nahi mila ya khali hai!" << std::endl;
    }

    // 1. Socket Create karna (Server ka endpoint)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        perror("Socket hi nahi bana");
        exit(EXIT_FAILURE);
    }

    // 2. Socket Options Set karna
    // Agar server crash ho ke restart ho, toh "Port already in use" ka error na aaye,
    // isliye REUSEADDR flag laga rahe hain.
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Setsockopt fail ho gaya");
        exit(EXIT_FAILURE);
    }

    // 3. Bind: Socket ko Port (e.g., 8080) ke saath jodna
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Kisi bhi IP se connect ho sake
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind nahi ho paya (shayad port busy hai?)");
        exit(EXIT_FAILURE);
    }

    // 4. Listen: Ab server requests ka intezaar karega
    // Backlog 100 ka matlab hai ki agar sab threads busy hain, toh 100 requests queue mein rahengi.
    if (listen(server_fd, 100) < 0)
    {
        perror("Listen fail ho gaya");
        exit(EXIT_FAILURE);
    }
}

// --- Main Server Loop ---
void WebServer::run()
{
    std::cout << "Server start ho gaya port " << port << " par. Threads: 100" << std::endl;

    while (true)
    {
        struct sockaddr_in address;
        int addrlen = sizeof(address);

        // Nayi connection accept kar rahe hain
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        if (new_socket >= 0)
        {
            // Connection mil gaya! Ab isko ThreadPool mein daal do.
            // Main thread free rehna chahiye taaki nayi requests le sake.
            thread_pool.enqueue([this, new_socket]
                                { this->handle_client(new_socket); });
        }
        else
        {
            perror("Accept fail ho gaya");
        }
    }
}

// --- Helper: Memory Usage ---
long WebServer::get_memory_usage()
{
    // Ye system call se pata lagata hai ki process kitni RAM kha raha hai (KB mein)
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

// --- Helper: File Loading ---
std::string WebServer::load_file(const std::string &filename)
{
    std::ifstream file(filename);
    if (file)
    {
        std::stringstream buffer;
        buffer << file.rdbuf(); // Pura file buffer mein
        return buffer.str();
    }
    return ""; // Agar file na mile toh khali string
}

// --- Client Handler (Asli Kaam Yahan Hota Hai) ---
void WebServer::handle_client(int client_socket)
{
    // Active thread counter badha do taaki dashboard pe dikhe
    active_threads++;

    // Timeout Logic:
    // Agar client 0.5 seconds tak shant baitha hai, toh connection kaat denge.
    // Nahi toh wo resources block karke rakhega.
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000; // 500ms
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    char buffer[4096];

    // Keep-Alive Loop:
    // Ek hi connection par multiple requests handle karenge (Efficiency badhane ke liye)
    while (true)
    {
        memset(buffer, 0, 4096); // Buffer saaf kar rahe hain

        // Client se data padh rahe hain
        int valread = read(client_socket, buffer, 4096);

        if (valread <= 0)
        {
            // Ya toh timeout ho gaya, ya client ne disconnect kar diya
            break;
        }

        total_requests++; // Stats update

        // Request Parsing (Jugaad style string stream parsing)
        std::string request(buffer);
        std::stringstream ss(request);
        std::string method, path;
        ss >> method >> path; // Pehle do words utha liye: "GET" aur "/index.html"

        std::string response;

        // --- Routing Logic ---

        // Case 1: Homepage maanga hai
        if (path == "/" || path == "/index.html")
        {
            if (!cached_index_html.empty())
            {
                response = "HTTP/1.1 200 OK\r\n"
                           "Connection: keep-alive\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: " +
                           std::to_string(cached_index_html.length()) + "\r\n\r\n" +
                           cached_index_html;
            }
            else
            {
                // Agar index.html missing hai
                std::string msg = "<h1>404 File Not Found (Bhai public/ folder check kar le)</h1>";
                response = "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: " +
                           std::to_string(msg.length()) + "\r\n\r\n" + msg;
            }
        }
        // Case 2: Stats maange hain (Dashboard ke liye)
        else if (path == "/status")
        {
            auto now = std::chrono::steady_clock::now();
            auto uptime_sec = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();

            // JSON bana rahe hain manually
            std::string json = "{";
            json += "\"total_requests\": " + std::to_string(total_requests) + ",";
            json += "\"active_threads\": " + std::to_string(active_threads) + ",";
            json += "\"memory_usage_kb\": " + std::to_string(get_memory_usage()) + ",";
            json += "\"uptime_sec\": " + std::to_string(uptime_sec);
            json += "}";

            response = "HTTP/1.1 200 OK\r\n"
                       "Connection: keep-alive\r\n"
                       "Content-Type: application/json\r\n"
                       "Content-Length: " +
                       std::to_string(json.length()) + "\r\n"
                                                       "\r\n" +
                       json;
        }
        // Case 3: Benchmark Test (AB Command Run karo)
        else if (path == "/benchmark")
        {
            // NOTE: Ye command server pe load dalegi.
            // Hum 'ab' tool run kar rahe hain background mein.
            std::string cmd = "ab -k -n 20000 -c 50 http://127.0.0.1:8080/";

            // Upar jo exec_command banaya tha, usse call kar rahe hain
            std::string benchmark_result = exec_command(cmd.c_str());

            response = "HTTP/1.1 200 OK\r\n"
                       "Connection: close\r\n"        // Keep-alive band, kyunki test lamba chal sakta hai
                       "Content-Type: text/plain\r\n" // Text bhej rahe hain taaki pre tag mein dikhe
                       "Content-Length: " +
                       std::to_string(benchmark_result.length()) + "\r\n"
                                                                   "\r\n" +
                       benchmark_result;
        }
        // Case 4: Kuch aisi file maangi jo hai hi nahi
        else
        {
            std::string msg = "<h1>404 Not Found</h1>";
            response = "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: " +
                       std::to_string(msg.length()) + "\r\n\r\n" + msg;
        }

        // Response wapas bhej rahe hain client ko
        ssize_t bytes_sent = write(client_socket, response.c_str(), response.length());

        // Agar bhejte waqt error aaya (client bhaag gaya), toh loop tod do
        if (bytes_sent < 0)
            break;
    }

    // Kaam khatam, socket band, aur thread count kam karo
    close(client_socket);
    active_threads--;
}