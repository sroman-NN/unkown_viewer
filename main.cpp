// programa monolitico , unkown viewer , editor de texto simple con soporte UTF-8 sin resaltado de syntax ni  
// configuraciones adicionales solo un programa similar a un bloc de notas pero ligero y sin dependencias 
// en este codigo solo se utilizan funciones nativas de WIN32 y WINAPI y c++ 17
// -----------------------------------------------------------------------------------
//  version de codigo : 0.3
//  ultima compilacion: 11/10/2026 21:49 GMT-4
//  version de programa: 1.0.0



#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include "resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "kernel32.lib")

constexpr int   STATUS_BAR_HEIGHT = 28;
constexpr COLORREF COLOR_CANVAS_BG  = RGB(255, 255, 255);
constexpr COLORREF COLOR_CANVAS_TXT = RGB(15, 15, 15);
constexpr COLORREF COLOR_BAR_BG     = RGB(38, 38, 42);
constexpr COLORREF COLOR_BAR_BORDER = RGB(55, 55, 60);
constexpr COLORREF COLOR_BAR_TXT    = RGB(240, 240, 240);
constexpr COLORREF COLOR_BAR_ACCENT = RGB(130, 185, 250);

// Estados Globales 
struct AppState {
    HWND        hwndMain   = nullptr;
    HWND        hwndEdit   = nullptr;
    WNDPROC     origEditProc = nullptr;
    HFONT       hFontEditor = nullptr;
    HFONT       hFontStatus = nullptr;
    HBRUSH      hBrushWhite = nullptr;
    HBRUSH      hBrushBar   = nullptr;
    HPEN        hPenBorder  = nullptr;
    
    std::wstring currentFilePath;
    std::wstring statusMessage;
    DWORD       statusTimerId = 0;
    
    int charWidth  = 8;
    int charHeight = 16;
    int currentLine = 1;
    int currentCol  = 1;
    bool isModified = false;
};

static AppState g_app;

// Prototipos
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SubclassedEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateCursorPosition();
void OpenFileDialog();
void SaveFileDialog(bool saveAs = false);
bool LoadFileContent(const std::wstring& path);
bool SaveFileContent(const std::wstring& path);
void SetStatusTempMessage(const std::wstring& msg, UINT durationMs = 3000);
void UpdateWindowTitle();
void SetupEditorCaret();

// Conversiones UTF-8 / UTF-16
std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int req = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
    if (req <= 0) {
        req = MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), nullptr, 0);
        std::wstring out(req, 0);
        MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), &out[0], req);
        return out;
    }
    std::wstring out(req, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &out[0], req);
    return out;
}

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int req = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (req <= 0) return "";
    std::string out(req, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &out[0], req, nullptr, nullptr);
    return out;
}

// Configuracion de Caret/cursor estilo terminal
void SetupEditorCaret() {
    if (!g_app.hwndEdit) return;
    CreateCaret(g_app.hwndEdit, nullptr, g_app.charWidth, g_app.charHeight);
    ShowCaret(g_app.hwndEdit);
}

// Procedimiento-Subclase para el Control Edit
LRESULT CALLBACK SubclassedEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_SETFOCUS: {
            LRESULT res = CallWindowProcW(g_app.origEditProc, hwnd, uMsg, wParam, lParam);
            SetupEditorCaret();
            UpdateCursorPosition();
            return res;
        }
        case WM_KILLFOCUS: {
            DestroyCaret();
            return CallWindowProcW(g_app.origEditProc, hwnd, uMsg, wParam, lParam);
        }
        case WM_KEYDOWN: {
            bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (isCtrl) {
                switch (wParam) {
                    case 'O':
                    case 'o':
                        OpenFileDialog();
                        return 0;
                    case 'S':
                    case 's':
                        SaveFileDialog(false);
                        return 0;
                    case 'X':
                    case 'x':
                        PostMessageW(g_app.hwndMain, WM_CLOSE, 0, 0);
                        return 0;
                    case 'A':
                    case 'a':
                        SendMessageW(hwnd, EM_SETSEL, 0, -1);
                        UpdateCursorPosition();
                        return 0;
                    case 'Z':
                    case 'z':
                        SendMessageW(hwnd, EM_UNDO, 0, 0);
                        UpdateCursorPosition();
                        return 0;
                    case 'Y':
                    case 'y':
                        // Redo / toggle undo
                        SendMessageW(hwnd, EM_UNDO, 0, 0);
                        UpdateCursorPosition();
                        return 0;
                    default:
                        break;
                }
            }
            break;
        }
        case WM_KEYUP:
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE: {
            LRESULT res = CallWindowProcW(g_app.origEditProc, hwnd, uMsg, wParam, lParam);
            UpdateCursorPosition();
            return res;
        }
        default:
            break;
    }
    return CallWindowProcW(g_app.origEditProc, hwnd, uMsg, wParam, lParam);
}

// Actualiza las cordenadas de linea y columna
void UpdateCursorPosition() {
    if (!g_app.hwndEdit) return;
    DWORD selStart = 0;
    SendMessageW(g_app.hwndEdit, EM_GETSEL, (WPARAM)&selStart, 0);
    LRESULT line = SendMessageW(g_app.hwndEdit, EM_LINEFROMCHAR, (WPARAM)selStart, 0);
    LRESULT lineIndex = SendMessageW(g_app.hwndEdit, EM_LINEINDEX, (WPARAM)line, 0);
    LRESULT col = (LRESULT)selStart - lineIndex;

    g_app.currentLine = (int)line + 1;
    g_app.currentCol  = (int)col + 1;

    // Redibujar la barra de estado
    RECT rcClient;
    GetClientRect(g_app.hwndMain, &rcClient);
    RECT rcBar = { 0, rcClient.bottom - STATUS_BAR_HEIGHT, rcClient.right, rcClient.bottom };
    InvalidateRect(g_app.hwndMain, &rcBar, FALSE);
}

// Dialogo Abrir Archivo
void OpenFileDialog() {
    wchar_t szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = g_app.hwndMain;
    ofn.lpstrFilter = L"All Files (*.*)\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        if (LoadFileContent(szFile)) {
            g_app.currentFilePath = szFile;
            g_app.isModified = false;
            UpdateWindowTitle();
            SetStatusTempMessage(L"document_opened");
            UpdateCursorPosition();
        } else {
            SetStatusTempMessage(L"error_opening_file!");
        }
    }
}

// Dialogo save
void SaveFileDialog(bool saveAs) {
    if (!saveAs && !g_app.currentFilePath.empty()) {
        if (SaveFileContent(g_app.currentFilePath)) {
            g_app.isModified = false;
            UpdateWindowTitle();
            SetStatusTempMessage(L"file_saved");
        } else {
            SetStatusTempMessage(L"error_saving_file");
        }
        return;
    }

    wchar_t szFile[MAX_PATH] = L"";
    if (!g_app.currentFilePath.empty()) {
        wcsncpy_s(szFile, g_app.currentFilePath.c_str(), _TRUNCATE);
    }

    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = g_app.hwndMain;
    ofn.lpstrFilter = L"All Files (*.*)\0*";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameW(&ofn)) {
        if (SaveFileContent(szFile)) {
            g_app.currentFilePath = szFile;
            g_app.isModified = false;
            UpdateWindowTitle();
            SetStatusTempMessage(L"file_saved");
        } else {
            SetStatusTempMessage(L"error_saving_file");
        }
    }
}

// carga fisica del contenido , win32 IO
bool LoadFileContent(const std::wstring& path) {
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return false;
    }

    if (fileSize.QuadPart > 128 * 1024 * 1024) { // limite de seguridad de 128 MB ============ ! ! ! !
        CloseHandle(hFile);
        return false;
    }

    DWORD bytesToRead = (DWORD)fileSize.QuadPart;
    std::string buffer(bytesToRead, '\0');
    DWORD bytesRead = 0;

    if (bytesToRead > 0) {
        if (!ReadFile(hFile, &buffer[0], bytesToRead, &bytesRead, nullptr)) {
            CloseHandle(hFile);
            return false;
        }
        buffer.resize(bytesRead);
    }
    CloseHandle(hFile);

    if (buffer.size() >= 3 && 
        (unsigned char)buffer[0] == 0xEF && 
        (unsigned char)buffer[1] == 0xBB && 
        (unsigned char)buffer[2] == 0xBF) {
        buffer = buffer.substr(3);
    }

    std::wstring wideContent = Utf8ToWide(buffer);
    SetWindowTextW(g_app.hwndEdit, wideContent.c_str());
    SendMessageW(g_app.hwndEdit, EM_EMPTYUNDOBUFFER, 0, 0);
    return true;
}

// Guardar contenido con : win 32 IO
bool SaveFileContent(const std::wstring& path) {
    int length = GetWindowTextLengthW(g_app.hwndEdit);
    std::wstring wideContent(length + 1, L'\0');
    GetWindowTextW(g_app.hwndEdit, &wideContent[0], length + 1);
    wideContent.resize(length);

    std::string utf8Content = WideToUtf8(wideContent);

    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD bytesWritten = 0;
    if (!utf8Content.empty()) {
        if (!WriteFile(hFile, utf8Content.data(), (DWORD)utf8Content.size(), &bytesWritten, nullptr)) {
            CloseHandle(hFile);
            return false;
        }
    }
    CloseHandle(hFile);
    return true;
}

// Actualiza el title window
void UpdateWindowTitle() {
    std::wstring title = L"Unkown Viewer";
    if (!g_app.currentFilePath.empty()) {
        size_t idx = g_app.currentFilePath.find_last_of(L"\\/");
        std::wstring fname = (idx == std::wstring::npos) ? g_app.currentFilePath : g_app.currentFilePath.substr(idx + 1);
        title += L" - " + fname;
    } else {
        title += L" - [Unamed]";
    }
    if (g_app.isModified) {
        title += L" *";
    }
    SetWindowTextW(g_app.hwndMain, title.c_str());
}

// Mensjaes temporales del toolbar de estado
void SetStatusTempMessage(const std::wstring& msg, UINT durationMs) {
    g_app.statusMessage = msg;
    if (g_app.statusTimerId) {
        KillTimer(g_app.hwndMain, g_app.statusTimerId);
    }
    g_app.statusTimerId = SetTimer(g_app.hwndMain, 1, durationMs, nullptr);

    RECT rcClient;
    GetClientRect(g_app.hwndMain, &rcClient);
    RECT rcBar = { 0, rcClient.bottom - STATUS_BAR_HEIGHT, rcClient.right, rcClient.bottom };
    InvalidateRect(g_app.hwndMain, &rcBar, FALSE);
}

// Procedimientos de la ventan principal
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            g_app.hwndMain = hwnd;

            // brochas y plumas
            g_app.hBrushWhite = CreateSolidBrush(COLOR_CANVAS_BG);
            g_app.hBrushBar   = CreateSolidBrush(COLOR_BAR_BG);
            g_app.hPenBorder  = CreatePen(PS_SOLID, 1, COLOR_BAR_BORDER);

            // editor de fuente
            HDC hdc = GetDC(hwnd);
            int fontHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            g_app.hFontEditor = CreateFontW(
                fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas"
            );

            // medir dimensiones del caracter para el caret
            HFONT hOldFont = (HFONT)SelectObject(hdc, g_app.hFontEditor);
            TEXTMETRICW tm;
            GetTextMetricsW(hdc, &tm);
            g_app.charWidth  = tm.tmAveCharWidth;
            g_app.charHeight = tm.tmHeight;
            SelectObject(hdc, hOldFont);

            // fuente para barra de estado
            int statusHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            g_app.hFontStatus = CreateFontW(
                statusHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Segoe UI"
            );
            ReleaseDC(hwnd, hdc);

            // control de edicion multilinea
            RECT rc;
            GetClientRect(hwnd, &rc);
            int editH = (rc.bottom > STATUS_BAR_HEIGHT) ? (rc.bottom - STATUS_BAR_HEIGHT) : 0;

            g_app.hwndEdit = CreateWindowExW(
                0,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN | ES_NOHIDESEL,
                0, 0, rc.right, editH,
                hwnd,
                (HMENU)1001,
                ((LPCREATESTRUCT)lParam)->hInstance,
                nullptr
            );

            SendMessageW(g_app.hwndEdit, WM_SETFONT, (WPARAM)g_app.hFontEditor, TRUE);
            SendMessageW(g_app.hwndEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(10, 10));

            // subclasear para caret y atajos
            g_app.origEditProc = (WNDPROC)SetWindowLongPtrW(g_app.hwndEdit, GWLP_WNDPROC, (LONG_PTR)SubclassedEditProc);

            UpdateWindowTitle();
            SetFocus(g_app.hwndEdit);
            return 0;
        }

        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            int editH = (h > STATUS_BAR_HEIGHT) ? (h - STATUS_BAR_HEIGHT) : 0;

            if (g_app.hwndEdit) {
                MoveWindow(g_app.hwndEdit, 0, 0, w, editH, TRUE);
            }

            RECT rcBar = { 0, editH, w, h };
            InvalidateRect(hwnd, &rcBar, FALSE);
            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == 1001 && HIWORD(wParam) == EN_CHANGE) {
                if (!g_app.isModified) {
                    g_app.isModified = true;
                    UpdateWindowTitle();
                }
                UpdateCursorPosition();
            }
            return 0;
        }

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, COLOR_CANVAS_TXT);
            SetBkColor(hdc, COLOR_CANVAS_BG);
            return (LRESULT)g_app.hBrushWhite;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rcClient;
            GetClientRect(hwnd, &rcClient);

            // resrevar espacio para barra de estado
            RECT rcBar = { 0, rcClient.bottom - STATUS_BAR_HEIGHT, rcClient.right, rcClient.bottom };

            // barra de estado - background - color
            FillRect(hdc, &rcBar, g_app.hBrushBar);

            // hr
            HPEN hOldPen = (HPEN)SelectObject(hdc, g_app.hPenBorder);
            MoveToEx(hdc, rcBar.left, rcBar.top, nullptr);
            LineTo(hdc, rcBar.right, rcBar.top);
            SelectObject(hdc, hOldPen);

            // renderizar texto para H y M
            HFONT hOldFont = (HFONT)SelectObject(hdc, g_app.hFontStatus);
            SetBkMode(hdc, TRANSPARENT);

            // comandos
            const wchar_t* cmds = L"CTRL+O ()   CTRL+S (SAVE)   CTRL+X (CLOSE)   CTRL+Z (UNDO)   CTRL+A (ALL)";
            RECT rcLeft = rcBar;
            rcLeft.left += 12;
            SetTextColor(hdc, COLOR_BAR_TXT);
            DrawTextW(hdc, cmds, -1, &rcLeft, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // alerts
            wchar_t szRight[128];
            if (!g_app.statusMessage.empty()) {
                swprintf_s(szRight, L"%s   |   Ln %d, Col %d   |   UTF-8", g_app.statusMessage.c_str(), g_app.currentLine, g_app.currentCol);
                SetTextColor(hdc, COLOR_BAR_ACCENT);
            } else {
                swprintf_s(szRight, L"Ln %d, Col %d   |   UTF-8", g_app.currentLine, g_app.currentCol);
                SetTextColor(hdc, RGB(180, 180, 185));
            }

            RECT rcRight = rcBar;
            rcRight.right -= 14;
            DrawTextW(hdc, szRight, -1, &rcRight, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_TIMER: {
            if (wParam == 1) {
                KillTimer(hwnd, 1);
                g_app.statusTimerId = 0;
                g_app.statusMessage.clear();
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                RECT rcBar = { 0, rcClient.bottom - STATUS_BAR_HEIGHT, rcClient.right, rcClient.bottom };
                InvalidateRect(hwnd, &rcBar, FALSE);
            }
            return 0;
        }

        case WM_SETFOCUS: {
            if (g_app.hwndEdit) {
                SetFocus(g_app.hwndEdit);
            }
            return 0;
        }

        case WM_CLOSE: {
            if (g_app.isModified) {
                int ans = MessageBoxW(
                    hwnd,
                    L"¿Want save your changes before exit?",
                    L"Unkown Viewer",
                    MB_YESNOCANCEL | MB_ICONQUESTION
                );
                if (ans == IDYES) {
                    SaveFileDialog(false);
                } else if (ans == IDCANCEL) {
                    return 0;
                }
            }
            DestroyWindow(hwnd);
            return 0;
        }

        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            wchar_t szFile[MAX_PATH] = L"";
            if (DragQueryFileW(hDrop, 0, szFile, MAX_PATH)) {
                if (LoadFileContent(szFile)) {
                    g_app.currentFilePath = szFile;
                    g_app.isModified = false;
                    UpdateWindowTitle();
                    SetStatusTempMessage(L"File Load");
                    UpdateCursorPosition();
                }
            }
            DragFinish(hDrop);
            return 0;
        }

        // clean
        case WM_DESTROY: {
            if (g_app.hBrushWhite) DeleteObject(g_app.hBrushWhite);
            if (g_app.hBrushBar)   DeleteObject(g_app.hBrushBar);
            if (g_app.hPenBorder)  DeleteObject(g_app.hPenBorder);
            if (g_app.hFontEditor) DeleteObject(g_app.hFontEditor);
            if (g_app.hFontStatus) DeleteObject(g_app.hFontStatus);
            PostQuitMessage(0);
            return 0;
        }

        default:
            break;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// punto de entrada para winapi-window
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    const wchar_t CLASS_NAME[] = L"UnkownViewerClass";

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainWindowProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = CLASS_NAME;
    wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"Fail to register the window", L"Error", MB_ICONERROR);
        return 1;
    }

    // creacion y parametros de ventana
    HWND hwnd = CreateWindowExW(
        WS_EX_ACCEPTFILES,
        CLASS_NAME,
        L"Unkown Viewer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 960, 640,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd) {
        MessageBoxW(nullptr, L"Fail in creating window", L"Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    DragAcceptFiles(hwnd, TRUE);

    //                                 ---
    //                                 ---
    //                                 ---

    int numArgs = 0;
    LPWSTR* argList = CommandLineToArgvW(GetCommandLineW(), &numArgs);
    if (argList) {
        if (numArgs > 1 && wcslen(argList[1]) > 0) {
            if (LoadFileContent(argList[1])) {
                g_app.currentFilePath = argList[1];
                g_app.isModified = false;
                UpdateWindowTitle();
                UpdateCursorPosition();
            }
        }
        LocalFree(argList);
    }

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    return wWinMain(hInstance, nullptr, GetCommandLineW(), nCmdShow);
}

// el uso comercial de este codigo o su modificacion para uso cerrado (closed source) esta prohibido , 
// protegido bajo la licencia GPLV3