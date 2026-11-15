#ifndef MUSIC_BOX_H
#define MUSIC_BOX_H

#include <string>

// Music box sound
class MusicBox
{
public:
	static const int SHAKES_TO_START = 3;

	void Init(const char* musicPath, const char* windingPath);
	void Update(int shakeCount, float dt);
	void Shutdown();

	bool MusicStarted() const { return musicOn; }

private:
	bool musicLoaded = false;
	bool windingLoaded = false;
	bool musicOn = false;
	int seenShakes = 0;
	float loopTimer = 0.0f;
};

#endif
