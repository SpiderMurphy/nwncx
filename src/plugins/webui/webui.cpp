/*
	Copyright(C) 2016  Cyan, based on nwn client extender by virusman.
	Net parser by skywing.

	This program is free software : you can redistribute it and / or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.If not, see <http://www.gnu.org/licenses/>.
*/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <map>
#include <string>
#include <pluginapi.h>
#include "detours.h"
#include <gl\GL.h>
#include "WebGUI.h"
#include <nwnapi\nwnapi.h>
#include <nwnapi\CNWMessage.h>
#include <nwnapi\custom\nwn_globals.h>
#include <nwnapi\custom\nwn_globals.cpp>
#include <Skywing\NetBuffer\NetBuffer.h>
#include <Skywing\Protocol\Protocol.h>
#include "GUIMessages.h"

// Some message type (just guess it, still working on)
#define		NWMESSAGE_TYPE_LOAD			0x4
#define		NWMESSAGE_SUB_AREA			0x1
#define		NWMESSAGE_TYPE_MOVEMENT		0x5
#define		NWMESSAGE_SUB_STOP			0x2

using namespace std;
using namespace Awesomium;


//***************************************************************************************************
//* Entry point Init and nwncx stuff
//***************************************************************************************************

// NWNCX Init method
void InitPlugin();

// Hook functions
void HookFunctions();


//***************************************************************************************************
//* Hook prototypes
//***************************************************************************************************

// GL Swap buffers
typedef BOOL(__stdcall *HOOK_GL_SWAPBUFFERS)(HDC, UINT);


//***************************************************************************************************
//* Hook addresses
//***************************************************************************************************

// Open GL swap buffers for custom rendering
HOOK_GL_SWAPBUFFERS o_wglSwapLayerBuffers;

void(__thiscall *CGuiMan__HandleMouseLButton)(void *pThis, int a1);

// Server messages dispatcher
int(__fastcall *CNWCMessage__HandleServerToPlayerMessage)(CNWMessage *pMessage, int edx, unsigned char *pData, int nLength);

HWND(__cdecl *GetHWnd)();

// Internals
HWND *g_hWnd = (HWND *)0x0092DC28;
HWND *g_hRenderWnd = (HWND *)0x0092DC2C;

// X, Y mouse coordinates
int mx, my;

// Hinstance
HINSTANCE hinstance;

// Mouse hook address
HHOOK MouseActionsHook;


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Plugin info / link
/////////////////////////////////////////////////////////////////////////////////////////////////////
PLUGINLINK *pluginLink = 0;
PLUGININFO pluginInfo = {
	sizeof(PLUGININFO),
	"NWNCX WEB UI",
	PLUGIN_MAKE_VERSION(0,0,0,2),
	"Web UI extender plugin for nwncx, check also nwnx webui for server implementation",
	"Cyan",
	"cyan@gmail.com",
	"© 2016 cyan",
	"",
	0		//not transient
};


extern "C" __declspec(dllexport) PLUGININFO* GetPluginInfo(DWORD nwnxVersion)
{
	return &pluginInfo;
}

extern "C" __declspec(dllexport) int InitPlugin(PLUGINLINK *link)
{
	pluginLink = link;
	InitPlugin();
	return 0;
}


//**************************************************************************************************
//* Variables, const and stuff
//**************************************************************************************************

// Context by pixelformat
map<int, HGLRC> swap_rc_pixelformat;

// Last hooked rc
HGLRC hook_last_rc_created = NULL;


//**************************************************************************************************
// System related stuff / hook functions
//**************************************************************************************************

// Log File and name
FILE *logFile;
char logFileName[] = "logs/nwncx_webui.txt";

// Temp
char *buttonText = "Webui";
char *icon = "NWNX2";

// UI interface
WebGUI webui(logFile);



//**************************************************************************************************
//* Dummy / test functions
//**************************************************************************************************

void dump_packet(const unsigned char *buffer, unsigned char *dest, int length) {
	for (int i = 0; i < length; i++) {
		char chr = buffer[i];
		
		if (chr == 0)
			chr = '.';

		dest[i] = chr;
	}
}



//**************************************************************************************************
//* Hook functions
//**************************************************************************************************


int OnClientLoaded(WPARAM wParam, LPARAM lParam)
{
	fprintf(logFile, "Game loaded\n");
	fflush(logFile);
	return 1;
}

int OnPluginsLoaded(WPARAM wParam, LPARAM lParam)
{
	fprintf(logFile, "Plugins loaded\n");
	HookEvent("NWClient/ExoApp/Initialized", OnClientLoaded);
	return 1;
}

void InitPlugin() {
	logFile = fopen(logFileName, "w");
	fprintf(logFile, "NWN Client Extender 0.1 - WEBUI plugin\n");
	fprintf(logFile, "(c) 2016 by cyan\n");

	HookFunctions();
	HookEvent("NWNX/Core/PluginsLoaded", OnPluginsLoaded);
	
	fflush(logFile);
}

// Open GL wrapper / swap layers
BOOL __stdcall hook_wglSwapLayerBuffers(HDC context, UINT layer) {
	// Context not initialized
	if (!context) {
		return o_wglSwapLayerBuffers(context, layer);
	}

	// Format rc
	int const px_format = GetPixelFormat(context);
	// Hook context
	HGLRC hook_rc;

	// Get hooked context count / none created
	if (0 == swap_rc_pixelformat.count(px_format)) {
		fprintf(logFile, "Open GL context init\n");
		fflush(logFile);

		// Create context
		hook_rc = wglCreateContext(context);

		// Something wrong
		if (!hook_rc) {
			return o_wglSwapLayerBuffers(context, layer);
		}

		// Save rc
		swap_rc_pixelformat[px_format] = hook_rc;

		// if rc was created share all things (VBO, PBO and stuff)
		if (hook_last_rc_created) {
			wglShareLists(hook_last_rc_created, hook_rc);
		}

		RECT rend;

		GetClientRect(*g_hRenderWnd, &rend);

		long w = rend.right - rend.left;
		long h = rend.bottom - rend.top;

		webui.SetX(w);
		webui.SetY(h);
		webui.SetWidth(w);
		webui.SetHeight(h);
		// set ui
		webui.Init(NULL, "webui");

		hook_last_rc_created = hook_rc;
	}
	else
		// Set same formt saved rc
		hook_rc = swap_rc_pixelformat[px_format];

	// Host rc
	HGLRC const host_rc = wglGetCurrentContext();
	// Swap context
	wglMakeCurrent(context, hook_rc);

	// ........ CUSTOM RENDERING HERE
	webui.Draw();

	wglMakeCurrent(context, host_rc);

	// Return original
	return o_wglSwapLayerBuffers(context, layer);
}


void __fastcall CGuiMan__HandleMouseLButton_Hook(void *pThis, void *trash, int a1) {
	// Check transparency
	if (webui.IsActive()) {
		POINT point;
		if (GetCursorPos(&point)) {
			if (ScreenToClient(*g_hRenderWnd, &point)) {
				fprintf(logFile, "Screen to client coords, x %d, y %d\n", point.x, point.y);
				fflush(logFile);
			}
		}

		unsigned char opacity = webui.MouseLClick(point.x, point.y);

		if(opacity == 0)
			CGuiMan__HandleMouseLButton(pThis, a1);
		else {
			webui.SignalMouseEvent(0, point.x, point.y);
		}

		fprintf(logFile, "Opacity %d\n", opacity);
		fflush(logFile);

		// TODO : Gui logic
	}
	else
		CGuiMan__HandleMouseLButton(pThis, a1);
}

int __fastcall CNWCMessage__HandleServerToPlayerMessage_Hook(CNWMessage *pMessage, int edx, unsigned char *pData, int nLength)
{
	int nType = pData[1];
	int nSubtype = pData[2];
	pMessage->SetReadMessage(pData + 3, nLength - 3, -1, 1);
	fprintf(logFile, "Message: type %x, subtype %x\n", nType, nSubtype);


	/*
		* Check protocol message to hide / show GUI like area change and
		* kinda messages (trying to find a better way), however the gui
		* behaviors should be done  by nwscript except minimal client related
		* stuff
	*/

	// Get sent message
	NWN::ProtocolMessage *message = new NWN::ProtocolMessage(pData, nLength);

	// Load AREA / hide gui if active
	if (message->GetMajorFunction() == NWMESSAGE_TYPE_LOAD && message->GetMinorFunction() == NWMESSAGE_SUB_AREA) {
		// Check if gui is active
		if (webui.IsActive()) {
			webui.SetHidden(TRUE);
		}
	}
	// I guess this message is related to player movements, being called everytime load
	// area complete is a good starting point to respawn previously hidden gui
	else if (message->GetMajorFunction() == NWMESSAGE_TYPE_MOVEMENT && message->GetMinorFunction() == NWMESSAGE_SUB_STOP) {
		// Check if gui is active and hidden
		if (webui.IsActive() && webui.IsHidden()) {
			webui.SetHidden(FALSE);
		}
	}
	// Testing custom nwmessage to drive gui
	else if (message->GetMajorFunction() == WEBUI_MAJOR_MESSAGE && message->GetMinorFunction() == WEBUI_MINOR_MESSAGE) {
		NWN::ExoParseBuffer *parser = message->GetParser();

		if (parser != NULL) {
			// Function type
			int cmd;
			string data;

			parser->ReadINT(cmd);
			parser->ReadCExoString(data);
			
			fprintf(logFile, "Command gui %d, %s\n", cmd, data.c_str());

			// Load and show GUI
			if (cmd == GUI_CMD_SHOW) {
				webui.LoadLayout(data);
				webui.SetActive(true);
			}
			// Close gui
			else if (cmd == GUI_CMD_CLOSE) {
				webui.SetActive(false);
			}
			// Callback to set data
			else if (cmd == GUI_CMD_DATA) {
				if (!webui.IsActive())
					return CNWCMessage__HandleServerToPlayerMessage(pMessage, edx, pData, nLength);
				
				// Set data
				webui.ExecuteSetData(data.c_str());
			}
		}

	}

	fflush(logFile);
	
	return CNWCMessage__HandleServerToPlayerMessage(pMessage, edx, pData, nLength);
}

// Hook functions
void HookFunctions() {
	// Error code
	LONG error;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	*(DWORD*)&CGuiMan__HandleMouseLButton = 0x0042AD70;
	error = DetourAttach(&(LPVOID&)CGuiMan__HandleMouseLButton, CGuiMan__HandleMouseLButton_Hook);
	fprintf(logFile, "HandleMouseLButton hook: %d\n", error);

	*(DWORD*)&CNWCMessage__HandleServerToPlayerMessage = 0x00452420;
	error = DetourAttach(&(LPVOID&)CNWCMessage__HandleServerToPlayerMessage, CNWCMessage__HandleServerToPlayerMessage_Hook);
	fprintf(logFile, "HandleServerToPlayerMessage hook %d\n", error);

	// Hook wglSwapLayerBuffers
	o_wglSwapLayerBuffers = (HOOK_GL_SWAPBUFFERS)GetProcAddress(GetModuleHandleA("opengl32.dll"), "wglSwapLayerBuffers");
	DetourAttach(&(LPVOID&)o_wglSwapLayerBuffers, hook_wglSwapLayerBuffers);

	error = DetourTransactionCommit();

	if (error == NO_ERROR)
		fprintf(logFile, "WebUI Hook success\n");
	else
		fprintf(logFile, "WebUI Hook errors %d\n", error);
}



//**************************************************************************************************
//* DLL entry /not used
//**************************************************************************************************

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		//hinstance = hModule;
		//InitPlugin();
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		//delete plugin;
	}
	return TRUE;
}
