#include <iostream>
#include <Windows.h>
#include <mmsystem.h>
#include <cmath>
#include <string>
#include <thread>

#pragma comment(lib, "winmm.lib")

#define ID_BTN_BASLAT 1001
#define ID_EDT_ESIK 1002
#define ID_LBL_DURUM 1003

HWND hEsikKutusu, hDurumYazisi, hAnaPencere;
bool izlemeDevamEdiyor = false;
int girilenEsik = 3000;
int toplamGurultu = 0;

void MikrofonMotoru() {
    WAVEFORMATEX kules;
    kules.wFormatTag = WAVE_FORMAT_PCM;
    kules.nChannels = 1;
    kules.nSamplesPerSec = 48000;
    kules.wBitsPerSample = 16;
    kules.nBlockAlign = kules.nChannels * kules.wBitsPerSample / 8;
    kules.nAvgBytesPerSec = kules.nSamplesPerSec * kules.nBlockAlign;
    kules.cbSize = 0;

    HWAVEIN mikrofonhatti;
    if (waveInOpen(&mikrofonhatti, WAVE_MAPPER, &kules, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        MessageBox(NULL, L"Mikrofon acilamadi!", L"Hata", MB_ICONERROR);
        return;
    }

    const int KovaBoyutu = 2048;
    short kova[KovaBoyutu] = { 0 };
    WAVEHDR kovaBasligi = { (LPSTR)kova, KovaBoyutu * sizeof(short), 0, 0, 0, 0, 0, 0 };

    waveInPrepareHeader(mikrofonhatti, &kovaBasligi, sizeof(WAVEHDR));
    waveInAddBuffer(mikrofonhatti, &kovaBasligi, sizeof(WAVEHDR));
    waveInStart(mikrofonhatti);

    int gurultusayaci = 0;

    while (izlemeDevamEdiyor) {
        Sleep(50);
        for (int i = 0; i < KovaBoyutu; i++) {
            if (std::abs(kova[i]) > girilenEsik) {
                gurultusayaci++;
                toplamGurultu++;

                std::wstring stats = L"Anlik Gurultu: " + std::to_wstring(gurultusayaci) + L" / 250";
                SetWindowText(hDurumYazisi, stats.c_str());
                break;
            }
        }

        if (gurultusayaci >= 250) {
            PlaySound(TEXT("SystemExclamation"), NULL, SND_ALIAS | SND_ASYNC);
            gurultusayaci = 0;

            MessageBox(hAnaPencere,
                L"OFIS COK GURULTULU!\nLutfen sessiz olun.",
                L"VIBEN DISIPLIN MODU",
                MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        }
        waveInAddBuffer(mikrofonhatti, &kovaBasligi, sizeof(WAVEHDR));
    }

    waveInStop(mikrofonhatti);
    waveInClose(mikrofonhatti);
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowW(L"Static", L"VIBEN YONETICI PANELI", WS_VISIBLE | WS_CHILD | SS_CENTER, 20, 10, 250, 20, hWnd, NULL, NULL, NULL);

        CreateWindowW(L"Static", L"Esik Degeri (1000-15000):", WS_VISIBLE | WS_CHILD, 20, 50, 180, 20, hWnd, NULL, NULL, NULL);
        hEsikKutusu = CreateWindowW(L"Edit", L"3000", WS_VISIBLE | WS_CHILD | WS_BORDER, 200, 50, 60, 20, hWnd, (HMENU)ID_EDT_ESIK, NULL, NULL);

        CreateWindowW(L"Button", L"IZLEMEYI BASLAT / DURDUR", WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 90, 240, 40, hWnd, (HMENU)ID_BTN_BASLAT, NULL, NULL);

        hDurumYazisi = CreateWindowW(L"Static", L"Sistem Hazir...", WS_VISIBLE | WS_CHILD | SS_CENTER, 20, 150, 250, 40, hWnd, (HMENU)ID_LBL_DURUM, NULL, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wp) == ID_BTN_BASLAT) {
            if (!izlemeDevamEdiyor) {
                // Değeri kutudan oku
                wchar_t buffer[10];
                GetWindowText(hEsikKutusu, buffer, 10);
                girilenEsik = _wtoi(buffer);

                izlemeDevamEdiyor = true;
                std::thread(MikrofonMotoru).detach(); // Motoru ayrı odaya (thread) at
                SetWindowText(hDurumYazisi, L"DINLENILIYOR...");
            }
            else {
                izlemeDevamEdiyor = false;
                SetWindowText(hDurumYazisi, L"DURDURULDU.");
            }
        }
        break;

    case WM_DESTROY:
        izlemeDevamEdiyor = false;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {
    WNDCLASSW wc = { 0 };
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInst;
    wc.lpszClassName = L"VibenUI";
    wc.lpfnWndProc = WindowProcedure;

    if (!RegisterClassW(&wc)) return -1;

    hAnaPencere = CreateWindowW(L"VibenUI", L"VIBEN v1.0", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 400, 200, 300, 250, NULL, NULL, hInst, NULL);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}