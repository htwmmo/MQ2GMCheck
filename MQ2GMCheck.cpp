// MQ2GMCheck.cpp : Defines the entry point for the DLL application.
//
// Check to see if a GM is in the zone. This is not fool proof. It is absolutely
// true that a GM could be right in front of you and you'd never know it. This
// plugin will simply find those who are in the zone and not gm-invis, or who
// just came into the zone and were not gm-invised at the time. If a GM comes
// into the zone already gm-invised, we will not know about that.
//
//
// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.
//

// TODO:  Sound settings are loaded by character but saved globally (in the /gmcheck save command and write settings)
//        need separate settings to handle this (an interim fix might be to just track when it was loaded by char)

#include <mq/Plugin.h>
#include <vector>
#include <mmsystem.h>

PreSetup("MQ2GMCheck");
PLUGIN_VERSION(5.10);

constexpr const char* PluginMsg = "\ay[\aoMQ2GMCheck\ax] ";

char szLastGMName[MAX_STRING] = { 0 };
char szLastGMTime[MAX_STRING] = { 0 };
char szLastGMDate[MAX_STRING] = { 0 };
char szLastGMZone[MAX_STRING] = { 0 };

uint32_t bmMQ2GMCheck = 0;
uint64_t StopSoundTimer = 0;

DWORD dwVolume;
DWORD NewVol;

bool bGMCmdActive = false;
bool bVolSet = false;

std::vector<std::string> GMNames;

//----------------------------------------------------------------------------
// this class holds persisted settings for this plugin.
class Settings
{
public:
	static constexpr inline bool default_GMCheckEnabled = true;
	static constexpr inline bool default_GMQuietEnabled = false;
	static constexpr inline bool default_GMSoundEnabled = true;
	static constexpr inline bool default_GMBeepEnabled = false;
	static constexpr inline bool default_GMPopupEnabled = false;
	static constexpr inline bool default_GMCorpseEnabled = false;
	static constexpr inline bool default_GMChatAlertEnabled = false;
	static constexpr inline int default_ReminderInterval = 30;

	std::string szGMEnterCmd = std::string();
	std::string szGMEnterCmdIf = std::string();
	std::string szGMLeaveCmd = std::string();
	std::string szGMLeaveCmdIf = std::string();
	std::filesystem::path Sound_GMEnter = std::filesystem::path(gPathResources) / "Sounds/gmenter.mp3";
	std::filesystem::path Sound_GMLeave = std::filesystem::path(gPathResources) / "Sounds/gmleave.mp3";
	std::filesystem::path Sound_GMRemind = std::filesystem::path(gPathResources) / "Sounds/gmremind.mp3";

	inline bool IsGMCheckEnabled() const { return m_GMCheckEnabled; }
	void SetGMCheckEnabled(bool toggle = true, bool enabled = false, bool quiet = true);

	inline bool IsGMQuietEnabled() const { return m_GMQuietEnabled; }
	void SetGMQuietEnabled(bool toggle = true, bool enabled = false, bool quiet = true);

	inline bool IsGMSoundEnabled() const { return m_GMSoundEnabled; }
	void SetGMSoundEnabled(bool toggle = true, bool enabled = false, bool quiet = true);

	inline bool IsGMBeepEnabled() const { return m_GMBeepEnabled; }
	void SetGMBeepEnabled(bool toggle = true, bool enabled = false, bool quiet = true);

	inline bool IsGMPopupEnabled() const { return m_GMPopupEnabled; }
	void SetGMPopupEnabled(bool toggle = true, bool enabled = false, bool quiet = true);

	inline bool IsGMCorpseEnabled() const { return m_GMCorpseEnabled; }
	void SetGMCorpseEnabled(bool toggle = true, bool enabled = false, bool quiet = true);

	inline bool IsGMChatAlertEnabled() const { return m_GMChatAlertEnabled; }
	void SetGMChatAlertEnabled(bool toggle = true, bool enabled = false, bool quiet = true);

	inline int GetReminderInterval() const { return m_ReminderInterval; }
	void SetReminderInterval(int reminderinterval);
	void Load();
	void Reset();

	[[nodiscard]] std::filesystem::path SearchSoundPaths(std::filesystem::path file_path);
	[[nodiscard]] std::filesystem::path GetBestSoundFile(const std::filesystem::path& file_path, bool try_alternate_extension = true);
	void SetGMSoundFile(const char* friendly_name, std::filesystem::path* global_path);
	void SetAllGMSoundFiles();

private:
	bool m_GMCheckEnabled = default_GMCheckEnabled;
	bool m_GMQuietEnabled = default_GMQuietEnabled;
	bool m_GMSoundEnabled = default_GMSoundEnabled;
	bool m_GMBeepEnabled = default_GMBeepEnabled;
	bool m_GMPopupEnabled = default_GMPopupEnabled;
	bool m_GMCorpseEnabled = default_GMCorpseEnabled;
	bool m_GMChatAlertEnabled = default_GMChatAlertEnabled;
	int m_ReminderInterval = default_ReminderInterval;
};
Settings s_settings;

void Settings::Load()
{
	m_GMQuietEnabled = false;
	m_GMCheckEnabled = GetPrivateProfileBool("Settings", "GMCheck", default_GMCheckEnabled, INIFileName);
	m_GMSoundEnabled = GetPrivateProfileBool("Settings", "GMSound", default_GMSoundEnabled, INIFileName);
	m_GMBeepEnabled = GetPrivateProfileBool("Settings", "GMBeep", default_GMBeepEnabled, INIFileName);
	m_GMPopupEnabled = GetPrivateProfileBool("Settings", "GMPopup", default_GMPopupEnabled, INIFileName);
	m_GMCorpseEnabled = GetPrivateProfileBool("Settings", "GMCorpse", default_GMCorpseEnabled, INIFileName);
	m_GMChatAlertEnabled = GetPrivateProfileBool("Settings", "GMChat", default_GMChatAlertEnabled, INIFileName);
	m_ReminderInterval = GetPrivateProfileInt("Settings", "RemInt", default_ReminderInterval, INIFileName);
	if (m_ReminderInterval < 10 && m_ReminderInterval)
		m_ReminderInterval = 10;
	SetAllGMSoundFiles();
	szGMEnterCmd = GetPrivateProfileString("Settings", "GMEnterCmd", std::string(), INIFileName);
	szGMEnterCmdIf = GetPrivateProfileString("Settings", "GMEnterCmdIf", std::string(), INIFileName);
	szGMLeaveCmd = GetPrivateProfileString("Settings", "GMLeaveCmd", std::string(), INIFileName);
	szGMLeaveCmdIf = GetPrivateProfileString("Settings", "GMLeaveCmdIf", std::string(), INIFileName);
}

void Settings::Reset()
{
	SetGMCheckEnabled(default_GMCheckEnabled);
	SetGMSoundEnabled(default_GMSoundEnabled);
	SetGMBeepEnabled(default_GMBeepEnabled);
	SetGMPopupEnabled(default_GMPopupEnabled);
	SetGMCorpseEnabled(default_GMCorpseEnabled);
	SetGMChatAlertEnabled(default_GMChatAlertEnabled);
	m_GMQuietEnabled = default_GMQuietEnabled;
	szGMEnterCmd = "";
	szGMEnterCmdIf = "";
	szGMLeaveCmd = "";
	szGMLeaveCmdIf = "";
	m_ReminderInterval = default_ReminderInterval;
	std::filesystem::path Sound_GMEnter = std::filesystem::path(gPathResources) / "Sounds/gmenter.mp3";
	std::filesystem::path Sound_GMLeave = std::filesystem::path(gPathResources) / "Sounds/gmleave.mp3";
	std::filesystem::path Sound_GMRemind = std::filesystem::path(gPathResources) / "Sounds/gmremind.mp3";
}

void Settings::SetGMCheckEnabled(bool toggle, bool enabled, bool quiet)
{
	if (toggle)
		m_GMCheckEnabled = !m_GMCheckEnabled;
	else
	{
		if (enabled == m_GMCheckEnabled)
			return;
		m_GMCheckEnabled = enabled;
	}
	WritePrivateProfileBool("Settings", "GMCheck", m_GMCheckEnabled, INIFileName);
	if (!quiet)
		WriteChatf("%s\amGM checking is now %s\am.", PluginMsg, m_GMCheckEnabled ? "\agENABLED" : "\arDISABLED");
}

void Settings::SetGMQuietEnabled(bool toggle, bool enabled, bool quiet)
{
	if (toggle)
		m_GMQuietEnabled = !m_GMQuietEnabled;
	else
		m_GMQuietEnabled = enabled;
	if (!quiet)
		WriteChatf("%s\amGM alert and reminder sounds %s\am.", PluginMsg, m_GMQuietEnabled ? "temporarily \arDISABLED" : "\agENABLED");
}

void Settings::SetGMSoundEnabled(bool toggle, bool enabled, bool quiet)
{
	if (toggle)
		m_GMSoundEnabled = !m_GMSoundEnabled;
	else
	{
		if (enabled == m_GMSoundEnabled)
			return;
		m_GMSoundEnabled = enabled;
	}
	WritePrivateProfileBool("Settings", "GMSound", m_GMSoundEnabled, INIFileName);
	if (!quiet)
		WriteChatf("%s\amSound playing on GM detection is now %s\am.", PluginMsg, m_GMSoundEnabled ? "\agENABLED" : "\arDISABLED");
}

void Settings::SetGMBeepEnabled(bool toggle, bool enabled, bool quiet)
{
	if (toggle)
		m_GMBeepEnabled = !m_GMBeepEnabled;
	else
	{
		if (enabled == m_GMBeepEnabled)
			return;
		m_GMBeepEnabled = enabled;
	}
	WritePrivateProfileBool("Settings", "GMBeep", m_GMBeepEnabled, INIFileName);
	if (!quiet)
		WriteChatf("%s\amBeeping on GM detection is now %s\am.", PluginMsg, m_GMBeepEnabled ? "\agENABLED" : "\arDISABLED");
}

void Settings::SetGMPopupEnabled(bool toggle, bool enabled, bool quiet)
{
	if (toggle)
		m_GMPopupEnabled = !m_GMPopupEnabled;
	else
	{
		if (enabled == m_GMPopupEnabled)
			return;
		m_GMPopupEnabled = enabled;
	}
	WritePrivateProfileBool("Settings", "GMPopup", m_GMPopupEnabled, INIFileName);
	if (!quiet)
		WriteChatf("%s\amShowing popup message on GM detection is now %s\am.", PluginMsg, m_GMPopupEnabled ? "\agENABLED" : "\arDISABLED");
}

void Settings::SetGMCorpseEnabled(bool toggle, bool enabled, bool quiet)
{
	if (toggle)
		m_GMCorpseEnabled = !m_GMCorpseEnabled;
	else
	{
		if (enabled == m_GMCorpseEnabled)
			return;
		m_GMCorpseEnabled = enabled;
	}
	WritePrivateProfileBool("Settings", "GMCorpse", m_GMCorpseEnabled, INIFileName);
	if (!quiet)
		WriteChatf("%s\amCorpse exclusion from GM alerts is now %s\am.", PluginMsg, !m_GMCorpseEnabled ? "\agENABLED" : "\arDISABLED");
}

void Settings::SetGMChatAlertEnabled(bool toggle, bool enabled, bool quiet)
{
	if (toggle)
		m_GMChatAlertEnabled = !m_GMChatAlertEnabled;
	else
	{
		if (enabled == m_GMChatAlertEnabled)
			return;
		m_GMChatAlertEnabled = enabled;
	}
	WritePrivateProfileBool("Settings", "GMChat", m_GMChatAlertEnabled, INIFileName);
	if (!quiet)
		WriteChatf("%s\amDisplaying GM detection alerts in MQ chat window is now %s\am.", PluginMsg, m_GMChatAlertEnabled ? "\agENABLED" : "\arDISABLED");
}

void Settings::SetReminderInterval(int ReminderInterval)
{
	if (ReminderInterval == m_ReminderInterval)
		return;

	m_ReminderInterval = ReminderInterval;
	WritePrivateProfileInt("Settings", "RemInt", m_ReminderInterval, INIFileName);
}

[[nodiscard]] std::filesystem::path Settings::SearchSoundPaths(std::filesystem::path file_path)
{
	std::error_code ec;
	const std::filesystem::path resources_path = gPathResources;

	// If they gave an absolute path, no sense checking other locations
	if (file_path.is_relative())
	{
		// Try relative to the Sounds directory first
		if (exists(resources_path / "Sounds" / file_path, ec))
		{
			file_path = resources_path / "Sounds" / file_path;
		}
		// Then relative to the resources directory
		else if (exists(resources_path / file_path, ec))
		{
			file_path = resources_path / file_path;
		}
	}

	return file_path;
}

[[nodiscard]] std::filesystem::path Settings::GetBestSoundFile(const std::filesystem::path& file_path, bool try_alternate_extension)
{
	std::error_code ec;
	std::filesystem::path return_path = file_path;
	// Only need to worry about it if it doesn't exist (could also not be a file, but that's bad input)
	if (!file_path.empty() && !exists(return_path, ec))
	{
		// If there is no extension, assume mp3
		if (!return_path.has_extension())
		{
			return_path.replace_extension("mp3");
		}

		std::filesystem::path tmp = SearchSoundPaths(return_path);
		if (exists(tmp, ec))
		{
			return_path = tmp;
		}
		else
		{
			tmp = SearchSoundPaths(return_path.filename());

			if (exists(tmp, ec))
			{
				return_path = tmp;
			}
			else if (try_alternate_extension)
			{
				tmp = return_path;
				if (tmp.extension() == ".mp3")
				{
					tmp = GetBestSoundFile(tmp.replace_extension("wav"), false);
				}
				else
				{
					tmp = GetBestSoundFile(tmp.replace_extension("mp3"), false);
				}

				if (exists(tmp, ec))
				{
					return_path = tmp;
				}
			}
		}
	}

	if (return_path != file_path)
	{
		WriteChatf("%s\atWARNING - Sound file could not be found. Replacing \"\ay%s\ax\" with \"\ay%s\ax\"", PluginMsg, file_path.string().c_str(), return_path.string().c_str());
	}

	return return_path;
}

void Settings::SetGMSoundFile(const char* friendly_name, std::filesystem::path* global_path)
{
	std::error_code ec;
	std::filesystem::path tmp;
	if (pLocalPC && PrivateProfileKeyExists(pLocalPC->Name, friendly_name, INIFileName))
	{
		tmp = GetBestSoundFile(GetPrivateProfileString(pLocalPC->Name, friendly_name, (*global_path).string(), INIFileName));
		if (!exists(tmp, ec))
		{
			WriteChatf("%s\atWARNING - GM '%s' file not found for %s (Global Setting will be used instead): \am%s", PluginMsg, friendly_name, pLocalPC->Name, tmp.string().c_str());
		}
	}

	if (tmp.empty() || !exists(tmp, ec))
	{
		tmp = GetBestSoundFile(GetPrivateProfileString("Settings", friendly_name, (*global_path).string(), INIFileName));
	}

	if (!exists(tmp, ec))
	{
		WriteChatf("%s\atWARNING - GM '%s' file not found: \am%s", PluginMsg, friendly_name, tmp.string().c_str());
	}
	else
	{
		*global_path = tmp;
	}
}

void Settings::SetAllGMSoundFiles()
{
	SetGMSoundFile("EnterSound", &Sound_GMEnter);
	SetGMSoundFile("LeaveSound", &Sound_GMLeave);
	SetGMSoundFile("RemindSound", &Sound_GMRemind);
}

enum HistoryType {
	eHistory_Zone,
	eHistory_Server,
	eHistory_All
};

int MCEval(const char* zBuffer)
{
	char zOutput[MAX_STRING] = { 0 };
	if (zBuffer[0] == '\0')
		return 1;

	strcpy_s(zOutput, zBuffer);
	ParseMacroData(zOutput, MAX_STRING);
	return GetIntFromString(zOutput, 0);
}

class MQ2GMCheckType* pGMCheckType = nullptr;

class MQ2GMCheckType : public MQ2Type
{
public:
	enum class GMCheckMembers
	{
		Status = 1,
		GM,
		Names,
		Sound,
		Beep,
		Popup,
		Corpse,
		Quiet,
		Interval,
		Enter,
		Leave,
		Remind,
		LastGMName,
		LastGMTime,
		LastGMDate,
		LastGMZone,
		GMEnterCmd,
		GMEnterCmdIf,
		GMLeaveCmd,
		GMLeaveCmdIf,
	};

	MQ2GMCheckType() :MQ2Type("GMCheck")
	{
		ScopedTypeMember(GMCheckMembers, Status);
		ScopedTypeMember(GMCheckMembers, GM);
		ScopedTypeMember(GMCheckMembers, Names);
		ScopedTypeMember(GMCheckMembers, Sound);
		ScopedTypeMember(GMCheckMembers, Beep);
		ScopedTypeMember(GMCheckMembers, Popup);
		ScopedTypeMember(GMCheckMembers, Corpse);
		ScopedTypeMember(GMCheckMembers, Quiet);
		ScopedTypeMember(GMCheckMembers, Interval);
		ScopedTypeMember(GMCheckMembers, Enter);
		ScopedTypeMember(GMCheckMembers, Leave);
		ScopedTypeMember(GMCheckMembers, Remind);
		ScopedTypeMember(GMCheckMembers, LastGMName);
		ScopedTypeMember(GMCheckMembers, LastGMTime);
		ScopedTypeMember(GMCheckMembers, LastGMDate);
		ScopedTypeMember(GMCheckMembers, LastGMZone);
		ScopedTypeMember(GMCheckMembers, GMEnterCmd);
		ScopedTypeMember(GMCheckMembers, GMEnterCmdIf);
		ScopedTypeMember(GMCheckMembers, GMLeaveCmd);
		ScopedTypeMember(GMCheckMembers, GMLeaveCmdIf);
	}

	virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override
	{
		using namespace mq::datatypes;
		MQTypeMember* pMember = MQ2GMCheckType::FindMember(Member);

		if (!pMember)
			return false;

		switch ((GMCheckMembers)pMember->ID)
		{
		case GMCheckMembers::Status:
			Dest.DWord = s_settings.IsGMCheckEnabled();
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::GM:
			Dest.DWord = !GMNames.empty();
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Names:
		{
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;

			if (GMNames.empty())
				return false;

			const std::string joined_names = join(GMNames, ", ");
			strcpy_s(DataTypeTemp, joined_names.c_str());
			return true;
		}

		case GMCheckMembers::Sound:
			Dest.DWord = s_settings.IsGMSoundEnabled();
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Beep:
			Dest.DWord = s_settings.IsGMBeepEnabled();
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Popup:
			Dest.DWord = s_settings.IsGMPopupEnabled();
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Corpse:
			Dest.DWord = s_settings.IsGMCorpseEnabled();
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Quiet:
			Dest.DWord = s_settings.IsGMQuietEnabled();
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Interval:
			Dest.Int = s_settings.GetReminderInterval();
			Dest.Type = pIntType;
			return true;

		case GMCheckMembers::Enter:
			strcpy_s(DataTypeTemp, s_settings.Sound_GMEnter.string().c_str());
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::Leave:
			strcpy_s(DataTypeTemp, s_settings.Sound_GMLeave.string().c_str());
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::Remind:
			strcpy_s(DataTypeTemp, s_settings.Sound_GMRemind.string().c_str());
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::LastGMName:
			strcpy_s(DataTypeTemp, szLastGMName);
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::LastGMTime:
			strcpy_s(DataTypeTemp, szLastGMTime);
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::LastGMDate:
			strcpy_s(DataTypeTemp, szLastGMDate);
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::LastGMZone:
			strcpy_s(DataTypeTemp, szLastGMZone);
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::GMEnterCmd:
			strcpy_s(DataTypeTemp, s_settings.szGMEnterCmd.c_str());
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::GMEnterCmdIf:
			if (MCEval(s_settings.szGMEnterCmdIf.c_str()))
				strcpy_s(DataTypeTemp, "TRUE");
			else
				strcpy_s(DataTypeTemp, "FALSE");

			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::GMLeaveCmd:
			strcpy_s(DataTypeTemp, s_settings.szGMLeaveCmd.c_str());
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::GMLeaveCmdIf:
			if (MCEval(s_settings.szGMLeaveCmdIf.c_str()))
				strcpy_s(DataTypeTemp, "TRUE");
			else
				strcpy_s(DataTypeTemp, "FALSE");

			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;
		}

		return false;
	}

	virtual bool ToString(MQVarPtr VarPtr, char* Destination) override
	{
		strcpy_s(Destination, MAX_STRING, GMNames.empty() ? "FALSE" : "TRUE");
		return true;
	}

	static bool dataGMCheck(const char* szName, MQTypeVar& Ret)
	{
		Ret.DWord = 1;
		Ret.Type = pGMCheckType;
		return true;
	}
};

void GMCheckStatus(bool MentionHelp = false)
{
	WriteChatf("\at%s \agv%1.2f", mqplugin::PluginName, MQ2Version);
	char szTemp[MAX_STRING] = { 0 };

	if (s_settings.GetReminderInterval())
		sprintf_s(szTemp, "\ag%u \atsecs", s_settings.GetReminderInterval());
	else
		strcpy_s(szTemp, "\arDisabled");

	WriteChatf("%s\ar- \atGM Check is: %s \at(Chat: %s \at- Sound: %s \at- Beep: %s \at- Popup: %s \at- Corpses: %s\at) - Reminder Interval: %s",
		PluginMsg,
		s_settings.IsGMCheckEnabled() ? "\agON" : "\arOFF",
		s_settings.IsGMChatAlertEnabled() ? "\agON" : "\arOFF",
		s_settings.IsGMSoundEnabled() ? "\agON" : "\arOFF",
		s_settings.IsGMBeepEnabled() ? "\agON" : "\arOFF",
		s_settings.IsGMPopupEnabled() ? "\agON" : "\arOFF",
		s_settings.IsGMCorpseEnabled() ? "\agIGNORED" : "\arINCLUDED",
		szTemp);

	if (MentionHelp)
		WriteChatf("%s\ayUse '/gmcheck help' for command help", PluginMsg);
}

void PlayErrorSound(const char* sound = "SystemDefault")
{
	PlaySound(nullptr, nullptr, SND_NODEFAULT);
	PlaySound(sound, nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
}

void StopGMSound()
{
	mciSendString("Close mySound", nullptr, 0, nullptr);
}

void PlayGMSound(const std::filesystem::path& sound_file)
{
	StopGMSound();

	std::error_code ec;
	if (!exists(sound_file, ec))
	{
		WriteChatf("%s\atERROR - Sound file not found: \am%s", PluginMsg, sound_file.string().c_str());
	}
	else
	{
		if (!bVolSet)
		{
			if (waveOutGetVolume(nullptr, &dwVolume) == MMSYSERR_NOERROR)
			{
				bVolSet = true;
				waveOutSetVolume(nullptr, NewVol);
			}
		}

		std::string sound_open;
		if (sound_file.extension() == ".mp3")
		{
			sound_open = "MPEGVideo";
		}
		else if (sound_file.extension() == ".wav")
		{
			sound_open = "waveaudio";
		}

		if (sound_open.empty())
		{
			WriteChatf("%s\atERROR - Sound file not supported: \am%s", PluginMsg, sound_file.string().c_str());
		}
		else
		{
			sound_open = fmt::format("Open \"{}\" type {} Alias mySound", absolute(sound_file, ec).string(), sound_open);
			int mci_error = mciSendString(sound_open.c_str(), nullptr, 0, nullptr);
			if (mci_error == 0)
			{
				char szMsg[MAX_STRING] = { 0 };
				mci_error = mciSendString("status mySound length", szMsg, MAX_STRING, nullptr);
				if (mci_error == 0)
				{
					const int i = std::clamp(GetIntFromString(szMsg, 0), 0, 9000);

					StopSoundTimer = MQGetTickCount64() + i;

					const std::string play_command = fmt::format("play mySound from 0 {}notify", i < 9000 ? "" : "to 9000 ");

					mci_error = mciSendString(play_command.c_str(), nullptr, 0, nullptr);

					if (mci_error == 0)
						return;

					WriteChatf("%s\atERROR - Something went wrong playing: \am%s\ax", PluginMsg, sound_file.string().c_str());
				}
				else
				{
					WriteChatf("%s\atERROR - Something went wrong checking length of: \am%s\ax", PluginMsg, sound_file.string().c_str());
				}
			}
			else
			{
				WriteChatf("%s\atERROR - Something went wrong opening: \am%s", PluginMsg, sound_file.string().c_str());
			}
		}
	}

	PlayErrorSound();
	StopSoundTimer = MQGetTickCount64() + 1000;
}

const char* DisplayDT(const char* Format)
{
	static char CurrentDT[MAX_STRING] = { 0 };
	struct tm currentDT;
	time_t long_dt;
	CurrentDT[0] = 0;
	time(&long_dt);
	localtime_s(&currentDT, &long_dt);
	strftime(CurrentDT, MAX_STRING - 1, Format, &currentDT);
	return(CurrentDT);
}

void GMReminder(char* szLine)
{
	char Interval[MAX_STRING];
	GetArg(Interval, szLine, 1);

	if (Interval[0] == 0)
	{
		WriteChatf("%s\aw: Usage is /gmcheck rem VALUE    (where value is num of seconds to set reminder to, min 10 secs - or 0 to disable)", PluginMsg);
		return;
	}

	s_settings.SetReminderInterval(GetIntFromString(Interval, 0));
	if (s_settings.GetReminderInterval() < 10 && s_settings.GetReminderInterval())
		s_settings.SetReminderInterval(10);

	if (s_settings.GetReminderInterval())
		WriteChatf("%s\aw: Reminder interval set to \ar%u \awseconds.", PluginMsg, s_settings.GetReminderInterval());
	else
		WriteChatf("%s\aw: Reminder interval set to \ar%u \awseconds (\arDISABLED\aw).", PluginMsg, s_settings.GetReminderInterval());
}

void GMQuiet(char* szLine)
{
	char szArg[MAX_STRING];
	GetArg(szArg, szLine, 1);

	if (!szArg[0])
		s_settings.SetGMQuietEnabled(true, false, false);
	else if (!_strnicmp(szArg, "on", 2))
		s_settings.SetGMQuietEnabled(false, true, false);
	else if (!_strnicmp(szArg, "off", 3))
		s_settings.SetGMQuietEnabled(false, false, false);
}

void TrackGMs(const char* GMName)
{
	char szSection[MAX_STRING] = { 0 };
	char szTemp[MAX_STRING] = { 0 };
	int iCount = 0;
	char szTime[MAX_STRING] = { 0 };

	sprintf_s(szTime, "Date: %s Time: %s", DisplayDT("%m-%d-%y"), DisplayDT("%I:%M:%S %p"));

	// Store total GM count regardless of server
	strcpy_s(szSection, "GM");
	iCount = GetPrivateProfileInt(szSection, GMName, 0, INIFileName) + 1;
	sprintf_s(szTemp, "%d,%s,%s", iCount, GetServerShortName(), szTime);
	WritePrivateProfileString(szSection, GMName, szTemp, INIFileName);

	// Store GM count by Server
	sprintf_s(szSection, "%s", GetServerShortName());
	iCount = GetPrivateProfileInt(szSection, GMName, 0, INIFileName) + 1;
	sprintf_s(szTemp, "%d,%s", iCount, szTime);
	WritePrivateProfileString(szSection, GMName, szTemp, INIFileName);

	// Store GM count by Server-Zone
	sprintf_s(szSection, "%s-%s", GetServerShortName(), pZoneInfo->LongName);
	iCount = GetPrivateProfileInt(szSection, GMName, 0, INIFileName) + 1;
	sprintf_s(szTemp, "%d,%s", iCount, szTime);
	WritePrivateProfileString(szSection, GMName, szTemp, INIFileName);
}

enum class GMStatuses
{
	Enter,
	Leave,
	Reminder
};

void DoGMAlert(const char* gm_name, GMStatuses status, bool test = false)
{
	char szMsg[MAX_STRING] = { 0 };
	std::filesystem::path sound_to_play;
	int overlay_color = CONCOLOR_RED;
	std::string beep_sound = "SystemDefault";
	switch (status)
	{
	case GMStatuses::Enter:
		sprintf_s(szMsg, "\arGM %s \ayhas entered the zone at \ar%s", gm_name, DisplayDT("%I:%M:%S %p"));
		sound_to_play = s_settings.Sound_GMEnter;
		beep_sound = "SystemAsterisk";
		break;
	case GMStatuses::Leave:
		sprintf_s(szMsg, "\agGM %s \ayhas left the zone (or gone GM Invis) at \ag%s", gm_name, DisplayDT("%I:%M:%S %p"));
		sound_to_play = s_settings.Sound_GMLeave;
		overlay_color = CONCOLOR_GREEN;
		break;
	case GMStatuses::Reminder:
		sprintf_s(szMsg, "\arGM ALERT!!  \ayGM in zone.  \at(%s\at)", gm_name);
		sound_to_play = s_settings.Sound_GMRemind;
		break;
	}

	if (s_settings.IsGMChatAlertEnabled())
		WriteChatf("%s%s", PluginMsg, szMsg);

	if (test || (status == GMStatuses::Enter && !bGMCmdActive) || (status == GMStatuses::Leave && bGMCmdActive && GMNames.empty()))
	{
		// TODO: This could use some cleanup -- is MCEval even necessary?
		char szTmpCmd[MAX_STRING] = { 0 };
		char szTmpIf[MAX_STRING] = { 0 };
		strcpy_s(szTmpCmd, status == GMStatuses::Enter ? s_settings.szGMEnterCmd.c_str() : s_settings.szGMLeaveCmd.c_str());
		strcpy_s(szTmpIf, status == GMStatuses::Enter ? s_settings.szGMEnterCmdIf.c_str() : s_settings.szGMLeaveCmdIf.c_str());
		if (test)
		{
			const int lResult = MCEval(szTmpIf);
			WriteChatf("%s\at(If GM %s zone): GMEnterCmdIf evaluates to %s\at.  Plugin would %s \atGMEnterCmd: \am%s",
				PluginMsg, status == GMStatuses::Enter ? "entered" : "left",
				lResult ? "\agTRUE" : "\arFALSE", lResult ? (szTmpCmd[0] ? (szTmpCmd[0] == '/' ? "\agEXECUTE" : "\arNOT EXECUTE") : "\arNOT EXECUTE") : "\arNOT EXECUTE",
				szTmpCmd[0] ? (szTmpCmd[0] == '/' ? szTmpCmd : "<IGNORED>") : "<NONE>");
		}
		else if (szTmpCmd[0] == '/' && MCEval(szTmpIf))
		{
			EzCommand(szTmpCmd);
			bGMCmdActive = status == GMStatuses::Enter;
		}
	}

	if (!s_settings.IsGMQuietEnabled() && s_settings.IsGMSoundEnabled())
	{
		PlayGMSound(sound_to_play);
	}

	if (!s_settings.IsGMQuietEnabled() && s_settings.IsGMBeepEnabled())
	{
		PlayErrorSound(beep_sound.c_str());
	}

	if (s_settings.IsGMPopupEnabled())
	{
		StripMQChat(szMsg, szMsg);
		DisplayOverlayText(szMsg, overlay_color, 100, 500, 500, 3000);
	}
}

void AddGM(const char* gm_name)
{
	// Do nothing if we're already tracking a GM with this display name
	if (std::find(GMNames.begin(), GMNames.end(), gm_name) == GMNames.end())
	{
		TrackGMs(gm_name);
		GMNames.emplace_back(gm_name);
		strcpy_s(szLastGMName, gm_name);
		strcpy_s(szLastGMTime, DisplayDT("%I:%M:%S %p"));
		strcpy_s(szLastGMDate, DisplayDT("%m-%d-%y"));
		strcpy_s(szLastGMZone, "UNKNOWN");

		const int zoneid = pLocalPC->get_zoneId();
		if (zoneid <= MAX_ZONES)
		{
			strcpy_s(szLastGMZone, pWorldData->ZoneArray[zoneid]->LongName);
		}

		DoGMAlert(gm_name, GMStatuses::Enter);
	}
}

void GMTest(char* szLine)
{
	if (gGameState != GAMESTATE_INGAME)
	{
		WriteChatf("%s\arMust be in game to use /gmcheck test", PluginMsg);
		return;
	}

	char szArg[MAX_STRING] = { 0 };
	GetArg(szArg, szLine, 1);
	if (ci_equals(szArg, "enter"))
	{
		DoGMAlert("TestGMEnter", GMStatuses::Enter, true);
	}
	else if (ci_equals(szArg, "leave"))
	{
		DoGMAlert("TestGMLeave", GMStatuses::Leave, true);
	}
	else if (ci_equals(szArg, "remind"))
	{
		DoGMAlert("TestGMRemind", GMStatuses::Reminder, true);
	}
	else
	{
		WriteChatf("%s\atUsage: \am/gmcheck test {enter|leave|remind}", PluginMsg);
	}
}

void GMSS(const char* szLine)
{
	char szArg[MAX_STRING] = { 0 };
	char szFile[MAX_STRING] = { 0 };

	GetArg(szArg, szLine, 1);
	GetArg(szFile, szLine, 2);

	if (szFile[0] == '\0')
	{
		WriteChatf("%s\arFilename required.  Usage: \at/gmcheck ss {enter|leave|remind} SoundFileName", PluginMsg);
	}
	else
	{
		std::error_code ec;
		std::filesystem::path tmp = s_settings.GetBestSoundFile(szFile);
		if (!exists(tmp, ec))
		{
			WriteChatf("%s\arSound file not found (%s).  No settings changed", PluginMsg, szFile);
		}
		else
		{
			if (ci_equals(szArg, "enter"))
			{
				s_settings.Sound_GMEnter = tmp;
			}
			else if (ci_equals(szArg, "leave"))
			{
				s_settings.Sound_GMLeave = tmp;
			}
			else if (ci_equals(szArg, "remind"))
			{
				s_settings.Sound_GMRemind = tmp;
			}
			else
			{
				WriteChatf("%s\arBad option (%s), usage: \at/gmcheck ss {enter|leave|remind} SoundFileName", PluginMsg, szArg);
				return;
			}
		}
	}
}

void HistoryGMs(HistoryType histValue)
{
	// TODO: Clean up this format, left it for backwards compatibility

	std::vector<std::string> vKeys;
	char szSection[MAX_STRING] = { 0 };
	switch (histValue)
	{
	case eHistory_All:
		strcpy_s(szSection, "GM");
		vKeys = GetPrivateProfileKeys(szSection, INIFileName);
		break;
	case eHistory_Server:
		strcpy_s(szSection, GetServerShortName());
		vKeys = GetPrivateProfileKeys(szSection, INIFileName);
		break;
	case eHistory_Zone:
		sprintf_s(szSection, "%s-%s", GetServerShortName(), pZoneInfo->LongName);
		vKeys = GetPrivateProfileKeys(szSection, INIFileName);
		break;
	}

	std::vector<std::string> Outputs;
	for (const std::string& GMName : vKeys)//cycle through all the entries
	{
		if (GMName.empty())
			continue;

		//Collect Information for the currently listed GM.
		char szTemp[MAX_STRING] = { 0 };
		GetPrivateProfileString(szSection, GMName.c_str(), "", szTemp, MAX_STRING, INIFileName);

		//1: Count
		char SeenCount[MAX_STRING] = { 0 };
		GetArg(SeenCount, szTemp, 1, 0, 0, 0, ',', 0);

		char LastSeenDate[MAX_STRING] = { 0 };
		char ServerName[MAX_STRING] = { 0 };
		//All History also has the server.
		if (histValue == eHistory_All)
		{
			//2: ServerName
			GetArg(ServerName, szTemp, 2, 0, 0, 0, ',', 0);

			//3: Date
			GetArg(LastSeenDate, szTemp, 3, 0, 0, 0, ',', 0);
		}
		else {
			//2: Date
			GetArg(LastSeenDate, szTemp, 2, 0, 0, 0, ',', 0);
		}

		switch (histValue)
		{
		case eHistory_All:
			sprintf_s(szTemp, "%sGM \ap%s\ax - seen \a-t%s\ax times on server \a-t%s\ax, last seen \a-t%s", PluginMsg, GMName.c_str(), SeenCount, ServerName, LastSeenDate);
			break;
		case eHistory_Server:
			sprintf_s(szTemp, "%sGM \ap%s\ax - seen \a-t%s\ax times on this server, last seen \a-t%s", PluginMsg, GMName.c_str(), SeenCount, LastSeenDate);
			break;
		case eHistory_Zone:
			sprintf_s(szTemp, "%sGM \ap%s\ax - seen \a-t%s\ax times in this zone, last seen \a-t%s", PluginMsg, GMName.c_str(), SeenCount, LastSeenDate);
			break;
		}

		Outputs.push_back(szTemp);
	}

	// What GM's have been seen on all servers?
	if (!Outputs.empty())
	{
		WriteChatf("\n%sHistory of GM's in \ag%s\ax section", PluginMsg, (histValue == eHistory_All ? "All" : histValue == eHistory_Server ? "Server" : "Zone"));
		for (const std::string& GMInfo : Outputs)
		{
			WriteChatf("%s", GMInfo.c_str());//already has PluginMsg input when pushed into the vector.
		}
	}
	else {
		WriteChatf("%s\ayWe were unable to find any history for \ag%s\ax section", PluginMsg, (histValue == eHistory_All ? "All" : histValue == eHistory_Server ? "Server" : "Zone"));
	}

	return;
}

void GMHelp()
{
	WriteChatf("\n%s\ayMQ2GMCheck Commands:\n", PluginMsg);
	WriteChatf("%s\ay/gmcheck [status] \ax: \agShow current settings/status.", PluginMsg);
	WriteChatf("%s\ay/gmcheck quiet [off|on]\ax: \agToggle all GM alert & reminder sounds, or force on/off.", PluginMsg);
	WriteChatf("%s\ay/gmcheck [off|on]\ax: \agTurn GM alerting on or off.", PluginMsg);
	WriteChatf("%s\ay/gmcheck sound [off|on]\ax: \agToggle playing sounds for GM alerts, or force on/off.", PluginMsg);
	WriteChatf("%s\ay/gmcheck beep [off|on]\ax: \agToggle playing beeps for GM alerts, or force on/off.", PluginMsg);
	WriteChatf("%s\ay/gmcheck popup [off|on]\ax: \agToggle showing popup messages for GM alerts, or force on/off.", PluginMsg);
	WriteChatf("%s\ay/gmcheck chat [off|on]\ax: \agToggle GM alert being output to the MQ2 chat window, or force on/off.", PluginMsg);
	WriteChatf("%s\ay/gmcheck corpse [off|on]\ax: \agToggle GM alert being ignored if the spawn is a corpse, or force on/off.", PluginMsg);
	WriteChatf("%s\ay/gmcheck rem \ax: \agChange alert reminder interval, in seconds.  e.g.: /gmcheck rem 15 (0 to disable)", PluginMsg);
	WriteChatf("%s\ay/gmcheck load \ax: \agLoad settings from INI file.", PluginMsg);
	WriteChatf("%s\ay/gmcheck test {enter|leave|remind} \ax: Test alerts & sounds for the indicated type.  e.g.: /gmcheck test leave", PluginMsg);
	WriteChatf("%s\ay/gmcheck ss {enter|leave|remind} SoundFileName \ax: Set the filename (wav/mp3) to play for indicated alert. Full path if sound file is not in your MQ/resources/sounds dir.", PluginMsg);
	WriteChatf("%s\ay/gmcheck zone \ax: History of GMs in this zone.", PluginMsg);
	WriteChatf("%s\ay/gmcheck server \ax: History of GMs on this server.", PluginMsg);
	WriteChatf("%s\ay/gmcheck all \ax: History of GMs on all servers.", PluginMsg);

	WriteChatf("%s\ay/gmcheck help \ax: \agThis help.\n", PluginMsg);
}

void GMCheckCmd(PlayerClient* pChar, char* szLine)
{
	char szArg1[MAX_STRING];
	char szArg2[MAX_STRING];
	GetArg(szArg1, szLine, 1);
	if (!_stricmp(szArg1, "on"))
	{
		s_settings.SetGMCheckEnabled(false, true, false);
	}
	else if (!_stricmp(szArg1, "off"))
	{
		s_settings.SetGMCheckEnabled(false, false, false);
	}
	else if (!_stricmp(szArg1, "quiet"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		s_settings.SetGMQuietEnabled(szArg2[0] ? false : true, !_stricmp(szArg2, "on") ? true : false, false);
	}
	else if (!_stricmp(szArg1, "sound"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		s_settings.SetGMSoundEnabled(szArg2[0] ? false : true, !_stricmp(szArg2, "on") ? true : false, false);
	}
	else if (!_stricmp(szArg1, "beep"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		s_settings.SetGMBeepEnabled(szArg2[0] ? false : true, !_stricmp(szArg2, "on") ? true : false, false);
	}
	else if (!_stricmp(szArg1, "corpse"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		s_settings.SetGMCorpseEnabled(szArg2[0] ? false : true, !_stricmp(szArg2, "on") ? true : false, false);
	}
	else if (!_stricmp(szArg1, "popup"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		s_settings.SetGMPopupEnabled(szArg2[0] ? false : true, !_stricmp(szArg2, "on") ? true : false, false);
	}
	else if (!_stricmp(szArg1, "chat"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		s_settings.SetGMChatAlertEnabled(szArg2[0] ? false : true, !_stricmp(szArg2, "on") ? true : false, false);
	}
	else if (!_stricmp(szArg1, "test"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		GMTest(szArg2);
	}
	else if (!_stricmp(szArg1, "ss"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		GMSS(szArg2);
	}
	else if (!_stricmp(szArg1, "rem"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		GMReminder(szArg2);
	}
	else if (!_stricmp(szArg1, "load"))
	{
		s_settings.Load();
		GMCheckStatus();
		WriteChatf("%s\amSettings loaded.", PluginMsg);
	}
	else if (!_stricmp(szArg1, "help"))
	{
		GMHelp();
	}
	else if (!_stricmp(szArg1, "status"))
	{
		GMCheckStatus();
	}
	else if (!_stricmp(szArg1, "Zone"))
	{
		HistoryGMs(eHistory_Zone);
	}
	else if (!_stricmp(szArg1, "Server"))
	{
		HistoryGMs(eHistory_Server);
	}
	else if (!_stricmp(szArg1, "All"))
	{
		HistoryGMs(eHistory_All);
	}
	else
		GMCheckStatus(true);
}

void SetupVolumesFromINI()
{
	//LeftVolume
	int i = GetPrivateProfileInt("Settings", "LeftVolume", -1, INIFileName);
	if (i > 100 || i < 0)
	{
		i = 50;
		WritePrivateProfileInt("Settings", "LeftVolume", i, INIFileName);
	}

	float x = 65535.0f * (static_cast<float>(i) / 100.0f);
	NewVol = static_cast<DWORD>(x);

	//RightVolume
	i = GetPrivateProfileInt("Settings", "RightVolume", -1, INIFileName);
	if (i > 100 || i < 0)
	{
		i = 50;
		WritePrivateProfileInt("Settings", "RightVolume", i, INIFileName);
	}

	x = 65535.0f * (static_cast<float>(i) / 100.0f);
	NewVol = NewVol + (static_cast<DWORD>(x) << 16);
}

void DrawGMCheckSettingsPanel()
{
	/*
	static constexpr inline int default_ReminderInterval = 30;

	std::string szGMEnterCmd = std::string();
	std::string szGMEnterCmdIf = std::string();
	std::string szGMLeaveCmd = std::string();
	std::string szGMLeaveCmdIf = std::string();
	std::filesystem::path Sound_GMEnter = std::filesystem::path(gPathResources) / "Sounds/gmenter.mp3";
	std::filesystem::path Sound_GMLeave = std::filesystem::path(gPathResources) / "Sounds/gmleave.mp3";
	std::filesystem::path Sound_GMRemind = std::filesystem::path(gPathResources) / "Sounds/gmremind.mp3";
	*/
	bool GMCheckEnabled = s_settings.IsGMCheckEnabled();
	if (ImGui::Checkbox("Checking Enabled", &GMCheckEnabled))
	{
		s_settings.SetGMCheckEnabled(GMCheckEnabled);
	}

	bool GMSoundEnabled = s_settings.IsGMSoundEnabled();
	if (ImGui::Checkbox("Sound Playing Enabled", &GMSoundEnabled))
	{
		s_settings.SetGMSoundEnabled(GMSoundEnabled);
	}

	bool GMBeepEnabled = s_settings.IsGMBeepEnabled();
	if (ImGui::Checkbox("Beep Enabled", &GMBeepEnabled))
	{
		s_settings.SetGMBeepEnabled(GMBeepEnabled);
	}

	bool GMPopupEnabled = s_settings.IsGMPopupEnabled();
	if (ImGui::Checkbox("Popup Enabled", &GMPopupEnabled))
	{
		s_settings.SetGMPopupEnabled(GMPopupEnabled);
	}

	bool GMCorpseEnabled = s_settings.IsGMCorpseEnabled();
	if (ImGui::Checkbox("Include Corpses", &GMCorpseEnabled))
	{
		s_settings.SetGMCorpseEnabled(GMCorpseEnabled);
	}

	bool GMChatAlertEnabled = s_settings.IsGMChatAlertEnabled();
	if (ImGui::Checkbox("Alert in MQ Chat", &GMChatAlertEnabled))
	{
		s_settings.SetGMChatAlertEnabled(GMChatAlertEnabled);
	}

	int GMReminderInterval = s_settings.GetReminderInterval();
	if (ImGui::SliderInt("Reminder Interval", &GMReminderInterval, 0, 600))
	{
		s_settings.SetReminderInterval(GMReminderInterval);
	}

	int LeftVolume = GetPrivateProfileInt("Settings", "LeftVolume", -1, INIFileName);
	if (LeftVolume > 100 || LeftVolume < 0)
	{
		LeftVolume = 50;
		WritePrivateProfileInt("Settings", "LeftVolume", LeftVolume, INIFileName);
	}
	if (ImGui::SliderInt("Left Volume", &LeftVolume, 0, 100))
	{
		WritePrivateProfileInt("Settings", "LeftVolume", LeftVolume, INIFileName);
		SetupVolumesFromINI();
	}

	int RightVolume = GetPrivateProfileInt("Settings", "RightVolume", -1, INIFileName);
	if (RightVolume > 100 || RightVolume < 0)
	{
		RightVolume = 50;
		WritePrivateProfileInt("Settings", "RightVolume", RightVolume, INIFileName);
	}
	if (ImGui::SliderInt("Right Volume", &RightVolume, 0, 100))
	{
		WritePrivateProfileInt("Settings", "RightVolume", RightVolume, INIFileName);
		SetupVolumesFromINI();
	}

	ImGui::NewLine();

	static char szSoundGMEnter[MAX_STRING] = { 0 };
	strcpy_s(szSoundGMEnter, MAX_STRING, s_settings.Sound_GMEnter.string().c_str());
	ImGui::SetNextItemWidth(320.0f);
	if (ImGui::InputText("GM Enter Sound", szSoundGMEnter, MAX_STRING, ImGuiInputTextFlags_EnterReturnsTrue) && strlen(szSoundGMEnter) > 0)
	{
		WriteChatf("Set GM Enter Sound to:  \ay%s\ax", szSoundGMEnter);
		s_settings.Sound_GMEnter = szSoundGMEnter;
		WritePrivateProfileString("Settings", "EnterSound", szSoundGMEnter, INIFileName);
	}

	static char szSoundGMLeave[MAX_STRING] = { 0 };
	strcpy_s(szSoundGMLeave, MAX_STRING, s_settings.Sound_GMLeave.string().c_str());
	ImGui::SetNextItemWidth(320.0f);
	if (ImGui::InputText("GM Leave Sound", szSoundGMLeave, MAX_STRING, ImGuiInputTextFlags_EnterReturnsTrue) && strlen(szSoundGMLeave) > 0)
	{
		WriteChatf("Set GM Enter Leave to:  \ay%s\ax", szSoundGMLeave);
		s_settings.Sound_GMLeave = szSoundGMLeave;
		WritePrivateProfileString("Settings", "LeaveSound", szSoundGMLeave, INIFileName);
	}

	static char szSoundGMRemind[MAX_STRING] = { 0 };
	strcpy_s(szSoundGMRemind, MAX_STRING, s_settings.Sound_GMRemind.string().c_str());
	ImGui::SetNextItemWidth(320.0f);
	if (ImGui::InputText("GM Reminder Sound", szSoundGMRemind, MAX_STRING, ImGuiInputTextFlags_EnterReturnsTrue) && strlen(szSoundGMRemind) > 0)
	{
		WriteChatf("Set GM Enter Reminder to:  \ay%s\ax", szSoundGMRemind);
		s_settings.Sound_GMRemind = szSoundGMRemind;
		WritePrivateProfileString("Settings", "RemindSound", szSoundGMRemind, INIFileName);
	}

	ImGui::NewLine();

	static char szGMEnterCmd[MAX_STRING] = { 0 };
	strcpy_s(szGMEnterCmd, s_settings.szGMEnterCmd.c_str());
	ImGui::SetNextItemWidth(320.0f);
	if (ImGui::InputText("GM Enter Cmd", szGMEnterCmd, MAX_STRING, ImGuiInputTextFlags_EnterReturnsTrue) && strlen(szGMEnterCmd) > 0)
	{
		WriteChatf("Set GMEnterCmd to:  \ay%s\ax", szGMEnterCmd);
		s_settings.szGMEnterCmd = szGMEnterCmd;
		WritePrivateProfileString("Settings", "GMEnterCmd", szGMEnterCmd, INIFileName);
	}

	static char szGMEnterCmdIf[MAX_STRING] = { 0 };
	strcpy_s(szGMEnterCmdIf, s_settings.szGMEnterCmdIf.c_str());
	ImGui::SetNextItemWidth(320.0f);
	if (ImGui::InputText("GM Enter CmdIf", szGMEnterCmdIf, MAX_STRING, ImGuiInputTextFlags_EnterReturnsTrue) && strlen(szGMEnterCmdIf) > 0)
	{
		WriteChatf("Set GMEnterCmdIf to:  \ay%s\ax", szGMEnterCmdIf);
		s_settings.szGMEnterCmdIf = szGMEnterCmdIf;
		WritePrivateProfileString("Settings", "GMEnterCmdIf", szGMEnterCmdIf, INIFileName);
	}

	static char szGMLeaveCmd[MAX_STRING] = { 0 };
	strcpy_s(szGMLeaveCmd, s_settings.szGMLeaveCmd.c_str());
	ImGui::SetNextItemWidth(320.0f);
	if (ImGui::InputText("GM Leave Cmd", szGMLeaveCmd, MAX_STRING, ImGuiInputTextFlags_EnterReturnsTrue) && strlen(szGMLeaveCmd) > 0)
	{
		WriteChatf("Set GMLeaveCmd to:  \ay%s\ax", szGMLeaveCmd);
		s_settings.szGMLeaveCmd = szGMLeaveCmd;
		WritePrivateProfileString("Settings", "GMLeaveCmd", szGMLeaveCmd, INIFileName);
	}

	static char szGMLeaveCmdIf[MAX_STRING] = { 0 };
	strcpy_s(szGMLeaveCmdIf, s_settings.szGMLeaveCmdIf.c_str());
	ImGui::SetNextItemWidth(320.0f);
	if (ImGui::InputText("GM Leave CmdIf", szGMLeaveCmdIf, MAX_STRING, ImGuiInputTextFlags_EnterReturnsTrue) && strlen(szGMLeaveCmdIf) > 0)
	{
		WriteChatf("Set GMLeaveCmdIf to:  \ay%s\ax", szGMLeaveCmdIf);
		s_settings.szGMLeaveCmdIf = szGMLeaveCmdIf;
		WritePrivateProfileString("Settings", "GMLeaveCmdIf", szGMLeaveCmdIf, INIFileName);
	}

	ImGui::NewLine();
	ImGui::Separator();

	if (ImGui::Button("Reload Settings"))
	{
		s_settings.Load();
	}

	ImGui::SameLine();
	if (ImGui::Button("Reset Settings"))
	{
		s_settings.Reset();
	}
}

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2GMCheck");
	SetupVolumesFromINI();

	strcpy_s(szLastGMName, "NONE");
	strcpy_s(szLastGMTime, "NEVER");
	strcpy_s(szLastGMDate, "NEVER");
	strcpy_s(szLastGMZone, "NONE");
	GMNames.clear();
	AddSettingsPanel("plugins/GMCheck", DrawGMCheckSettingsPanel);
	s_settings.Load();

	AddMQ2Data("GMCheck", MQ2GMCheckType::dataGMCheck);
	bmMQ2GMCheck = AddMQ2Benchmark(mqplugin::PluginName);
	pGMCheckType = new MQ2GMCheckType;
	AddCommand("/gmcheck", GMCheckCmd);
}

PLUGIN_API void ShutdownPlugin()
{
	WriteChatf("%s\amUnloading plugin.", PluginMsg);
	DebugSpewAlways("Shutting down MQ2GMCheck");
	RemoveCommand("/gmcheck");
	GMNames.clear();
	RemoveMQ2Data("GMCheck");
	RemoveMQ2Benchmark(bmMQ2GMCheck);
	delete pGMCheckType;
	if (bVolSet)
		waveOutSetVolume(nullptr, dwVolume);
	RemoveSettingsPanel("plugins/GMCheck");
}

PLUGIN_API void OnPulse()
{
	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::duration<float, std::milli> duration;
	static clock::time_point pulsestart = clock::now();
	static clock::time_point reminderstart = clock::now();

	MQScopedBenchmark bm(bmMQ2GMCheck);

	if (bVolSet && StopSoundTimer && MQGetTickCount64() >= StopSoundTimer)
	{
		StopSoundTimer = 0;
		waveOutSetVolume(nullptr, dwVolume);
	}

	if (gGameState == GAMESTATE_INGAME)
	{
		duration elapsed = clock::now() - pulsestart;
		if (elapsed.count() > 15000)
		{
			pulsestart = clock::now();
			// Remove ourself if we were placed in the list
			if (!GMNames.empty())
			{
				GMNames.erase(std::remove_if(GMNames.begin(), GMNames.end(), [](const std::string& gm_name)
					{
						const PlayerClient* pSpawn = GetSpawnByName(gm_name.c_str());
				if (pSpawn && pSpawn->GM && pSpawn->SpawnID != pLocalPlayer->SpawnID)
					return false;
				return true;
					}), GMNames.end());
			}
			// Remove any GMs that left
			if (!GMNames.empty())
			{
				GMNames.erase(std::remove_if(GMNames.begin(), GMNames.end(), [](const std::string& gm_name)
					{
						const PlayerClient* pSpawn = GetSpawnByName(gm_name.c_str());
				if (pSpawn && pSpawn->GM)
					return false;

				DoGMAlert(pSpawn->DisplayedName, GMStatuses::Leave);
				return true;
					}), GMNames.end());
			}
			// Add any GMs that appeared
			SPAWNINFO* pSpawn = pSpawnList;
			while (pSpawn) {
				if (pSpawn->GM && pSpawn->SpawnID != pLocalPlayer->SpawnID)
				{
					AddGM(pSpawn->DisplayedName);
				}
				pSpawn = pSpawn->GetNext();
			}
		}

		if (s_settings.GetReminderInterval() > 0)
		{
			duration elapsed = clock::now() - reminderstart;
			if (elapsed.count() > s_settings.GetReminderInterval() * 1000)
			{
				reminderstart = clock::now();
				if (!GMNames.empty() && !s_settings.IsGMQuietEnabled() && s_settings.IsGMCheckEnabled())
				{
					std::string joined_names = "\ag";
					joined_names += join(GMNames, "\ax\am,\ax \ag");

					DoGMAlert(joined_names.c_str(), GMStatuses::Reminder);
				}
			}
		}
	}
}

PLUGIN_API void OnAddSpawn(PlayerClient* pSpawn)
{
	if (pLocalPC && s_settings.IsGMCheckEnabled() && pSpawn && pSpawn->GM && (s_settings.IsGMCorpseEnabled() || pSpawn->Type != SPAWN_CORPSE))
	{
		if (pSpawn->DisplayedName[0] != '\0')
		{
			AddGM(pSpawn->DisplayedName);
		}
	}
}

PLUGIN_API void OnRemoveSpawn(PlayerClient* pSpawn)
{
	if (s_settings.IsGMCheckEnabled() && !GMNames.empty() && pSpawn && pSpawn->GM)
	{
		const size_t start_size = GMNames.size();
		GMNames.erase(std::remove_if(GMNames.begin(), GMNames.end(), [pSpawn](const std::string& i)
			{
				return ci_equals(i, pSpawn->DisplayedName);
			}), GMNames.end());

		if (GMNames.size() != start_size)
			DoGMAlert(pSpawn->DisplayedName, GMStatuses::Leave);
	}
}

PLUGIN_API void OnEndZone()
{
	GMNames.clear();
}

PLUGIN_API void OnZoned()
{
	s_settings.SetGMQuietEnabled(false);
}

PLUGIN_API void SetGameState(int GameState)
{
	// In case the character name has changed
	if (GameState == GAMESTATE_INGAME)
	{
		s_settings.SetAllGMSoundFiles();
	}
}