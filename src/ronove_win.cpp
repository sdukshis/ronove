#include "ronove/ronove.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

using ronove::Daemon;

Daemon::Daemon(const std::string &name)
    : running{false}
    , name_{name}
{
    instance = this;
}

Daemon::~Daemon() { instance = nullptr; }

int Daemon::run(bool background)
{
    if (background) {
        SERVICE_TABLE_ENTRY dispatch_table[] = {
            {const_cast<LPSTR>(name.c_str()),
             (LPSERVICE_MAIN_FUNCTION)service_main},
            {nullptr, nullptr},
        };

        if (!StartServiceCtrlDispatcher(dispatch_table)) {
            throw std::system_error(GetLastError(), std::system_categoty(),
                                    "fail StartServiceCtrlDispatcher");
        }
    }

    SetConcoleCtrlHandler((PHANDLER_ROUTINE)console_ctrl_handler, TRUE);

    app_thread = std::thread([this]() { on_start(); });

    app_thread.join();
    return EXIT_SUCCESS;
}

void Daemon::service_main(DWORD, LPSTR *)
{
    instance->service_status_handle =
        RegisterServiceCtrlHandler(const_cast<LPSTR>(instance->name_.data()),
                                   (LPHANDLER_FUNCTION)service_ctrl_handler);
    if (!instance->service_status_handle) {
        throw std::system_error(GetLastError(), std::system_categoty(),
                                "fail RegisterServiceCtrlHandler");
    }

    instance->service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    instance->service_status.dwCurrentState = SERVICE_START_PENDING;
    instance->service_status.dwControlsAccepted =
        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    instance->service_status.dwControlsAccepted |= SERVICE_ACCEPT_PARAMCHANGE;
    instance->service_status.dwWin32ExitCode = NO_ERROR;
    instance->service_status.dwServiceSpecificExitCode = 0;
    instance->service_status.dwWaitHint = 0;

    SetServiceStatus(instance->service_status_handle,
                     &instance->service_status);

    instance->service_status.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(instance->service_status_handle,
                     &instance->service_status);

    app_thread = std::thread([instance]() { instance->on_start(); });

    app_thread.join();

    instance->service_status.dwWin32ExitCode = 0;
    instance->service_status.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(instance->service_status_handle,
                     &instance->service_status);
}

void Daemon::service_ctrl_handler(DWORD control)
{
    switch (control) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        instance->service_status.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(instance->service_status_handle,
                         &instance->service_status);    
            instance->on_stop();
        break;
    case SERVICE_CONTROL_PARAMCHANGE:
        instance->on_reconfigure();
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    }
}

void Daemon::console_ctrl_handler(DWORD control)
{
    switch (control) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
        instance->on_stop();
        return TRUE;
    default:
        return FALSE;
    }    
}
