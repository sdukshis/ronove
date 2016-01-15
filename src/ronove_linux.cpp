#include "ronove/ronove.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <system_error>

#include <fcntl.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using ronove::Daemon;

const int intercepting_signals[] = {SIGINT, SIGUSR1};

void daemonize()
{
    umask(0); // Set file creation mask to zero

    // Become a session leader
    pid_t pid;
    if ((pid = fork()) < 0) {
        throw std::system_error(errno, std::system_category(),
                                "fail to fork process");
    } else if (pid != 0) {
        // parent
        exit(0);
    }

    setsid();

    if ((pid = fork()) < 0) {
        throw std::system_error(errno, std::system_category(),
                                "fail to fork process");
    } else if (pid != 0) {
        // parent
        exit(0);
    }

    if (chdir("/") < 0) {
        throw std::system_error(errno, std::system_category(),
                                "fail to chdir to /");
    }

    // Attach file descriptors 0, 1, and 2 to /dev/null.
    int devnull = -1;
    if (-1 != (devnull = open("/dev/null", O_RDWR))) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO) {
            close(devnull);
        }
    } else {
        throw std::system_error(errno, std::system_category(),
                                "fail to open /dev/null");
    }
}

template <typename ForwardIter>
sigset_t to_signal_mask(ForwardIter begin, ForwardIter end)
{
    sigset_t mask;
    sigemptyset(&mask);
    std::for_each(begin, end, [&mask](int signum) {
        if (sigaddset(&mask, signum) == -1) {
            std::stringstream ss;
            ss << "fail to sigaddset " << signum << " signal";
            throw std::system_error(errno, std::system_category(), ss.str());
        }
    });
    return mask;
}

void block_signals(sigset_t mask)
{
    if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1) {
        throw std::system_error(errno, std::system_category(),
                                "fail to sigprocmask");
    }
}

int create_signalfd(sigset_t mask)
{
    int sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        throw std::system_error(errno, std::system_category(),
                                "fail to create signalfd");
    }
    return sfd;
}

signalfd_siginfo read_siginfo(int sfd)
{
    signalfd_siginfo fdsi;
    std::memset(&fdsi, 0, sizeof(fdsi));
    ssize_t rbytes = read(sfd, &fdsi, sizeof(fdsi));
    if (rbytes != sizeof(fdsi)) {
        throw std::system_error(errno, std::system_category(),
                                "fail to read from signalfd");
    }
    return fdsi;
}

Daemon::Daemon(const std::string &name)
    : running{false}
    , name_{name}
{
}

Daemon::~Daemon() {}

int Daemon::run(bool background)
{
    if (background) {
        daemonize();
    }

    block_signals(to_signal_mask(std::begin(intercepting_signals),
                                 std::end(intercepting_signals)));
    int sfd = create_signalfd(to_signal_mask(std::begin(intercepting_signals),
                                             std::end(intercepting_signals)));

    app_thread = std::thread([this]() { on_start(); });

    running = true;

    while (running) {
        signalfd_siginfo fdsi = read_siginfo(sfd);
        switch (fdsi.ssi_signo) {
        case SIGINT:
            on_stop();
            running = false;
            break;
        case SIGUSR1:
            on_reconfigure();
            break;
        default:
            throw std::logic_error("Got unexpected signal number");
        }
    }
    app_thread.join();
    close(sfd);
    // TODO: unblock signals ?
    return EXIT_SUCCESS;
}
