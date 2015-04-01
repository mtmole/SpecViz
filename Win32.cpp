// SpecViz.cpp : Defines the entry point for the application.
//

#include "SpecViz.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <windowsx.h>

#include <tchar.h>
#include "resource.h"
#include "gl/wglew.h"
 
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "glew\\lib\\Release\\Win32\\glew32.lib")
 
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HGLRC glContext = NULL;							// OpenGL context
HDC glDC = 0;									// OpenGL device context
HWND gWnd = 0;
Viewer* currentViewer = NULL;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPTSTR    lpCmdLine,
					 int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SPECVIZ, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SPECVIZ));
	
	// Start high resolution timer
	LARGE_INTEGER intoFreq;
	QueryPerformanceFrequency(&intoFreq);

	LARGE_INTEGER lastTime;
	QueryPerformanceCounter(&lastTime);

	// Main message loop:
	while (gWnd) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} else if (glContext) {
			LARGE_INTEGER curTime;
			QueryPerformanceCounter(&curTime);

			double deltaTime = (double) (curTime.QuadPart - lastTime.QuadPart) / (double) intoFreq.QuadPart;
			if (currentViewer) {
				currentViewer->MainLoop((float) deltaTime);
				SwapBuffers(glDC);
			}

			lastTime = curTime;
		}
	};

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPECVIZ));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SPECVIZ);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   gWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!gWnd)
   {
	  return FALSE;
   }

   ShowWindow(gWnd, nCmdShow);
   UpdateWindow(gWnd);

   return TRUE;
}

// Set up and show open file dialog and return result
bool OpenFile(char* intoBuffer, char* fileFilter, bool isSave = false) {
	char dir[512];
	GetCurrentDirectory(512, dir);

	OPENFILENAME ofn ;

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = gWnd;
	ofn.lpstrFile = intoBuffer;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = fileFilter;
	ofn.nFilterIndex = 0;
	ofn.lpstrInitialDir = dir;

	if (isSave) {
		ofn.lpstrTitle = "Save file";
		ofn.lpstrDefExt = "prj";
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;
		GetSaveFileName(&ofn);
	} else {
		ofn.lpstrTitle = "Select file";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;
		GetOpenFileName(&ofn);
	}


	// make sure the current directory stays the same so our relative path names still work
	SetCurrentDirectory(dir);

	if (strlen(intoBuffer) == 0) return false;

	return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
			case ID_CREATETEXTUREPROJECTION:
			{
				if (currentViewer) {
					delete currentViewer;
					currentViewer = NULL;
				}
				char textureFile[512];
				char modelFile[512];
				if (OpenFile(textureFile, "Color Map Files\0*.ppm;*.png;*.jpg\0")) {
					if (OpenFile(modelFile, "PLY Files\0*.ply\0")) {
						currentViewer = CreateCreateProjViewer(textureFile, modelFile);
					}
				}
				break;
			}
			case ID_SAVETEXTUREPROJ:
			{
				if (currentViewer) {
					char projFile[512];
					if (OpenFile(projFile, "Projection Files\0*.prj\0", true)) {
						currentViewer->Save(projFile);
					}
				}
				break;
			}
			case ID_CREATEDEPTHFIELD:
			{
				char projFile[512];
				if (OpenFile(projFile, "Projection Files\0*.prj\0")) {
					CreateDepthField(projFile);
					MessageBox(hWnd, "Depth Field created.", "Done", MB_OK);
				}
				break;
			}
			case ID_OPENMODEL:
			{
				if (currentViewer) {
					delete currentViewer;
					currentViewer = NULL;
				}
				char file[512];
				if (OpenFile(file, "PLY Files\0*.ply\0")) {
					currentViewer = CreateModelViewer(file);
				}
				break;
			}
			case ID_OPENNORMALMAP:
			{
				if (currentViewer) {
					delete currentViewer;
					currentViewer = NULL;
				}
				char normalFile[512];
				char colorFile[512];
				if (OpenFile(normalFile, "Normal Map Files\0*.ppm;*.png;*.jpg\0")) {
					if (OpenFile(colorFile, "Color Map Files\0*.ppm;*.png;*.jpg\0")) {
						currentViewer = CreateNormalMapViewer(normalFile, colorFile);
					}
				}
				break;
			}
			case ID_OPENPROJECTED:
			{
				if (currentViewer) {
					delete currentViewer;
					currentViewer = NULL;
				}
				
				char projFile[512];
				if (OpenFile(projFile, "Projection Files\0*.prj\0")) {
					currentViewer = CreateProjViewer(projFile);
				}
				break;
			}
			case ID_OPENMULTIPROJECTED:
			{
				if (currentViewer) {
					delete currentViewer;
					currentViewer = NULL;
				}
				
				char projFile[512];
				std::vector<const char*> projFiles;

				for (uint32_t i = 0; i < 16; i++) {
					if (OpenFile(projFile, "Projection Files\0*.prj\0")) {
						if (projFiles.size() && strcmp(projFiles[projFiles.size()-1], projFile) == 0) {
							i = 15;
						} else {
							projFiles.push_back(_strdup(projFile));
						}
						if (i == 15) {
							currentViewer = CreateMultiProjViewer(projFiles);
							break;
						}
					} else {
						break;
					}
				}
				break;
			}
			case ID_CLOSE: 
				if (currentViewer) {
					delete currentViewer;
					currentViewer = NULL;
					SwapBuffers(glDC);
				}
				break;
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				gWnd = 0;
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		wglDeleteContext(glContext);
		DestroyWindow(hWnd);
		gWnd = 0;
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		{
			if (glContext) {
				RECT clientRect;
				BOOL ret = GetClientRect(hWnd,&clientRect);
				glViewport(0,0,clientRect.right,clientRect.bottom);
			}
			break;
		}
	case WM_CREATE:
		{
			PIXELFORMATDESCRIPTOR pfd =
			{
				sizeof(PIXELFORMATDESCRIPTOR),
				1,
				PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
				PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
				32,                        //Colordepth of the framebuffer.
				0, 0, 0, 0, 0, 0,
				0,
				0,
				0,
				0, 0, 0, 0,
				24,                        //Number of bits for the depthbuffer
				8,                        //Number of bits for the stencilbuffer
				0,                        //Number of Aux buffers in the framebuffer.
				PFD_MAIN_PLANE,
				0,
				0, 0, 0
			};
 
			glDC = GetDC(hWnd);
 
			int  letWindowsChooseThisPixelFormat;
			letWindowsChooseThisPixelFormat = ChoosePixelFormat(glDC, &pfd); 
			SetPixelFormat(glDC,letWindowsChooseThisPixelFormat, &pfd);
 
			glContext = wglCreateContext(glDC);
			wglMakeCurrent (glDC, glContext);
			
			GLenum err = glewInit();
			assert(err == GLEW_OK);
		}
		break;
	case WM_KEYDOWN:
	{
		const char* key = NULL;
		switch (wParam) {
			case 'r':
			case 'R':
				key = "r";
				break;
			case 'g':
			case 'G':
				key = "g";
				break;
			case 'b':
			case 'B':
				key = "b";
				break;
			case VK_LEFT:
				key = "left";
				break;
			case VK_RIGHT:
				key = "right";
				break;
			case VK_DOWN:
				key = "down";
				break;
			case VK_UP:
				key = "up";
				break;
			case VK_SPACE:
				key = "space";
				break;
		}
		if (key && currentViewer) {
			currentViewer->NotifyKeyPress(key);
		}
		break;
	}
	case WM_MOUSEMOVE:
	{
		static int lastXPos = 0;
		static int lastYPos = 0;
		int xPos = GET_X_LPARAM(lParam); 
		int yPos = GET_Y_LPARAM(lParam); 	

		if (currentViewer) {
			bool control = (wParam & MK_CONTROL) != 0;
			if (wParam & MK_LBUTTON) {
				currentViewer->NotifyMouseDrag((float) xPos - lastXPos, (float) yPos - lastYPos, 0, control);
			} else if (wParam & MK_MBUTTON) {
				currentViewer->NotifyMouseDrag((float) xPos - lastXPos, (float) yPos - lastYPos, 1, control);
			} else if (wParam & MK_RBUTTON) {
				currentViewer->NotifyMouseDrag((float) xPos - lastXPos, (float) yPos - lastYPos, 2, control);
			}
		}

		lastXPos = xPos;
		lastYPos = yPos;
		break;
	}
	case WM_MOUSEWHEEL:
	{
		short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		currentViewer->NotifyMouseWheel((float) zDelta / 120.0f, (wParam & MK_CONTROL) != 0);
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

float GetAspectRatio() {
	RECT clientRect;

	BOOL ret = GetClientRect(gWnd,&clientRect);

	return fabs((float) (clientRect.right - clientRect.left) / (float) (clientRect.bottom - clientRect.top));
}

void OutputDebug(const char* line) {
	OutputDebugString(line);
	OutputDebugString("\n");
	printf(line);
	printf("\n");
}