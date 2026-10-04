// language: C++17, file: main.cpp, app: tradelog — crypto trade journal
// Windows 11, MSVC, WinAPI + comdlg32. No external dependencies, offline.
// Records trades (pair, side, price, size) in a ListView and saves/loads
// the journal as CSV via the standard file dialogs.
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

struct Trade { std::wstring pair, side, price, size; };

static HWND g_list, g_pair, g_side, g_price, g_size;
static std::vector<Trade> g_trades;

static void AddRow() {
    wchar_t pb[64], pp[32], pr[32], sz[32];
    GetWindowTextW(g_pair, pb, 64); GetWindowTextW(g_side, pp, 32);
    GetWindowTextW(g_price, pr, 32); GetWindowTextW(g_size, sz, 32);
    if (!pb[0] || !pr[0]) return;
    g_trades.push_back({ pb, pp, pr, sz });
    LVITEMW it{}; it.mask = LVIF_TEXT;
    it.iItem = ListView_GetItemCount(g_list);
    it.pszText = pb;
    ListView_InsertItem(g_list, &it);
    ListView_SetItemText(g_list, it.iItem, 1, pp);
    ListView_SetItemText(g_list, it.iItem, 2, pr);
    ListView_SetItemText(g_list, it.iItem, 3, sz);
    SetWindowTextW(g_pair, L""); SetWindowTextW(g_price, L""); SetWindowTextW(g_size, L"");
}

// Save the journal as CSV via GetSaveFileName.
static void SaveCsv(HWND owner) {
    wchar_t file[MAX_PATH] = L"trades.csv";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = owner;
    ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"CSV files\0*.csv\0All\0*.*\0";
    ofn.lpstrDefExt = L"csv";
    if (!GetSaveFileNameW(&ofn)) return;
    HANDLE f = CreateFileW(file, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (f == INVALID_HANDLE_VALUE) return;
    std::string csv = "pair,side,price,size\n";
    char a[128];
    for (auto& t : g_trades) {
        char pb[64], pp[32], pr[32], sz[32];
        WideCharToMultiByte(CP_UTF8, 0, t.pair.c_str(), -1, pb, 64, nullptr, nullptr);
        WideCharToMultiByte(CP_UTF8, 0, t.side.c_str(), -1, pp, 32, nullptr, nullptr);
        WideCharToMultiByte(CP_UTF8, 0, t.price.c_str(), -1, pr, 32, nullptr, nullptr);
        WideCharToMultiByte(CP_UTF8, 0, t.size.c_str(), -1, sz, 32, nullptr, nullptr);
        snprintf(a, 128, "%s,%s,%s,%s\n", pb, pp, pr, sz);
        csv += a;
    }
    DWORD w; WriteFile(f, csv.data(), (DWORD)csv.size(), &w, nullptr);
    CloseHandle(f);
}

static LRESULT CALLBACK WndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_LISTVIEW_CLASSES };
        InitCommonControlsEx(&icc);
        HFONT f = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI");
        auto input = [&](HWND& s, const wchar_t* ph, int x, int wd) {
            s = CreateWindowW(L"EDIT", ph, WS_CHILD | WS_VISIBLE | WS_BORDER, x, 18, wd, 26, w, nullptr, nullptr, nullptr);
            SendMessageW(s, WM_SETFONT, (WPARAM)f, TRUE);
        };
        input(g_pair, L"BTC/USDT", 16, 120);
        input(g_side, L"BUY", 144, 70);
        input(g_price, L"61240", 222, 100);
        input(g_size, L"0.5", 330, 80);
        CreateWindowW(L"BUTTON", L"Add", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 420, 16, 70, 30, w, (HMENU)1, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Save CSV", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 500, 16, 100, 30, w, (HMENU)2, nullptr, nullptr);
        g_list = CreateWindowExW(0, WC_LISTVIEWW, nullptr,
                                 WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                 16, 60, 740, 340, w, (HMENU)3, nullptr, nullptr);
        LVCOLUMNW col{}; col.mask = LVCF_TEXT | LVCF_WIDTH;
        const wchar_t* heads[] = { L"Pair", L"Side", L"Price", L"Size" };
        int widths[] = { 200, 120, 200, 180 };
        for (int i = 0; i < 4; ++i) {
            col.pszText = (LPWSTR)heads[i]; col.cx = widths[i];
            ListView_InsertColumn(g_list, i, &col);
        }
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == 1) AddRow();
        if (LOWORD(wp) == 2) SaveCsv(w);
        return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(w, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int show) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc; wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(13, 17, 23));
    wc.lpszClassName = L"TradeLogWnd";
    RegisterClassW(&wc);
    HWND w = CreateWindowExW(0, L"TradeLogWnd", L"TradeLog — crypto trade journal (offline)",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 790, 460,
                             nullptr, nullptr, inst, nullptr);
    ShowWindow(w, show);
    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) { TranslateMessage(&m); DispatchMessageW(&m); }
    return 0;
}
