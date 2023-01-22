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
PLUGIN_VERSION(4.00);

#pragma comment(lib,"winmm.lib") 

char szEnterSound[MAX_STRING] = { 0 }, szLeaveSound[MAX_STRING] = { 0 }, szRemindSound[MAX_STRING] = { 0 }, szLastGMName[MAX_STRING] = { 0 },
	szLastGMTime[MAX_STRING] = { 0 }, szLastGMDate[MAX_STRING] = { 0 }, szLastGMZone[MAX_STRING] = { 0 }, szGM[50] = { 0 },
	szGMEnterCmd[MAX_STRING] = { 0 }, szGMEnterCmdIf[MAX_STRING] = { 0 }, szGMLeaveCmd[MAX_STRING] = { 0 }, szGMLeaveCmdIf[MAX_STRING] = { 0 };
unsigned long Check_PulseCount = 0, Update_PulseCount = 0, Reminder_Interval = 0, StopSoundTimer = 0, dwVolume, NewVol;;
bool bGMAlert = false, bGMCheck = true, bGMQuiet = false, bGMSound = true, bGMBeep = false, bGMPopup = false, bGMCorpse = false,
	bGMCmdActive = false, bVolSet = false, bGMChatAlert = true;
std::vector<std::string> GMNames;

bool GMCheck()
{
	return GMNames.size() ? true : false;
}

long MCEval(const char* zBuffer)
{
	char zOutput[MAX_STRING] = { 0 };
	if (!zBuffer[0])
		return 1;
	strcpy_s(zOutput, zBuffer);
	ParseMacroData(zOutput, MAX_STRING);
	return atoi(zOutput);
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

		char szTmp[MAX_STRING] = { 0 };
		unsigned int x;
		long lResult;
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
			szTmp[0] = 0;
			for (x = 0; x < GMNames.size(); x++)
			{
				std::string& VectorRef = GMNames[x];
				if (szTmp[0])
					strcat_s(szTmp, ", ");
				strcat_s(szTmp, VectorRef.c_str());
			}
			if (szTmp[0])
				strcpy_s(DataTypeTemp, szTmp);
			else
				strcpy_s(DataTypeTemp, "");
			Dest.Ptr = DataTypeTemp;
			Dest.Type = pStringType;
			return true;
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
			strcpy_s(szTmp, szGMEnterCmdIf);
			lResult = MCEval(szTmp);
			if (lResult)
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
			strcpy_s(szTmp, szGMLeaveCmdIf);
			lResult = MCEval(szTmp);
			if (lResult)
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

bool IsGMCorpse(BYTE bType)
{
	if (bGMCorpse)
		if (bType == SPAWN_CORPSE)
			return true;
	return false;
}

void GMCheckStatus(bool MentionHelp = false)
{
	WriteChatf("\at%s \agv%1.2f", PLUGIN_NAME, MQ2Version);
	if (Reminder_Interval)
		WriteChatf("  \ar- \atGM Check is: %s \at(Chat: %s \at- Sound: %s \at- Beep: %s \at- Popup: %s \at- Corpses: %s\at) - Reminder Interval: \ag%u \atsecs", bGMCheck ? "\agON" : "\arOFF", bGMChatAlert ? "\agON" : "\arOFF", bGMSound ? "\agON" : "\arOFF", bGMBeep ? "\agON" : "\arOFF", bGMPopup ? "\agON" : "\arOFF", bGMCorpse ? "\agIGNORED" : "\arINCLUDED", (int)(Reminder_Interval / 1000));
	else
		WriteChatf("  \ar- \atGM Check is: %s \at(Chat: %s \at- Sound: %s \at- Beep: %s \at- Popup: %s \at- Corpses: %s\at) - Reminder Interval: \arDISABLED\at", bGMCheck ? "\agON" : "\arOFF", bGMChatAlert ? "\agON" : "\arOFF", bGMSound ? "\agON" : "\arOFF", bGMBeep ? "\agON" : "\arOFF", bGMPopup ? "\agON" : "\arOFF", bGMCorpse ? "\agIGNORED" : "\arINCLUDED");
	if (MentionHelp)
		WriteChatf("\ayUse '/gmcheck help' for command help");
}

void ReadSettings()
{
	char szTemp[MAX_STRING], szDefaultEnter[MAX_STRING], szDefaultLeave[MAX_STRING], szDefaultRemind[MAX_STRING];

	if (GetPrivateProfileString("Settings", "RemInt", NULL, szTemp, MAX_STRING, INIFileName))
		Reminder_Interval = atoi(szTemp) * 1000;
	else
		Reminder_Interval = 30000;
	if (Reminder_Interval < 10000 && Reminder_Interval)
		Reminder_Interval = 10000;

	if (GetPrivateProfileString("Settings", "GMCheck", NULL, szTemp, MAX_STRING, INIFileName))
		if (!_stricmp(szTemp, "on"))
			bGMCheck = true;
		else
			bGMCheck = false;

	if (GetPrivateProfileString("Settings", "GMSound", NULL, szTemp, MAX_STRING, INIFileName))
		if (!_stricmp(szTemp, "on"))
			bGMSound = true;
		else
			bGMSound = false;

	if (GetPrivateProfileString("Settings", "GMCorpse", NULL, szTemp, MAX_STRING, INIFileName))
		if (!_stricmp(szTemp, "on"))
			bGMCorpse = true;
		else
			bGMCorpse = false;

	if (GetPrivateProfileString("Settings", "GMBeep", NULL, szTemp, MAX_STRING, INIFileName))
		if (!_stricmp(szTemp, "on"))
			bGMBeep = true;
		else
			bGMBeep = false;

	if (GetPrivateProfileString("Settings", "GMPopup", NULL, szTemp, MAX_STRING, INIFileName))
		if (!_stricmp(szTemp, "on"))
			bGMPopup = true;
		else
			bGMPopup = false;

	if (GetPrivateProfileString("Settings", "GMChat", NULL, szTemp, MAX_STRING, INIFileName))
		if (!_stricmp(szTemp, "on"))
			bGMChatAlert = true;
		else
			bGMChatAlert = false;

	sprintf_s(szDefaultEnter, "%s\\Sounds\\gmenter.mp3", gPathResources);
	sprintf_s(szDefaultLeave, "%s\\Sounds\\gmleave.mp3", gPathResources);
	sprintf_s(szDefaultRemind, "%s\\Sounds\\gmremind.mp3", gPathResources);
	if (!_FileExists(szDefaultEnter))
	{
		sprintf_s(szDefaultEnter, "%s\\Sounds\\gmenter.wav", gPathResources);
		sprintf_s(szDefaultLeave, "%s\\Sounds\\gmleave.wav", gPathResources);
		sprintf_s(szDefaultRemind, "%s\\Sounds\\gmremind.wav", gPathResources);
	}
	if (!_FileExists(szDefaultEnter))
	{
		sprintf_s(szDefaultEnter, "%s\\Sounds\\gmenter.mp3", gPathResources);
		sprintf_s(szDefaultLeave, "%s\\Sounds\\gmleave.mp3", gPathResources);
		sprintf_s(szDefaultRemind, "%s\\Sounds\\gmremind.mp3", gPathResources);
	}
	GetPrivateProfileString("Settings", "EnterSound", szDefaultEnter, szEnterSound, MAX_STRING, INIFileName);
	GetPrivateProfileString("Settings", "LeaveSound", szDefaultLeave, szLeaveSound, MAX_STRING, INIFileName);
	GetPrivateProfileString("Settings", "RemindSound", szDefaultRemind, szRemindSound, MAX_STRING, INIFileName);
	if (!_FileExists(szEnterSound))
		WriteChatf("\ayMQ2GMCheck: \atWARNING - GM 'enter' sound file not found: \am%s", szEnterSound);
	if (!_FileExists(szLeaveSound))
		WriteChatf("\ayMQ2GMCheck: \atWARNING - GM 'leave' sound file not found: \am%s", szLeaveSound);
	if (!_FileExists(szRemindSound))
		WriteChatf("\ayMQ2GMCheck: \atWARNING - GM 'remind' sound file not found: \am%s", szRemindSound);

	GetPrivateProfileString("Settings", "GMEnterCmd", "", szGMEnterCmd, MAX_STRING, INIFileName);
	GetPrivateProfileString("Settings", "GMEnterCmdIf", "", szGMEnterCmdIf, MAX_STRING, INIFileName);
	GetPrivateProfileString("Settings", "GMLeaveCmd", "", szGMLeaveCmd, MAX_STRING, INIFileName);
	GetPrivateProfileString("Settings", "GMLeaveCmdIf", "", szGMLeaveCmdIf, MAX_STRING, INIFileName);
}

void WriteSettings()
{
	char szTemp[MAX_STRING];
	_itoa_s(int(Reminder_Interval / 1000), szTemp, 10);
	WritePrivateProfileString("Settings", "RemInt", szTemp, INIFileName);
	WritePrivateProfileString("Settings", "GMCheck", bGMCheck ? "on" : "off", INIFileName);
	WritePrivateProfileString("Settings", "GMSound", bGMSound ? "on" : "off", INIFileName);
	WritePrivateProfileString("Settings", "GMBeep", bGMBeep ? "on" : "off", INIFileName);
	WritePrivateProfileString("Settings", "GMPopup", bGMPopup ? "on" : "off", INIFileName);
	WritePrivateProfileString("Settings", "GMChat", bGMChatAlert ? "on" : "off", INIFileName);
	WritePrivateProfileString("Settings", "GMCorpse", bGMCorpse ? "on" : "off", INIFileName);
	WritePrivateProfileString("Settings", "EnterSound", szEnterSound, INIFileName);
	WritePrivateProfileString("Settings", "LeaveSound", szLeaveSound, INIFileName);
	WritePrivateProfileString("Settings", "RemindSound", szRemindSound, INIFileName);
}

void SaveSettings()
{
	WriteSettings();
	GMCheckStatus();
	WriteChatf("\ayMQ2GMCheck: \amSettings saved.");
}

void LoadSettings()
{
	ReadSettings();
	GMCheckStatus();
	WriteChatf("\ayMQ2GMCheck: \amSettings loaded.");
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
				if ((unsigned)atoi(szMsg) < 9000)
					StopSoundTimer = GetTickCount() + (unsigned)atoi(szMsg);
				else
					StopSoundTimer = GetTickCount() + 9000;
				if (!_stricmp(ext, ".mp3"))
				{
					if ((unsigned)atoi(szMsg) < 9000)
						sprintf_s(lpszPlayCommand, "play mySound from 0 notify");
				}
				else
				{
					if ((unsigned)atoi(szMsg) < 9000)
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
		StopSoundTimer = GetTickCount() + 1000;
	}
}

void GMCorpseToggle(char* szLine)
{
	char szArg[MAX_STRING];
	GetArg(szArg, szLine, 1);
	if (!szArg[0])
		bGMCorpse = !bGMCorpse;
	else if (!_strnicmp(szArg, "on", 2))
		bGMCorpse = true;
	else if (!_strnicmp(szArg, "off", 3))
		bGMCorpse = false;
	WriteChatf("\ayMQ2GMCheck: \amCorpse exclusion from GM alerts is now %s\am.", bGMCorpse ? "\agENABLED" : "\arDISABLED");
}

void GMSoundToggle(char* szLine)
{
	char szArg[MAX_STRING];
	GetArg(szArg, szLine, 1);
	if (!szArg[0])
		bGMSound = !bGMSound;
	else if (!_strnicmp(szArg, "on", 2))
		bGMSound = true;
	else if (!_strnicmp(szArg, "off", 3))
		bGMSound = false;
	WriteChatf("\ayMQ2GMCheck: \amSound playing on GM detection is now %s\am.", bGMSound ? "\agENABLED" : "\arDISABLED");
}

void GMBeepToggle(char* szLine)
{
	char szArg[MAX_STRING];
	GetArg(szArg, szLine, 1);
	if (!szArg[0])
		bGMBeep = !bGMBeep;
	else if (!_strnicmp(szArg, "on", 2))
		bGMBeep = true;
	else if (!_strnicmp(szArg, "off", 3))
		bGMBeep = false;
	WriteChatf("\ayMQ2GMCheck: \amBeeping on GM detection is now %s\am.", bGMBeep ? "\agENABLED" : "\arDISABLED");
}

void GMPopupToggle(char* szLine)
{
	char szArg[MAX_STRING];
	GetArg(szArg, szLine, 1);
	if (!szArg[0])
		bGMPopup = !bGMPopup;
	else if (!_strnicmp(szArg, "on", 2))
		bGMPopup = true;
	else if (!_strnicmp(szArg, "off", 3))
		bGMPopup = false;
	WriteChatf("\ayMQ2GMCheck: \amShowing popup message on GM detection is now %s\am.", bGMPopup ? "\agENABLED" : "\arDISABLED");
}

void GMChatToggle(char* szLine)
{
	char szArg[MAX_STRING];
	GetArg(szArg, szLine, 1);
	if (!szArg[0])
		bGMChatAlert = !bGMChatAlert;
	else if (!_strnicmp(szArg, "on", 2))
		bGMChatAlert = true;
	else if (!_strnicmp(szArg, "off", 3))
		bGMChatAlert = false;
	WriteChatf("\ayMQ2GMCheck: %s \amdisplaying alerts in chat window.", bGMChatAlert ? "\agNow" : "\arNo longer");
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
		WriteChatf("\ay/MQ2GMCheck\aw: Usage is /gmcheck rem VALUE    (where value is num of seconds to set reminder to, min 10 secs - or 0 to disable)");
		return;
	}

	Reminder_Interval = atoi(Interval) * 1000;
	if (Reminder_Interval < 10000 && Reminder_Interval)
		Reminder_Interval = 10000;
	if (Reminder_Interval)
		WriteChatf("\ay/MQ2GMCheck\aw: Reminder interval set to \ar%u \awseconds.  Remember to use \ay/gmsave \awif you want this to be a permanent change.", Reminder_Interval / 1000);
	else
		WriteChatf("\ay/MQ2GMCheck\aw: Reminder interval set to \ar%u \awseconds (\arDISABLED\aw).  Remember to use \ay/gmsave \awif you want this to be a permanent change.", Reminder_Interval / 1000);
}

void GMHelp()
{
	WriteChatf("");
	WriteChatf("\ayMQ2GMCheck Commands:");
	WriteChatf("");
	WriteChatf("\ay/gmcheck [status] \ax: \agShow current settings/status.");
	WriteChatf("\ay/gmcheck quiet [off|on]\ax: \agToggle all GM alert & reminder sounds, or force on/off.");
	WriteChatf("\ay/gmcheck [off|on]\ax: \agTurn GM alerting on or off.");
	WriteChatf("\ay/gmcheck sound [off|on]\ax: \agToggle playing sounds for GM alerts, or force on/off.");
	WriteChatf("\ay/gmcheck beep [off|on]\ax: \agToggle playing beeps for GM alerts, or force on/off.");
	WriteChatf("\ay/gmcheck popup [off|on]\ax: \agToggle showing popup messages for GM alerts, or force on/off.");
	WriteChatf("\ay/gmcheck chat [off|on]\ax: \agToggle GM alert being output to the MQ2 chat window, or force on/off.");
	WriteChatf("\ay/gmcheck corpse [off|on]\ax: \agToggle GM alert being ignored if the spawn is a corpse, or force on/off.");
	WriteChatf("\ay/gmcheck rem \ax: \agChange alert reminder interval, in seconds.  e.g.: /gmcheck rem 15 (0 to disable)");
	WriteChatf("\ay/gmcheck save \ax: \agSave current settings.");
	WriteChatf("\ay/gmcheck load \ax: \agLoad settings from INI file.");
	WriteChatf("\ay/gmcheck test {enter|leave|remind} \ax: Test alerts & sounds for the indicated type.  e.g.: /gmcheck test leave");
	WriteChatf("\ay/gmcheck ss {enter|leave|remind} SoundFileName \ax: Set the filename (wav/mp3) to play for indicated alert. Full path if sound file is not in your MQ2 dir.");
	WriteChatf("\ay/gmcheck help \ax: \agThis help.");
	WriteChatf("");
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
	WriteChatf("\ayMQ2GMCheck: \amGM alert and reminder sounds %s%s\am.", bGMQuiet ? "temporarily " : "", bGMQuiet ? "\arDISABLED" : "\agENABLED");
}

void GMTest(char* szLine)
{
	char szArg[MAX_STRING], szTmp[MAX_STRING] = { 0 };
	char szMsg[MAX_STRING], szPopup[MAX_STRING];
	if (gGameState != GAMESTATE_INGAME)
	{
		WriteChatf("\ayMQ2GMCheck: \arMust be in game to use /gmcheck test");
		return;
	}
	GetArg(szArg, szLine, 1);
	if (!strncmp(szArg, "enter", 5))
	{
		sprintf_s(szMsg, "(TEST) \arGM %s \ayhas entered the zone at \ar%s", GetCharInfo()->Name, DisplayTime());
		sprintf_s(szPopup, "(TEST) GM %s has entered the zone at %s", GetCharInfo()->Name, DisplayTime());
		WriteChatf(szMsg);
		strcpy_s(szTmp, szGMEnterCmdIf);
		long lResult = MCEval(szTmp);
		WriteChatf("\ayMQ2GMCheck: \at(If first GM entered zone): GMEnterCmdIf evaluates to %s\at.  Plugin would %s \atGMEnterCmd: \am%s",
			lResult ? "\agTRUE" : "\arFALSE", lResult ? (szGMEnterCmd[0] ? (szGMEnterCmd[0] == '/' ? "\agEXECUTE" : "\arNOT EXECUTE") : "\arNOT EXECUTE") : "\arNOT EXECUTE",
			szGMEnterCmd[0] ? (szGMEnterCmd[0] == '/' ? szGMEnterCmd : "<IGNORED>") : "<NONE>");
		if (bGMPopup)
			DisplayOverlayText(szPopup, CONCOLOR_RED, 100, 500, 500, 3000);
		if (!bGMQuiet)
		{
			if (bGMSound)
			{
				GetPrivateProfileString(GetCharInfo()->Name, "EnterSound", "", szTmp, MAX_STRING, INIFileName);
				if (szTmp[0] && _FileExists(szTmp))
					PlayGMSound(szTmp);
				else
					PlayGMSound(szEnterSound);
			}
			if (bGMBeep)
			{
				Beep(0x500, 250);
				Beep(0x3000, 250);
				Beep(0x500, 250);
				Beep(0x3000, 250);
			}
		}
	}
	else if (!strncmp(szArg, "leave", 5))
	{
		sprintf_s(szMsg, "(TEST) \agGM %s \ayhas left the zone at \ag%s", GetCharInfo()->Name, DisplayTime());
		sprintf_s(szPopup, "(TEST) GM %s has left the zone at %s", GetCharInfo()->Name, DisplayTime());
		WriteChatf(szMsg);
		strcpy_s(szTmp, szGMLeaveCmdIf);
		long lResult = MCEval(szTmp);
		WriteChatf("\ayMQ2GMCheck: \at(If last GM left zone): GMLeaveCmdIf evaluates to %s\at.  Plugin would %s \atGMLeaveCmd: \am%s",
			lResult ? "\agTRUE" : "\arFALSE", lResult ? (szGMLeaveCmd[0] ? (szGMLeaveCmd[0] == '/' ? "\agEXECUTE" : "\arNOT EXECUTE") : "\arNOT EXECUTE") : "\arNOT EXECUTE",
			szGMLeaveCmd[0] ? (szGMLeaveCmd[0] == '/' ? szGMLeaveCmd : "<IGNORED>") : "<NONE>");
		if (!bGMQuiet)
		{
			if (bGMSound)
			{
				GetPrivateProfileString(GetCharInfo()->Name, "LeaveSound", "", szTmp, MAX_STRING, INIFileName);
				if (szTmp[0] && _FileExists(szTmp))
					PlayGMSound(szTmp);
				else
					PlayGMSound(szLeaveSound);
			}
			if (bGMBeep)
			{
				Beep(0x600, 250);
				Beep(0x600, 250);
				Beep(0x600, 250);
				Beep(0x600, 250);
			}
		}
		if (bGMPopup)
			DisplayOverlayText(szPopup, CONCOLOR_GREEN, 100, 500, 500, 3000);
	}
	else if (!strncmp(szArg, "remind", 6))
	{
		sprintf_s(szMsg, "(TEST) \arGM ALERT!!  \ayGM in zone.  \at(\ag%s\at)", GetCharInfo()->Name);
		WriteChatf(szMsg);
		if (bGMSound)
		{
			PlayGMSound(szRemindSound);
		}
	}
	else
	{
		WriteChatf("\ayMQ2GMCheck:  \atUsage: \am/gmcheck test {enter|leave|remind}");
	}
}

void GMSS(char* szLine)
{
	char szArg[MAX_STRING], szFile[MAX_STRING] = { 0 };
	bool bOK = false;
	GetArg(szArg, szLine, 1);
	GetArg(szFile, szLine, 2);
	if (!strncmp(szArg, "enter", 5))
	{
		if (szFile[0] && _FileExists(szFile))
		{
			bOK = true;
			strcpy_s(szEnterSound, szFile);
			WriteChatf("\ayMQ2GMCheck: \at'enter' sound set to: \am%s", szEnterSound);
			WriteChatf("\ayMQ2GMCheck: \agDon't forget to use '/gmcheck save' if you want this to be persistant!");
		}
	}
	else if (!strncmp(szArg, "leave", 5))
	{
		if (szFile[0] && _FileExists(szFile))
		{
			bOK = true;
			strcpy_s(szLeaveSound, szFile);
			WriteChatf("\ayMQ2GMCheck: \at'leave' sound set to: \am%s", szLeaveSound);
			WriteChatf("\ayMQ2GMCheck: \agDon't forget to use '/gmcheck save' if you want this to be persistant!");
		}
	}
	else if (!strncmp(szArg, "remind", 5))
	{
		if (szFile[0] && _FileExists(szFile))
		{
			bOK = true;
			strcpy_s(szRemindSound, szFile);
			WriteChatf("\ayMQ2GMCheck: \at'remind' sound set to: \am%s", szRemindSound);
			WriteChatf("\ayMQ2GMCheck: \agDon't forget to use '/gmcheck save' if you want this to be persistant!");
		}
	}
	else
	{
		bOK = true;
		WriteChatf("\ayMQ2GMCheck: \arBad option, usage: \at/gmcheck ss {enter|leave|remind} SoundFileName");
	}
	if (!bOK)
		WriteChatf("\ayMQ2GMCheck: \arSound file not found, setting not changed, tried: \am%s", szFile[0] ? szFile : "(No filename supplied)");
}

void UpdateAlerts()
{
	PSPAWNINFO pSpawn;
	unsigned long Tmp;
	unsigned int index;
	char szTmp[MAX_STRING], szMsg[MAX_STRING], szPopup[MAX_STRING];
	if (!GMNames.size())
		return;
	if (gGameState != GAMESTATE_INGAME)
		return;
	Tmp = GetTickCount();
	if (Tmp < Update_PulseCount + 15000)
		return;
	Update_PulseCount = GetTickCount();
	index = 0;
	while (index < GMNames.size())
	{
		std::string& VectorRef = GMNames[index];
		pSpawn = (PSPAWNINFO)GetSpawnByName((char*)VectorRef.c_str());
		if (pSpawn && pSpawn->GM)
			index++;
		else
		{
			sprintf_s(szMsg, "\agGM %s \ayhas left the zone at \ag%s", VectorRef.c_str(), DisplayTime());
			sprintf_s(szPopup, "GM %s has left the zone at %s", VectorRef.c_str(), DisplayTime());
			if (bGMChatAlert)
				WriteChatf(szMsg);
			if (!(GMNames.size()))
			{
				if (bGMCmdActive)
				{
					strcpy_s(szTmp, szGMEnterCmdIf);
					if (MCEval(szTmp))
					{
						if (szGMLeaveCmd[0])
						{
							if (szGMLeaveCmd[0] == '/')
							{
								DoCommand((GetCharInfo() && GetCharInfo()->pSpawn) ? GetCharInfo()->pSpawn : NULL, szGMLeaveCmd);
								bGMCmdActive = false;
							}
						}
					}
				}
			}
			if (!bGMQuiet)
			{
				if (bGMSound)
				{
					GetPrivateProfileString(VectorRef.c_str(), "LeaveSound", "", szTmp, MAX_STRING, INIFileName);
					if (szTmp[0] && _FileExists(szTmp))
						PlayGMSound(szTmp);
					else
						PlayGMSound(szLeaveSound);
				}
				if (bGMBeep)
				{
					PlaySound(NULL, NULL, SND_NODEFAULT);
					PlaySound("SystemDefault", NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
				}
			}
			if (bGMPopup)
				DisplayOverlayText(szPopup, CONCOLOR_GREEN, 100, 500, 500, 3000);
			GMNames.erase(GMNames.begin() + index);
		}
	}
}

void GMCheckCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	char szArg1[MAX_STRING], szArg2[MAX_STRING];
	GetArg(szArg1, szLine, 1);
	if (!_stricmp(szArg1, "on"))
	{
		bGMCheck = true;
		WriteChatf("\ayMQ2GMCheck: \amGM checking is now %s\am.", bGMCheck ? "\agENABLED" : "\arDISABLED");
	}
	else if (!_stricmp(szArg1, "off"))
	{
		bGMCheck = false;
		WriteChatf("\ayMQ2GMCheck: \amGM checking is now %s\am.", bGMCheck ? "\agENABLED" : "\arDISABLED");
	}
	else if (!_stricmp(szArg1, "quiet"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		GMQuiet(szArg2);
	}
	else if (!_stricmp(szArg1, "sound"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		GMSoundToggle(szArg2);
	}
	else if (!_stricmp(szArg1, "beep"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		GMBeepToggle(szArg2);
	}
	else if (!_stricmp(szArg1, "corpse"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		GMCorpseToggle(szArg2);
	}
	else if (!_stricmp(szArg1, "popup"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		GMPopupToggle(szArg2);
	}
	else if (!_stricmp(szArg1, "chat"))
	{
		strcpy_s(szArg2, GetNextArg(szLine));
		GMChatToggle(szArg2);
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
	else
		GMCheckStatus(true);
}

PLUGIN_API VOID InitializePlugin(VOID)
{
	DebugSpewAlways("Initializing MQ2GMCheck");
	char szTmp[MAX_STRING] = { 0 }, szTemp[MAX_STRING] = { 0 };
	DWORD i;
	float x;
	GetPrivateProfileString("Settings", "LeftVolume", "50", szTmp, MAX_STRING, INIFileName);
	i = atoi(szTmp);
	if (i < 0 || i>100)
		i = 50;
	_itoa_s(i, szTemp, 10);
	WritePrivateProfileString("Settings", "LeftVolume", szTemp, INIFileName);
	x = (float)65535.0 * ((float)i / (float)100.0);
	NewVol = (DWORD)x;
	GetPrivateProfileString("Settings", "RightVolume", "50", szTmp, MAX_STRING, INIFileName);
	i = atoi(szTmp);
	if (i < 0 || i>100)
		i = 50;
	_itoa_s(i, szTemp, 10);
	WritePrivateProfileString("Settings", "RightVolume", szTemp, INIFileName);
	x = (float)65535.0 * ((float)i / (float)100.0);
	NewVol = NewVol + (((DWORD)x) << 16);
	strcpy_s(szLastGMName, "NONE");
	strcpy_s(szLastGMTime, "NEVER");
	strcpy_s(szLastGMDate, "NEVER");
	strcpy_s(szLastGMZone, "NONE");
	GMNames.clear();
	Check_PulseCount = GetTickCount();
	Update_PulseCount = GetTickCount();
	AddMQ2Data("GMCheck", MQ2GMCheckType::dataGMCheck);
	pGMCheckType = new MQ2GMCheckType;
	ReadSettings();
	MMORequiredAccess = AL_Registered;
	AddCommand("/gmcheck", GMCheckCmd);
	if (gGameState == GAMESTATE_INGAME)
		bGMAlert = GMCheck();
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
	WriteChatf("\ayMQ2GMCheck: \amUnloading plugin.");
	DebugSpewAlways("Shutting down MQ2GMCheck");
	RemoveCommand("/gmcheck");
	GMNames.clear();
	RemoveMQ2Data("GMCheck");
	delete pGMCheckType;
	if (bVolSet)
		waveOutSetVolume(NULL, dwVolume);
}

PLUGIN_API VOID OnPulse(VOID)
{
	char szTmp[MAX_STRING] = { 0 }, szNames[MAX_STRING];
	if (bVolSet && StopSoundTimer && GetTickCount() >= StopSoundTimer)
	{
		StopSoundTimer = 0;
		waveOutSetVolume(NULL, dwVolume);
	}
	unsigned int Tmp;
	if (!Reminder_Interval)
		return;
	if (gGameState == GAMESTATE_INGAME)
	{
		UpdateAlerts();
		Tmp = GetTickCount();
		if (Tmp >= Check_PulseCount + Reminder_Interval && Reminder_Interval)
		{
			Check_PulseCount = GetTickCount();
			if (bGMAlert = GMCheck())
			{
				if (!bGMQuiet)
				{
					if (!GMNames.size())
						return;
					if (!bGMCheck)
						return;
					szNames[0] = 0;
					for (unsigned int x = 0; x < GMNames.size(); x++)
					{
						if (strlen(szNames) > 500)
						{
							strcat_s(szNames, " ...");
							break;
						}
						std::string& VectorRef = GMNames[x];
						if (szNames[0])
							strcat_s(szNames, "\am, ");
						strcat_s(szNames, "\ag");
						strcat_s(szNames, VectorRef.c_str());
					}
					sprintf_s(szTmp, "\arGM ALERT!!  \ayGM in zone.  \at(%s\at)", szNames);
					if (bGMChatAlert)
						WriteChatf(szTmp);
					if (bGMSound)
					{
						PlayGMSound(szRemindSound);
					}
				}
			}
		}
	}
}

PLUGIN_API VOID OnAddSpawn(PSPAWNINFO pSpawn)
{
	char szTmp[MAX_STRING] = { 0 };
	char szMsg[MAX_STRING], szPopup[MAX_STRING];
	if (bGMCheck)
	{
		if (pSpawn)
		{
			if (pSpawn->GM && !IsGMCorpse(pSpawn->Type))
			{
				if (!strlen(pSpawn->DisplayedName))
					return;
				GMNames.push_back(pSpawn->DisplayedName);
				strcpy_s(szLastGMName, pSpawn->DisplayedName);
				strcpy_s(szLastGMTime, DisplayTime());
				strcpy_s(szLastGMDate, DisplayDate());
				strcpy_s(szLastGMZone, "UNKNOWN");
				if (pLocalPC)
				{
					int zoneid = (pLocalPC->zoneId & 0x7FFF);
					if (zoneid <= MAX_ZONES)
					{
						strcpy_s(szLastGMZone, pWorldData->ZoneArray[zoneid]->LongName);
					}
				}
				sprintf_s(szMsg, "\arGM %s \ayhas entered the zone at \ar%s", pSpawn->DisplayedName, DisplayTime());
				sprintf_s(szPopup, "GM %s has entered the zone at %s", pSpawn->DisplayedName, DisplayTime());
				if (bGMChatAlert)
					WriteChatf(szMsg);
				if (!bGMCmdActive)
				{
					strcpy_s(szTmp, szGMEnterCmdIf);
					if (MCEval(szTmp))
					{
						if (szGMEnterCmd[0])
						{
							if (szGMEnterCmd[0] == '/')
							{
								DoCommand((GetCharInfo() && GetCharInfo()->pSpawn) ? GetCharInfo()->pSpawn : NULL, szGMEnterCmd);
								bGMCmdActive = true;
							}
						}
					}
				}
				if (bGMPopup)
					DisplayOverlayText(szPopup, CONCOLOR_RED, 100, 500, 500, 3000);
				if (!bGMQuiet)
				{
					if (bGMSound)
					{
						GetPrivateProfileString(pSpawn->DisplayedName, "EnterSound", "", szTmp, MAX_STRING, INIFileName);
						if (szTmp[0] && _FileExists(szTmp))
							PlayGMSound(szTmp);
						else
							PlayGMSound(szEnterSound);
					}
					if (bGMBeep)
					{
						PlaySound(NULL, NULL, SND_NODEFAULT);
						PlaySound("SystemAsterisk", NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
					}
				}
			}
		}
	}
}

PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn)
{
	char szTmp[MAX_STRING] = { 0 };
	char szMsg[MAX_STRING], szPopup[MAX_STRING];
	bool GMFound = false;
	if (bGMCheck)
	{
		if (pSpawn)
		{
			if (pSpawn->GM)
			{
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
				sprintf_s(szMsg, "\agGM %s \ayhas left the zone at \ag%s", pSpawn->DisplayedName, DisplayTime());
				sprintf_s(szPopup, "GM %s has left the zone at %s", pSpawn->DisplayedName, DisplayTime());
				if (bGMChatAlert)
					WriteChatf(szMsg);
				if (!(GMNames.size()))
				{
					if (bGMCmdActive)
					{
						strcpy_s(szTmp, szGMEnterCmdIf);
						if (MCEval(szTmp))
						{
							if (szGMLeaveCmd[0])
							{
								if (szGMLeaveCmd[0] == '/')
								{
									DoCommand((GetCharInfo() && GetCharInfo()->pSpawn) ? GetCharInfo()->pSpawn : NULL, szGMLeaveCmd);
									bGMCmdActive = false;
								}
							}
						}
					}
				}
				if (!bGMQuiet)
				{
					if (bGMSound)
					{
						GetPrivateProfileString(pSpawn->DisplayedName, "LeaveSound", "", szTmp, MAX_STRING, INIFileName);
						if (szTmp[0] && _FileExists(szTmp))
							PlayGMSound(szTmp);
						else
							PlayGMSound(szLeaveSound);
					}
					if (bGMBeep)
					{
						PlaySound(NULL, NULL, SND_NODEFAULT);
						PlaySound("SystemDefault", NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
					}
				}
				if (bGMPopup)
					DisplayOverlayText(szPopup, CONCOLOR_GREEN, 100, 500, 500, 3000);
			}
		}
	}
}

PLUGIN_API VOID OnEndZone(VOID)
{
	GMNames.clear();
}

PLUGIN_API VOID OnZoned(VOID)
{
	bGMQuiet = FALSE;
}
