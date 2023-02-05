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

#include <mq/Plugin.h>
#include <vector>
#include <mmsystem.h>

PreSetup("MQ2GMCheck");
PLUGIN_VERSION(5.00);

#pragma comment(lib,"winmm.lib")

const char PluginMsg[] = "\ay[\aoMQ2GMCheck\ax] ";

char szEnterSound[MAX_STRING] = { 0 };
char szLeaveSound[MAX_STRING] = { 0 };
char szRemindSound[MAX_STRING] = { 0 };
char szLastGMName[MAX_STRING] = { 0 };
char szLastGMTime[MAX_STRING] = { 0 };
char szLastGMDate[MAX_STRING] = { 0 };
char szLastGMZone[MAX_STRING] = { 0 };
char szGMEnterCmd[MAX_STRING] = { 0 };
char szGMEnterCmdIf[MAX_STRING] = { 0 };
char szGMLeaveCmd[MAX_STRING] = { 0 };
char szGMLeaveCmdIf[MAX_STRING] = { 0 };

uint32_t  Reminder_Interval = 0;
uint32_t bmMQ2GMCheck = 0;

uint64_t Check_PulseCount = 0;
uint64_t Update_PulseCount = 0;
uint64_t StopSoundTimer = 0;

DWORD dwVolume;
DWORD NewVol;

bool bGMAlert = false;
bool bGMCheck = true;
bool bGMQuiet = false;
bool bGMSound = true;
bool bGMBeep = false;
bool bGMPopup = false;
bool bGMCorpse = false;
bool bGMCmdActive = false;
bool bVolSet = false;
bool bGMChatAlert = true;

std::vector<std::string> GMNames;

enum HistoryType {
	eHistory_Zone,
	eHistory_Server,
	eHistory_All
};


bool GMCheck()
{
	return !GMNames.empty();
}

int MCEval(const char* zBuffer)
{
	char zOutput[MAX_STRING] = { 0 };
	if (!zBuffer[0])
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
			Dest.DWord = bGMCheck;
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::GM:
			Dest.DWord = GMCheck();
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Names:
		{
			char szTmp[MAX_STRING] = { 0 };
			for (const std::string& GMName : GMNames)
			{
				if (szTmp[0])
					strcat_s(DataTypeTemp, ", ");

				strcat_s(DataTypeTemp, GMName.c_str());
			}

			if (!DataTypeTemp[0])
				strcpy_s(DataTypeTemp, "");

			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;
		}

		case GMCheckMembers::Sound:
			Dest.DWord = bGMSound;
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Beep:
			Dest.DWord = bGMBeep;
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Popup:
			Dest.DWord = bGMPopup;
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Corpse:
			Dest.DWord = bGMCorpse;
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Quiet:
			Dest.DWord = bGMQuiet;
			Dest.Type = pBoolType;
			return true;

		case GMCheckMembers::Interval:
			Dest.Int = Reminder_Interval / 1000;
			Dest.Type = pIntType;
			return true;

		case GMCheckMembers::Enter:
			strcpy_s(DataTypeTemp, szEnterSound);
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::Leave:
			strcpy_s(DataTypeTemp, szLeaveSound);
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::Remind:
			strcpy_s(DataTypeTemp, szRemindSound);
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
			strcpy_s(DataTypeTemp, szGMEnterCmd);
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::GMEnterCmdIf:
			if (MCEval(szGMEnterCmdIf))
				strcpy_s(DataTypeTemp, "TRUE");
			else
				strcpy_s(DataTypeTemp, "FALSE");

			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::GMLeaveCmd:
			strcpy_s(DataTypeTemp, szGMLeaveCmd);
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;

		case GMCheckMembers::GMLeaveCmdIf:
			if (MCEval(szGMLeaveCmdIf))
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
		strcpy_s(Destination, MAX_STRING, GMCheck() ? "TRUE" : "FALSE");
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

	if (Reminder_Interval)
		sprintf_s(szTemp, "\ag%u \atsecs", Reminder_Interval);
	else
		strcpy_s(szTemp, "\arDisabled");

	WriteChatf("%s\ar- \atGM Check is: %s \at(Chat: %s \at- Sound: %s \at- Beep: %s \at- Popup: %s \at- Corpses: %s\at) - Reminder Interval: %s",
		PluginMsg,
		bGMCheck ? "\agON" : "\arOFF",
		bGMChatAlert ? "\agON" : "\arOFF",
		bGMSound ? "\agON" : "\arOFF",
		bGMBeep ? "\agON" : "\arOFF",
		bGMPopup ? "\agON" : "\arOFF",
		bGMCorpse ? "\agIGNORED" : "\arINCLUDED",
		szTemp);

	if (MentionHelp)
		WriteChatf("%s\ayUse '/gmcheck help' for command help", PluginMsg);
}

void ReadSettings()
{
	//Reminder Interval
	Reminder_Interval = GetPrivateProfileInt("Settings", "RemInt", 0, INIFileName);
	if (Reminder_Interval)
		Reminder_Interval *= 1000;
	else
		Reminder_Interval = 30000;

	if (Reminder_Interval < 10000 && Reminder_Interval)
		Reminder_Interval = 10000;

	bGMCheck = GetPrivateProfileBool("Settings", "GMCheck", false, INIFileName);
	bGMSound = GetPrivateProfileBool("Settings", "GMSound", false, INIFileName);
	bGMCorpse = GetPrivateProfileBool("Settings", "GMCorpse", false, INIFileName);
	bGMBeep = GetPrivateProfileBool("Settings", "GMBeep", false, INIFileName);
	bGMPopup = GetPrivateProfileBool("Settings", "GMPopup", false, INIFileName);
	bGMChatAlert = GetPrivateProfileBool("Settings", "GMChat", false, INIFileName);

	{//DefaultEnter
		char szDefaultEnter[MAX_STRING] = { 0 };

		sprintf_s(szDefaultEnter, "%s\\Sounds\\gmenter.mp3", gPathResources);
		GetPrivateProfileString("Settings", "EnterSound", szDefaultEnter, szEnterSound, MAX_STRING, INIFileName);
		if (!_FileExists(szDefaultEnter))
			sprintf_s(szDefaultEnter, "%s\\Sounds\\gmenter.wav", gPathResources);

		if (!_FileExists(szDefaultEnter))
			sprintf_s(szDefaultEnter, "%s\\Sounds\\gmenter.mp3", gPathResources);

		if (!_FileExists(szEnterSound))
			WriteChatf("%s\atWARNING - GM 'enter' sound file not found: \am%s", PluginMsg, szEnterSound);
	}

	{//DefaultLeave
		char szDefaultLeave[MAX_STRING] = { 0 };

		sprintf_s(szDefaultLeave, "%s\\Sounds\\gmleave.mp3", gPathResources);
		GetPrivateProfileString("Settings", "LeaveSound", szDefaultLeave, szLeaveSound, MAX_STRING, INIFileName);
		if (!_FileExists(szDefaultLeave))
			sprintf_s(szDefaultLeave, "%s\\Sounds\\gmleave.wav", gPathResources);
		if (!_FileExists(szDefaultLeave))
			sprintf_s(szDefaultLeave, "%s\\Sounds\\gmleave.mp3", gPathResources);

		if (!_FileExists(szLeaveSound))
			WriteChatf("%s\atWARNING - GM 'leave' sound file not found: \am%s", PluginMsg, szLeaveSound);
	}

	{//DefaultRemind
		char szDefaultRemind[MAX_STRING] = { 0 };
		sprintf_s(szDefaultRemind, "%s\\Sounds\\gmremind.mp3", gPathResources);
		GetPrivateProfileString("Settings", "RemindSound", szDefaultRemind, szRemindSound, MAX_STRING, INIFileName);
		if (!_FileExists(szDefaultRemind))
			sprintf_s(szDefaultRemind, "%s\\Sounds\\gmremind.wav", gPathResources);

		if (!_FileExists(szDefaultRemind))
			sprintf_s(szDefaultRemind, "%s\\Sounds\\gmremind.mp3", gPathResources);

		if (!_FileExists(szRemindSound))
			WriteChatf("%s\atWARNING - GM 'remind' sound file not found: \am%s", PluginMsg, szRemindSound);
	}

	GetPrivateProfileString("Settings", "GMEnterCmd", "", szGMEnterCmd, MAX_STRING, INIFileName);
	GetPrivateProfileString("Settings", "GMEnterCmdIf", "", szGMEnterCmdIf, MAX_STRING, INIFileName);
	GetPrivateProfileString("Settings", "GMLeaveCmd", "", szGMLeaveCmd, MAX_STRING, INIFileName);
	GetPrivateProfileString("Settings", "GMLeaveCmdIf", "", szGMLeaveCmdIf, MAX_STRING, INIFileName);
}

void WriteSettings()
{
	WritePrivateProfileInt("Settings", "RemInt", Reminder_Interval / 1000, INIFileName);

	WritePrivateProfileBool("Settings", "GMCheck", bGMCheck, INIFileName);
	WritePrivateProfileBool("Settings", "GMSound", bGMSound, INIFileName);
	WritePrivateProfileBool("Settings", "GMBeep", bGMBeep, INIFileName);
	WritePrivateProfileBool("Settings", "GMPopup", bGMPopup, INIFileName);
	WritePrivateProfileBool("Settings", "GMChat", bGMChatAlert, INIFileName);
	WritePrivateProfileBool("Settings", "GMCorpse", bGMCorpse, INIFileName);

	WritePrivateProfileString("Settings", "EnterSound", szEnterSound, INIFileName);
	WritePrivateProfileString("Settings", "LeaveSound", szLeaveSound, INIFileName);
	WritePrivateProfileString("Settings", "RemindSound", szRemindSound, INIFileName);
}

void SaveSettings()
{
	WriteSettings();
	GMCheckStatus();
	WriteChatf("%s\amSettings saved.", PluginMsg);
}

void LoadSettings()
{
	ReadSettings();
	GMCheckStatus();
	WriteChatf("%s\amSettings loaded.", PluginMsg);
}

void StopGMSound()
{
	mciSendString("Close mySound", NULL, 0, NULL);
}

void PlayGMSound(char* pFileName)
{
	char lpszOpenCommand[MAX_STRING] = { 0 }, lpszPlayCommand[MAX_STRING] = { 0 }, szMsg[MAX_STRING] = { 0 },
		drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

	StopGMSound();
	if (!bVolSet)
	{
		if (waveOutGetVolume(NULL, &dwVolume) == MMSYSERR_NOERROR)
		{
			bVolSet = true;
			waveOutSetVolume(NULL, NewVol);
		}
	}

	_splitpath_s(pFileName, drive, dir, fname, ext);
	bool bFound = false;

	if (!_stricmp(ext, ".mp3"))
	{
		sprintf_s(lpszOpenCommand, "Open %s type MPEGVideo Alias mySound", pFileName);
		sprintf_s(lpszPlayCommand, "play mySound from 0 to 9000 notify");
		bFound = true;
	}
	else if (!_stricmp(ext, ".wav"))
	{
		sprintf_s(lpszOpenCommand, "Open %s type waveaudio Alias mySound", pFileName);
		sprintf_s(lpszPlayCommand, "play mySound from 0 to 9000 notify");
		bFound = true;
	}

	if (bFound)
	{
		int error = mciSendString(lpszOpenCommand, NULL, 0, NULL);
		if (!error)
		{
			error = mciSendString("status mySound length", szMsg, MAX_STRING, NULL);
			if (!error)
			{
				if ((unsigned)GetIntFromString(szMsg, 0) < 9000)
					StopSoundTimer = MQGetTickCount64() + (unsigned)GetIntFromString(szMsg, 0);
				else
					StopSoundTimer = MQGetTickCount64() + 9000;

				if (!_stricmp(ext, ".mp3"))
				{
					if ((unsigned)GetIntFromString(szMsg, 0) < 9000)
						sprintf_s(lpszPlayCommand, "play mySound from 0 notify");
				}
				else
				{
					if ((unsigned)GetIntFromString(szMsg, 0) < 9000)
						sprintf_s(lpszPlayCommand, "play mySound from 0 notify");
				}

				error = mciSendString(lpszPlayCommand, NULL, 0, NULL);
			}
		}

		if (error)
			bFound = false;
	}

	if (!bFound)
	{
		PlaySound(NULL, NULL, SND_NODEFAULT);
		PlaySound("SystemDefault", NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
		StopSoundTimer = MQGetTickCount64() + 1000;
	}
}

void ToggleBool(char* szLine, bool* theOption, char* msg)
{
	char szArg[MAX_STRING] = { 0 };
	GetArg(szArg, szLine, 1);

	if (!szArg[0])
		*theOption = !*theOption;
	else if (!_strnicmp(szArg, "on", 2))
		*theOption = true;
	else if (!_strnicmp(szArg, "off", 3))
		*theOption = false;

	WriteChatf("%s\am%s is now %s", PluginMsg, *theOption ? "\agEnabled" : "\arDISABLED");
}

char* DisplayTime()
{
	static char* CurrentTime = NULL;
	if (!CurrentTime)
		CurrentTime = new char[MAX_STRING];

	if (!CurrentTime)
		return(NULL);

	struct tm currentTime;
	time_t long_time;
	CurrentTime[0] = 0;
	time(&long_time);
	localtime_s(&currentTime, &long_time);
	strftime(CurrentTime, MAX_STRING - 1, "%I:%M:%S %p", &currentTime);

	return(CurrentTime);
}

char* DisplayDate()
{
	static char* CurrentDate = NULL;
	if (!CurrentDate)
		CurrentDate = new char[MAX_STRING];

	if (!CurrentDate)
		return(NULL);

	struct tm currentDate;
	time_t long_time;
	CurrentDate[0] = 0;
	time(&long_time);
	localtime_s(&currentDate, &long_time);
	strftime(CurrentDate, MAX_STRING - 1, "%m-%d-%y", &currentDate);

	return(CurrentDate);
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

	Reminder_Interval = GetIntFromString(Interval, 0) * 1000;
	if (Reminder_Interval < 10000 && Reminder_Interval)
		Reminder_Interval = 10000;

	if (Reminder_Interval)
		WriteChatf("%s\aw: Reminder interval set to \ar%u \awseconds.  Remember to use \ay/gmsave \awif you want this to be a permanent change.", PluginMsg, Reminder_Interval / 1000);
	else
		WriteChatf("%s\aw: Reminder interval set to \ar%u \awseconds (\arDISABLED\aw).  Remember to use \ay/gmsave \awif you want this to be a permanent change.", PluginMsg, Reminder_Interval / 1000);
}

void GMQuiet(char* szLine)
{
	char szArg[MAX_STRING];
	GetArg(szArg, szLine, 1);

	if (!szArg[0])
		bGMQuiet = !bGMQuiet;
	else if (!_strnicmp(szArg, "on", 2))
		bGMQuiet = true;
	else if (!_strnicmp(szArg, "off", 3))
		bGMQuiet = false;

	WriteChatf("%s\amGM alert and reminder sounds %s%s\am.", PluginMsg, bGMQuiet ? "temporarily " : "", bGMQuiet ? "\arDISABLED" : "\agENABLED");
}

void TrackGMs(const char* GMName)
{
	char szSection[MAX_STRING] = { 0 };
	char szTemp[MAX_STRING] = { 0 };
	int iCount = 0;
	char szTime[MAX_STRING] = { 0 };

	sprintf_s(szTime, "Date: %s Time: %s", DisplayDate(), DisplayTime());

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

	return;
}

void GMTest(char* szLine)
{
	char szArg[MAX_STRING], szTmp[MAX_STRING] = { 0 };
	char szMsg[MAX_STRING], szPopup[MAX_STRING];
	if (gGameState != GAMESTATE_INGAME)
	{
		WriteChatf("%s\arMust be in game to use /gmcheck test", PluginMsg);
		return;
	}

	GetArg(szArg, szLine, 1);
	if (!strncmp(szArg, "enter", 5))
	{
		sprintf_s(szMsg, "(TEST) \arGM %s \ayhas entered the zone at \ar%s", GetCharInfo()->Name, DisplayTime());
		sprintf_s(szPopup, "(TEST) GM %s has entered the zone at %s", GetCharInfo()->Name, DisplayTime());
		WriteChatf("%s%s", PluginMsg, szMsg);
		strcpy_s(szTmp, szGMEnterCmdIf);
		int lResult = MCEval(szTmp);
		WriteChatf("%s\at(If first GM entered zone): GMEnterCmdIf evaluates to %s\at.  Plugin would %s \atGMEnterCmd: \am%s",
			PluginMsg,
			lResult ? "\agTRUE" : "\arFALSE", lResult ? (szGMEnterCmd[0] ? (szGMEnterCmd[0] == '/' ? "\agEXECUTE" : "\arNOT EXECUTE") : "\arNOT EXECUTE") : "\arNOT EXECUTE",
			szGMEnterCmd[0] ? (szGMEnterCmd[0] == '/' ? szGMEnterCmd : "<IGNORED>") : "<NONE>");

		if (!bGMQuiet && bGMSound)
		{
			GetPrivateProfileString(GetCharInfo()->Name, "EnterSound", "", szTmp, MAX_STRING, INIFileName);
			if (szTmp[0] && _FileExists(szTmp))
				PlayGMSound(szTmp);
			else
				PlayGMSound(szEnterSound);
		}

		if (!bGMQuiet && bGMBeep)
		{
			Beep(0x500, 250);
			Beep(0x3000, 250);
			Beep(0x500, 250);
			Beep(0x3000, 250);
		}

		if (bGMPopup)
			DisplayOverlayText(szPopup, CONCOLOR_RED, 100, 500, 500, 3000);

		TrackGMs("NotARealGMThisIsMQTestNPC");
	}
	else if (!strncmp(szArg, "leave", 5))
	{
		sprintf_s(szMsg, "(TEST) \agGM %s \ayhas left the zone at \ag%s", GetCharInfo()->Name, DisplayTime());
		sprintf_s(szPopup, "(TEST) GM %s has left the zone at %s", GetCharInfo()->Name, DisplayTime());
		WriteChatf("%s%s", PluginMsg, szMsg);
		strcpy_s(szTmp, szGMLeaveCmdIf);
		int lResult = MCEval(szTmp);
		WriteChatf("%s\at(If last GM left zone): GMLeaveCmdIf evaluates to %s\at.  Plugin would %s \atGMLeaveCmd: \am%s",
			PluginMsg,
			lResult ? "\agTRUE" : "\arFALSE",
			lResult ? (szGMLeaveCmd[0] ? (szGMLeaveCmd[0] == '/' ? "\agEXECUTE" : "\arNOT EXECUTE") : "\arNOT EXECUTE") : "\arNOT EXECUTE",
			szGMLeaveCmd[0] ? (szGMLeaveCmd[0] == '/' ? szGMLeaveCmd : "<IGNORED>") : "<NONE>");

		if (!bGMQuiet && bGMSound)
		{
			GetPrivateProfileString(GetCharInfo()->Name, "LeaveSound", "", szTmp, MAX_STRING, INIFileName);
			if (szTmp[0] && _FileExists(szTmp))
				PlayGMSound(szTmp);
			else
				PlayGMSound(szLeaveSound);
		}

		if (!bGMQuiet && bGMBeep)
		{
			Beep(0x600, 250);
			Beep(0x600, 250);
			Beep(0x600, 250);
			Beep(0x600, 250);
		}

		if (bGMPopup)
			DisplayOverlayText(szPopup, CONCOLOR_GREEN, 100, 500, 500, 3000);
	}
	else if (!strncmp(szArg, "remind", 6))
	{
		sprintf_s(szMsg, "(TEST) \arGM ALERT!!  \ayGM in zone.  \at(\ag%s\at)", GetCharInfo()->Name);
		WriteChatf("%s%s", PluginMsg, szMsg);
		if (bGMSound)
			PlayGMSound(szRemindSound);
	}
	else
	{
		WriteChatf("%s\atUsage: \am/gmcheck test {enter|leave|remind}", PluginMsg);
	}
}

void GMSS(char* szLine)
{
	char szArg[MAX_STRING], szFile[MAX_STRING] = { 0 };
	bool bOK = false;
	GetArg(szArg, szLine, 1);
	GetArg(szFile, szLine, 2);
	if (szFile[0] && _FileExists(szFile))
		bOK = true;

	if (!bOK)
	{
		WriteChatf("%s\arSound file not found, setting not changed, tried: \am%s", PluginMsg, szFile[0] ? szFile : "(No filename supplied)");
		return;
	}

	if (!strncmp(szArg, "enter", 5))
	{
		strcpy_s(szEnterSound, szFile);
		WriteChatf("%s\at'enter' sound set to: \am%s", PluginMsg, szEnterSound);
		WriteChatf("%s\agDon't forget to use '/gmcheck save' if you want this to be persistant!", PluginMsg);
	}
	else if (!strncmp(szArg, "leave", 5))
	{
		strcpy_s(szLeaveSound, szFile);
		WriteChatf("%s\at'leave' sound set to: \am%s", PluginMsg, szLeaveSound);
		WriteChatf("%s\agDon't forget to use '/gmcheck save' if you want this to be persistant!", PluginMsg);
	}
	else if (!strncmp(szArg, "remind", 5))
	{
		strcpy_s(szRemindSound, szFile);
		WriteChatf("%s\at'remind' sound set to: \am%s", PluginMsg, szRemindSound);
		WriteChatf("%s\agDon't forget to use '/gmcheck save' if you want this to be persistant!", PluginMsg);
	}
	else
	{
		WriteChatf("%s\arBad option, usage: \at/gmcheck ss {enter|leave|remind} SoundFileName", PluginMsg);
	}
}

void UpdateAlerts()
{
	if (GMNames.empty() || gGameState != GAMESTATE_INGAME || MQGetTickCount64() < Update_PulseCount + 15000)
		return;

	Update_PulseCount = MQGetTickCount64();
	uint32_t index = 0;
	for (const std::string& GMName : GMNames)
	{
		PlayerClient* pSpawn = GetSpawnByName(GMName.c_str());
		if (pSpawn && pSpawn->GM)
		{
			index++;
			continue;
		}

		if (bGMChatAlert)
		{
			char szMsg[MAX_STRING] = { 0 };
			sprintf_s(szMsg, "\agGM %s \ayhas left the zone at \ag%s", GMName.c_str(), DisplayTime());
			WriteChatf("%s%s", PluginMsg, szMsg);
		}

		if (GMNames.empty() && bGMCmdActive)
		{
			char szTmp[MAX_STRING] = { 0 };
			strcpy_s(szTmp, szGMLeaveCmdIf);
			if (MCEval(szTmp) && szGMLeaveCmd[0] && szGMLeaveCmd[0] == '/')
			{
				EzCommand(szGMLeaveCmd);
				bGMCmdActive = false;
			}
		}

		if (!bGMQuiet && bGMSound)
		{
			char szTmp[MAX_STRING] = { 0 };
			GetPrivateProfileString(GMName.c_str(), "LeaveSound", "", szTmp, MAX_STRING, INIFileName);
			if (szTmp[0] && _FileExists(szTmp))
				PlayGMSound(szTmp);
			else
				PlayGMSound(szLeaveSound);
		}

		if (!bGMQuiet && bGMBeep)
		{
			PlaySound(NULL, NULL, SND_NODEFAULT);
			PlaySound("SystemDefault", NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
		}

		if (bGMPopup)
		{
			char szPopup[MAX_STRING];
			sprintf_s(szPopup, "GM %s has left the zone at %s", GMName.c_str(), DisplayTime());
			DisplayOverlayText(szPopup, CONCOLOR_GREEN, 100, 500, 500, 3000);
		}

		GMNames.erase(GMNames.begin() + index);
	}
}

void HistoryGMs(HistoryType histValue)
{
	/* TODO: Clean up this format, left it for backwards compatibility
		Format recommended by Knightly
		[GM]
		Firhumrng=something

		[Firhumrng_firiona]
		LastSeen=Somedate
		LastZone=somezone
		TotalTimes=50
	*/

	/* Current Format.
		List of GMs.
			[GM]
			Firhumrng=20,firiona,Date: 02-02-23 Time: 03:24:05 PM
			Firhumnec=20,firiona,Date: 02-02-23 Time: 03:24:05 PM
			Morrganne=4,firiona,Date: 02-02-23 Time: 03:24:05 PM

		List of GMs for this server.
			[firiona]
			Firhumrng=20,Date: 02-02-23 Time: 03:24:05 PM
			Treantz=1,Date: 02-02-23 Time: 03:24:05 PM
			Niente=29,Date: 02-02-23 Time: 03:24:05 PM

		Zone specific example :
			[firiona-Cobalt Scar]
			Firhumrng=1,Date: 02-02-23 Time: 03:24:05 PM
	*/
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
	{

		sprintf_s(szSection, "%s-%s", GetServerShortName(), pZoneInfo->LongName);
		vKeys = GetPrivateProfileKeys(szSection, INIFileName);
		break;
	}
	default:
		WriteChatf("%s\ar Sorry we have encountered an error. Please submit an issue to git [Line: %d] %s, histValue: %i", PluginMsg, __LINE__, __FUNCTION__, histValue);
		return;
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
		default:
			WriteChatf("%s\ar Sorry we have encountered an error. Please submit an issue to git [Line: \at%d\ax] \at%s", PluginMsg, __LINE__, __FUNCTION__);
			return;
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
	WriteChatf("%s\ay/gmcheck save \ax: \agSave current settings.", PluginMsg);
	WriteChatf("%s\ay/gmcheck load \ax: \agLoad settings from INI file.", PluginMsg);
	WriteChatf("%s\ay/gmcheck test {enter|leave|remind} \ax: Test alerts & sounds for the indicated type.  e.g.: /gmcheck test leave", PluginMsg);
	WriteChatf("%s\ay/gmcheck ss {enter|leave|remind} SoundFileName \ax: Set the filename (wav/mp3) to play for indicated alert. Full path if sound file is not in your MQ2 dir.", PluginMsg);
	WriteChatf("%s\ay/gmcheck zone - history of GM's in this zone.", PluginMsg);
	WriteChatf("%s\ay/gmcheck server - history of GM's on this server.", PluginMsg);
	WriteChatf("%s\ay/gmcheck all - history of GM's on all servers.", PluginMsg);

	WriteChatf("%s\ay/gmcheck help \ax: \agThis help.\n", PluginMsg);
}

void GMCheckCmd(PlayerClient* pChar, char* szLine)
{
	char szArg1[MAX_STRING], szArg2[MAX_STRING];
	GetArg(szArg1, szLine, 1);
	if (!_stricmp(szArg1, "on"))
	{
		bGMCheck = true;
		WriteChatf("%s\amGM checking is now %s\am.", PluginMsg, bGMCheck ? "\agENABLED" : "\arDISABLED");
	}
	else if (!_stricmp(szArg1, "off"))
	{
		bGMCheck = false;
		WriteChatf("%s\amGM checking is now %s\am.", PluginMsg, bGMCheck ? "\agENABLED" : "\arDISABLED");
	}
	else if (!_stricmp(szArg1, "quiet"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		GMQuiet(szArg2);
	}
	else if (!_stricmp(szArg1, "sound"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		ToggleBool(szArg2, &bGMSound, "Sound playing on GM detection");
	}
	else if (!_stricmp(szArg1, "beep"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		ToggleBool(szArg2, &bGMBeep, "Beeping on GM detection");
	}
	else if (!_stricmp(szArg1, "corpse"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		ToggleBool(szArg2, &bGMCorpse, "Corpse exclusion from GM alerts");

	}
	else if (!_stricmp(szArg1, "popup"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		ToggleBool(szArg2, &bGMPopup, "Showing popup message on GM detection");
	}
	else if (!_stricmp(szArg1, "chat"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		ToggleBool(szArg2, &bGMChatAlert, "displaying alerts in chat window");
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
		LoadSettings();
	}
	else if (!_stricmp(szArg1, "save"))
	{
		SaveSettings();
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
	} else
		GMCheckStatus(true);
}

void SetupVolumesFromINI()
{
	//LeftVolume
	DWORD i = (DWORD)GetPrivateProfileInt("Settings", "LeftVolume", 50, INIFileName);
	if (i > 100 || i < 0)
		i = 50;

	WritePrivateProfileInt("Settings", "LeftVolume", i, INIFileName);
	float x = (float)65535.0 * ((float)i / (float)100.0);
	NewVol = (DWORD)x;

	//RightVolume
	i = (DWORD)GetPrivateProfileInt("Settings", "RightVolume", 50, INIFileName);
	if (i > 100 || i < 0)
		i = 50;

	WritePrivateProfileInt("Settings", "RightVolume", i, INIFileName);
	x = (float)65535.0 * ((float)i / (float)100.0);
	NewVol = NewVol + (((DWORD)x) << 16);
}


PLUGIN_API VOID InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2GMCheck");
	SetupVolumesFromINI();

	strcpy_s(szLastGMName, "NONE");
	strcpy_s(szLastGMTime, "NEVER");
	strcpy_s(szLastGMDate, "NEVER");
	strcpy_s(szLastGMZone, "NONE");
	GMNames.clear();
	Check_PulseCount = MQGetTickCount64();
	Update_PulseCount = MQGetTickCount64();
	ReadSettings();

	AddMQ2Data("GMCheck", MQ2GMCheckType::dataGMCheck);
	bmMQ2GMCheck = AddMQ2Benchmark(mqplugin::PluginName);
	pGMCheckType = new MQ2GMCheckType;
	AddCommand("/gmcheck", GMCheckCmd);

	if (gGameState == GAMESTATE_INGAME)
		bGMAlert = GMCheck();
}

PLUGIN_API VOID ShutdownPlugin()
{
	WriteChatf("%s\amUnloading plugin.", PluginMsg);
	DebugSpewAlways("Shutting down MQ2GMCheck");
	RemoveCommand("/gmcheck");
	GMNames.clear();
	RemoveMQ2Data("GMCheck");
	RemoveMQ2Benchmark(bmMQ2GMCheck);
	delete pGMCheckType;
	if (bVolSet)
		waveOutSetVolume(NULL, dwVolume);
}

PLUGIN_API VOID OnPulse()
{
	MQScopedBenchmark bm(bmMQ2GMCheck);

	if (bVolSet && StopSoundTimer && MQGetTickCount64() >= StopSoundTimer)
	{
		StopSoundTimer = 0;
		waveOutSetVolume(NULL, dwVolume);
	}

	if (!Reminder_Interval)
		return;

	if (gGameState == GAMESTATE_INGAME)
	{
		UpdateAlerts();
		if (MQGetTickCount64() >= Check_PulseCount + Reminder_Interval && Reminder_Interval)
		{
			Check_PulseCount = MQGetTickCount64();
			if (bGMAlert = GMCheck() && !bGMQuiet && bGMCheck)
			{
				char szNames[MAX_STRING] = { 0 };
				for (const std::string& GMName : GMNames)
				{
					if (strlen(szNames) > 500)
					{
						strcat_s(szNames, " ...");
						break;
					}

					if (szNames[0])
						strcat_s(szNames, "\am, ");

					strcat_s(szNames, "\ag");
					strcat_s(szNames, GMName.c_str());
				}

				char szTmp[MAX_STRING] = { 0 };
				sprintf_s(szTmp, "\arGM ALERT!!  \ayGM in zone.  \at(%s\at)", szNames);
				if (bGMChatAlert)
					WriteChatf("%s%s", PluginMsg, szTmp);

				if (bGMSound)
					PlayGMSound(szRemindSound);
			}
		}
	}
}

PLUGIN_API VOID OnAddSpawn(PlayerClient* pSpawn)
{
	if (pLocalPC && bGMCheck && pSpawn && pSpawn->GM && (bGMCorpse || pSpawn->Type == SPAWN_CORPSE))
	{
		if (!strlen(pSpawn->DisplayedName))
			return;

		TrackGMs(pSpawn->DisplayedName);
		GMNames.push_back(pSpawn->DisplayedName);
		strcpy_s(szLastGMName, pSpawn->DisplayedName);
		strcpy_s(szLastGMTime, DisplayTime());
		strcpy_s(szLastGMDate, DisplayDate());
		strcpy_s(szLastGMZone, "UNKNOWN");

		int zoneid = (pLocalPC->zoneId & 0x7FFF);
		if (zoneid <= MAX_ZONES)
		{
			strcpy_s(szLastGMZone, pWorldData->ZoneArray[zoneid]->LongName);
		}

		char szMsg[MAX_STRING] = { 0 };
		sprintf_s(szMsg, "\arGM %s \ayhas entered the zone at \ar%s", pSpawn->DisplayedName, DisplayTime());

		char szPopup[MAX_STRING];
		sprintf_s(szPopup, "GM %s has entered the zone at %s", pSpawn->DisplayedName, DisplayTime());

		if (bGMChatAlert)
			WriteChatf("%s%s", PluginMsg, szMsg);

		if (!bGMCmdActive)
		{
			char szTmp[MAX_STRING] = { 0 };
			strcpy_s(szTmp, szGMEnterCmdIf);
			if (MCEval(szTmp) && szGMEnterCmd[0] && szGMEnterCmd[0] == '/')
			{
				DoCommand((GetCharInfo() && GetCharInfo()->pSpawn) ? GetCharInfo()->pSpawn : NULL, szGMEnterCmd);
				bGMCmdActive = true;
			}
		}

		if (!bGMQuiet && bGMSound)
		{
			char szTmp[MAX_STRING] = { 0 };
			GetPrivateProfileString(pSpawn->DisplayedName, "EnterSound", "", szTmp, MAX_STRING, INIFileName);
			if (szTmp[0] && _FileExists(szTmp))
				PlayGMSound(szTmp);
			else
				PlayGMSound(szEnterSound);
		}

		if (!bGMQuiet && bGMBeep)
		{
			PlaySound(NULL, NULL, SND_NODEFAULT);
			PlaySound("SystemAsterisk", NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
		}

		if (bGMPopup)
			DisplayOverlayText(szPopup, CONCOLOR_RED, 100, 500, 500, 3000);
	}
}

PLUGIN_API VOID OnRemoveSpawn(PlayerClient* pSpawn)
{
	if (bGMCheck && pSpawn && pSpawn->GM)
	{
		bool GMFound = false;
		for (unsigned int x = 0; x < GMNames.size(); x++)
		{
			std::string& VectorRef = GMNames[x];
			if (!_stricmp(pSpawn->DisplayedName, VectorRef.c_str()))
			{
				GMNames.erase(GMNames.begin() + x);
				GMFound = true;
			}
		}

		if (!GMFound)
			return;

		if (bGMChatAlert)
		{
			char szMsg[MAX_STRING] = { 0 };
			sprintf_s(szMsg, "\agGM %s \ayhas left the zone at \ag%s", pSpawn->DisplayedName, DisplayTime());
			WriteChatf("%s%s", PluginMsg, szMsg);
		}

		if (GMNames.empty() && bGMCmdActive)
		{
			char szTmp[MAX_STRING] = { 0 };
			strcpy_s(szTmp, szGMLeaveCmdIf);
			if (MCEval(szTmp) && szGMLeaveCmd[0] && szGMLeaveCmd[0] == '/')
			{
				EzCommand(szGMLeaveCmd);
				bGMCmdActive = false;
			}
		}

		if (!bGMQuiet && bGMSound)
		{
			char szTmp[MAX_STRING] = { 0 };
			GetPrivateProfileString(pSpawn->DisplayedName, "LeaveSound", "", szTmp, MAX_STRING, INIFileName);
			if (szTmp[0] && _FileExists(szTmp))
				PlayGMSound(szTmp);
			else
				PlayGMSound(szLeaveSound);
		}

		if (!bGMQuiet && bGMBeep)
		{
			PlaySound(NULL, NULL, SND_NODEFAULT);
			PlaySound("SystemDefault", NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
		}

		if (bGMPopup)
		{
			char szPopup[MAX_STRING] = { 0 };
			sprintf_s(szPopup, "GM %s has left the zone at %s", pSpawn->DisplayedName, DisplayTime());
			DisplayOverlayText(szPopup, CONCOLOR_GREEN, 100, 500, 500, 3000);
		}
	}
}

PLUGIN_API VOID OnEndZone()
{
	GMNames.clear();
}

PLUGIN_API VOID OnZoned()
{
	bGMQuiet = false;
}
