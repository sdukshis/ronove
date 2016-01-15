#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "ronove/ronove.h"

class Timer : public ronove::Daemon {
public:
    Timer()
        : Daemon("timer")
        , running{false}
    {
    }

private:
    void on_start() override
    {
        std::clog << "Starting timer" << std::endl;
        running = true;
        auto start = std::chrono::system_clock::now();
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto now = std::chrono::system_clock::now();
            auto uptime = now - start;
            std::clog << "Uptime: "
                      << std::chrono::duration_cast<std::chrono::seconds>(
                             uptime)
                             .count()
                      << " seconds" << std::endl;
        }
    }

    void on_stop() override
    {
        std::clog << "Stop rrequested" << std::endl;
        running = false;
    }

    std::atomic_bool running;
};

int main(int argc, char *argv[]) try {
    if (argc < 2) {
        throw std::invalid_argument("Usage: timer [0|1]");
    }
    bool background = std::stoi(argv[1]);
    Timer timer;
    return timer.run(background);
} catch (const std::system_error &e) {
    std::cerr << "Error: " << e.code() << ", Message: " << e.what()
              << std::endl;
} catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
} catch (...) {
    std::cerr << "Unknonwn error" << std::endl;
}