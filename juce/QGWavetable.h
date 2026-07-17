#pragma once

#include <JuceHeader.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <thread>
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
    QGServerProcess() : serverPid(-1), startupDelay(0)
    {
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

        for (const char* pythonCmd : pythonPaths)
        {
            if (tryLaunchServer(pythonCmd, scriptFile))
            {
                DBG("QG server started successfully");
                startupDelay = 2000;  // Wait 2s for server to bind socket
                return true;
            }
        }

        DBG("Could not find Python installation");
        return false;
    }

    void stop()
    {
        if (serverPid > 0)
        {
            kill(serverPid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            kill(serverPid, SIGKILL);
            serverPid = -1;
        }
    }

    bool isRunning() const { return serverPid > 0; }
    int getStartupDelay() const { return startupDelay; }

private:
    bool tryLaunchServer(const char* pythonCmd, const juce::File& scriptFile)
    {
        std::string cmd = std::string(pythonCmd) + " \"" + scriptFile.getFullPathName().toStdString()
                         + "\" --device cpu 2>/dev/null &";

        int result = system(cmd.c_str());
        if (result == 0)
        {
            // Crude: just check if process likely started
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            serverPid = 1;  // Placeholder (proper way would use posix_spawn)
            return true;
        }
        return false;
    }

    juce::File getServerScriptPath()
    {
        // Try relative to plugin bundle / app resources
        juce::File pluginDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

        // Try several common locations
        std::vector<juce::File> candidates = {
            pluginDir.getChildFile("qg_wavetable_server.py"),
            pluginDir.getParentDirectory().getChildFile("qg_wavetable_server.py"),
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("qg_wavetable_server.py"),
            juce::File("/tmp/qg_wavetable_server.py"),
        };

        for (const auto& candidate : candidates)
        {
            if (candidate.exists())
                return candidate;
        }

        return juce::File();
    }

    pid_t serverPid;
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
            close(socketFd);
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
            close(socketFd);
            socketFd = -1;
        }
    }

    bool connectSocket()
    {
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
            if (recv(socketFd, &size, 4, 0) != 4) break;
            if (size != QGWavetableBuffer::WAVEFORM_SIZE) break;

            int bytesToRead = size * sizeof(float);
            if (recv(socketFd, waveform.data(), bytesToRead, 0) != bytesToRead)
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
