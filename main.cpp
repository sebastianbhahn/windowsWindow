#include <windows.h>

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

//#include <d2d1.h>
//#include <d3dx12.h>
#include<vector>
#include <array>
#include <dwmapi.h> 
#include <string>
//#include <cstdlib>
#include <atomic>
#include <chrono>
#include <thread>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "d2d1.lib")


//global variables

RECT currentMonitorSize;
RECT defaultWindowSize;
RECT currentWindowSize;
RECT currentAdjustedSize;

int defaultWindowLeft;
int defaultWindowTop;
int currentWindowLeft;
int currentWindowTop;
int oldLeft = currentWindowLeft;
int oldTop = currentWindowTop;

int captionBaseHeight = GetSystemMetrics(SM_CYCAPTION);

RECT rcWindowTop;

RECT rcWindowLeft;
RECT rcWindowRight;
RECT rcWindowBottom;

RECT rcCaptionEdge;

RECT rcExitButton;
RECT rcMaxButton;
RECT rcMinButton;
RECT rcCaptionBar;

RECT rcCaptionDrag;

RECT rcMenuBar;

RECT rcMenuArea;

std::vector<std::wstring> menuItems = {
    L"File",
    L"About",
};

std::vector<std::wstring> subMenu1 = {
    L"test",
    L"testing"
};

std::vector<std::wstring> subMenu2 = {
    L"testy",
    L"testeroo",
    L"flump"
};

std::wstring fontName;
int fontSize;

std::vector<RECT> menuItemRects;

std::vector<std::vector<std::wstring>> submenus = { subMenu1, subMenu2 };

std::vector<std::vector<RECT>> submenuItemRects;
std::vector<int> submenusWidth;
std::vector<int> submenusHeight;
std::vector<RECT> submenuWindowRects;
std::vector<HWND> submenuWindows;

std::vector<std::vector<RECT>> submenuHoverRects;

std::vector<std::vector<Bitmap*>> submenuItemBitmaps;
std::vector<std::vector<Bitmap*>> submenuItemHBitmaps;
std::vector<std::vector<Bitmap*>> submenuItemIBitmaps;

std::vector<BOOL> menuActive;
BOOL submenuInit = false;

BOOL submenuActive = false;

std::vector<std::vector<BOOL>> wasSubItemHovering;

int currentActiveSubmenu = 0;
int lastCurrentActiveSubmenu = 0;

RECT rcClientArea;

int colorAdjustThreshhold = 140;
int adjustCaptionBrightness = 20;

COLORREF captionColor = RGB(0, 0, 0);
COLORREF captionHighlight = RGB(50, 50, 50);
COLORREF captionText = RGB(255, 255, 255);
COLORREF secondaryColor = RGB(20, 20, 20);
COLORREF inactiveCaptionColor = RGB(255, 255, 255);
COLORREF inactiveEdgeColor = RGB(255, 255, 255);

std::vector<Bitmap*> windowControlBitmaps;
std::vector<Bitmap*> menuItemBitmaps;
std::vector<Bitmap*> menuItemHBitmaps;
std::vector<Bitmap*> menuItemIBitmaps;
ULONG_PTR gdiplusToken;

BOOL windowInitFinished = false;

BOOL bitmapsLoaded = false;
BOOL bitmapsReset = false;
BOOL isMaximized = false;
BOOL isMinimized = false;
BOOL isInactive = false;
BOOL cursorChanged = false;
BOOL uiColorUpdate = false;
BOOL cursorInWindow = false;

BOOL windowDragable = false;
BOOL windowMoved = false;

BOOL wasExitHovering = false;
BOOL wasMaxHovering = false;
BOOL wasMinHovering = false;

std::vector<BOOL> wasHovering;

BOOL dontRepaint = false;

BOOL inactiveFromDragWindow = false;
BOOL inactiveFromSubMenu = false;

POINT dummyPt = { 0, 0 };

POINT currentScreenPt;

std::atomic<bool> runCursorTracking(false);
std::atomic<bool> CursorTrackingRunning(false);

POINTS lastMousePos;
BOOL dragging = false;
BOOL draggingLeft = false;
BOOL draggingTop = false;
BOOL draggingRight = false;
BOOL draggingBottom = false;

BOOL dragCursorH = false;
BOOL dragCursorV = false;

HWND hDragWnd = NULL;

int dragWindowLeft;
int dragWindowTop;
RECT dragWindowSize;

HWND hMainWnd = NULL;

HHOOK hMouseHook;


void sendDebugMessage(std::string string) {
    _CrtDbgReport(_CRT_WARN, nullptr, 0, nullptr, string.c_str());
}


HINSTANCE hInst;
WCHAR windowTitle[] = L"windowTitle";
WCHAR windowClass[] = L"windowClass";

ATOM                MyWindowClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DragWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    subMenuWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MouseProc(int, WPARAM, LPARAM);



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    // Initialize global strings
    MyWindowClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        //Render();
    }

    return (int)msg.wParam;
}

ATOM MyWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr; // No icon
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = nullptr; // No menu
    wcex.lpszClassName = windowClass; // Directly provide class name
    wcex.hIconSm = nullptr; // No small icon

    return RegisterClassExW(&wcex);
}
//color related stuff

//function to get default theme color
COLORREF GetCaptionColor()
{
    DWORD color;
    BOOL opaqueBlend;
    HRESULT hr = DwmGetColorizationColor(&color, &opaqueBlend);
    if (SUCCEEDED(hr))
    {
        int rval = GetBValue(color);
        int gval = GetGValue(color);
        int bval = GetRValue(color);

        int rgbValues[] = { rval, gval, bval };

        for (int i = 0; i < 3; ++i) {
            if (rgbValues[i] > colorAdjustThreshhold) {
                if (rgbValues[i] + adjustCaptionBrightness < 255) {
                    rgbValues[i] += adjustCaptionBrightness;
                }
                else {
                    rgbValues[i] = 255; // Ensure it doesn't exceed 255
                }
            }
        }

        // Assign the modified values back
        rval = rgbValues[0];
        gval = rgbValues[1];
        bval = rgbValues[2];

        COLORREF invert = RGB(rval, gval, bval);
        return invert;
    }
    return RGB(0, 0, 0); // Default to white if the function fails
}

// Function to create window title text color
COLORREF GetContrastingTextColor(COLORREF backgroundColor) {
    // Extract RGB components
    BYTE red = GetRValue(backgroundColor);
    BYTE green = GetGValue(backgroundColor);
    BYTE blue = GetBValue(backgroundColor);

    // Calculate brightness
    double brightness = 0.299 * red + 0.587 * green + 0.114 * blue;

    // Determine text color based on brightness
    //COLORREF textColor;
    if (brightness > colorAdjustThreshhold) {
        return RGB(0, 0, 0); // Use black text if brightness is high
    }
    else {
        return RGB(255, 255, 255); // Use white text if brightness is low
    }

    return RGB(255, 255, 255);
}

//-----------
// Custom clamp function
int privateClamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Function to adjust brightness of a color
COLORREF AdjustBrightness(COLORREF color, int amount) {
    int red = GetRValue(color);
    int green = GetGValue(color);
    int blue = GetBValue(color);

    // Adjust each component
    red = privateClamp(red + amount, 0, 255);
    green = privateClamp(green + amount, 0, 255);
    blue = privateClamp(blue + amount, 0, 255);

    return RGB(red, green, blue);
}

// Function to set highlight color by adjusting lightness
COLORREF createHighlightColor(COLORREF TitleText, COLORREF background)
{

    double bgRVal = GetRValue(background);
    double bgGVal = GetGValue(background);
    double bgBVal = GetBValue(background);

    double avgBGval = (bgRVal + bgGVal + bgBVal) / 3.0;

    //Adjust lightness: make lighter or darker
    if (avgBGval < 50) {
        return AdjustBrightness(background, 100); // Brighten
    }

    else if (avgBGval < 100) {
        return AdjustBrightness(background, 75); // Brighten
    }

    else if (avgBGval < 130) {
        return AdjustBrightness(background, 50); // Brighten
    }

    else if (avgBGval > 175) {
        return AdjustBrightness(background, -75); // Darken
    }
    else {
        double ttRVal = GetRValue(TitleText);
        double ttGVal = GetGValue(TitleText);
        double ttBVal = GetBValue(TitleText);

        double avgTTval = (ttRVal + ttGVal + ttBVal) / 3.0;

        if (avgTTval < 10) {
            return AdjustBrightness(background, 50); // Brighten
        }

        else
        {
            return AdjustBrightness(background, -50); // Brighten
        }
    }
}
//-----------

//-----------
int getAverage(std::vector<int> numbers) {

    int numberCount = static_cast<int>(numbers.size());

    int sum = 0;
    for (int i = 0; i < numberCount; ++i) {
        sum += numbers[i];
    }

    int result = sum / numberCount;
    return result;
}

COLORREF createSecondaryColor(COLORREF color1, COLORREF color2) {

    //split the difference between two colors

    int rVal1 = GetRValue(color1);
    int rVal2 = GetRValue(color2);

    int gVal1 = GetGValue(color1);
    int gVal2 = GetGValue(color2);

    int bVal1 = GetBValue(color1);
    int bVal2 = GetBValue(color2);

    int rgbVal1 = getAverage(std::vector<int> {rVal1, gVal1, bVal1});
    int rgbVal2 = getAverage(std::vector<int> {rVal2, gVal2, bVal2});

    if (rgbVal1 > rgbVal2) {
        //if colors is lighter than color2
        //get and split the difference
        int split = (rgbVal1 - rgbVal2) / 2;
        //subtract split difference from RBG values
        return RGB(rVal1 - split, gVal1 - split, bVal1 - split);
    }
    else if (rgbVal1 < rgbVal2) {
        int split = (rgbVal2 - rgbVal1) / 2;
        return RGB(rVal2 - split, gVal2 - split, bVal2 - split);
    }
    return color1;
}
//-----------

void getSystemColors() {

    static BOOL colorInit = false;

    COLORREF currentCaptionColor = GetCaptionColor();

    if (captionColor != currentCaptionColor || colorInit == false) {
        captionColor = currentCaptionColor;
        captionText = GetContrastingTextColor(captionColor);
        captionHighlight = createHighlightColor(captionText, captionColor);
        secondaryColor = createSecondaryColor(captionColor, captionHighlight);
        inactiveCaptionColor = createSecondaryColor(captionHighlight, RGB(255, 255, 255));
        inactiveEdgeColor = createSecondaryColor(secondaryColor, RGB(255, 255, 255));
        if (colorInit == true) {
            uiColorUpdate = true;
        }
        colorInit = true;
    }
}

//end of color related stuff

//ui sizing stuff


int getPriorWidth(int menu) {
    if (menu > 0) {
        return (menuItemRects[menu - 1].right - menuItemRects[menu - 1].left);
    }
    else {
        return 0;
    }
}


void populateSubmenuItemHoverRects() {

    if (submenuHoverRects.size() > 0) {
        submenuHoverRects.clear();

    }

    int count = submenuWindowRects.size();
    int i = 0;
    int width = rcWindowLeft.right - rcWindowLeft.left;
    while (i < count) {
        std::vector<RECT> rects;
        int countB = submenuItemRects[i].size();
        int B = 0;
        width += getPriorWidth(i);
        int topPos = currentWindowTop + rcMenuBar.bottom;
        while (B < countB) {
            int leftPos = currentWindowLeft + width;
            RECT rect = {
                leftPos, topPos,
                leftPos + (submenuWindowRects[i].right - submenuWindowRects[i].left),
                topPos + (submenuItemRects[i][B].bottom - submenuItemRects[i][B].top)
            };
            rects.push_back(rect);
            topPos += (rect.bottom - rect.top);
            B++;
        }
        submenuHoverRects.push_back(rects);
        i++;
    }
}

void updateCurrentWindowPositions() {

    RECT windowRect;
    if (GetWindowRect(hMainWnd, &windowRect)) {
        currentWindowLeft = windowRect.left;
        currentWindowTop = windowRect.top;
    }

}


void resizeWindow(HWND hwnd, RECT input) {
    MoveWindow(hwnd, input.left, input.top, input.right, input.bottom, TRUE);
}

void createSubmenuWindowRects(HWND hWnd) {

    if (submenuWindowRects.size() > 0) {
        submenuWindowRects.clear();
    }


    int count = menuItems.size();
    int i = 0;
    int left = currentWindowLeft + rcWindowLeft.right;
    int prevItems = 0;
    int top = currentWindowTop + rcMenuBar.bottom;

    while (i < count) {
        left += prevItems;
        prevItems += (menuItemRects[i].right - menuItemRects[i].left);

        RECT rect = { left, top, (left + submenusWidth[i]), (top + submenusHeight[i])};
        submenuWindowRects.push_back(rect);
        i++;
    }
	
}

void moveSubmenuWindows() {
    int count = submenuWindows.size();
    int i = 0;
    while (i < count) {
        resizeWindow(submenuWindows[i], submenuWindowRects[i]);
        i++;
    }
}

void createSubmenuRects(HWND hWnd) {

    if (submenuItemRects.size() > 0) {
        submenuItemRects.clear();
    }

    HDC hdc = GetDC(hWnd);

    // Set up the HDC to use the global font and size
    LOGFONT logfont = { 0 };
    logfont.lfHeight = -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72); // Convert points to logical units
    logfont.lfWeight = FW_BOLD; // Example weight
    wcscpy_s(logfont.lfFaceName, fontName.c_str());

    HFONT hFont = CreateFontIndirect(&logfont);
    HFONT hFontOld = (HFONT)SelectObject(hdc, hFont);

    int xPadding = -2;
    int menuHeight = rcMenuBar.bottom - rcMenuBar.top;
    int count = menuItems.size();
    int i = 0;
    while (i < count) {
        int count2 = submenus[i].size();
        int i2 = 0;
        std::vector<RECT> submenuItems;
        int width = 0;
        int itemNumber = 0;
        while (i2 < count2) {
            itemNumber++;
            SIZE textSize;
            GetTextExtentPoint32(hdc, submenus[i][i2].c_str(), submenus[i][i2].length(), &textSize);
            int twidth = textSize.cx + xPadding;
            if (twidth > width) {
                width = twidth;
            }
            i2++;
        }
        int topPos = 0;
        int i3 = 0;
        while (i3 < itemNumber) {
            RECT rect = { 0, topPos, width, topPos + menuHeight };
            submenuItems.push_back(rect);
            topPos = rect.bottom;
            i3++;
        }
        submenusWidth.push_back(width);
        submenusHeight.push_back(topPos);
        submenuItemRects.push_back(submenuItems);
        i++;
    }
    // Restore original font
    SelectObject(hdc, hFontOld);
    // Cleanup
    DeleteObject(hFont);
    ReleaseDC(hWnd, hdc);
}

std::array<int, 2> setMenuItemRects(HWND hWnd) {

    if (menuItemRects.size() > 0) {
        menuItemRects.clear();
    }

    HDC hdc = GetDC(hWnd);

    // Define the LOGFONT structure for the desired font
    LOGFONT logfont = { 0 };
    logfont.lfHeight = -15; // Negative height for point size (24 points in this example)
    logfont.lfWeight = FW_BOLD; // Normal weight
    wcscpy_s(logfont.lfFaceName, L"Helvetica"); // Desired font name

    // Create the font
    HFONT hFont = CreateFontIndirect(&logfont);
    HFONT hFontOld = (HFONT)SelectObject(hdc, hFont);

    // Grab the font name
    wchar_t getFont[LF_FACESIZE];
    GetTextFace(hdc, LF_FACESIZE, getFont);
    // Pass to global wstring
    fontName = getFont;

    // Retrieve font information
    GetObject(hFont, sizeof(LOGFONT), &logfont);

    // Calculate font size in points
    int height = logfont.lfHeight;
    HDC screenHdc = GetDC(nullptr);
    fontSize = -MulDiv(height, 72, GetDeviceCaps(screenHdc, LOGPIXELSY));
    ReleaseDC(nullptr, screenHdc);
    
    
    int finalRightPos = 0;
    int xPadding = -2;
    int yPadding = 2;
    int topPos = rcCaptionEdge.bottom;
    int bottomPos = 0;
    int leftPos = rcWindowLeft.right;
    int itemCount = menuItems.size();
    int i = 0;
    while (i < itemCount) {
        SIZE textSize;
        GetTextExtentPoint32(hdc, menuItems[i].c_str(), menuItems[i].length(), &textSize);
        if (bottomPos == 0) {
            bottomPos = topPos + textSize.cy + yPadding;
        }

        int twidth = textSize.cx + xPadding;

        RECT rect = {leftPos, topPos, leftPos + twidth, bottomPos};
        menuItemRects.push_back(rect);
        leftPos = rect.right;
        finalRightPos = rect.right;
        i++;
    }

    // Restore original font 
    SelectObject(hdc, hFontOld); 
    // Cleanup 
    ReleaseDC(hWnd, hdc); 

    return {finalRightPos, bottomPos};
}

void setUISizes(HWND hWnd) { //should be run first after a resize and maybe after a window move

    int edgeWidth = 3;
    int buttonWidthadjusted = captionBaseHeight * 1.2;
    int captionBottomAdjusted = captionBaseHeight + edgeWidth;

    //exit button
    rcExitButton = { currentWindowSize.right - buttonWidthadjusted, 0, currentWindowSize.right, captionBottomAdjusted };

    //maximize button
    rcMaxButton = { rcExitButton.left - buttonWidthadjusted, 0, rcExitButton.left, captionBottomAdjusted };

    //minimize button
    rcMinButton = { rcMaxButton.left - buttonWidthadjusted, 0, rcMaxButton.left , captionBottomAdjusted };

    //unrendered window top edge for resize dragging purposes
    rcWindowTop = { 0, 0, rcMinButton.left, edgeWidth };

    //caption bar
    rcCaptionBar = { 0, 0, rcMinButton.left , captionBottomAdjusted};

    //unrendered rect for dragging the window around from the captionbar
    rcCaptionDrag = {rcCaptionBar.left, rcWindowTop.bottom, rcCaptionBar.right, rcCaptionBar.bottom};

    //client window edges
    rcWindowLeft = { 0, captionBottomAdjusted, edgeWidth, currentWindowSize.bottom };
    rcWindowRight = { currentWindowSize.right - edgeWidth, captionBottomAdjusted, currentWindowSize.right, currentWindowSize.bottom };
    rcWindowBottom = { rcWindowLeft.right, currentWindowSize.bottom - edgeWidth,  rcWindowRight.left, currentWindowSize.bottom };

    //splits the menu and caption area
    rcCaptionEdge = { rcWindowLeft.right, captionBottomAdjusted, rcWindowRight.left, captionBottomAdjusted + edgeWidth };

     std::array<int, 2> menuBarLeftPlusBottom = setMenuItemRects(hWnd);

    //menu bar; no items yet
    //temp, partial
    rcMenuBar = { menuBarLeftPlusBottom[0], rcCaptionEdge.bottom, rcWindowRight.left, menuBarLeftPlusBottom[1] };

    rcMenuArea = { rcWindowLeft.right, rcMenuBar.top, rcMenuBar.left, rcMenuBar.bottom };

    //client area rect
    rcClientArea = {rcWindowLeft.right, rcMenuBar.bottom, rcWindowRight.left, rcWindowBottom.top };

    createSubmenuRects(hWnd);
    createSubmenuWindowRects(hWnd);
	populateSubmenuItemHoverRects();
}

// Function to get the screen size of the monitor the window is on
void setCurrentMonitorSize(HWND hwnd) {
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(hMonitor, &monitorInfo)) {
        currentMonitorSize = { monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right, monitorInfo.rcMonitor.bottom };
    }
}

void setDefaultWindowSize() {

    int screenWidth = currentMonitorSize.right;
    int screenHeight = currentMonitorSize.bottom;

    int windowRight = (5 * screenWidth) / 8;
    int windowBottom = (5 * screenHeight) / 8;

    defaultWindowLeft = (screenWidth - windowRight) / 2;
    defaultWindowTop = (screenHeight - windowBottom) / 2;

    currentWindowLeft = defaultWindowLeft;
    currentWindowTop = defaultWindowTop;


    defaultWindowSize = { 0, 0, windowRight, windowBottom };

}

//ui rendering stuff

void InitGDIPlus() {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
}

void clearBitmaps() {
    for (Bitmap* bmp : windowControlBitmaps) {
        delete bmp;
    }
    windowControlBitmaps.clear();

    for (Bitmap* bmp : menuItemBitmaps) {
        delete bmp;
    }
    menuItemBitmaps.clear();

    for (Bitmap* bmp : menuItemHBitmaps) {
        delete bmp;
    }
    menuItemHBitmaps.clear();

    for (Bitmap* bmp : menuItemIBitmaps) {
        delete bmp;
    }
    menuItemIBitmaps.clear();

    // Ensure that other vectors are populated before attempting to clear them
    if (submenuItemBitmaps.size() > 0) {
        int count = submenuItemBitmaps.size();
        for (int i = 0; i < count; ++i) {
            // Clear submenuItemBitmaps
            for (Bitmap* bmp : submenuItemBitmaps[i]) {
                if (bmp != nullptr) {
                    delete bmp;
                }
            }
            submenuItemBitmaps[i].clear();
        }
        submenuItemBitmaps.clear();
    }

    if (submenuItemHBitmaps.size() > 0) {
        int count = submenuItemHBitmaps.size();
        for (int i = 0; i < count; i++) {

            // Clear submenuItemHBitmaps
            for (Bitmap* bmp : submenuItemHBitmaps[i]) {
                if (bmp != nullptr) {
                    delete bmp;
                }
            }
            submenuItemHBitmaps[i].clear();

        }
        submenuItemHBitmaps.clear();
    }

    if (submenuItemIBitmaps.size() > 0) {
        int count = submenuItemIBitmaps.size();
        for (int i = 0; i < count; i++){
            // Clear submenuItemIBitmaps
            for (Bitmap* bmp : submenuItemIBitmaps[i]) {
                if (bmp != nullptr) {
                    delete bmp;
                }
            }
            submenuItemIBitmaps[i].clear();
        }
        submenuItemIBitmaps.clear();
    }

}

void CleanupGDIPlus() {

    clearBitmaps();
    GdiplusShutdown(gdiplusToken);
    bitmapsReset = true;
}

RECT getButtonIconRect(RECT input, int percentage) {
    int width = input.right - input.left;
    int height = input.bottom - input.top;

    int rWidth = (width * percentage) / 100;
    int rHeight = (height * percentage) / 100;

    int widthDif = (width - rWidth) / 2;
    int heightDif = (height - rHeight) / 2;


    int left = input.left + widthDif;
    int top = input.top + heightDif;
    int right = input.right - widthDif;
    int bottom = input.bottom - heightDif;

    return { left, top, right, bottom };
}

void fillGraphicsRect(int width, int height, COLORREF color, Graphics& graphic) {

    SolidBrush brush(Color(255, GetRValue(color), GetGValue(color), GetBValue(color)));
    graphic.FillRectangle(&brush, 0, 0, width, height);
}

void paintX(int width, int height, Graphics& graphic) {

    
    RECT temp = { 0, 0, width, height };
    RECT newR = getButtonIconRect(temp, 30);
    Pen pen(Color(255, GetRValue(captionText), GetGValue(captionText), GetBValue(captionText)), 2.5);
    graphic.DrawLine(&pen, (int)newR.left, (int)newR.top, (int)newR.right, (int)newR.bottom);
    graphic.DrawLine(&pen, (int)newR.right, (int)newR.top, (int)newR.left, (int)newR.bottom);
    


    /*
    // Create a font and brush for the text
    FontFamily fontFamily(L"Arial Black");
    Font font(&fontFamily, 13, FontStyleRegular, UnitPixel);
    SolidBrush textBrush(Color(255, GetRValue(captionText), GetGValue(captionText), GetBValue(captionText)));
    // Text to draw
    std::wstring text = L"X";
    // Measure the size of the text
    RectF layoutRect(0, 0, (int)width, (int)height);
    RectF boundingBox;
    graphic.MeasureString(text.c_str(), -1, &font, layoutRect, &boundingBox);
    // Calculate the position to center the text
    float x = (width - boundingBox.Width) / 2;
    float y = (height + 1 - boundingBox.Height) / 2;

    // Draw the text
    graphic.DrawString(text.c_str(), -1, &font, PointF(x, y), &textBrush);
    */
}

void paintmMax(int width, int height, Graphics& graphic) {
    RECT temp = {0, 0, width, height};
    RECT newR = getButtonIconRect(temp, 30);
    Pen pen(Color(255, GetRValue(captionText), GetGValue(captionText), GetBValue(captionText)), 2);
    graphic.DrawLine(&pen, (int)newR.left, (int)newR.top, (int)newR.right, (int)newR.top);
    graphic.DrawLine(&pen, (int)newR.left, (int)newR.bottom, (int)newR.right, (int)newR.bottom);
    graphic.DrawLine(&pen, (int)newR.left, (int)newR.top, (int)newR.left, (int)newR.bottom);
    graphic.DrawLine(&pen, (int)newR.right, (int)newR.top, (int)newR.right, (int)newR.bottom);
}

void paintMMax(int width, int height, Graphics& graphic) {
    RECT temp = { 0, 0, width, height };
    RECT newR = getButtonIconRect(temp, 25);
    Pen pen(Color(255, GetRValue(captionText), GetGValue(captionText), GetBValue(captionText)), 2);
    graphic.DrawLine(&pen, (int)newR.left - 2, (int)newR.top + 2, (int)newR.right - 2, (int)newR.top + 2);
    graphic.DrawLine(&pen, (int)newR.left - 2, (int)newR.bottom + 2, (int)newR.right - 2, (int)newR.bottom + 2);
    graphic.DrawLine(&pen, (int)newR.left - 2, (int)newR.top + 2, (int)newR.left - 2, (int)newR.bottom + 2);
    graphic.DrawLine(&pen, (int)newR.right - 2, (int)newR.top + 2, (int)newR.right - 2, (int)newR.bottom + 2);
    graphic.DrawLine(&pen, (int)newR.left, (int)newR.top - 1, (int)newR.right + 1, (int)newR.top - 1);
    graphic.DrawLine(&pen, (int)newR.left, (int)newR.top - 1, (int)newR.left, (int)newR.top + 2);
    graphic.DrawLine(&pen, (int)newR.right + 1, (int)newR.top - 1, (int)newR.right + 1, (int)newR.bottom - 1);
    graphic.DrawLine(&pen, (int)newR.right + 1, (int)newR.bottom - 1, (int)newR.right - 2, (int)newR.bottom - 1);
}

void paintMin(int width, int height, Graphics& graphic) {
    RECT temp = { 0, 0, width, height };
    RECT newR = getButtonIconRect(temp, 30);
    int halfheight = (newR.right - newR.left) / 2;
    Pen pen(Color(255, GetRValue(captionText), GetGValue(captionText), GetBValue(captionText)), 2);
    graphic.DrawLine(&pen, (int)newR.left, (int)newR.bottom - halfheight, (int)newR.right, (int)newR.bottom - halfheight);

}

void createWindowControlBitmaps() { //should only need to be run once; creates window controls

    int eWidth = rcExitButton.right - rcExitButton.left;
    int eHeight = rcExitButton.bottom - rcExitButton.top;

    Bitmap* bmExit = new Bitmap(eWidth, eHeight, PixelFormat32bppARGB);
    Graphics grExit(bmExit);
    fillGraphicsRect(eWidth, eHeight, captionColor, grExit);
    paintX(eWidth, eHeight, grExit);

    Bitmap* bmExitH = new Bitmap(eWidth, eHeight, PixelFormat32bppARGB);
    Graphics grExitH(bmExitH);
    fillGraphicsRect(eWidth, eHeight, captionHighlight, grExitH);
    paintX(eWidth, eHeight, grExitH);

    Bitmap* bmExitI = new Bitmap(eWidth, eHeight, PixelFormat32bppARGB);
    Graphics grExitI(bmExitI);
    fillGraphicsRect(eWidth, eHeight, inactiveCaptionColor, grExitI);
    paintX(eWidth, eHeight, grExitI);

    windowControlBitmaps.push_back(bmExit);
    windowControlBitmaps.push_back(bmExitH);
    windowControlBitmaps.push_back(bmExitI);

    int mxWidth = rcMaxButton.right - rcMaxButton.left;
    int mxHeight = rcMaxButton.bottom - rcMaxButton.top;

    Bitmap* bmmMax = new Bitmap(mxWidth, mxHeight, PixelFormat32bppARGB);
    Graphics grmMax(bmmMax);
    fillGraphicsRect(mxWidth, mxHeight, captionColor, grmMax);
    paintmMax(mxWidth, mxHeight, grmMax);

    Bitmap* bmmMaxH = new Bitmap(mxWidth, mxHeight, PixelFormat32bppARGB);
    Graphics grmMaxH(bmmMaxH);
    fillGraphicsRect(mxWidth, mxHeight, captionHighlight, grmMaxH);
    paintmMax(mxWidth, mxHeight, grmMaxH);

    Bitmap* bmmMaxI = new Bitmap(mxWidth, mxHeight, PixelFormat32bppARGB);
    Graphics grmMaxI(bmmMaxI);
    fillGraphicsRect(mxWidth, mxHeight, inactiveCaptionColor, grmMaxI);
    paintmMax(mxWidth, mxHeight, grmMaxI);

    windowControlBitmaps.push_back(bmmMax);
    windowControlBitmaps.push_back(bmmMaxH);
    windowControlBitmaps.push_back(bmmMaxI);

    Bitmap* bmMMax = new Bitmap(mxWidth, mxHeight, PixelFormat32bppARGB);
    Graphics grMMax(bmMMax);
    fillGraphicsRect(mxWidth, mxHeight, captionColor, grMMax);
    paintMMax(mxWidth, mxHeight, grMMax);

    Bitmap* bmMMaxH = new Bitmap(mxWidth, mxHeight, PixelFormat32bppARGB);
    Graphics grMMaxH(bmMMaxH);
    fillGraphicsRect(mxWidth, mxHeight, captionHighlight, grMMaxH);
    paintMMax(mxWidth, mxHeight, grMMaxH);

    Bitmap* bmMMaxI = new Bitmap(mxWidth, mxHeight, PixelFormat32bppARGB);
    Graphics grMMaxI(bmMMaxI);
    fillGraphicsRect(mxWidth, mxHeight, inactiveCaptionColor, grMMaxI);
    paintMMax(mxWidth, mxHeight, grMMaxI);

    windowControlBitmaps.push_back(bmMMax);
    windowControlBitmaps.push_back(bmMMaxH);
    windowControlBitmaps.push_back(bmMMaxI);

    int mnWidth = rcMinButton.right - rcMinButton.left;
    int mnHeight = rcMinButton.bottom - rcMinButton.top;

    Bitmap* bmMin = new Bitmap(mnWidth, mnHeight, PixelFormat32bppARGB);
    Graphics grMin(bmMin);
    fillGraphicsRect(mnWidth, mnHeight, captionColor, grMin);
    paintMin(mnWidth, mnHeight, grMin);

    Bitmap* bmMinH = new Bitmap(mnWidth, mnHeight, PixelFormat32bppARGB);
    Graphics grMinH(bmMinH);
    fillGraphicsRect(mnWidth, mnHeight, captionHighlight, grMinH);
    paintMin(mnWidth, mnHeight, grMinH);

    Bitmap* bmMinI = new Bitmap(mnWidth, mnHeight, PixelFormat32bppARGB);
    Graphics grMinI(bmMinI);
    fillGraphicsRect(mnWidth, mnHeight, inactiveCaptionColor, grMinI);
    paintMin(mnWidth, mnHeight, grMinI);

    windowControlBitmaps.push_back(bmMin);
    windowControlBitmaps.push_back(bmMinH);
    windowControlBitmaps.push_back(bmMinI);
}

void renderExitButton(HWND hWnd, BOOL highlighted) { // run when exit button needs to update
    InvalidateRect(hWnd, &rcExitButton, FALSE);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    Graphics graphics(hdc);
    if (highlighted) {
        graphics.DrawImage(windowControlBitmaps[1], (int)rcExitButton.left, (int)rcExitButton.top);
    }
    else if(isInactive) {
        graphics.DrawImage(windowControlBitmaps[2], (int)rcExitButton.left, (int)rcExitButton.top);
    }
    else {
        graphics.DrawImage(windowControlBitmaps[0], (int)rcExitButton.left, (int)rcExitButton.top);
    }
    EndPaint(hWnd, &ps);
}

void renderMaxButton(HWND hWnd, BOOL highlighted) { //run when max button needs to update
    InvalidateRect(hWnd, &rcMaxButton, FALSE);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    Graphics graphics(hdc);
    if (!isMaximized) {
        if (highlighted) {
            graphics.DrawImage(windowControlBitmaps[4], (int)rcMaxButton.left, (int)rcMaxButton.top);
        }
        else if (isInactive) {
            graphics.DrawImage(windowControlBitmaps[5], (int)rcMaxButton.left, (int)rcMaxButton.top);
        }
        else {
            graphics.DrawImage(windowControlBitmaps[3], (int)rcMaxButton.left, (int)rcMaxButton.top);
        }
    }
    else {
        if (highlighted) {
            graphics.DrawImage(windowControlBitmaps[7], (int)rcMaxButton.left, (int)rcMaxButton.top);
        }
        else if (isInactive) {
            graphics.DrawImage(windowControlBitmaps[8], (int)rcMaxButton.left, (int)rcMaxButton.top);
        }
        else {
            graphics.DrawImage(windowControlBitmaps[6], (int)rcMaxButton.left, (int)rcMaxButton.top);
        }
    }
    EndPaint(hWnd, &ps);
}

void renderMinButton(HWND hWnd, BOOL highlighted) { //run when min button needs to update
    InvalidateRect(hWnd, &rcMinButton, FALSE);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    Graphics graphics(hdc);
    if (highlighted) {
        graphics.DrawImage(windowControlBitmaps[10], (int)rcMinButton.left, (int)rcMinButton.top);
    }
    else if (isInactive) {
        graphics.DrawImage(windowControlBitmaps[11], (int)rcMinButton.left, (int)rcMinButton.top);
    }
    else {
        graphics.DrawImage(windowControlBitmaps[9], (int)rcMinButton.left, (int)rcMinButton.top);
    }
    EndPaint(hWnd, &ps);
}

void renderWindowControls(HWND hWnd) { // run to update all window controls to default color
    renderExitButton(hWnd, false);
    renderMaxButton(hWnd, false);
    renderMinButton(hWnd, false);
}

void handleExitHighlight(HWND hWnd, POINT pt) {

    BOOL isExitHovering = PtInRect(&rcExitButton, pt);

    if (isExitHovering != wasExitHovering) {
        if (isExitHovering) {
            renderExitButton(hWnd, true);
        }
        else {
            renderExitButton(hWnd, false);
        }
        wasExitHovering = isExitHovering;
    }

}

void handleMaxHighlight(HWND hWnd, POINT pt) {

    BOOL isMaxHovering = PtInRect(&rcMaxButton, pt);

    if (isMaxHovering != wasMaxHovering) {
        if (isMaxHovering) {
            renderMaxButton(hWnd, true);
        }
        else {
            renderMaxButton(hWnd, false);
        }
        wasMaxHovering = isMaxHovering;
    }
}

void handleMinHighlight(HWND hWnd, POINT pt) {

    BOOL isMinHovering = PtInRect(&rcMinButton, pt);

    if (isMinHovering != wasMinHovering) {
        if (isMinHovering) {
            renderMinButton(hWnd, true);
        }
        else {
            renderMinButton(hWnd, false);
        }
        wasMinHovering = isMinHovering;
    }
}

void handleCaptionHighlight(HWND hWnd, POINT pt) {
    handleExitHighlight(hWnd, pt);
    handleMaxHighlight(hWnd, pt);
    handleMinHighlight(hWnd, pt);
}

void fillGDIText(int width, int height, Graphics& graphic, std::wstring text, BOOL leftAlign) {

    // Create a font and brush for the text
    FontFamily fontFamily(fontName.c_str());
    Font font(&fontFamily, fontSize, FontStyleRegular, UnitPixel);
    SolidBrush textBrush(Color(255, GetRValue(captionText), GetGValue(captionText), GetBValue(captionText)));
    // Measure the size of the text
    RectF layoutRect(0, 0, (int)width, (int)height);
    RectF boundingBox;
    graphic.MeasureString(text.c_str(), -1, &font, layoutRect, &boundingBox);
    // Calculate the position to center the text
    float x = 1;
    if (!leftAlign) {
        x = (width - boundingBox.Width) / 2;
    }
    float y = (height + 1 - boundingBox.Height) / 2;

    // Draw the text
    graphic.DrawString(text.c_str(), -1, &font, PointF(x, y), &textBrush);

}

void createMenuBarBitmaps() {
    int itemCount = menuItems.size();
    int b = 0;
    int i = 0;
    while (i < itemCount) {
        int iWidth = menuItemRects[i].right - menuItemRects[i].left;
        int iHeight = menuItemRects[i].bottom - menuItemRects[i].top;

        Bitmap* bm = new Bitmap(iWidth, iHeight, PixelFormat32bppARGB);
        Graphics gr(bm);
        fillGraphicsRect(iWidth, iHeight, captionColor, gr);
        fillGDIText(iWidth, iHeight, gr, menuItems[i], false);
        menuItemBitmaps.push_back(bm);

        Bitmap* bmH = new Bitmap(iWidth, iHeight, PixelFormat32bppARGB);
        Graphics grH(bmH);
        fillGraphicsRect(iWidth, iHeight, captionHighlight, grH);
        fillGDIText(iWidth, iHeight, grH, menuItems[i], false);
        menuItemHBitmaps.push_back(bmH);

        Bitmap* bmI = new Bitmap(iWidth, iHeight, PixelFormat32bppARGB);
        Graphics grI(bmI);
        fillGraphicsRect(iWidth, iHeight, inactiveCaptionColor, grI);
        fillGDIText(iWidth, iHeight, grI, menuItems[i], false);
        menuItemIBitmaps.push_back(bmI);

        i++;
        b++;
    }
}

void createSubmenuBitmaps() {

    int count = menuItems.size();
    int i = 0;
    while (i < count) {
        int count2 = submenus[i].size();
        int i2 = 0;
        std::vector<Bitmap*> itemMaps;
        std::vector<Bitmap*> itemHMaps;
        std::vector<Bitmap*> itemIMaps;
        while (i2 < count2) {
            int width = submenuItemRects[i][i2].right - submenuItemRects[i][i2].left;
            int height = submenuItemRects[i][i2].bottom - submenuItemRects[i][i2].top;

            Bitmap* itemPaint = new Bitmap(width, height, PixelFormat32bppARGB);
            Graphics grIP(itemPaint);
            fillGraphicsRect(width, height, captionColor, grIP);
            fillGDIText(width, height, grIP, submenus[i][i2], true);
            itemMaps.push_back(itemPaint);

            Bitmap* itemHPaint = new Bitmap(width, height, PixelFormat32bppARGB);
            Graphics grHIP(itemHPaint);
            fillGraphicsRect(width, height, captionHighlight, grHIP);
            fillGDIText(width, height, grHIP, submenus[i][i2], true);
            itemHMaps.push_back(itemHPaint);

            Bitmap* itemIPaint = new Bitmap(width, height, PixelFormat32bppARGB);
            Graphics grIIP(itemIPaint);
            fillGraphicsRect(width, height, captionHighlight, grIIP);
            fillGDIText(width, height, grIIP, submenus[i][i2], true);
            itemIMaps.push_back(itemIPaint);

             i2++;
        }
        submenuItemBitmaps.push_back(itemMaps);
        submenuItemHBitmaps.push_back(itemHMaps);
        submenuItemIBitmaps.push_back(itemIMaps);

        i++;
    }

}

void renderSingleMenuItem(HWND hWnd, int which, BOOL highlighted) {
    InvalidateRect(hWnd, &menuItemRects[which], FALSE);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    Graphics graphics(hdc);
    if (highlighted) {
        graphics.DrawImage(menuItemHBitmaps[which], (int)menuItemRects[which].left, (int)menuItemRects[which].top);
    }
    else if (isInactive) {
        graphics.DrawImage(menuItemIBitmaps[which], (int)menuItemRects[which].left, (int)menuItemRects[which].top);
    }
    else {
        graphics.DrawImage(menuItemBitmaps[which], (int)menuItemRects[which].left, (int)menuItemRects[which].top);
    }

    EndPaint(hWnd, &ps);

}

void renderMenuBarItems(HWND hWnd) {
    int itemCount = menuItems.size();
    int i = 0;
    while (i < itemCount) {
        renderSingleMenuItem(hWnd, i, false);
        i++;
    }

}

void clearMenuHover(){
	int count = wasHovering.size();
	int i = 0;
	while (i < count) {
		wasHovering[i] = false;
		i++;
	}
}

void handleMenuHighlight(HWND hWnd, POINT pt) {
    static BOOL setup = false;

    int itemCount = menuItems.size();

    if (!setup) {
        int a = 0;
        while (a < itemCount) {
            wasHovering.push_back(false);
            a++;
        }
        setup = true;
    }

    int i = 0;
    while (i < itemCount) {
        BOOL isHovering = PtInRect(&menuItemRects[i], pt);
        if (isHovering != wasHovering[i]) {
            if (isHovering) {
                renderSingleMenuItem(hWnd, i, true);
            }
            else {
                if (!submenuActive) {
                    renderSingleMenuItem(hWnd, i, false);
                }
                else if (i != currentActiveSubmenu) {
                    renderSingleMenuItem(hWnd, i, false);
                }
            }

        }
        wasHovering[i] = isHovering;
        i++;
    }
}



void paintRect(HWND hWnd, RECT rect, COLORREF color) {

    HDC hdc = GetWindowDC(hWnd);

    HBRUSH hBrush = CreateSolidBrush(color);
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);

    ReleaseDC(hWnd, hdc);
}

void clearRect(HWND hWnd, RECT rect) {
    HDC hdc = GetWindowDC(hWnd);

    HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    FillRect(hdc, &rect, hBrush);

    ReleaseDC(hWnd, hdc);
}


//temp
void paintClient(HWND hWnd) {
    paintRect(hWnd, rcClientArea, RGB(255, 255, 255));
}
//temp end

void createUiBitmaps() {
    createWindowControlBitmaps();
    createMenuBarBitmaps();
    createSubmenuBitmaps();
    bitmapsLoaded = true;
}

void paintSimpleUiElements(HWND hWnd) {
    if (isInactive) {
        paintRect(hWnd, rcCaptionBar, inactiveCaptionColor);
        paintRect(hWnd, rcMenuBar, inactiveCaptionColor);
        paintRect(hWnd, rcCaptionEdge, inactiveEdgeColor);
        paintRect(hWnd, rcWindowLeft, inactiveEdgeColor);
        paintRect(hWnd, rcWindowRight, inactiveEdgeColor);
        paintRect(hWnd, rcWindowBottom, inactiveEdgeColor);
    }
    else {
        paintRect(hWnd, rcCaptionBar, captionColor);
        paintRect(hWnd, rcMenuBar, captionColor);
        paintRect(hWnd, rcCaptionEdge, secondaryColor);
        paintRect(hWnd, rcWindowLeft, secondaryColor);
        paintRect(hWnd, rcWindowRight, secondaryColor);
        paintRect(hWnd, rcWindowBottom, secondaryColor);
    }

}

void renderWindowUI(HWND hWnd) {
    paintSimpleUiElements(hWnd);
    renderWindowControls(hWnd);
    renderMenuBarItems(hWnd);
    paintClient(hWnd);
}

void handleCaptionControlClicks(HWND hWnd, POINT pt) {

    if (PtInRect(&rcExitButton, pt)) {
        PostMessage(hWnd, WM_CLOSE, 0, 0);
    }
    else if (PtInRect(&rcMaxButton, pt)) {
        if (isMaximized) {
            SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        }
        else {
            SendMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }
    }
    else if (PtInRect(&rcMinButton, pt)) {
        renderMinButton(hWnd, false);
        SendMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    }
}


void destroyMenuChildWindows() {

    int count = submenuWindows.size();
    if (count > 0) {
        int i = 0;
        while (i < count) {

            if (submenuWindows[i]) {
                DestroyWindow(submenuWindows[i]);
                submenuWindows[i] = NULL;
            }

            i++;
        }

        submenuWindows.clear();
    }
}

void createSubMenuWindows(HWND hWnd) {
  
    if (submenuWindows.size() > 0) {
        destroyMenuChildWindows();
    }
    
    int count = menuItems.size();

    int i = 0;
    while (i < count) {
        int left = submenuWindowRects[i].left;
        int top = submenuWindowRects[i].top;
        int right = submenuWindowRects[i].right;
        int bottom = submenuWindowRects[i].bottom;

        HWND hWndSubMenu = CreateWindowW(
            L"MenuWindowClass", NULL, 
            WS_POPUP | WS_VISIBLE,
            left, top, right - left, bottom - top,
            hWnd, NULL, GetModuleHandle(NULL), NULL
            );
        ShowWindow(hWndSubMenu, SW_HIDE);
        submenuWindows.push_back(hWndSubMenu);
        
        i++;
    }
}

void resetInactive(HWND hWnd) {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);
    SendMessage(hWnd, WM_NCACTIVATE, TRUE, 0);
    inactiveFromDragWindow = false;
    inactiveFromSubMenu = false;
}

void renderSingleSubMenuItem(HWND hWnd, int menu, int item, BOOL highlighted) {
    InvalidateRect(hWnd, &submenuItemRects[menu][item], FALSE);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    Graphics graphics(hdc);

    if (highlighted) {
        graphics.DrawImage(submenuItemHBitmaps[menu][item], (int)submenuItemRects[menu][item].left, (int)submenuItemRects[menu][item].top);
    }
    else if (isInactive) {
        graphics.DrawImage(submenuItemIBitmaps[menu][item], (int)submenuItemRects[menu][item].left, (int)submenuItemRects[menu][item].top);
    }
    else {
        graphics.DrawImage(submenuItemBitmaps[menu][item], (int)submenuItemRects[menu][item].left, (int)submenuItemRects[menu][item].top);
    }

    EndPaint(hWnd, &ps);
}

void renderSubMenuBitmaps(int menuNumber) {

    int count = submenuItemRects[menuNumber].size();
    int i = 0;
    while (i < count) {

        renderSingleSubMenuItem(submenuWindows[menuNumber], menuNumber, i, false);

        i++;
    }
}

void hideSubMenus() {

    int count = menuActive.size();
    int i = 0;
    while (i < count) {
        if (menuActive[i]) {
            ShowWindow(submenuWindows[i], SW_HIDE);
            menuActive[i] = false;
        }
        i++;
    }
}


void handleMenuControlClicks(HWND hWnd, POINT pt) {

    int count = menuItems.size();
    if (!submenuInit) {
        menuActive.resize(count);
        int a = 0;
        while (a < count) {
            menuActive[a] = false;
            a++;
        }
        submenuInit = true;
    }

    int i = 0;
    while (i < count) {
        if (PtInRect(&menuItemRects[i], pt)) {
            inactiveFromSubMenu = true;
            submenuActive = true;
            if (!menuActive[i]) {

                int i2 = 0;
                while (i2 < count) {
                    if (i != i2) {
                        if (menuActive[i2]) {
                            ShowWindow(submenuWindows[i2], SW_HIDE);
                            menuActive[i2] = false;
                        }
                    }
                    i2++;
                }

                ShowWindow(submenuWindows[i], SW_SHOW);
                SetFocus(submenuWindows[i]);
                RECT rect = { 0, 0, submenusWidth[i], submenusHeight[i] };
                renderSubMenuBitmaps(i);
                currentActiveSubmenu = i;
				if (currentActiveSubmenu != lastCurrentActiveSubmenu) {
					renderSingleMenuItem(hWnd, lastCurrentActiveSubmenu, false);
					lastCurrentActiveSubmenu = currentActiveSubmenu;
				}
                menuActive[i] = true;
            }
            else {
                ShowWindow(submenuWindows[i], SW_HIDE);
                menuActive[i] = false;
                renderSingleMenuItem(hWnd, i, true);
            }
        }
        i++;
    }

    int b = 0;
    BOOL check = false;
    while (b < count) {
        if (menuActive[b]) {
            check = true;
        }
        b++;
    }
    if (!check) {
        dontRepaint = true;
        resetInactive(hWnd);
        submenuActive = false;
    }
}


void createUI(HWND hWnd) {

    setUISizes(hWnd);

    if (bitmapsLoaded) {
        clearBitmaps();
        bitmapsLoaded = false;
    }

    createUiBitmaps();

    while (!bitmapsLoaded) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    renderWindowUI(hWnd);

    createSubMenuWindows(hWnd);
}

void resetSubmenuBools() {
    
    int count = wasSubItemHovering.size();
    int a = 0;
    while (a < count) {
        int countB = wasSubItemHovering[a].size();
        int b = 0;
        while (b < count) {
            wasSubItemHovering[a][b] = false;
            b++;
        }
        a++;
    }
}

BOOL customInRectCheck(int menu, int item, POINT pt) {

    if (
        pt.x > submenuHoverRects[menu][item].left &&
        pt.x < submenuHoverRects[menu][item].right &&
        pt.y > submenuHoverRects[menu][item].top &&
        pt.y < submenuHoverRects[menu][item].bottom
        ) 
    {
        return true;
    }
    else { 
        return false; 
    }

}

void handleSubmenuHighlight(HWND hWnd, POINT pt) {

    static BOOL initSetup = false;
    if (!initSetup) {
        int Mcount = submenuWindows.size();
        int a = 0;
        while (a < Mcount) {
            std::vector<BOOL> vBool;
            int MIcount = submenuHoverRects[a].size();
            int b = 0;
            while (b < MIcount) {
                vBool.push_back(false);
                b++;
            }
            wasSubItemHovering.push_back(vBool);
            a++;
        }
        initSetup = true;
    }

    int count = submenuHoverRects[currentActiveSubmenu].size();
    int i = 0;
    while (i < count) {
        //BOOL isHovering = PtInRect(&submenuHoverRects[currentActiveSubmenu][i], pt);
        BOOL isHovering = customInRectCheck(currentActiveSubmenu, i, pt);

        if (isHovering != wasSubItemHovering[currentActiveSubmenu][i]) {
            if (isHovering) {
                renderSingleSubMenuItem(submenuWindows[currentActiveSubmenu], currentActiveSubmenu, i, true); 
            }
            else {
                renderSingleSubMenuItem(submenuWindows[currentActiveSubmenu], currentActiveSubmenu, i, false);
            }
        }
        wasSubItemHovering[currentActiveSubmenu][i] = isHovering;
        i++;
    }


}

void trackCursor(HWND hWnd) {

    CursorTrackingRunning = true;
    while (runCursorTracking == true) {
        POINT trackPt;
        GetCursorPos(&trackPt);
        currentScreenPt = trackPt;
        ScreenToClient(hWnd, &trackPt);

        if (!PtInRect(&currentWindowSize, trackPt)) {
            renderWindowControls(hWnd);
            wasExitHovering = false;
            wasMaxHovering = false;
            wasMinHovering = false;
            cursorInWindow = false;
        }
        else {
            cursorInWindow = true;
        }

        if (submenuActive) {
            handleSubmenuHighlight(submenuWindows[currentActiveSubmenu], currentScreenPt);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    CursorTrackingRunning = false;
}

void SetMouseHook() {
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
    if (!hMouseHook) {
        MessageBox(NULL, L"Failed to install mouse hook!", L"Error", MB_OK);
    }
}

void RemoveMouseHook() {
    UnhookWindowsHookEx(hMouseHook);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    currentMonitorSize = { 0, 0, screenWidth, screenHeight };
    setDefaultWindowSize();
    currentWindowSize = defaultWindowSize;
    currentAdjustedSize = currentWindowSize;

    hInst = hInstance;

    HWND hWnd = CreateWindowW(
        windowClass, windowTitle, 
        WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        defaultWindowLeft, defaultWindowTop, defaultWindowSize.right, defaultWindowSize.bottom,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }

    hMainWnd = hWnd;

    WNDCLASS wcDragChild = {};
    wcDragChild.lpfnWndProc = DragWndProc;
    wcDragChild.hInstance = GetModuleHandle(NULL);
    wcDragChild.lpszClassName = L"DragWindowClass";
    wcDragChild.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClass(&wcDragChild);

    /*
    if (!RegisterClass(&wcDragChild)) {
        MessageBox(NULL, L"RegisterClass 'Drag' failed!", L"Error", MB_OK);
        return 1;
    }
    */

    WNDCLASS wcMenuChild = {};
    if (GetClassInfo(GetModuleHandle(NULL), L"MenuWindowClass", &wcMenuChild)) {
        if (!UnregisterClass(L"MenuWindowClass", GetModuleHandle(NULL))) {
            DWORD error = GetLastError();
            WCHAR errorMsg[256];
            swprintf(errorMsg, 256, L"UnregisterClass failed with error %d", error);
            MessageBox(NULL, errorMsg, L"Error", MB_OK);
            return 1;
        }
    }

    wcMenuChild.lpfnWndProc = subMenuWndProc;
    wcMenuChild.hInstance = GetModuleHandle(NULL);
    wcMenuChild.lpszClassName = L"MenuWindowClass";
    wcMenuChild.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

    if (!RegisterClass(&wcMenuChild)) {
        DWORD error = GetLastError();
        WCHAR errorMsg[256];
        swprintf(errorMsg, 256, L"RegisterClass failed with error %d", error);
        MessageBox(NULL, errorMsg, L"Error", MB_OK);
        return 1;
    }


    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    getSystemColors();

    // Initialize GDI+
    InitGDIPlus();

    createUI(hWnd);

    runCursorTracking = true;
    std::thread cursorTrackingThread(trackCursor, hWnd);
    cursorTrackingThread.detach();

    SetMouseHook();

    windowInitFinished = true;

    return TRUE;
}

void handleInactiveWindow(HWND hWnd) {
    int delay = 10;
    int i = 0;
    while (i < delay) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        i++;
        if (isMinimized) {
            i = delay;
        }
    }
    
    if (!isMinimized) {
        isInactive = true;
        SendMessage(hWnd, WM_PAINT, 1, 0);
    }
}

void handleDragIcon(HWND hWnd, POINT pt) {
    if (PtInRect(&rcWindowLeft, pt)) {
        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        dragCursorH = true;
        cursorChanged = true;
    }
    else if (PtInRect(&rcWindowTop, pt)) {
        SetCursor(LoadCursor(NULL, IDC_SIZENS));
        dragCursorV = true;
        cursorChanged = true;
    }
    else if (PtInRect(&rcWindowRight, pt)) {
        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        dragCursorH = true;
        cursorChanged = true;
    }
    else if (PtInRect(&rcWindowBottom, pt)) {
        SetCursor(LoadCursor(NULL, IDC_SIZENS));
        dragCursorV = true;
        cursorChanged = true;
    }
    else if (cursorChanged) {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        dragCursorH = false;
        dragCursorV = false;
        cursorChanged = false;
    }
}

void handleDragging(HWND hWnd, POINT pt) {

    if (PtInRect(&rcWindowLeft, pt)) {
        dragging = true;
        draggingLeft = true;
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        LONG packedCursorPosX = MAKELONG(cursorPos.x, cursorPos.x);
        LONG packedCursorPosY = MAKELONG(cursorPos.y, cursorPos.y);
        POINTS currentCursorPosX = MAKEPOINTS(packedCursorPosX);
        POINTS currentCursorPosY = MAKEPOINTS(packedCursorPosY);
        POINTS newP;
        newP.x = static_cast<SHORT>(cursorPos.x);
        newP.y = static_cast<SHORT>(cursorPos.y);
        lastMousePos = MAKEPOINTS(newP);
    }
    else if (PtInRect(&rcWindowTop, pt)) {
        dragging = true;
        draggingTop = true;
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        LONG packedCursorPosX = MAKELONG(cursorPos.x, cursorPos.x);
        LONG packedCursorPosY = MAKELONG(cursorPos.y, cursorPos.y);
        POINTS currentCursorPosX = MAKEPOINTS(packedCursorPosX);
        POINTS currentCursorPosY = MAKEPOINTS(packedCursorPosY);
        POINTS newP;
        newP.x = static_cast<SHORT>(cursorPos.x);
        newP.y = static_cast<SHORT>(cursorPos.y);
        lastMousePos = MAKEPOINTS(newP);
    }
    else if (PtInRect(&rcWindowRight, pt)) {
        dragging = true;
        draggingRight = true;
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        LONG packedCursorPosX = MAKELONG(cursorPos.x, cursorPos.x);
        LONG packedCursorPosY = MAKELONG(cursorPos.y, cursorPos.y);
        POINTS currentCursorPosX = MAKEPOINTS(packedCursorPosX);
        POINTS currentCursorPosY = MAKEPOINTS(packedCursorPosY);
        POINTS newP;
        newP.x = static_cast<SHORT>(cursorPos.x);
        newP.y = static_cast<SHORT>(cursorPos.y);
        lastMousePos = MAKEPOINTS(newP);
    }
    else if (PtInRect(&rcWindowBottom, pt)) {
        dragging = true;
        draggingBottom = true;
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        LONG packedCursorPosX = MAKELONG(cursorPos.x, cursorPos.x);
        LONG packedCursorPosY = MAKELONG(cursorPos.y, cursorPos.y);
        POINTS currentCursorPosX = MAKEPOINTS(packedCursorPosX);
        POINTS currentCursorPosY = MAKEPOINTS(packedCursorPosY);
        POINTS newP;
        newP.x = static_cast<SHORT>(cursorPos.x);
        newP.y = static_cast<SHORT>(cursorPos.y);
        lastMousePos = MAKEPOINTS(newP);
    }
}

void releaseDragging() {
    dragging = false;
    draggingLeft = false;
    draggingTop = false;
    draggingRight = false;
    draggingBottom = false;

}

void createDragchild(HWND hWnd) {
    hDragWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, L"DragWindowClass", NULL, WS_POPUP | WS_CHILD,
        dragWindowLeft, dragWindowTop, dragWindowSize.right, dragWindowSize.bottom, hWnd, NULL, GetModuleHandle(NULL), NULL);
    SetLayeredWindowAttributes(hDragWnd, 0, (255 * 70) / 100, LWA_ALPHA); // 70% transparency
    ShowWindow(hDragWnd, SW_SHOW);
    paintRect(hDragWnd, dragWindowSize, RGB(155, 155, 255));
}

void destroyDragChild() {
    if (hDragWnd) {
        DestroyWindow(hDragWnd);
        hDragWnd = NULL;
    }
}

void dragLeft(HWND hWnd, POINT cursorPos) {

    static POINT lastCursorPos = {0, 0};

    if (cursorPos.x != lastCursorPos.x || cursorPos.y != lastCursorPos.y) {

        LONG packedCursorPosX = MAKELONG(cursorPos.x, cursorPos.x);
        LONG packedCursorPosY = MAKELONG(cursorPos.y, cursorPos.y);

        POINTS currentCursorPosX = MAKEPOINTS(packedCursorPosX);
        POINTS currentCursorPosY = MAKEPOINTS(packedCursorPosY);

        int dx = currentCursorPosX.x - lastMousePos.x;

        dragWindowLeft = dragWindowLeft + dx;
        currentAdjustedSize = { 0, 0,
            dragWindowSize.right - dx, dragWindowSize.bottom };
        dragWindowSize = currentAdjustedSize;
        SetWindowPos(hDragWnd, NULL, dragWindowLeft, dragWindowTop,
            dragWindowSize.right, dragWindowSize.bottom, SWP_NOZORDER | SWP_NOREDRAW);
        paintRect(hDragWnd, dragWindowSize, RGB(155, 155, 255));

        POINTS newP;
        newP.x = static_cast<SHORT>(cursorPos.x);
        newP.y = static_cast<SHORT>(cursorPos.y);

        lastMousePos = MAKEPOINTS(newP);
        lastCursorPos = cursorPos;
    }

}

void dragTop(HWND hWnd, POINT cursorPos) {

    static POINT lastCursorPos = { 0, 0 };

    if (cursorPos.x != lastCursorPos.x || cursorPos.y != lastCursorPos.y) {

        LONG packedCursorPosX = MAKELONG(cursorPos.x, cursorPos.x);
        LONG packedCursorPosY = MAKELONG(cursorPos.y, cursorPos.y);

        POINTS currentCursorPosX = MAKEPOINTS(packedCursorPosX);
        POINTS currentCursorPosY = MAKEPOINTS(packedCursorPosY);

        int dy = currentCursorPosY.y - lastMousePos.y;

        dragWindowTop = dragWindowTop + dy;
        currentAdjustedSize = { 0, 0,
            dragWindowSize.right, dragWindowSize.bottom - dy };
        dragWindowSize = currentAdjustedSize;
        SetWindowPos(hDragWnd, NULL, dragWindowLeft, dragWindowTop,
            dragWindowSize.right, dragWindowSize.bottom, SWP_NOZORDER | SWP_NOREDRAW);
        paintRect(hDragWnd, dragWindowSize, RGB(155, 155, 255));

        POINTS newP;
        newP.x = static_cast<SHORT>(cursorPos.x);
        newP.y = static_cast<SHORT>(cursorPos.y);

        lastMousePos = newP;
        lastCursorPos = cursorPos;
    }
}

void dragRight(HWND hWnd, POINT cursorPos) {

    static POINT lastCursorPos = { 0, 0 };

    if (cursorPos.x != lastCursorPos.x || cursorPos.y != lastCursorPos.y) {

    LONG packedCursorPosX = MAKELONG(cursorPos.x, cursorPos.x);
    LONG packedCursorPosY = MAKELONG(cursorPos.y, cursorPos.y);

    POINTS currentCursorPosX = MAKEPOINTS(packedCursorPosX);
    POINTS currentCursorPosY = MAKEPOINTS(packedCursorPosY);

    int dx = currentCursorPosX.x - lastMousePos.x;

    currentAdjustedSize = { 0, 0,
        dragWindowSize.right + dx, dragWindowSize.bottom };
    dragWindowSize = currentAdjustedSize;
    SetWindowPos(hDragWnd, NULL, dragWindowLeft, dragWindowTop,
        dragWindowSize.right, dragWindowSize.bottom, SWP_NOZORDER | SWP_NOREDRAW);
    paintRect(hDragWnd, dragWindowSize, RGB(155, 155, 255));

    POINTS newP;
    newP.x = static_cast<SHORT>(cursorPos.x);
    newP.y = static_cast<SHORT>(cursorPos.y);

    lastMousePos = MAKEPOINTS(newP);
    lastCursorPos = cursorPos;
    }
}

void dragBottom(HWND hWnd, POINT cursorPos) {

    static POINT lastCursorPos = { 0, 0 };

    if (cursorPos.x != lastCursorPos.x || cursorPos.y != lastCursorPos.y) {

    LONG packedCursorPosX = MAKELONG(cursorPos.x, cursorPos.x);
    LONG packedCursorPosY = MAKELONG(cursorPos.y, cursorPos.y);

    POINTS currentCursorPosX = MAKEPOINTS(packedCursorPosX);
    POINTS currentCursorPosY = MAKEPOINTS(packedCursorPosY);

    int dy = currentCursorPosY.y - lastMousePos.y;

    currentAdjustedSize = { 0, 0,
        dragWindowSize.right, dragWindowSize.bottom + dy };
    dragWindowSize = currentAdjustedSize;
    SetWindowPos(hDragWnd, NULL, dragWindowLeft, dragWindowTop,
        dragWindowSize.right, dragWindowSize.bottom, SWP_NOZORDER | SWP_NOREDRAW);
    paintRect(hDragWnd, dragWindowSize, RGB(155, 155, 255));

    POINTS newP;
    newP.x = static_cast<SHORT>(cursorPos.x);
    newP.y = static_cast<SHORT>(cursorPos.y);

    lastMousePos = newP;
    lastCursorPos = cursorPos;
    }
}


//wndproc, handles window messages-------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    getSystemColors();
    if (uiColorUpdate) {
        createUI(hWnd);
        uiColorUpdate = false;
    }

    static BOOL dragWindowExists = false;

    if (windowInitFinished == true) {
        switch (message) {

        case WM_PAINT:
        {
            renderWindowUI(hWnd);
        }
        break;

        case WM_NCHITTEST:
        {

            POINT wdPt;
            GetCursorPos(&wdPt);
            ScreenToClient(hWnd, &wdPt);

            // Check if the cursor is within the caption bar
            if (PtInRect(&rcCaptionDrag, wdPt)) {
                windowDragable = true;
                return HTCAPTION;
            }
            else {
                windowDragable = false;
            }

            if (!isMaximized && !isInactive) {
                handleDragIcon(hWnd, wdPt);
            }
            else if (cursorChanged) {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                dragCursorH = false;
                dragCursorV = false;
                cursorChanged = false;
            }

            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
        case WM_LBUTTONDOWN:
        {
            POINT clickPt;
            GetCursorPos(&clickPt);
            ScreenToClient(hWnd, &clickPt);

            handleCaptionControlClicks(hWnd, clickPt);

            handleMenuControlClicks(hWnd, clickPt);

            if (!isMaximized && !isInactive) {
                handleDragging(hWnd, clickPt);
            }

            if (windowDragable) {
                windowMoved = true;
            }

            if (dragging) {
                SetCapture(hWnd);
                inactiveFromDragWindow = true;
                dragWindowLeft = currentWindowLeft;
                dragWindowTop = currentWindowTop;
                dragWindowSize = currentWindowSize;
                createDragchild(hWnd);
                paintRect(hDragWnd, currentWindowSize, RGB(155, 155, 255));
            }
        }
        break;
        case WM_LBUTTONUP:
        {
            ReleaseCapture();
            if (dragging) {
                releaseDragging();
                destroyDragChild();
                currentWindowLeft = dragWindowLeft;
                currentWindowTop = dragWindowTop;
                currentWindowSize = dragWindowSize;
                SetWindowPos(hWnd, NULL, currentWindowLeft, currentWindowTop,
                    currentWindowSize.right, currentWindowSize.bottom, SWP_NOZORDER);
                setUISizes(hWnd);
                moveSubmenuWindows();
                SendMessage(hWnd, WM_PAINT, 1, 0);
                std::thread taskThread(resetInactive, hWnd);
                taskThread.detach();
            }
        }
        break;
        case WM_MOUSEMOVE:
        {
            POINT movePt;
            GetCursorPos(&movePt);
            ScreenToClient(hWnd, &movePt);

            if (!isMinimized) {
                handleCaptionHighlight(hWnd, movePt);
                handleMenuHighlight(hWnd, movePt);
            }

            if (!submenuActive) {
                if (dragCursorH) {
                    SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                }
                if (dragCursorV) {
                    SetCursor(LoadCursor(NULL, IDC_SIZENS));
                }
            }

            if (dragging) {

                if (draggingLeft) {
                    POINT cursorPos;
                    GetCursorPos(&cursorPos);
                    dragLeft(hWnd, cursorPos);
                }
                else if (draggingTop) {
                    POINT cursorPos;
                    GetCursorPos(&cursorPos);
                    dragTop(hWnd, cursorPos);

                }
                else if (draggingRight) {
                    POINT cursorPos;
                    GetCursorPos(&cursorPos);
                    dragRight(hWnd, cursorPos);
                }
                else if (draggingBottom) {
                    POINT cursorPos;
                    GetCursorPos(&cursorPos);
                    dragBottom(hWnd, cursorPos);
                }
            }

        }
        break;
        case WM_NCACTIVATE:
        {
            if (wParam == FALSE) {
                if (inactiveFromSubMenu) {
                    if (!cursorInWindow) {
                        inactiveFromSubMenu = false;
                        submenuActive = false;
                        hideSubMenus();
                        std::thread taskThread(handleInactiveWindow, hWnd);
                        taskThread.detach();
                    }
                }
                
                else if (!inactiveFromDragWindow) {
                    std::thread taskThread(handleInactiveWindow, hWnd);
                    taskThread.detach();
                }
                return 0;

            }
            if (wParam == TRUE) {
                isInactive = false;
                if (!dontRepaint) {
                    SendMessage(hWnd, WM_PAINT, 1, 0);
                }
                else {
                    dontRepaint = false;
                }
                return 0;
            }
            return 0;
        }
        break;
        case WM_SIZE:
        {
            if (wParam == SIZE_RESTORED) {
                // The window has been resized, but neither minimized nor maximized
                isMinimized = false;
                if (isMaximized == true) {
                    currentWindowSize = currentAdjustedSize;
                    setUISizes(hWnd);
                    moveSubmenuWindows();
                    isMaximized = false;
                }
                //renderWindowUI(hWnd);
                SendMessage(hWnd, WM_PAINT, 1, 0);
                return 0;
            }
            else if (wParam == SIZE_MAXIMIZED) {
                // The window has been maximized
                setCurrentMonitorSize(hWnd);
                resizeWindow(hWnd, currentMonitorSize);
                currentWindowSize = currentMonitorSize;
                setUISizes(hWnd);
                moveSubmenuWindows();
                isMaximized = true;
                isMinimized = false;
                SendMessage(hWnd, WM_PAINT, 1, 0);
                return 0;
            }
            else if (wParam == SIZE_MINIMIZED) {
                // The window has been minimized
                isMinimized = true;
                return 0;
            }
            else if (wParam == SIZE_MAXSHOW) {
                // The window has been maximized from minimized
                isMinimized = false;
                SendMessage(hWnd, WM_PAINT, 1, 0);
                return 0;
            }
            return 0;
        }
        break;
        case WM_MOVE: 
        {
            currentWindowLeft = LOWORD(lParam);
            currentWindowTop = HIWORD(lParam);
        }
        break;
        case WM_ERASEBKGND:
        {
            return 1; // Prevent background erasure to reduce flicker
        }
        break;
        case WM_DESTROY:
        {
            destroyMenuChildWindows();
            CleanupGDIPlus();
            runCursorTracking = false;
            while (CursorTrackingRunning == true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            while (bitmapsReset == false) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            PostQuitMessage(0);
        }
        break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }
}

LRESULT CALLBACK DragWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: 
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // Paint the semi-transparent window content here
        EndPaint(hWnd, &ps);
        return 0;
    }
    break;
    case WM_NCCALCSIZE:
    {
        return 0;
    }
    break;
    case WM_ERASEBKGND:
    {
        return TRUE;
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

LRESULT CALLBACK subMenuWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

        case WM_ERASEBKGND:
        {
            return 1;
        }
        break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

}

// Callback function for mouse events
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        switch (wParam) {
        case WM_LBUTTONDOWN: 
        {
            if (submenuActive) {
                POINT downPt;
                GetCursorPos(&downPt);
                ScreenToClient(hMainWnd, &downPt);

                if (!PtInRect(&rcMenuArea, downPt)) {
                    ShowWindow(submenuWindows[currentActiveSubmenu], SW_HIDE);
                    menuActive[currentActiveSubmenu] = false;
                    submenuActive = false;
                    renderMenuBarItems(hMainWnd);
                    if (wasHovering.size() > 0) {
                        clearMenuHover();
                    }
                    
                }
            }
        }
        break;
        case WM_LBUTTONUP:
        {
            POINT upPt;
            GetCursorPos(&upPt);
            ScreenToClient(hMainWnd, &upPt);

            if (PtInRect(&rcCaptionDrag, upPt)) {
                
                updateCurrentWindowPositions();
                
                if (oldLeft != currentWindowLeft || oldTop != currentWindowTop) {
                    createSubmenuWindowRects(hMainWnd);
                    moveSubmenuWindows();
                    oldLeft = currentWindowLeft;
                    oldTop = currentWindowTop;
                }
            }
        }
        break;
        }
    }
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}
