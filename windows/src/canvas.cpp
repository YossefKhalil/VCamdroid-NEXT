#include "canvas.h"
#include "gui/theme.h"

#include <wx/dcbuffer.h>
#include <wx/rawbmp.h>
#include <wx/graphics.h>
#include <wx/dcgraph.h>

namespace
{
	wxSize GetSafeClientSize(wxWindow* window)
	{
		wxSize size = window->GetClientSize();
		// wxWidgets can report zero-sized clients while the frame is being
		// created. Keep the scaler math valid in that case.
		if (size.x < 16) size.x = 16;
		if (size.y < 16) size.y = 16;
		return size;
	}
}

Canvas::Canvas(wxWindow* parent, wxPoint position, wxSize size) 
	: wxPanel(parent, wxID_ANY, position, size)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(false);

	drawX = 0;
	drawY = 0;

	const wxSize clientSize = GetSafeClientSize(this);
	scaler = std::make_unique<Video::GuiPreviewScaler>(clientSize.x, clientSize.y);
	
	this->Bind(wxEVT_PAINT, &Canvas::OnPaint, this);
	this->Bind(wxEVT_ERASE_BACKGROUND, &Canvas::OnEraseBackground, this);
	this->Bind(wxEVT_SIZE, &Canvas::OnSize, this);
}

void Canvas::SetPlaceholder(const wxString& title, const wxString& subtitle)
{
	placeholderTitle = title;
	placeholderSubtitle = subtitle;
	Refresh(false);
}

void Canvas::SetCornerRadius(int radius)
{
	cornerRadius = radius;
	Refresh(false);
}

void Canvas::OnSize(wxSizeEvent& event)
{
	event.Skip();

	const wxSize clientSize = GetSafeClientSize(this);
	{
		std::lock_guard<std::mutex> lock(scalerMutex);
		scaler = std::make_unique<Video::GuiPreviewScaler>(clientSize.x, clientSize.y);
	}

	shouldDraw = false;
	this->Refresh(false);
}

void Canvas::Render(const uint8_t* bytes, int width, int height)
{
	if (!bytes || width <= 0 || height <= 0)
		return;

	if (!bitmap.IsOk() || bitmap.GetWidth() != width || bitmap.GetHeight() != height) 
	{
		bitmap.Create(width, height, 24);
	}

	const wxSize clientSize = GetSafeClientSize(this);
	drawX = (clientSize.x - width) / 2;
	drawY = (clientSize.y - height) / 2;

	wxNativePixelData data(bitmap);
	if (!data) 
		return;

	wxNativePixelData::Iterator p(data);
	int stride = width * 3;

	for (int y = 0; y < height; y++)
	{
		uint8_t* dstrow = (uint8_t*)p.m_ptr;
		std::memcpy(dstrow, bytes + (y * stride), stride);

		p.OffsetY(data, 1);
	}

	shouldDraw = true;
	this->Refresh(false);
	this->Update();
}

void Canvas::ProcessRawFrameAsync(const AVFrame* frame)
{
	if (isRendering)
		return;

	isRendering = true;

	int width = 0, height = 0;
	std::vector<uint8_t> safeFrame;

	{
		std::lock_guard<std::mutex> lock(scalerMutex);
		if (!scaler)
		{
			isRendering = false;
			return;
		}

		const uint8_t* data = scaler->Process(frame, width, height);

		// If scaler failed (nullptr) or returned invalid dimensions, abort immediately.
		// Do NOT try to construct the vector below, or it will segfault.
		if (!data || width <= 0 || height <= 0)
		{
			isRendering = false;
			return;
		}

		size_t size = static_cast<size_t>(width) * static_cast<size_t>(height) * 3;
		try {
			safeFrame.assign(data, data + size);
		}
		catch (const std::exception&) {
			// Allocation failed (out of memory?)
			isRendering = false;
			return;
		}
	}

	this->CallAfter([this, bytes = std::move(safeFrame), width, height]() {
		this->Render(bytes.data(), width, height);
		this->isRendering = false;
	});
}

void Canvas::ClearBeforeNextRender()
{
	shouldDraw = false;
	this->Refresh(false);
}

void Canvas::Clear()
{
	shouldDraw = false;
	isRendering = false;
	Refresh(false);
	Update();
}

void Canvas::DrawPlaceholder(wxGraphicsContext* gc, const wxRect& rect)
{
	if (!gc)
		return;

	const int iconSize = FromDIP(56);
	const wxRect iconRect(
		rect.x + (rect.width - iconSize) / 2,
		rect.y + rect.height / 2 - FromDIP(72),
		iconSize,
		iconSize);
	Material::DrawIcon(gc, Material::Icon::Snapshot, iconRect, Material::Colors().onSurfaceVariant, 1.8);

	wxFont titleFont = Material::Font(Material::TypeRole::TitleMedium);
	gc->SetFont(titleFont, Material::Colors().onSurface);
	wxDouble tw = 0, th = 0;
	gc->GetTextExtent(placeholderTitle, &tw, &th, nullptr, nullptr);
	gc->DrawText(placeholderTitle, rect.x + (rect.width - tw) / 2.0, rect.y + rect.height / 2.0 - FromDIP(6));

	wxFont subtitleFont = Material::Font(Material::TypeRole::BodySmall);
	gc->SetFont(subtitleFont, Material::Colors().onSurfaceVariant);
	gc->GetTextExtent(placeholderSubtitle, &tw, &th, nullptr, nullptr);
	gc->DrawText(placeholderSubtitle, rect.x + (rect.width - tw) / 2.0, rect.y + rect.height / 2.0 + FromDIP(20));
}

void Canvas::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);

	const wxColour backdrop = Material::Colors().surfaceContainerHighest;
	dc.SetBackground(wxBrush(backdrop));
	dc.Clear();

	wxGCDC gcdc(dc);
	wxGraphicsContext* gc = gcdc.GetGraphicsContext();
	if (!gc)
		return;

	const wxRect rect = GetClientRect();
	const int radius = FromDIP(cornerRadius);

	Material::DrawRoundedRect(gc, rect, radius, Material::Colors().videoBackground);

	if (shouldDraw && bitmap.IsOk())
	{
		gc->PushState();
		gc->Clip(wxRegion(rect.x, rect.y, rect.width, rect.height));
		gc->DrawBitmap(bitmap, drawX, drawY, bitmap.GetWidth(), bitmap.GetHeight());
		gc->PopState();
	}
	else
	{
		DrawPlaceholder(gc, rect);
	}
}

void Canvas::OnEraseBackground(wxEraseEvent& event)
{
	// Empty to remove flickering created by repainting the background
	// before actually rendering the video frame
}
