// Copyright (c) Asobo Studio, All rights reserved. www.asobostudio.com

#include <MSFS\MSFS.h>
#include "MSFS\MSFS_Render.h"
#include "MSFS\Render\nanovg.h"
#include <MSFS\Legacy\gauges.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <map>

#include "doom/doomgeneric.h"

#ifdef _MSC_VER
#define snprintf _snprintf_s
#elif !defined(__MINGW32__)
#include <iconv.h>
#endif

struct sAttitudeVars
{
	ENUM m_eDegrees;
	ENUM m_eAttitudeIndicatorPitchDegrees;
	ENUM m_eAttitudeIndicatorBankDegrees;
	int m_iFont;
	int m_nanovg_fb;
};

sAttitudeVars g_AttitudeVars;
std::map < FsContext, NVGcontext*> g_AttitudeNVGcontext;

extern "C" double simulation_time;
NVGcontext * cur_nvgctx;

void doom_draw(NVGcontext* nvgctx, sGaugeDrawData* p_draw_data)
{
	doomgeneric_Tick();

	for (int i = 0; i < DOOMGENERIC_RESX * DOOMGENERIC_RESY; ++i)
	{
		DG_ScreenBuffer[i] |= 0xFF000000;
	}

	nvgUpdateImage(nvgctx, g_AttitudeVars.m_nanovg_fb, (unsigned char*)DG_ScreenBuffer);

	//g_NetworkGetInfo.m_iImage = nvgCreateImage(nvgctx, g_NetworkGetInfo.imagePath.c_str(), 0);

	NVGpaint imgPaint = nvgImagePattern(nvgctx, 0, 0, p_draw_data->winWidth, p_draw_data->winHeight, 0, g_AttitudeVars.m_nanovg_fb, 1);

	nvgBeginPath(nvgctx);
	nvgRect(nvgctx, 0, 0, p_draw_data->winWidth, p_draw_data->winHeight);
	nvgFillPaint(nvgctx, imgPaint);
	nvgFill(nvgctx);
}

int first_frame = 1;

// ------------------------
// Callbacks
extern "C" {

	MSFS_CALLBACK bool Attitude_gauge_callback(FsContext ctx, int service_id, void* pData)
	{
		switch (service_id)
		{
		case PANEL_SERVICE_PRE_INSTALL:
		{
			sGaugeInstallData* p_install_data = (sGaugeInstallData*)pData;
			// Width given in p_install_data->iSizeX
			// Height given in p_install_data->iSizeY
			g_AttitudeVars.m_eDegrees = get_units_enum("DEGREES");
			g_AttitudeVars.m_eAttitudeIndicatorPitchDegrees = get_aircraft_var_enum("ATTITUDE INDICATOR PITCH DEGREES");
			g_AttitudeVars.m_eAttitudeIndicatorBankDegrees = get_aircraft_var_enum("ATTITUDE INDICATOR BANK DEGREES");
			return true;
		}
		break;
		case PANEL_SERVICE_POST_INSTALL:
		{
			NVGparams params;
			params.userPtr = ctx;
			params.edgeAntiAlias = true;
			g_AttitudeNVGcontext[ctx] = nvgCreateInternal(&params);
			NVGcontext* nvgctx = g_AttitudeNVGcontext[ctx];
			g_AttitudeVars.m_iFont = nvgCreateFont(nvgctx, "sans", "./data/Roboto-Regular.ttf");

			g_AttitudeVars.m_nanovg_fb = nvgCreateImageRGBA(nvgctx, DOOMGENERIC_RESX, DOOMGENERIC_RESY, NVG_IMAGE_NEAREST, (unsigned char*)DG_ScreenBuffer);

			static const char* args[] = { "doom" };

			doomgeneric_Create(sizeof(args)/sizeof(args[0]), (char**)args);

			return true;
		}
		break;
		case PANEL_SERVICE_PRE_DRAW:
		{
			sGaugeDrawData* p_draw_data = (sGaugeDrawData*)pData;

			simulation_time = p_draw_data->t;
			
			FLOAT64 fPitch = aircraft_varget(g_AttitudeVars.m_eAttitudeIndicatorPitchDegrees, g_AttitudeVars.m_eDegrees, 0);
			FLOAT64 fBank = aircraft_varget(g_AttitudeVars.m_eAttitudeIndicatorBankDegrees, g_AttitudeVars.m_eDegrees, 0);
			float fSize = sqrt(p_draw_data->winWidth * p_draw_data->winWidth + p_draw_data->winHeight * p_draw_data->winHeight) * 1.1f;
			float pxRatio = (float)p_draw_data->fbWidth / (float)p_draw_data->winWidth;
			NVGcontext* nvgctx = g_AttitudeNVGcontext[ctx];
			//printf("W: %d, H: %d, FB_W: %d, FB_H: %d\n", p_draw_data->winWidth, p_draw_data->winHeight, p_draw_data->fbWidth, p_draw_data->fbHeight);
			nvgBeginFrame(nvgctx, p_draw_data->winWidth, p_draw_data->winHeight, pxRatio);
			{
				cur_nvgctx = nvgctx;

				// Will be drawn over if doom loads as expected, otherwise will stay displayed as the gauge crashes
				if (first_frame)
				{
					nvgTextBounds(nvgctx, 0, 0, NULL, NULL, NULL);
					nvgFontSize(nvgctx, 40.0f);
					nvgFontFace(nvgctx, "sans");
					nvgFillColor(nvgctx, nvgRGBA(255, 0, 0, 255));
					nvgTextAlign(nvgctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
					nvgText(nvgctx, 0, 0, "Please make sure a .wad file is present in this package's work/ folder", NULL);
					first_frame = 0;
				}
				else
					doom_draw(nvgctx, p_draw_data);
			}
			nvgEndFrame(nvgctx);
			return true;
		}
		break;
		case PANEL_SERVICE_PRE_KILL:
		{
			NVGcontext* nvgctx = g_AttitudeNVGcontext[ctx];
			nvgDeleteInternal(nvgctx);
			g_AttitudeNVGcontext.erase(ctx);
			return true;
		}
		break;
		}
		return false;
	}

}
