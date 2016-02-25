#pragma once
#include <Windows.h>
#include <stdio.h>
#include <string>
#include <functional>
#include <map>
#include <gl\GL.h>
#include <Awesomium\WebCore.h>
#include <Awesomium\BitmapSurface.h>
#include <Awesomium\STLHelpers.h>

#define MOUSE_EVENT_LDOWN  0
#define MOUSE_EVENT_LUP	   1

// GUI Commands
#define GUI_CMD_SHOW	1
#define GUI_CMD_CLOSE	0
#define GUI_CMD_DATA	2

using namespace std;
using namespace Awesomium;

class WebGUI : public JSMethodHandler
{
public:
	WebGUI(FILE *logFile);
	~WebGUI();

	// Init
	void Init(HWND *parent, string resources = "webui", string layoutfile = "");

	// Load new layout
	void LoadLayout(string layout);

	// Draw
	void Draw();

	// Update
	void Update();

	// Send mouse click
	unsigned char MouseLClick(int x, int y);

	// Inject mouse click
	void SignalMouseEvent(unsigned char type, int x, int y);

	// Server messages handler
	int ExecGuiCmd(const char *buffer, int length);

	// Execute data render passing json string
	// NB : json args need to be a string, it won't
	// handle object as it's.
	// Define a js function called function WebUISetData(args);
	void ExecuteSetData(const char *json);

	// JS Methods binding
	void OnClose();

	// Getters
	GLdouble GetWidth() { return this->width; };
	GLdouble GetHeight() { return this->height; }
	GLdouble GetX() { return this->x; };
	GLdouble GetY() { return this->y; };
	string GetResources() { return this->resources; };
	string GetLayout() { return this->layoutfile; };
	GLubyte *GetTexture() { return this->texture; };
	bool IsActive() { return active; };
	bool IsHidden() { return hidden; }

	// Setters
	void SetWidth(GLdouble width) { this->width = width; };
	void SetHeight(GLdouble height) { this->height = height; };
	void SetX(GLdouble x) { this->x = x; };
	void SetY(GLdouble y) { this->y = y; };
	void SetResources(string resources) { this->resources = resources; }
	void SetActive(bool active) { this->active = active; };
	void SetHidden(bool hidden) { this->hidden = hidden; }

private:
	// Js methods binding def
	typedef pair<unsigned int, WebString> JSCallerKey;
	
	// Log file
	FILE *logFile;
	// Width, height
	GLdouble width, height;
	// Orthoaxis
	GLdouble x, y;
	// Resourcs path
	string resources;
	// HTML GUI file nme
	string layoutfile;

	WebCore *webcore;
	WebView *webview;

	// Windowed mode window
	HWND *renderhwnd;

	// Status
	bool active = false;
	// Hidden
	bool hidden = false;

	// Texture
	GLubyte *texture;
	// Texture name
	GLuint texname;

	// Update flag for surface changes
	bool redraw = false;

	// Global method handler
	void OnMethodCall(WebView * caller, unsigned int remoteObjectId, WebString const & methodName, JSArray const & args);
	JSValue OnMethodCallWithReturnValue(Awesomium::WebView *caller, unsigned int remoteObjectId, WebString const & methodName, Awesomium::JSArray const & args);

	// Methods, draw and stuff
	void DrawProjectionOrtho();
	// Prospettiva
	void DrawPerspective();
	// Bin methods
	void BindMethods();

};

