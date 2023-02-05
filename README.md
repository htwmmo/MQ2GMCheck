# MQ2GMCheck

Alerts when GM in zone, as long as they are not in "stealth"

# Features

This plugin has the ability to alert you in different ways such as sound and visual messages if a GM is detected.

## Getting Started

```
/plugin MQ2GMCheck
```

### Commands

MQ2GMCheck provides one command with several options:

```
/gmcheck
```

<span style="color: blue;">/gmcheck status</span> : <span style="color: green;">Displays plugin status and options.</span>  
<span style="color: blue;">/gmcheck [off|on]</span> : <span style="color: green;">Toggles gm checking on/off, or force on/off.</span>  
<span style="color: blue;">/gmcheck quiet [off|on]</span> : <span style="color: green;">Toggles all gm alert and reminder sounds, or force on/off (resets at next zone).</span>  
<span style="color: blue;">/gmcheck sound [off|on]</span> : <span style="color: green;">Toggles playing the sound files for alerts, or force on/off.</span>  
<span style="color: blue;">/gmcheck beep [off|on]</span> : <span style="color: green;">Toggles making beeping sounds for alerts, or force on/off.</span>  
<span style="color: blue;">/gmcheck popup [off|on]</span> : <span style="color: green;">Toggles showing a popup msg for alerts, or force on/off.</span>  
<span style="color: blue;">/gmcheck chat [off|on]</span> : <span style="color: green;">Toggles showing gm alerts in the mq2 chat window, or force on/off.</span>  
<span style="color: blue;">/gmcheck corpse [off|on]</span> : <span style="color: green;">Toggles filtering of alerts for GM corpses, or force on/off.</span>  
<span style="color: blue;">/gmcheck rem ##</span> : <span style="color: green;">Change alert reminder interval, in seconds (0 to disable).</span>  
<span style="color: blue;">/gmcheck save</span> : <span style="color: green;">Saves current options to MQ2GMCheck.ini</span>  
<span style="color: blue;">/gmcheck load</span> : <span style="color: green;">Load settings from MQ2GMCheck.ini</span>  
<span style="color: blue;">/gmcheck test [enter|leave|remind]</span> : <span style="color: green;">Tests alerts & sounds for the indicated type.</span>  
<span style="color: blue;">/gmcheck ss [enter|leave|remind] SoundFileName</span> : <span style="color: green;">Set the filename (wav/mp3) to play for indicated alert. Full path if sound file is not in your MQRoot\Resources\Sounds dir.</span>  
<span style="color: blue;">/gmcheck Zone</span> : <span style="color: green;">history of GM's in this zone.</span>
<span style="color: blue;">/gmcheck Server</span> : <span style="color: green;">history of GM's on this server.</span>
<span style="color: blue;">/gmcheck All</span> : <span style="color: green;">history of GM's on all servers.</span>
<span style="color: blue;">/gmcheck help</span> : <span style="color: green;">Shows command syntax and help.</span>

### Configuration File

The MQ2GMCheck.ini file can have the following options, some of which correspond to parameters available via slash commands in game:

`[Settings]`

GMCheck - GM checking is active by default, on plugin load.  
Sound - Play sound files for alerts.  
Beep - Beep the PC speaker for alerts (or play PC beep).  
Popup - Show popup overlays for alerts.  
Corpse - Exclude GM corpses from alerts.  
RemInt - Number of seconds for reminder interval.  
EnterSound - Alert enter sound filename.  
LeaveSound - Alert leave sound filename.  
RemindSound - Alert reminder sound filename.  
GMEnterCmd - Command to execute when 1st GM enters zone.  
GMEnterCmdIf - Optional evaluation to fine tune GMEnterCmd.  
GMLeaveCmd - Command to execute when last GM exits zone.  
GMLeaveCmdIf - Optional evaluation to fine tune GMLeaveCmd.  

In addition, you can have a Section Name corresponding to a GM name, and those custom enter/leave sounds will be played for that GM instead:

`[GMFirstName]`

EnterSound - GM enter sound filename for this GM.  
LeaveSound - GM leave sound filename for this GM.

### Sample Configuration

`MQ2GMCheck.ini`

[Settings]  
GMCheck=on  
GMSound=on  
GMBeep=off  
GMPopup=on  
GMChat=on  
GMCorpse=off  
RemInt=30  
EnterSound=c:\mq\resources\sounds\gmenter.mp3  
LeaveSound=c:\mq\resources\sounds\gmleave.mp3  
RemindSound=c:\mq\resources\sounds\gmremind.mp3  
GMEnterCmdIf=${If[${Zone.ID}==344 || ${Zone.ID}==345,1,0]}  
GMEnterCmd=/multiline ; /end ; /camp desktop  
GMLeaveCmdIf=${If[${Zone.ID}==344 || ${Zone.ID}==345,1,0]}  
GMLeaveCmd=  

[Deodan]  
EnterSound=c:\mq\resources\sounds\prickishere.wav  
LeaveSound=c:\mq\resources\sounds\thankgod.wav

Finally, your INI stores a history of GM names you've encountered in your travels.
[GM] section lists all GMs you've encountered and in what zone.
[ServerName] section will list all GMs you've encountered in the corresponding server
[Server-Zone] section will list all GMs you've encountered in a specific zone on a server

## Authors

* **htw** - *Initial work*
* **ChatWithThisName** - *Refactor and merge two versions features together*
