#include "SoundClip.hpp"

#include "GameCommon.hpp"
#include "Game.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/Vec3.hpp"

SoundClip::SoundClip(const std::string& path, float volume /*= 1.0f*/, float pitch /*= 1.0f*/, float speed /*= 1.0f*/, bool paused /*= false*/)
	: m_soundId(g_theAudio->CreateOrGetSound(path))
	, m_volume(volume)
	, m_pitch(pitch)
	, m_speed(speed)
	, m_startPaused(paused)
{
}

SoundClip::~SoundClip()
{
}

SoundInst SoundClip::PlaySound(float volume /*= 1.0f*/, float balance /*= 0.0f*/) const
{
	return g_theAudio->StartSound(m_soundId, false, g_theGame->m_soundVolume * m_volume * volume, balance, m_pitch, m_speed, m_startPaused);
}

SoundInst SoundClip::PlayMusic(float volume /*= 1.0f*/, float balance /*= 0.0f*/) const
{
	return g_theAudio->StartSound(m_soundId, true, g_theGame->m_soundVolume * m_volume * volume, balance, m_pitch, m_speed, m_startPaused);
}

SoundInst SoundClip::PlaySoundAt(const Vec3& position, float volume /*= 1.0f*/, float balance /*= 0.0f*/) const
{
	return g_theAudio->StartSoundAt(m_soundId, position, false, g_theGame->m_soundVolume * m_volume * volume, balance, m_pitch, m_speed, m_startPaused);
}

SoundInst SoundClip::PlayMusicAt(const Vec3& position, float volume /*= 1.0f*/, float balance /*= 0.0f*/) const
{
	return g_theAudio->StartSoundAt(m_soundId, position, true, g_theGame->m_soundVolume * m_volume * volume, balance, m_pitch, m_speed, m_startPaused);
}

SoundInst::SoundInst(SoundPlaybackID playbackId)
	: m_playbackId(playbackId)
{
}

SoundInst::SoundInst(const SoundInst& copyFrom)
	: m_playbackId(copyFrom.m_playbackId)
{
}

void SoundInst::Stop()
{
	if (m_playbackId != MISSING_SOUND_ID)
		g_theAudio->StopSound(m_playbackId);
}

void SoundInst::SetSpeed(float speed)
{
	if (m_playbackId != MISSING_SOUND_ID)
		g_theAudio->SetSoundPlaybackSpeed(m_playbackId, speed);
}

bool SoundCollection::LoadFromXmlElement(const XmlElement& element)
{
	for (const XmlElement* sound = element.FirstChildElement("Sound"); sound; sound = sound->NextSiblingElement("Sound"))
	{
		std::string soundName = ParseXmlAttribute(*sound, "sound", "");
		std::string soundPath = ParseXmlAttribute(*sound, "name", "");
		m_sounds.emplace_back(soundName, SoundClip(soundPath));
	}

	return true;
}

SoundClip* SoundCollection::FindSound(const char* name)
{
	for (auto& pair : m_sounds)
	{
		if (_stricmp(pair.first.c_str(), name) == 0) {
			return &pair.second;
		}
	}
	return nullptr;
}

const SoundClip* SoundCollection::FindSound(const char* name) const
{
	for (auto& pair : m_sounds)
	{
		if (_stricmp(pair.first.c_str(), name) == 0) {
			return &pair.second;
		}
	}
	return nullptr;
}

bool SoundCollection::IsEmpty() const
{
	return m_sounds.empty();
}
