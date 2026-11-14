#pragma once

#include <wx/wx.h>
#include <atomic>
#include <memory>
#include <mutex>
#include "video/guipreviewscaler.h"

class Canvas : public wxPanel
{
public:
	Canvas(wxWindow* parent, wxPoint position, wxSize size);

	void ProcessRawFrameAsync(const AVFrame* frame);
	void ClearBeforeNextRender();
	void Clear();

	void SetPlaceholder(const wxString& title, const wxString& subtitle);
	void SetCornerRadius(int radius);

private:
	wxBitmap bitmap;

	bool shouldDraw = false;
	int drawX, drawY;
	int cornerRadius = 12;

	wxString placeholderTitle = "Waiting for camera";
	wxString placeholderSubtitle = "Select a device or scan the QR code";

	std::atomic<bool> isRendering{ false };

	// The preview is fitted into the current client area. The scaler is
	// recreated whenever the window is resized, and both the RTSP callback
	// thread and the UI thread can touch it, so guard it with a mutex.
	std::mutex scalerMutex;
	std::unique_ptr<Video::GuiPreviewScaler> scaler;

	void Render(const uint8_t* bytes, int width, int height);

	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event);
	void OnSize(wxSizeEvent& event);
	void DrawPlaceholder(wxGraphicsContext* gc, const wxRect& rect);
};
