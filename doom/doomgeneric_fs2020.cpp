#include "doomkeys.h"

#include "doomgeneric.h"

#include <MSFS\MSFS.h>
#include "MSFS\MSFS_Render.h"
#include "MSFS\Render\nanovg.h"
#include <MSFS\Legacy\gauges.h>
#include <SimConnect.h>

#include <stdio.h>
#include <ctype.h>
#include <time.h>

#define KEYQUEUE_SIZE 16

extern NVGcontext* cur_nvgctx;
extern const char* doom_err_msg;

extern "C"
{

	static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
	static unsigned int s_KeyQueueWriteIndex = 0;
	static unsigned int s_KeyQueueReadIndex = 0;

	static void addKeyToQueue(int pressed, uint8_t key)
	{
		unsigned short keyData = (pressed << 8) | key;

		s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
		s_KeyQueueWriteIndex++;
		s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
	}

	/*

	static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			ExitProcess(0);
			break;
		case WM_KEYDOWN:
			addKeyToQueue(1, wParam);
			break;
		case WM_KEYUP:
			addKeyToQueue(0, wParam);
			break;
		default:
			return DefWindowProcA(hwnd, msg, wParam, lParam);
		}
		return 0;
	}
	*/

	HANDLE g_hSimConnect;


	static enum GROUP_ID {
		GROUP0,
	};

	static enum INPUT_ID {
		INPUT0,
	};
	 
	struct KeyEvent
	{
		const char* fs_name;
		unsigned char doom_key;
	};

	static KeyEvent event_list[] =
	{
		{"VK_NUMPAD4", KEY_LEFTARROW},
		{"VK_NUMPAD6", KEY_RIGHTARROW},
		{"VK_NUMPAD8", KEY_UPARROW},
		{"VK_NUMPAD2", KEY_DOWNARROW},
		{"VK_NUMPAD5", KEY_FIRE},
		{"VK_NUMPAD0", KEY_ENTER},
		{"VK_NUMPAD7", KEY_ESCAPE},
		{"VK_NUMPAD1", KEY_USE},
	};
	constexpr const size_t event_count = sizeof(event_list) / sizeof(event_list[0]);
	constexpr const size_t key_event_offset = 0x1000;

	void CALLBACK dispatchMessage(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
	{
		switch (pData->dwID)
		{
		case SIMCONNECT_RECV_ID_EVENT:
		{
			SIMCONNECT_RECV_EVENT* evt = (SIMCONNECT_RECV_EVENT*)pData;

			if (evt->uEventID < event_count)
			{
				addKeyToQueue(evt->dwData, event_list[evt->uEventID].doom_key);
				printf("Event ID: %d, %d\n", evt->uEventID, evt->dwData);
			}
			else if (evt->uEventID >= key_event_offset)
			{
				char c = tolower(evt->uEventID - key_event_offset);

				addKeyToQueue(evt->dwData, c);
				printf("Event text: %c, %d\n", c, evt->dwData);
			}
			break;
		}

		default:
			break;
		}
	}

	void DG_Init()
	{
		setvbuf(stdout, (char*)NULL, _IONBF, 0);

		HRESULT hr = SimConnect_Open(&g_hSimConnect, "DOOM_GAUGE", (HWND)nullptr, 0, 0, 0);

		for (unsigned ev = 0; ev < event_count; ++ev)
		{
			hr = SimConnect_MapClientEventToSimEvent(g_hSimConnect, ev);

			hr = SimConnect_MapInputEventToClientEvent(g_hSimConnect, INPUT0, event_list[ev].fs_name, ev, 1, ev);

			hr = SimConnect_AddClientEventToNotificationGroup(g_hSimConnect, GROUP0, ev);
		}

		// register text input events
		for (unsigned char c = '0'; c <= 'Z'; ++c)
		{
			if (!isalnum(c))
				continue;

			const uint32_t ev = key_event_offset + c;
			hr = SimConnect_MapClientEventToSimEvent(g_hSimConnect, ev);

			char name_buf[2];
			name_buf[0] = c;
			name_buf[1] = '\0';
			hr = SimConnect_MapInputEventToClientEvent(g_hSimConnect, INPUT0, name_buf, ev, 1, ev);

			hr = SimConnect_AddClientEventToNotificationGroup(g_hSimConnect, GROUP0, ev);
		}

		hr = SimConnect_SetNotificationGroupPriority(g_hSimConnect, GROUP0, SIMCONNECT_GROUP_PRIORITY_HIGHEST);
		hr = SimConnect_SetInputGroupState(g_hSimConnect, INPUT0, SIMCONNECT_STATE_ON);


		hr = SimConnect_CallDispatch(g_hSimConnect, dispatchMessage, nullptr);
	}

	void DG_DrawFrame()
	{
		/*
		MSG msg;
		memset(&msg, 0, sizeof(msg));

		while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		StretchDIBits(s_Hdc, 0, 0, DOOMGENERIC_RESX, DOOMGENERIC_RESY, 0, 0, DOOMGENERIC_RESX, DOOMGENERIC_RESY, DG_ScreenBuffer, &s_Bmi, 0, SRCCOPY);

		SwapBuffers(s_Hdc);
		*/

		SimConnect_CallDispatch(g_hSimConnect, dispatchMessage, nullptr);
	}

	void DG_SleepMs(uint32_t ms)
	{
		//Sleep(ms);
	}

	double simulation_time;

	uint32_t DG_GetTicksMs()
	{
		struct timespec tms;

		clock_gettime(CLOCK_REALTIME, &tms);
		int64_t micros = tms.tv_sec * 1000000;
		micros += tms.tv_nsec / 1000;

		return micros / 1000;
		//return GetTickCount();
	}

	int DG_GetKey(int* pressed, unsigned char* doomKey)
	{
		if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex)
		{
			//key queue is empty

			return 0;
		}
		else
		{
			unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
			s_KeyQueueReadIndex++;
			s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

			*pressed = keyData >> 8;
			*doomKey = keyData & 0xFF;

			return 1;
		}
		return 0;
	}

	void DG_Error(const char* msg)
	{
		doom_err_msg = msg;
	}

	void DG_SetWindowTitle(const char* title)
	{

	}

	/*
	int main(int argc, char **argv)
	{
		doomgeneric_Create(argc, argv);

		for (int i = 0; ; i++)
		{
			doomgeneric_Tick();
		}


		return 0;
	}
	*/

}