#ifndef RONOVE_H
#define RONOVE_H

#include <string>
#include <thread>
#include <system_error>

namespace ronove {

class Daemon {
public:
    virtual ~Daemon();

    Daemon(const std::string& name);

    const std::string& name() const { return name_; }

    int run(bool background = true);

private:
    virtual void on_start() = 0;

    virtual void on_stop() = 0;

    virtual void on_reconfigure() {};

    bool running;
    std::string name_;
    std::thread app_thread;
};

}
#endif
