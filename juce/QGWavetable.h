#pragma once

#include <JuceHeader.h>
#include <vector>
#include <thread>
#include <atomic>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <processthreadsapi.h>
    #include <handleapi.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <sys/un.h>
    #include <unistd.h>
    #include <signal.h>
    #include <sys/types.h>
#endif
#include <cstdlib>
#include <cstdio>

class QGWavetableBuffer
{
public:
    static constexpr int WAVEFORM_SIZE = 2048;
    static constexpr int BUFFER_COUNT = 4;

    QGWavetableBuffer()
        : currentBuffer(0), nextBuffer(0), newDataReady(false)
    {
        buffers.resize(BUFFER_COUNT);
        for (auto& buf : buffers)
            buf.resize(WAVEFORM_SIZE, 0.0f);
    }

    void setWaveform(const std::vector<float>& waveform)
    {
        if (waveform.size() != WAVEFORM_SIZE)
            return;

        int writeIdx = (currentBuffer + 1) % BUFFER_COUNT;
        buffers[writeIdx] = waveform;
        nextBuffer = writeIdx;
        newDataReady = true;
    }

    void updateBuffer()
    {
        if (newDataReady)
        {
            currentBuffer = nextBuffer;
            newDataReady = false;
        }
    }

    float getSample(float phase)
    {
        float idx = phase * (WAVEFORM_SIZE - 1);
        int i0 = static_cast<int>(idx);
        int i1 = (i0 + 1) % WAVEFORM_SIZE;
        float frac = idx - i0;

        float s0 = buffers[currentBuffer][i0];
        float s1 = buffers[currentBuffer][i1];
        return s0 + frac * (s1 - s0);
    }

private:
    std::vector<std::vector<float>> buffers;
    int currentBuffer, nextBuffer;
    std::atomic<bool> newDataReady;
};


class QGServerProcess
{
public:
    QGServerProcess() : startupDelay(0)
    {
        #ifdef _WIN32
            hProcess = NULL;
        #else
            serverPid = -1;
        #endif
    }

    ~QGServerProcess()
    {
        stop();
    }

    bool start()
    {
        // Try to find Python
        const char* pythonPaths[] = {
            "python3",
            "python",
            "/usr/bin/python3",
            "/usr/bin/python",
            "/opt/homebrew/bin/python3"
        };

        juce::File scriptFile = getServerScriptPath();
        if (!scriptFile.exists())
        {
            DBG("QG server script not found at: " + scriptFile.getFullPathName());
            return false;
        }

        #ifdef _WIN32
            return tryLaunchServerWindows(scriptFile);
        #else
            for (const char* pythonCmd : pythonPaths)
            {
                if (tryLaunchServerUnix(pythonCmd, scriptFile))
                {
                    DBG("QG server started successfully");
                    startupDelay = 2000;
                    return true;
                }
            }
            DBG("Could not find Python installation");
            return false;
        #endif
    }

    void stop()
    {
        #ifdef _WIN32
            if (hProcess != NULL)
            {
                TerminateProcess(hProcess, 1);
                CloseHandle(hProcess);
                hProcess = NULL;
            }
        #else
            if (serverPid > 0)
            {
                kill(serverPid, SIGTERM);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                kill(serverPid, SIGKILL);
                serverPid = -1;
            }
        #endif
    }

    bool isRunning() const
    {
        #ifdef _WIN32
            return hProcess != NULL;
        #else
            return serverPid > 0;
        #endif
    }

    int getStartupDelay() const { return startupDelay; }

private:
    #ifdef _WIN32
        bool tryLaunchServerWindows(const juce::File& scriptFile)
        {
            std::string pythonExe = "python";
            std::string script = scriptFile.getFullPathName().toStdString();
            std::string cmdLine = pythonExe + " \"" + script + "\" --device cpu 2>nul";

            STARTUPINFOA si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            if (!CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, FALSE,
                               CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
            {
                return false;
            }

            hProcess = pi.hProcess;
            CloseHandle(pi.hThread);
            startupDelay = 2000;
            return true;
        }
    #else
        bool tryLaunchServerUnix(const char* pythonCmd, const juce::File& scriptFile)
        {
            std::string cmd = std::string(pythonCmd) + " \"" + scriptFile.getFullPathName().toStdString()
                             + "\" --device cpu 2>/dev/null &";

            int result = system(cmd.c_str());
            if (result == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                serverPid = 1;
                return true;
            }
            return false;
        }
    #endif

    juce::File getServerScriptPath()
    {
        juce::File pluginDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

        std::vector<juce::File> candidates = {
            pluginDir.getChildFile("qg_wavetable_server.py"),
            pluginDir.getParentDirectory().getChildFile("qg_wavetable_server.py"),
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("qg_wavetable_server.py"),
            #ifdef _WIN32
                juce::File("C:\\Temp\\qg_wavetable_server.py"),
            #else
                juce::File("/tmp/qg_wavetable_server.py"),
            #endif
        };

        for (const auto& candidate : candidates)
        {
            if (candidate.exists())
                return candidate;
        }

        return juce::File();
    }

    #ifdef _WIN32
        HANDLE hProcess;
    #else
        pid_t serverPid;
    #endif
    int startupDelay;
};


class QGSocketClient
{
public:
    static constexpr const char* WAVE_SOCKET_PATH = "/tmp/qg_wavetable_wave_9999.sock";
    static constexpr const char* CMD_SOCKET_PATH = "/tmp/qg_wavetable_cmd_9999.sock";

    QGSocketClient() : socketFd(-1), cmdSocketFd(-1), running(false), connected(false) {}

    ~QGSocketClient()
    {
        stop();
    }

    void start(QGWavetableBuffer* buffer)
    {
        if (running) return;
        running = true;
        connected = false;
        waveBuffer = buffer;

        readerThread = std::thread([this]() { socketReaderThread(); });
    }

    void stop()
    {
        running = false;
        if (socketFd >= 0)
        {
            #ifdef _WIN32
                closesocket(socketFd);
            #else
                close(socketFd);
            #endif
            socketFd = -1;
        }
        if (readerThread.joinable())
            readerThread.join();
        connected = false;
    }

    bool isConnected() const { return connected; }

    void sendSamplingPosition(int x, int y)
    {
        if (cmdSocketFd < 0) return;
        std::string cmd = "POS " + std::to_string(x) + " " + std::to_string(y) + "\n";
        send(cmdSocketFd, cmd.c_str(), cmd.size(), 0);
    }

    void sendCommand(const std::string& cmd)
    {
        if (cmdSocketFd < 0) return;
        std::string full_cmd = cmd + "\n";
        send(cmdSocketFd, full_cmd.c_str(), full_cmd.size(), 0);
    }

private:
    void socketReaderThread()
    {
        while (running)
        {
            if (!connectSocket())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            connected = true;
            readWaveforms();
            connected = false;
            #ifdef _WIN32
                closesocket(socketFd);
            #else
                close(socketFd);
            #endif
            socketFd = -1;
        }
    }

    bool connectSocket()
    {
        #ifdef _WIN32
            // Windows: use localhost TCP sockets
            WSADATA wsa_data;
            if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
                return false;

            socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (socketFd == INVALID_SOCKET)
                return false;

            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(9999);
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");

            if (connect(socketFd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
            {
                closesocket(socketFd);
                socketFd = INVALID_SOCKET;
                return false;
            }

            // Command socket
            cmdSocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (cmdSocketFd != INVALID_SOCKET)
            {
                struct sockaddr_in cmd_addr;
                cmd_addr.sin_family = AF_INET;
                cmd_addr.sin_port = htons(9998);
                cmd_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

                if (connect(cmdSocketFd, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr)) == SOCKET_ERROR)
                {
                    closesocket(cmdSocketFd);
                    cmdSocketFd = INVALID_SOCKET;
                }
            }

            return true;
        #else
            // Unix/Linux/macOS: use Unix sockets
            socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (socketFd < 0) return false;

            struct sockaddr_un addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, WAVE_SOCKET_PATH, sizeof(addr.sun_path) - 1);

            if (connect(socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
            {
                close(socketFd);
                socketFd = -1;
                return false;
            }

            // Also connect to command socket
            cmdSocketFd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (cmdSocketFd >= 0)
            {
                struct sockaddr_un cmd_addr;
                std::memset(&cmd_addr, 0, sizeof(cmd_addr));
                cmd_addr.sun_family = AF_UNIX;
                strncpy(cmd_addr.sun_path, CMD_SOCKET_PATH, sizeof(cmd_addr.sun_path) - 1);

                if (connect(cmdSocketFd, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr)) < 0)
                {
                    close(cmdSocketFd);
                    cmdSocketFd = -1;
                }
            }

            return true;
        #endif
    }

    void readWaveforms()
    {
        std::vector<float> waveform(QGWavetableBuffer::WAVEFORM_SIZE);
        char magic[4];

        while (running)
        {
            if (recv(socketFd, magic, 4, 0) != 4) break;
            if (std::memcmp(magic, "QGWT", 4) != 0) break;

            uint32_t size;
            char size_bytes[4];
            if (recv(socketFd, size_bytes, 4, 0) != 4) break;
            std::memcpy(&size, size_bytes, 4);

            if (size != QGWavetableBuffer::WAVEFORM_SIZE) break;

            int bytesToRead = size * sizeof(float);
            char* waveform_bytes = reinterpret_cast<char*>(waveform.data());
            if (recv(socketFd, waveform_bytes, bytesToRead, 0) != bytesToRead)
                break;

            if (waveBuffer)
                waveBuffer->setWaveform(waveform);
        }
    }

    int socketFd;
    int cmdSocketFd;
    std::atomic<bool> running;
    std::atomic<bool> connected;
    std::thread readerThread;
    QGWavetableBuffer* waveBuffer;
};
