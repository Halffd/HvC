#pragma once
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <json/json.h>

namespace H {

class MPVController {
public:
    MPVController() = default;
    ~MPVController() = default;
    
    bool Initialize();
    void Shutdown();
    
    // Basic controls
    void Play();
    void Pause();
    void TogglePause();
    void Stop();
    void Next();
    void Previous();
    
    // Volume control
    void SetVolume(int volume);
    int GetVolume() const;
    void ToggleMute();
    
    // Playback control
    void Seek(int seconds);
    void SetPosition(double position);
    double GetPosition() const;
    double GetDuration() const;
    
    // File operations
    bool LoadFile(const std::string& path);
    std::string GetCurrentFile() const;

    void SendCommand(const std::vector<std::string>& cmd) {
        Json::Value root;
        root["command"] = Json::arrayValue;
        for (const auto& arg : cmd) {
            root["command"].append(arg);
        }
        
        SendRaw(root.toStyledString());
    }

    void ToggleSubtitleVisibility() {
        SendCommand({"cycle", "sub-visibility"});
    }

    void CopySubtitle() {
        SendCommand({"script-binding", "copy_current_subtitle"});
    }

    void AdjustSubtitleTiming(double seconds) {
        SendCommand({"add", "sub-delay", std::to_string(seconds)});
    }

    void CycleSubtitles() {
        SendCommand({"cycle", "sub"});
    }

    void Screenshot() {
        SendCommand({"screenshot", "video"});
    }

    void ShowProgress() {
        auto response = SendCommandWithResponse({"get_property", "percent-pos"});
        if (response.isMember("data")) {
            double progress = response["data"].asDouble();
            Notifier::Show("Progress", std::to_string(progress) + "%");
        }
    }

    void SpeedControl(double multiplier) {
        SendCommand({"set", "speed", std::to_string(multiplier)});
    }

    void AudioDelay(double seconds) {
        SendCommand({"add", "audio-delay", std::to_string(seconds)});
    }

    void CycleAudioTracks() {
        SendCommand({"cycle", "aid"});
    }

    void FrameStep() {
        SendCommand({"frame-step"});
    }

    void FrameBackStep() {
        SendCommand({"frame-back-step"});
    }

private:
    bool initialized = false;
    int volume = 100;
    bool muted = false;
    double position = 0.0;
    double duration = 0.0;
    std::string currentFile;

    void SendRaw(const std::string& data) {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) throw std::runtime_error("Socket creation failed");

        struct sockaddr_un addr = {};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "/tmp/mpvsocket", sizeof(addr.sun_path)-1);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            throw std::runtime_error("Connection to MPV failed");
        }

        if (write(sock, data.c_str(), data.size()) < 0) {
            close(sock);
            throw std::runtime_error("Write to MPV socket failed");
        }

        close(sock);
    }

    Json::Value SendCommandWithResponse(const std::vector<std::string>& cmd) {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) throw std::runtime_error("Socket creation failed");

        struct sockaddr_un addr = {};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "/tmp/mpvsocket", sizeof(addr.sun_path)-1);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            throw std::runtime_error("Connection to MPV failed");
        }

        Json::Value root;
        root["command"] = Json::arrayValue;
        for (const auto& arg : cmd) {
            root["command"].append(arg);
        }
        
        if (write(sock, root.toStyledString().c_str(), root.toStyledString().size()) < 0) {
            close(sock);
            throw std::runtime_error("Write to MPV socket failed");
        }

        char buffer[4096];
        ssize_t count = read(sock, buffer, sizeof(buffer)-1);
        close(sock);
        
        if (count > 0) {
            buffer[count] = '\0';
            Json::Value response;
            JSONCPP_STRING err;
            Json::CharReaderBuilder builder;
            if (Json::parseFromStream(builder, buffer, &response, &err)) {
                return response;
            }
        }
        return Json::Value();
    }
};
} 