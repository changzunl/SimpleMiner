#pragma once

#include "Engine/Audio/AudioSystem.hpp"
#include <string>
#include <vector>

namespace tinyxml2
{
	class XMLElement;
}
typedef tinyxml2::XMLElement XmlElement;

struct Vec3;
class SoundClip;
class SoundInst;

class SoundInst
{
	friend class SoundClip;

public:
	SoundInst() {};
	SoundInst(const SoundInst& copyFrom);
	SoundInst(SoundPlaybackID playbackId);
	~SoundInst() {};

	void Stop();
	void SetSpeed(float speed);

public:
	SoundPlaybackID m_playbackId = MISSING_SOUND_ID;
};

class SoundClip
{
public:
	SoundClip(const std::string& path, float volume = 1.0f, float pitch = 1.0f, float speed = 1.0f, bool paused = false);
	~SoundClip();
	SoundInst PlaySound(float volume = 1.0f, float balance = 0.0f) const;
	SoundInst PlayMusic(float volume = 1.0f, float balance = 0.0f) const;
	SoundInst PlaySoundAt(const Vec3& position, float volume = 1.0f, float balance = 0.0f) const;
	SoundInst PlayMusicAt(const Vec3& position, float volume = 1.0f, float balance = 0.0f) const;

public:
	const SoundID  m_soundId        = 0;
	float          m_volume         = 1.0f;
	float          m_pitch          = 1.0f;
	float          m_speed          = 1.0f;
	bool           m_startPaused    = false;
};



class SoundCollection
{
public:
	bool LoadFromXmlElement(const XmlElement& element);
	SoundClip* FindSound(const char* name);
	const SoundClip* FindSound(const char* name) const;
	bool IsEmpty() const;

private:
	std::vector<std::pair<std::string, SoundClip>> m_sounds;
};

