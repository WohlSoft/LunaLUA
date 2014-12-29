#include "mciEmulator.h"
#include <vector>
#include "Globals.h"

MciEmulator::MciEmulator(void)
{
}


MciEmulator::~MciEmulator(void)
{
}

MCIERROR MciEmulator::mciEmulate(__in LPCSTR lpstrCommand, __out_ecount_opt(uReturnLength) LPSTR lpstrReturnString, __in UINT uReturnLength, __in_opt HWND hwndCallback)
{
	std::string cmd = lpstrCommand;
	std::vector<std::string> spCmd = split(cmd,' ');
	
	if(spCmd.size() == 2){
		if(spCmd[0] == "pause" && spCmd[1] == "all"){
			//Add pause code

		}else if(spCmd[0] == "close"){
			std::map<std::string, regSoundFile>::iterator it = registeredFiles.find(spCmd[1]);
			if(it != registeredFiles.end()){
				//remove registration
				registeredFiles.erase(it);
			}
		}else if(spCmd[0] == "stop"){
			std::map<std::string, regSoundFile>::iterator it = registeredFiles.find(spCmd[1]);
			if(it != registeredFiles.end()){
				//do stop code
			}
		}
	}else if(spCmd.size() == 3){
		if(spCmd[0] == "Status"){
			std::map<std::string, regSoundFile>::iterator it = registeredFiles.find(spCmd[1]);
			if(it != registeredFiles.end()){
				if(spCmd[2] == "Position"){

				}else if(spCmd[2] == "Length"){

				}
			}
		}
	}else if(spCmd.size() == 4){
		if(spCmd[0] == "open" && spCmd[2] == "alias"){
			//register music/sound file
			regSoundFile snFile;
			snFile.fileName = std::string(spCmd[1]);
			snFile.volume = 400;
			registeredFiles[spCmd[3]] = snFile;
		}else if(spCmd[0] == "play" && spCmd[2] == "from"){
			std::map<std::string, regSoundFile>::iterator it = registeredFiles.find(spCmd[1]);
			if(it != registeredFiles.end()){
				//play code
			}
		}
	}else if(spCmd.size() == 5){
		if(spCmd[0] == "setaudio" && spCmd[2] == "volume" && spCmd[3] == "to"){
			if(registeredFiles.find(spCmd[1])!=registeredFiles.end()){
				if(is_number(spCmd[4])){
					//set audio volume
					registeredFiles[spCmd[1]].volume = atoi(spCmd[4].c_str());
				}
			}
		}
	}

	

errorFinalize:;
	if(uReturnLength < 2){
		return MCIERR_UNSUPPORTED_FUNCTION;
	}
	lpstrReturnString[0] = '0';
	lpstrReturnString[1] = '\0';
	return 0;
}
