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
    explicit MPVController(const std::string& socketPath = "/tmp/mpvsocket")
        : socketPath(socketPath) {}

    void SendCommand(const std::vector<std::string>& cmd) {
        Json::Value root;
        root["command"] = Json::arrayValue;
        for (const auto& arg : cmd) {
            root["command"].append(arg);
        }
        
        SendRaw(root.toStyledString());
    }

    void Seek(int seconds) {
        SendCommand({"seek", std::to_string(seconds)});
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
    std::string socketPath;

    void SendRaw(const std::string& data) {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) throw std::runtime_error("Socket creation failed");

        struct sockaddr_un addr = {};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path)-1);

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
        int sock = CreateSocket();
        SendRaw(Json::Value(cmd).toStyledString(), sock);
        
        char buffer[4096];
        ssize_t count = read(sock, buffer, sizeof(buffer)-1);
        close(sock);
        
        if (count > 0) {
            buffer[count] = '\0';
            Json::Value root;
            JSONCPP_STRING err;
            Json::CharReaderBuilder builder;
            if (Json::parseFromStream(builder, buffer, &root, &err)) {
                return root;
            }
        }
        return Json::Value();
    }
};
} 