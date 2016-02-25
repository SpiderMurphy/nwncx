#include "WebGUI.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


using namespace rapidjson;

// Init
void WebGUI::Init(HWND *parent, string resources, string layoutfile) {
	// Set renderer hwnd
	this->renderhwnd = parent;

	// Set webcore
	this->webcore = WebCore::Initialize(WebConfig());
	this->webview = this->webcore->CreateWebView(this->width, this->height);
	this->resources = resources;
	this->layoutfile = layoutfile;

	this->BindMethods();
}

void WebGUI::LoadLayout(string layout) {
	// Get Path
	char buffer[MAX_PATH];

	memset(buffer, 0, MAX_PATH);

	strcpy(buffer, "file:///./webui");
	strcat(buffer, "/");
	strcat(buffer, layout.c_str());
	strcat(buffer, ".html");

	// Load url into webview
	webview->LoadURL(WebURL(WSLit(buffer)));
	webview->SetTransparent(true);

	while (webview->IsLoading())
		webcore->Update();

	webcore->Update();
	Sleep(200);

	layoutfile = string(buffer);

	// Set redraw
	redraw = true;
}

void WebGUI::Update() {
	webcore->Update();
}

void WebGUI::SignalMouseEvent(unsigned char type, int x, int y) {
	webview->InjectMouseMove(x, y);
	webview->InjectMouseDown(kMouseButton_Left);
	webview->InjectMouseUp(kMouseButton_Left);
}

unsigned char WebGUI::MouseLClick(int x, int y) {
	// Return to main handler?
	unsigned char fired = false;

	BitmapSurface *surface = (BitmapSurface*)webview->surface();

	fired = surface->GetAlphaAtPoint(x, y);

	// Exit
	return fired;
}

void WebGUI::Draw() {
	// If gui is hidden exit draw
	if (hidden)
		return;

	// Check redraw to load texture from surface
	if (redraw) {
		// Realloc texture
		texture = new GLubyte[4 * (int)width*(int)height];
		memset(texture, 0, 4 * (int)width*(int)height);

		// Copy texture
		BitmapSurface *surface = (BitmapSurface*)webview->surface();

		if (surface != 0) {
			// Copy surface raw bits
			surface->CopyTo(texture, (int)width * 4, 4, false, true);

			// Remove old texture
			if (texname)
				glDeleteTextures(1, &texname);

			// Generate texture
			glGenTextures(1, &texname);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glBindTexture(GL_TEXTURE_2D, texname);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, texture);

			delete[] texture;
			redraw = false;

		}
	}
	else {
		if (texname) {
			webcore->Update();

			// Copy texture
			BitmapSurface *surface = (BitmapSurface*)webview->surface();

			if (surface->is_dirty()) {
				redraw = true;
			}
		}
	}

	if (texname) {
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		DrawProjectionOrtho();

		if (active) {
			glBegin(GL_QUADS);

			glTexCoord2f(0, 1);
			glVertex2f(0, 0);
			glTexCoord2f(0, 0);
			glVertex2f(0, height);
			glTexCoord2f(1, 0);
			glVertex2f(width, height);
			glTexCoord2f(1, 1);
			glVertex2f(width, 0);

			glEnd();
		}
		DrawPerspective();
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}
	
}

void WebGUI::BindMethods() {
	// JS Global listener
	JSValue jsGlobal = webview->CreateGlobalJavascriptObject(WSLit("WebUI"));

	if (jsGlobal.IsObject()) {
		// UI methods binding / app object
		JSObject& ui = jsGlobal.ToObject();
		ui.SetCustomMethod(WSLit("Close"), false);
	}

	// Bind dispatcher
	webview->set_js_method_handler(this);
}

void WebGUI::DrawProjectionOrtho() {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, this->x, this->y, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

void WebGUI::DrawPerspective() {
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

WebGUI::WebGUI(FILE *logFile)
{
	this->logFile = logFile;
}


WebGUI::~WebGUI()
{
}



//************************************************************************************************
//* GUI cmd
//************************************************************************************************

// Gui command parser
int WebGUI::ExecGuiCmd(const char *buffer, int length) {
	// Executed
	int exec = 1;

	// Exit
	return exec;
}


//************************************************************************************************
//* JS Method handlers
//************************************************************************************************

// Global Method callback dispatcher
void WebGUI::OnMethodCall(Awesomium::WebView *caller, unsigned int remoteObjectId, WebString const & methodName, Awesomium::JSArray const & args) {
	// Check method
	if (methodName.Compare(WSLit("Close")) == 0) {
		this->OnClose();
	}
}

JSValue WebGUI::OnMethodCallWithReturnValue(Awesomium::WebView *caller, unsigned int remoteObjectId, WebString const & methodName, Awesomium::JSArray const & args) {
	return JSValue::Undefined();
}

void WebGUI::ExecuteSetData(const char *json) {
	// Get window
	JSValue window = webview->ExecuteJavascriptWithResult(WSLit("window"), WSLit(""));

	if (window.IsObject()) {
		// Call WebUISetData
		JSArray args;
		args.Push(JSValue(WSLit(json)));

		window.ToObject().Invoke(WSLit("WebUISetData"), args);
	}
}

// Custom methods
void WebGUI::OnClose() {
	this->SetActive(false);
}