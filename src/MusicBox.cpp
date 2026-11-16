#include "MusicBox.h"

#include <iostream>

// Backend: Windows MCI, or a printing stub on other systems
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

namespace
{
	const char* ALIAS_MUSIC = "snowglobe_music";
	const char* ALIAS_WINDING = "snowglobe_winding";

	// Sends one MCI command. Returns false (and prints the reason) on error.
	bool Mci(const std::string& command, std::string* result = nullptr, bool quiet = false)
	{
		char reply[128] = "";
		MCIERROR error = mciSendStringA(command.c_str(), reply, sizeof(reply), nullptr);
		if (error)
		{
			if (!quiet)
			{
				char text[256] = "";
				mciGetErrorStringA(error, text, sizeof(text));
				std::cerr << "MusicBox: \"" << command << "\" failed: " << text << std::endl;
			}
			return false;
		}
		if (result) *result = reply;
		return true;
	}

	bool Open(const char* alias, const char* path, int volume)
	{
		char full[MAX_PATH];
		DWORD n = GetFullPathNameA(path, MAX_PATH, full, nullptr);
		if (n == 0 || n >= MAX_PATH) return false;

		if (!Mci(std::string("open \"") + full + "\" type mpegvideo alias " + alias)) return false;
		Mci(std::string("setaudio ") + alias + " volume to " + std::to_string(volume), nullptr, true);
		return true;
	}

	bool IsPlaying(const char* alias)
	{
		std::string mode;
		return Mci(std::string("status ") + alias + " mode", &mode, true) && mode == "playing";
	}

	void PlayFromStart(const char* alias)
	{
		Mci(std::string("play ") + alias + " from 0", nullptr, true);
	}

	void CloseAll()
	{
		Mci(std::string("close ") + ALIAS_MUSIC, nullptr, true);
		Mci(std::string("close ") + ALIAS_WINDING, nullptr, true);
	}
}

#else   // not Windows: no sound, just report what would happen

namespace
{
	const char* ALIAS_MUSIC = "music";
	const char* ALIAS_WINDING = "winding";

	bool Open(const char* alias, const char* path, int)
	{
		std::cout << "[audio stub] open " << alias << " (" << path << ")" << std::endl;
		return true;
	}
	bool IsPlaying(const char*) { return false; }
	void PlayFromStart(const char* alias) { std::cout << "[audio stub] play " << alias << std::endl; }
	void CloseAll() { std::cout << "[audio stub] close" << std::endl; }
}

#endif

namespace
{
	const int VOLUME_MUSIC = 600;      // 0..1000
	const int VOLUME_WINDING = 900;
	const bool LOOP_MUSIC = false;     // true = the tune repeats by itself
	const float LOOP_CHECK_SECONDS = 0.5f;
}

void MusicBox::Init(const char* musicPath, const char* windingPath)
{
	musicLoaded = Open(ALIAS_MUSIC, musicPath, VOLUME_MUSIC);
	windingLoaded = Open(ALIAS_WINDING, windingPath, VOLUME_WINDING);

	if (!musicLoaded)   std::cerr << "MusicBox: could not open " << musicPath << std::endl;
	if (!windingLoaded) std::cerr << "MusicBox: could not open " << windingPath << std::endl;
}

void MusicBox::Update(int shakeCount, float dt)
{
	// New shake(s) since the last frame: play the key sound unless it is still playing
	if (shakeCount > seenShakes)
	{
		seenShakes = shakeCount;
		if (windingLoaded && !IsPlaying(ALIAS_WINDING))
			PlayFromStart(ALIAS_WINDING);

		// Wound again after the tune has ended: play it again
		if (musicOn && !LOOP_MUSIC && musicLoaded && !IsPlaying(ALIAS_MUSIC))
			PlayFromStart(ALIAS_MUSIC);
	}

	// Third shake: start the music box
	if (!musicOn && seenShakes >= SHAKES_TO_START)
	{
		musicOn = true;
		if (musicLoaded)
		{
			PlayFromStart(ALIAS_MUSIC);
			std::cout << "MusicBox: music started" << std::endl;
		}
	}

	// Optional loop: when the tune has ended, start it again
	if (LOOP_MUSIC && musicOn && musicLoaded)
	{
		loopTimer += dt;
		if (loopTimer >= LOOP_CHECK_SECONDS)
		{
			loopTimer = 0.0f;
			if (!IsPlaying(ALIAS_MUSIC))
				PlayFromStart(ALIAS_MUSIC);
		}
	}
}

void MusicBox::Shutdown()
{
	CloseAll();
	musicLoaded = windingLoaded = musicOn = false;
}
