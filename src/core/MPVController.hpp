#pragma once
#include <string>
#include <vector>
#include <functional>

namespace H {

class MPVController {
public:
    MPVController();
    ~MPVController();
    
    bool Initialize();
    void Shutdown();
    
    // Basic controls
    void Play();
    void Pause();
    void TogglePause();
    void PlayPause();
    void Stop();
    void Next();
    void Previous();
    
    // Volume control
    void SetVolume(int volume);
    int GetVolume() const;
    void VolumeUp();
    void VolumeDown();
    void ToggleMute();
    
    // Playback control
    void Seek(int seconds);
    void SetPosition(double position);
    double GetPosition() const;
    double GetDuration() const;
    
    // File operations
    bool LoadFile(const std::string& path);
    std::string GetCurrentFile() const;
    
    void SendCommand(const std::vector<std::string>& cmd);
    void ToggleSubtitleVisibility();
    
    bool EnsureInitialized();
    
private:
    bool initialized = false;
    int volume = 100;
    bool muted = false;
    double position = 0.0;
    double duration = 0.0;
    std::string currentFile;
    
    void SendRaw(const std::string& data);
};

} // namespace H 