#include "gui/theme.h"

#include "settings.h"

#include <wx/listctrl.h>
#include <wx/settings.h>
#include <wx/fontenum.h>
#include <wx/display.h>

#include <algorithm>
#include <cmath>

namespace
{
	constexpr double PI = 3.14159265358979323846;

	double Deg(double degrees)
	{
		return degrees * PI / 180.0;
	}

	wxColour C(unsigned char r, unsigned char g, unsigned char b)
	{
		return wxColour(r, g, b);
	}

	Material::ColorScheme MakeLightScheme()
	{
		Material::ColorScheme s;
		// Material 3 blue / ocean scheme, light.
		s.primary = C(11, 87, 208);          // #0B57D0
		s.onPrimary = C(255, 255, 255);
		s.primaryContainer = C(211, 227, 253); // #D3E3FD
		s.onPrimaryContainer = C(4, 30, 73);   // #041E49

		s.secondary = C(0, 99, 155);          // #00639B
		s.onSecondary = C(255, 255, 255);
		s.secondaryContainer = C(207, 229, 255); // #CFE5FF
		s.onSecondaryContainer = C(0, 29, 53);   // #001D35

		s.tertiary = C(0, 105, 109);          // #00696D
		s.onTertiary = C(255, 255, 255);
		s.tertiaryContainer = C(158, 240, 244); // #9EF0F4
		s.onTertiaryContainer = C(0, 32, 34);   // #002022

		s.error = C(186, 26, 26);
		s.onError = C(255, 255, 255);
		s.errorContainer = C(255, 218, 214);
		s.onErrorContainer = C(65, 0, 2);

		s.surface = C(248, 250, 253);         // #F8FAFD
		s.onSurface = C(26, 28, 30);          // #1A1C1E
		s.surfaceVariant = C(223, 226, 235);  // #DFE2EB
		s.onSurfaceVariant = C(67, 71, 78);   // #43474E
		s.background = s.surface;

		s.surfaceContainerLowest = C(255, 255, 255);
		s.surfaceContainerLow = C(242, 244, 249);
		s.surfaceContainer = C(236, 238, 244);
		s.surfaceContainerHigh = C(230, 232, 239);
		s.surfaceContainerHighest = C(224, 227, 234);

		s.outline = C(115, 119, 127);         // #73777F
		s.outlineVariant = C(195, 198, 207);  // #C3C6CF

		s.inverseSurface = C(47, 48, 51);
		s.inverseOnSurface = C(241, 240, 244);
		s.inversePrimary = C(168, 199, 250);  // #A8C7FA

		s.shadow = C(0, 0, 0);
		s.scrim = C(0, 0, 0);
		s.videoBackground = C(0, 0, 0);
		return s;
	}

	Material::ColorScheme MakeDarkScheme()
	{
		Material::ColorScheme s;
		// Material 3 blue / ocean scheme, dark.
		s.primary = C(168, 199, 250);         // #A8C7FA
		s.onPrimary = C(0, 46, 105);          // #002E69
		s.primaryContainer = C(8, 66, 160);   // #0842A0
		s.onPrimaryContainer = C(211, 227, 253); // #D3E3FD

		s.secondary = C(142, 205, 255);       // #8ECDFF
		s.onSecondary = C(0, 51, 84);
		s.secondaryContainer = C(0, 74, 119); // #004A77
		s.onSecondaryContainer = C(207, 229, 255);

		s.tertiary = C(79, 216, 222);         // #4FD8DE
		s.onTertiary = C(0, 55, 57);
		s.tertiaryContainer = C(0, 79, 82);   // #004F52
		s.onTertiaryContainer = C(158, 240, 244);

		s.error = C(255, 180, 171);
		s.onError = C(105, 0, 5);
		s.errorContainer = C(147, 0, 10);
		s.onErrorContainer = C(255, 218, 214);

		s.surface = C(17, 19, 24);            // #111318
		s.onSurface = C(226, 226, 233);
		s.surfaceVariant = C(68, 71, 78);     // #44474E
		s.onSurfaceVariant = C(196, 198, 207);
		s.background = s.surface;

		s.surfaceContainerLowest = C(12, 14, 19);
		s.surfaceContainerLow = C(25, 28, 32);
		s.surfaceContainer = C(29, 32, 36);
		s.surfaceContainerHigh = C(40, 42, 47);
		s.surfaceContainerHighest = C(51, 53, 58);

		s.outline = C(142, 144, 153);
		s.outlineVariant = C(68, 71, 78);

		s.inverseSurface = C(226, 226, 233);
		s.inverseOnSurface = C(47, 48, 51);
		s.inversePrimary = C(11, 87, 208);

		s.shadow = C(0, 0, 0);
		s.scrim = C(0, 0, 0);
		s.videoBackground = C(0, 0, 0);
		return s;
	}
}

namespace Material
{
	const ColorScheme& Colors()
	{
		static const ColorScheme light = MakeLightScheme();
		static const ColorScheme dark = MakeDarkScheme();
		return IsDark() ? dark : light;
	}

	bool IsDark()
	{
		// THEME_MODE: 0 = follow system, 1 = light, 2 = dark.
		const int mode = Settings::Get("THEME_MODE");
		if (mode == 1)
			return false;
		if (mode == 2)
			return true;
		return wxSystemSettings::GetAppearance().IsDark();
	}

	wxString UiFontFace()
	{
		static const wxString face = []()
		{
			if (wxFontEnumerator::IsValidFacename("Roboto"))
				return wxString("Roboto");
			if (wxFontEnumerator::IsValidFacename("Segoe UI Variable Text"))
				return wxString("Segoe UI Variable Text");
			return wxString("Segoe UI");
		}();
		return face;
	}

	wxFont Font(int pointSize, wxFontWeight weight)
	{
		wxFont font(wxFontInfo(pointSize).FaceName(UiFontFace()).Weight(weight));
		return font;
	}

	wxFont Font(TypeRole role)
	{
		switch (role)
		{
			case TypeRole::DisplayLarge: return Font(34, wxFONTWEIGHT_NORMAL);
			case TypeRole::DisplayMedium: return Font(28, wxFONTWEIGHT_NORMAL);
			case TypeRole::DisplaySmall: return Font(24, wxFONTWEIGHT_NORMAL);

			case TypeRole::HeadlineLarge: return Font(24, wxFONTWEIGHT_NORMAL);
			case TypeRole::HeadlineMedium: return Font(21, wxFONTWEIGHT_NORMAL);
			case TypeRole::HeadlineSmall: return Font(18, wxFONTWEIGHT_NORMAL);

			case TypeRole::TitleLarge: return Font(18, wxFONTWEIGHT_SEMIBOLD);
			case TypeRole::TitleMedium: return Font(14, wxFONTWEIGHT_SEMIBOLD);
			case TypeRole::TitleSmall: return Font(12, wxFONTWEIGHT_SEMIBOLD);

			case TypeRole::BodyLarge: return Font(12, wxFONTWEIGHT_NORMAL);
			case TypeRole::BodyMedium: return Font(11, wxFONTWEIGHT_NORMAL);
			case TypeRole::BodySmall: return Font(10, wxFONTWEIGHT_NORMAL);

			case TypeRole::LabelLarge: return Font(11, wxFONTWEIGHT_MEDIUM);
			case TypeRole::LabelMedium: return Font(10, wxFONTWEIGHT_MEDIUM);
			case TypeRole::LabelSmall: return Font(9, wxFONTWEIGHT_MEDIUM);
		}
		return Font(11);
	}

	int Radius(Shape shape)
	{
		return static_cast<int>(shape);
	}

	wxColour StateLayer(const wxColour& base, const wxColour& layer, double opacity)
	{
		const double t = std::clamp(opacity, 0.0, 1.0);
		return wxColour(
			static_cast<unsigned char>(base.Red() + (layer.Red() - base.Red()) * t),
			static_cast<unsigned char>(base.Green() + (layer.Green() - base.Green()) * t),
			static_cast<unsigned char>(base.Blue() + (layer.Blue() - base.Blue()) * t));
	}

	void DrawRoundedRect(wxGraphicsContext* gc, const wxRect& rect, int radius,
		const wxColour& fill, const wxColour& border, int borderWidth)
	{
		if (!gc)
			return;

		const double r = std::min<double>(radius, std::min(rect.width, rect.height) / 2.0);
		gc->SetBrush(fill.IsOk() ? wxBrush(fill) : *wxTRANSPARENT_BRUSH);
		if (border.IsOk())
			gc->SetPen(wxPen(border, borderWidth));
		else
			gc->SetPen(*wxTRANSPARENT_PEN);

		if (r <= 0.5)
			gc->DrawRectangle(rect.x, rect.y, rect.width, rect.height);
		else
			gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, r);
	}

	void DrawShadow(wxGraphicsContext* gc, const wxRect& rect, int radius, int elevation)
	{
		if (!gc || elevation <= 0)
			return;

		const int layers = elevation + 1;
		for (int i = layers; i >= 1; --i)
		{
			const int spread = i * 2;
			const unsigned char alpha = static_cast<unsigned char>(8 + (layers - i) * 7);
			gc->SetPen(*wxTRANSPARENT_PEN);
			gc->SetBrush(wxBrush(wxColour(0, 0, 0, alpha)));
			gc->DrawRoundedRectangle(
				rect.x - spread * 0.5,
				rect.y + spread * 0.5,
				rect.width + spread,
				rect.height + spread,
				radius + spread * 0.5);
		}
	}

	namespace
	{
		// Adds an SVG path definition (M/L/H/V/C/S/Z, absolute and relative)
		// to a wxGraphicsPath. This lets the UI use the exact Material icon
		// path data instead of hand-approximated shapes.
		void AddSvgPath(wxGraphicsPath& path, const wxString& definition)
		{
			const wxChar* p = definition.c_str();
			auto skipSeparators = [&]()
			{
				while (*p == ' ' || *p == ',' || *p == '\t' || *p == '\n' || *p == '\r')
					++p;
			};
			auto readNumber = [&]() -> double
			{
				skipSeparators();
				wxChar* end = nullptr;
				const double value = wxStrtod(p, &end);
				if (end != p)
					p = end;
				return value;
			};

			double x = 0.0, y = 0.0;
			double startX = 0.0, startY = 0.0;
			double lastControlX = 0.0, lastControlY = 0.0;
			char command = 0;
			char previousCommand = 0;

			skipSeparators();
			while (*p)
			{
				if (wxIsalpha(*p))
				{
					command = static_cast<char>(*p++);
				}
				else if (command == 'M')
				{
					command = 'L';
				}
				else if (command == 'm')
				{
					command = 'l';
				}

				const bool relative = wxIslower(command) != 0;
				const char upper = static_cast<char>(wxToupper(command));

				if (upper == 'Z')
				{
					path.CloseSubpath();
					x = startX;
					y = startY;
					previousCommand = 'Z';
				}
				else if (upper == 'M')
				{
					double nx = readNumber();
					double ny = readNumber();
					if (relative)
					{
						nx += x;
						ny += y;
					}
					x = nx;
					y = ny;
					startX = x;
					startY = y;
					path.MoveToPoint(x, y);
					previousCommand = 'M';
				}
				else if (upper == 'L')
				{
					double nx = readNumber();
					double ny = readNumber();
					if (relative)
					{
						nx += x;
						ny += y;
					}
					path.AddLineToPoint(nx, ny);
					x = nx;
					y = ny;
					previousCommand = 'L';
				}
				else if (upper == 'H')
				{
					double nx = readNumber();
					if (relative)
						nx += x;
					path.AddLineToPoint(nx, y);
					x = nx;
					previousCommand = 'H';
				}
				else if (upper == 'V')
				{
					double ny = readNumber();
					if (relative)
						ny += y;
					path.AddLineToPoint(x, ny);
					y = ny;
					previousCommand = 'V';
				}
				else if (upper == 'C')
				{
					double x1 = readNumber();
					double y1 = readNumber();
					double x2 = readNumber();
					double y2 = readNumber();
					double nx = readNumber();
					double ny = readNumber();
					if (relative)
					{
						x1 += x; y1 += y;
						x2 += x; y2 += y;
						nx += x; ny += y;
					}
					path.AddCurveToPoint(x1, y1, x2, y2, nx, ny);
					lastControlX = x2;
					lastControlY = y2;
					x = nx;
					y = ny;
					previousCommand = 'C';
				}
				else if (upper == 'S')
				{
					double x2 = readNumber();
					double y2 = readNumber();
					double nx = readNumber();
					double ny = readNumber();
					const bool smooth = previousCommand == 'C' || previousCommand == 'S';
					const double x1 = smooth ? 2.0 * x - lastControlX : x;
					const double y1 = smooth ? 2.0 * y - lastControlY : y;
					if (relative)
					{
						x2 += x; y2 += y;
						nx += x; ny += y;
					}
					path.AddCurveToPoint(x1, y1, x2, y2, nx, ny);
					lastControlX = x2;
					lastControlY = y2;
					x = nx;
					y = ny;
					previousCommand = 'S';
				}
				else
				{
					// Unsupported command: consume one number so the parser
					// cannot spin on malformed input.
					readNumber();
					previousCommand = upper;
				}

				skipSeparators();
			}
		}
	}

	void DrawIcon(wxGraphicsContext* gc, Icon icon, const wxRect& rect, const wxColour& color, double strokeWidth)
	{
		if (!gc)
			return;

		const double scale = std::min(rect.width, rect.height) / 24.0;
		if (scale <= 0.0)
			return;

		gc->PushState();
		gc->Translate(rect.x + (rect.width - 24.0 * scale) / 2.0, rect.y + (rect.height - 24.0 * scale) / 2.0);
		gc->Scale(scale, scale);
		gc->SetPen(wxPen(color, strokeWidth));
		gc->SetBrush(*wxTRANSPARENT_BRUSH);

		wxGraphicsPath path = gc->CreatePath();
		bool fill = false;

		switch (icon)
		{
			case Icon::QrCode:
			{
				// Material Icons "qr_code" (24dp), exact path data.
				AddSvgPath(path, "M3 11h8V3H3v8zm2-6h4v4H5V5zM3 21h8v-8H3v8zm2-6h4v4H5v-4zM13 3v8h8V3h-8zm6 6h-4V5h4v4zM19 19h2v2h-2zM13 13h2v2h-2zM15 15h2v2h-2zM13 17h2v2h-2zM15 19h2v2h-2zM17 17h2v2h-2zM17 13h2v2h-2zM19 15h2v2h-2z");
				fill = true;
				break;
			}
			case Icon::Devices:
			{
				// Material Icons "devices" (24dp), exact path data.
				AddSvgPath(path, "M4 6h18V4H4c-1.1 0-2 .9-2 2v11H0v3h14v-3H4V6zm19 2h-6c-.55 0-1 .45-1 1v10c0 .55.45 1 1 1h6c.55 0 1-.45 1-1V9c0-.55-.45-1-1-1zm-1 9h-4v-7h4v7z");
				fill = true;
				break;
			}
			case Icon::Settings:
			case Icon::StreamSettings:
			case Icon::Gear:
			{
				// Material Icons "settings" (24dp), exact path data.
				AddSvgPath(path, "M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94L14.4 2.81c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41L9.25 5.35C8.66 5.59 8.12 5.92 7.63 6.29L5.24 5.33c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58C4.84 11.36 4.8 11.69 4.8 12s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.03-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z");
				fill = true;
				break;
			}
			case Icon::Adjustments:
			{
				// Material Icons "tune" (24dp), exact path data.
				AddSvgPath(path, "M3 17v2h6v-2H3zM3 5v2h10V5H3zm10 16v-2h8v-2h-8v-2h-2v6h2zM7 9v2H3v2h4v2h2V9H7zm14 4v-2H11v2h10zm-6-4h2V7h4V5h-4V3h-2v6z");
				fill = true;
				break;
			}
			case Icon::MoreVert:
			{
				// Material Icons "more_vert" (24dp), exact path data.
				AddSvgPath(path, "M12 8c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm0 2c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2zm0 6c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2z");
				fill = true;
				break;
			}
			case Icon::RotateLeft:
			{
				// Material Icons "rotate_left" (24dp), exact path data.
				AddSvgPath(path, "M7.11 8.53L5.7 7.11C4.8 8.27 4.24 9.61 4.07 11h2.02c.14-.87.49-1.72 1.02-2.47zM6.09 13H4.07c.17 1.39.72 2.73 1.62 3.89l1.41-1.42c-.52-.75-.87-1.59-1.01-2.47zm1.01 5.32c1.16.9 2.51 1.44 3.9 1.61V17.9c-.87-.15-1.71-.49-2.46-1.03L7.1 18.32zM13 4.07V1L8.45 5.55 13 10V6.09c2.84.48 5 2.94 5 5.91s-2.16 5.43-5 5.91v2.02c3.95-.49 7-3.85 7-7.93s-3.05-7.44-7-7.93z");
				fill = true;
				break;
			}
			case Icon::RotateRight:
			{
				// Material Icons "rotate_right" (24dp), exact path data.
				AddSvgPath(path, "M15.55 5.55L11 1v3.07C7.06 4.56 4 7.92 4 12s3.05 7.44 7 7.93v-2.02c-2.84-.48-5-2.94-5-5.91s2.16-5.43 5-5.91V10l4.55-4.45zM19.93 11c-.17-1.39-.72-2.73-1.62-3.89l-1.42 1.42c.54.75.88 1.6 1.02 2.47h2.02zM13 17.9v2.02c1.39-.17 2.74-.71 3.9-1.61l-1.44-1.44c-.75.54-1.59.89-2.46 1.03zm3.89-2.42l1.42 1.41c.9-1.16 1.45-2.5 1.62-3.89h-2.02c-.14.87-.48 1.72-1.02 2.48z");
				fill = true;
				break;
			}
			case Icon::FlipHorizontal:
			{
				// Material Icons "flip" (24dp), exact path data.
				AddSvgPath(path, "M15 21h2v-2h-2v2zm4-12h2V7h-2v2zM3 5v14c0 1.1.9 2 2 2h4v-2H5V5h4V3H5c-1.1 0-2 .9-2 2zm16-2v2h2c0-1.1-.9-2-2-2zm-8 20h2V1h-2v22zm8-6h2v-2h-2v2zM15 5h2V3h-2v2zm4 8h2v-2h-2v2zm0 8c1.1 0 2-.9 2-2h-2v2z");
				fill = true;
				break;
			}
			case Icon::FlipVertical:
			{
				// The standard "flip" icon rotated a quarter turn.
				gc->PushState();
				gc->Translate(12, 12);
				gc->Rotate(Deg(90));
				gc->Translate(-12, -12);
				AddSvgPath(path, "M15 21h2v-2h-2v2zm4-12h2V7h-2v2zM3 5v14c0 1.1.9 2 2 2h4v-2H5V5h4V3H5c-1.1 0-2 .9-2 2zm16-2v2h2c0-1.1-.9-2-2-2zm-8 20h2V1h-2v22zm8-6h2v-2h-2v2zM15 5h2V3h-2v2zm4 8h2v-2h-2v2zm0 8c1.1 0 2-.9 2-2h-2v2z");
				fill = true;
				gc->PopState();
				break;
			}
			case Icon::ZoomIn:
			{
				// Material Icons "zoom_in" (24dp), exact path data.
				AddSvgPath(path, "M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14zM12 10h-2v2H9v-2H7V9h2V7h1v2h2v1z");
				fill = true;
				break;
			}
			case Icon::ZoomOut:
			{
				// Material Icons "zoom_out" (24dp), exact path data.
				AddSvgPath(path, "M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14zM7 9h5v1H7z");
				fill = true;
				break;
			}
			case Icon::Flash:
			{
				// Material Icons "flash_on" (24dp), exact path data.
				AddSvgPath(path, "M7 2v11h3v9l7-12h-4l4-8z");
				fill = true;
				break;
			}
			case Icon::SwapCamera:
			{
				// Material Icons "flip_camera_android" (24dp), exact path data.
				AddSvgPath(path, "M9 12c0 1.66 1.34 3 3 3s3-1.34 3-3-1.34-3-3-3-3 1.34-3 3zM8 10V8H5.09C6.47 5.61 9.05 4 12 4c3.72 0 6.85 2.56 7.74 6h2.06c-.93-4.56-4.96-8-9.8-8C8.73 2 5.82 3.58 4 6.01V4H2v6h6zM16 14v2h2.91c-1.38 2.39-3.96 4-6.91 4-3.72 0-6.85-2.56-7.74-6H2.2c.93 4.56 4.96 8 9.8 8 3.27 0 6.18-1.58 8-4.01V20h2v-6h-6z");
				fill = true;
				break;
			}
			case Icon::Snapshot:
			{
				// Material Icons "photo_camera" (24dp), exact path data.
				path.AddCircle(12, 12, 3.2);
				AddSvgPath(path, "M9 2L7.17 4H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2h-3.17L15 2H9zm3 15c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5z");
				fill = true;
				break;
			}
			case Icon::Close:
			{
				// Material Icons "close" (24dp), exact path data.
				AddSvgPath(path, "M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z");
				fill = true;
				break;
			}
			case Icon::Check:
			{
				// Material Icons "check" (24dp), exact path data.
				AddSvgPath(path, "M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z");
				fill = true;
				break;
			}
			case Icon::Info:
			{
				// Material Icons "info" (24dp), exact path data.
				AddSvgPath(path, "M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z");
				fill = true;
				break;
			}
			case Icon::Exit:
			{
				// Material Icons "logout" (24dp), exact path data.
				AddSvgPath(path, "M17 7l-1.41 1.41L18.17 11H8v2h10.17l-2.58 2.58L17 17l5-5zM4 5h8V3H4c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h8v-2H4V5z");
				fill = true;
				break;
			}
			case Icon::Stats:
			{
				// Material Icons "bar_chart" (24dp), exact path data.
				AddSvgPath(path, "M4 9h4v11H4zM16 13h4v7h-4zM10 4h4v16h-4z");
				fill = true;
				break;
			}
			case Icon::ChevronDown:
			{
				// Material Icons "expand_more" (24dp), exact path data.
				AddSvgPath(path, "M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z");
				fill = true;
				break;
			}
			case Icon::Refresh:
			{
				// Material Icons "refresh" (24dp), exact path data.
				AddSvgPath(path, "M17.65 6.35C16.2 4.9 14.21 4 12 4c-4.42 0-7.99 3.58-7.99 8s3.57 8 7.99 8c3.73 0 6.84-2.55 7.73-6h-2.08c-.82 2.33-3.04 4-5.65 4-3.31 0-6-2.69-6-6s2.69-6 6-6c1.66 0 3.14.69 4.22 1.78L13 11h7V4l-2.35 2.35z");
				fill = true;
				break;
			}
			case Icon::ArrowBack:
			{
				// Material Icons "arrow_back" (24dp), exact path data.
				AddSvgPath(path, "M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z");
				fill = true;
				break;
			}
		}

		if (fill)
		{
			gc->SetBrush(wxBrush(color));
			gc->FillPath(path);
		}
		else
		{
			gc->StrokePath(path);
		}
		gc->PopState();
	}

	Card::Card(wxWindow* parent, int padding, Shape shape, bool elevated)
		: wxPanel(parent, wxID_ANY),
		padding(FromDIP(padding)),
		radius(FromDIP(Radius(shape))),
		elevated(elevated),
		backdrop(Colors().surface),
		fill(Colors().surfaceContainerLow)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetDoubleBuffered(true);
		SetBackgroundColour(fill);

		content = new wxBoxSizer(wxVERTICAL);
		auto outer = new wxBoxSizer(wxVERTICAL);
		outer->Add(content, 1, wxEXPAND | wxALL, this->padding);
		SetSizer(outer);

		Bind(wxEVT_PAINT, &Card::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
		Bind(wxEVT_SIZE, [this](wxSizeEvent& event) { Refresh(false); event.Skip(); });
	}

	wxBoxSizer* Card::Content()
	{
		return content;
	}

	void Card::SetBackdrop(const wxColour& color)
	{
		backdrop = color;
		Refresh(false);
	}

	void Card::SetFill(const wxColour& color)
	{
		fill = color;
		SetBackgroundColour(fill);
		Refresh(false);
	}

	void Card::OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
		dc.SetBackground(wxBrush(backdrop));
		dc.Clear();

		wxGCDC gcdc(dc);
		wxGraphicsContext* gc = gcdc.GetGraphicsContext();
		if (!gc)
			return;

		const wxRect rect = GetClientRect();
		if (elevated)
			DrawShadow(gc, rect, radius, 2);
		DrawRoundedRect(gc, rect, radius, fill, wxNullColour);
	}

	Section::Section(wxWindow* parent, const wxString& title, const wxString& subtitle)
		: Card(parent, 20, Shape::Medium)
	{
		auto titleLabel = Label(this, title, TypeRole::TitleMedium);
		Content()->Add(titleLabel, 0, wxBOTTOM, FromDIP(subtitle.empty() ? 12 : 4));

		if (!subtitle.empty())
		{
			auto subtitleLabel = Label(this, subtitle, TypeRole::BodySmall, Colors().onSurfaceVariant);
			Content()->Add(subtitleLabel, 0, wxBOTTOM, FromDIP(12));
		}

		body = new wxBoxSizer(wxVERTICAL);
		Content()->Add(body, 1, wxEXPAND);
	}

	wxBoxSizer* Section::Body()
	{
		return body;
	}

	IconButton::IconButton(wxWindow* parent, Icon icon, const wxString& tooltip, bool primary, int size)
		: wxControl(parent, wxID_ANY, wxDefaultPosition, wxSize(40, 40), wxBORDER_NONE),
		icon(icon),
		primary(primary),
		backdrop(Colors().surfaceContainerLow)
	{
		buttonSize = FromDIP(size);
		SetMinSize(wxSize(buttonSize, buttonSize));
		SetToolTip(tooltip);
		SetCursor(wxCursor(wxCURSOR_HAND));
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetDoubleBuffered(true);
		SetBackgroundColour(backdrop);

		Bind(wxEVT_PAINT, &IconButton::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
		Bind(wxEVT_ENTER_WINDOW, &IconButton::OnEnter, this);
		Bind(wxEVT_LEAVE_WINDOW, &IconButton::OnLeave, this);
		Bind(wxEVT_LEFT_DOWN, &IconButton::OnLeftDown, this);
		Bind(wxEVT_LEFT_UP, &IconButton::OnLeftUp, this);
		Bind(wxEVT_MOUSE_CAPTURE_LOST, &IconButton::OnCaptureLost, this);
	}

	void IconButton::SetBackdrop(const wxColour& color)
	{
		backdrop = color;
		SetBackgroundColour(backdrop);
		Refresh(false);
	}

	void IconButton::SetActive(bool value)
	{
		active = value;
		Refresh(false);
	}

	void IconButton::OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
		dc.SetBackground(wxBrush(backdrop));
		dc.Clear();

		wxGCDC gcdc(dc);
		wxGraphicsContext* gc = gcdc.GetGraphicsContext();
		if (!gc)
			return;

		const wxRect rect = GetClientRect();
		const bool filled = primary || active;
		const int circleRadius = static_cast<int>(rect.height / 2.0);
		wxRect circleRect = rect;
		circleRect.Deflate(FromDIP(2));

		wxColour background = primary ? Colors().primary : Colors().secondaryContainer;
		wxColour foreground = primary ? Colors().onPrimary : Colors().onSecondaryContainer;

		if (filled)
		{
			if (pressed)
				background = StateLayer(background, foreground, 0.12);
			else if (hovered)
				background = StateLayer(background, foreground, 0.08);
			DrawRoundedRect(gc, circleRect, circleRadius, background, wxNullColour);
		}
		else
		{
			foreground = Colors().onSurfaceVariant;
			if (pressed || hovered)
			{
				const wxColour layer = StateLayer(backdrop, Colors().onSurface, pressed ? 0.12 : 0.08);
				DrawRoundedRect(gc, circleRect, circleRadius, layer, wxNullColour);
			}
		}

		const int iconSize = FromDIP(20);
		const wxRect iconRect(
			rect.x + (rect.width - iconSize) / 2,
			rect.y + (rect.height - iconSize) / 2,
			iconSize,
			iconSize);
		DrawIcon(gc, icon, iconRect, foreground, 1.9);
	}

	void IconButton::OnEnter(wxMouseEvent&)
	{
		hovered = true;
		Refresh(false);
	}

	void IconButton::OnLeave(wxMouseEvent&)
	{
		hovered = false;
		Refresh(false);
	}

	void IconButton::OnLeftDown(wxMouseEvent&)
	{
		if (!HasCapture())
			CaptureMouse();
		pressed = true;
		Refresh(false);
	}

	void IconButton::OnLeftUp(wxMouseEvent& event)
	{
		if (HasCapture())
			ReleaseMouse();

		const bool inside = GetClientRect().Contains(event.GetPosition());
		if (pressed && inside)
			Emit();

		pressed = false;
		Refresh(false);
	}

	void IconButton::OnCaptureLost(wxMouseCaptureLostEvent&)
	{
		pressed = false;
		Refresh(false);
	}

	wxSize IconButton::DoGetBestSize() const
	{
		return wxSize(buttonSize, buttonSize);
	}

	void IconButton::Emit()
	{
		wxCommandEvent event(wxEVT_BUTTON, GetId());
		event.SetEventObject(this);
		ProcessWindowEvent(event);
	}

	PillButton::PillButton(wxWindow* parent, const wxString& label, Variant variant,
		Icon icon, bool showIcon)
		: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
		label(label),
		variant(variant),
		icon(icon),
		showIcon(showIcon),
		backdrop(Colors().surface)
	{
		height = FromDIP(40);
		horizontalPadding = FromDIP(20);
		iconSize = FromDIP(18);
		iconGap = FromDIP(8);
		SetCursor(wxCursor(wxCURSOR_HAND));
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetDoubleBuffered(true);
		SetBackgroundColour(backdrop);

		Bind(wxEVT_PAINT, &PillButton::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
		Bind(wxEVT_ENTER_WINDOW, &PillButton::OnEnter, this);
		Bind(wxEVT_LEAVE_WINDOW, &PillButton::OnLeave, this);
		Bind(wxEVT_LEFT_DOWN, &PillButton::OnLeftDown, this);
		Bind(wxEVT_LEFT_UP, &PillButton::OnLeftUp, this);
		Bind(wxEVT_MOUSE_CAPTURE_LOST, &PillButton::OnCaptureLost, this);
	}

	void PillButton::SetBackdrop(const wxColour& color)
	{
		backdrop = color;
		SetBackgroundColour(backdrop);
		Refresh(false);
	}

	void PillButton::OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
		dc.SetBackground(wxBrush(backdrop));
		dc.Clear();

		wxGCDC gcdc(dc);
		wxGraphicsContext* gc = gcdc.GetGraphicsContext();
		if (!gc)
			return;

		const wxRect rect = GetClientRect();
		const int radius = rect.height / 2;

		wxColour background = wxNullColour;
		wxColour foreground = Colors().primary;
		wxColour border = wxNullColour;

		switch (variant)
		{
			case Variant::Filled:
				background = Colors().primary;
				foreground = Colors().onPrimary;
				break;
			case Variant::Tonal:
				background = Colors().secondaryContainer;
				foreground = Colors().onSecondaryContainer;
				break;
			case Variant::Outlined:
				foreground = Colors().primary;
				border = Colors().outline;
				break;
			case Variant::Text:
				foreground = Colors().primary;
				break;
		}

		if (pressed || hovered)
		{
			const wxColour base = background.IsOk() ? background : backdrop;
			const wxColour layerColour = background.IsOk() ? foreground : Colors().primary;
			background = StateLayer(base, layerColour, pressed ? 0.12 : 0.08);
		}

		DrawRoundedRect(gc, rect, radius, background, border);

		wxFont font = Font(TypeRole::LabelLarge);
		gc->SetFont(font, foreground);

		wxDouble textWidth = 0, textHeight = 0;
		gc->GetTextExtent(label, &textWidth, &textHeight, nullptr, nullptr);

		const double contentWidth = (showIcon ? iconSize + iconGap : 0) + textWidth;
		double x = rect.x + (rect.width - contentWidth) / 2.0;
		const double centerY = rect.y + rect.height / 2.0;

		if (showIcon)
		{
			const wxRect iconRect(static_cast<int>(x), static_cast<int>(centerY - iconSize / 2.0), iconSize, iconSize);
			DrawIcon(gc, icon, iconRect, foreground, 1.7);
			x += iconSize + iconGap;
		}

		gc->DrawText(label, x, centerY - textHeight / 2.0);
	}

	void PillButton::OnEnter(wxMouseEvent&)
	{
		hovered = true;
		Refresh(false);
	}

	void PillButton::OnLeave(wxMouseEvent&)
	{
		hovered = false;
		Refresh(false);
	}

	void PillButton::OnLeftDown(wxMouseEvent&)
	{
		if (!HasCapture())
			CaptureMouse();
		pressed = true;
		Refresh(false);
	}

	void PillButton::OnLeftUp(wxMouseEvent& event)
	{
		if (HasCapture())
			ReleaseMouse();

		const bool inside = GetClientRect().Contains(event.GetPosition());
		if (pressed && inside)
			Emit();

		pressed = false;
		Refresh(false);
	}

	void PillButton::OnCaptureLost(wxMouseCaptureLostEvent&)
	{
		pressed = false;
		Refresh(false);
	}

	wxSize PillButton::DoGetBestSize() const
	{
		wxClientDC dc(const_cast<PillButton*>(this));
		dc.SetFont(Font(TypeRole::LabelLarge));
		wxSize textSize = dc.GetTextExtent(label);
		int width = horizontalPadding * 2 + textSize.x;
		if (showIcon)
			width += iconSize + iconGap;
		return wxSize(width, height);
	}

	void PillButton::Emit()
	{
		wxCommandEvent event(wxEVT_BUTTON, GetId());
		event.SetEventObject(this);
		ProcessWindowEvent(event);
	}

	Chip::Chip(wxWindow* parent, const wxString& text, const wxColour& dotColor, bool outlined)
		: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
		text(text),
		dot(dotColor),
		outlined(outlined)
	{
		height = FromDIP(32);
		horizontalPadding = FromDIP(12);
		dotSize = FromDIP(8);
		dotGap = FromDIP(8);
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetDoubleBuffered(true);
		SetBackgroundColour(Colors().surface);

		Bind(wxEVT_PAINT, &Chip::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
	}

	void Chip::SetLabelText(const wxString& value)
	{
		text = value;
		InvalidateBestSize();
		Refresh(false);
	}

	void Chip::SetDotColor(const wxColour& color)
	{
		dot = color;
		Refresh(false);
	}

	void Chip::SetTonal(const wxColour& fill, const wxColour& text, const wxColour& dotColor)
	{
		outlined = false;
		fillColour = fill;
		textColour = text;
		if (dotColor.IsOk())
			dot = dotColor;
		Refresh(false);
	}

	void Chip::OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
		dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
		dc.Clear();

		wxGCDC gcdc(dc);
		wxGraphicsContext* gc = gcdc.GetGraphicsContext();
		if (!gc)
			return;

		const wxRect rect = GetClientRect();
		const wxColour fill = outlined ? wxNullColour
			: (fillColour.IsOk() ? fillColour : Colors().surfaceContainerHigh);
		const wxColour border = outlined ? Colors().outline : wxNullColour;
		DrawRoundedRect(gc, rect, rect.height / 2, fill, border);

		wxFont font = Font(TypeRole::LabelMedium);
		gc->SetFont(font, textColour.IsOk() ? textColour : Colors().onSurfaceVariant);

		wxDouble textWidth = 0, textHeight = 0;
		gc->GetTextExtent(text, &textWidth, &textHeight, nullptr, nullptr);

		double x = rect.x + horizontalPadding;
		const double centerY = rect.y + rect.height / 2.0;

		if (dot.IsOk())
		{
			gc->SetBrush(wxBrush(dot));
			gc->SetPen(*wxTRANSPARENT_PEN);
			gc->DrawEllipse(x, centerY - dotSize / 2.0, dotSize, dotSize);
			x += dotSize + dotGap;
		}

		gc->DrawText(text, x, centerY - textHeight / 2.0);
	}

	wxSize Chip::DoGetBestSize() const
	{
		wxClientDC dc(const_cast<Chip*>(this));
		dc.SetFont(Font(TypeRole::LabelMedium));
		wxSize textSize = dc.GetTextExtent(text);
		int width = horizontalPadding * 2 + textSize.x;
		if (dot.IsOk())
			width += dotSize + dotGap;
		return wxSize(width, height);
	}

	MenuItem MenuItem::Item(int id, const wxString& label, bool enabled)
	{
		MenuItem item;
		item.type = Type::Item;
		item.id = id;
		item.label = label;
		item.enabled = enabled;
		return item;
	}

	MenuItem MenuItem::Check(int id, const wxString& label, bool checked, bool enabled)
	{
		MenuItem item;
		item.type = Type::Check;
		item.id = id;
		item.label = label;
		item.checked = checked;
		item.enabled = enabled;
		return item;
	}

	MenuItem MenuItem::Radio(int id, const wxString& label, bool checked, bool enabled)
	{
		MenuItem item;
		item.type = Type::Radio;
		item.id = id;
		item.label = label;
		item.checked = checked;
		item.enabled = enabled;
		return item;
	}

	MenuItem MenuItem::Separator()
	{
		MenuItem item;
		item.type = Type::Separator;
		return item;
	}

	MenuItem MenuItem::Header(const wxString& label)
	{
		MenuItem item;
		item.type = Type::Header;
		item.label = label;
		return item;
	}

	MenuPopup::MenuPopup(wxWindow* parent, const std::vector<MenuItem>& menuItems,
		std::function<void(int, bool)> selectHandler)
		: wxPopupTransientWindow(parent, wxBORDER_NONE),
		items(menuItems),
		onSelect(std::move(selectHandler))
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetDoubleBuffered(true);
		SetBackgroundColour(Colors().surfaceContainerHigh);

		// The popup is a top-level window, so DWM would otherwise add its own
		// shadow margin and corner rounding on top of our drawn surface. That
		// shifted the menu away from its anchor and left stair-stepped notches
		// at the corners. Disabling non-client rendering makes the window rect
		// match the drawn client area exactly.
#ifdef __WXMSW__
		{
			HWND hwnd = static_cast<HWND>(GetHandle());
			HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
			if (dwm)
			{
				typedef HRESULT(WINAPI* Fn)(HWND, DWORD, LPCVOID, DWORD);
				auto setAttr = reinterpret_cast<Fn>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
				if (setAttr)
				{
					const int policy = 2; // DWMNCRP_DISABLED
					setAttr(hwnd, 2, &policy, sizeof(policy)); // DWMWA_NCRENDERING_POLICY
					const int cornerPref = 1; // DWMWCP_DONOTROUND
					setAttr(hwnd, 33, &cornerPref, sizeof(cornerPref)); // DWMWA_WINDOW_CORNER_PREFERENCE
				}
				FreeLibrary(dwm);
			}
		}
#endif

		Bind(wxEVT_PAINT, &MenuPopup::OnPaint, this);
		Bind(wxEVT_MOTION, &MenuPopup::OnMouseMove, this);
		Bind(wxEVT_LEFT_UP, &MenuPopup::OnLeftUp, this);
		Bind(wxEVT_LEAVE_WINDOW, &MenuPopup::OnLeaveWindow, this);
		Bind(wxEVT_KEY_DOWN, &MenuPopup::OnKeyDown, this);

		Recompute();
	}

	void MenuPopup::Recompute()
	{
		rowHeight = FromDIP(40);
		headerHeight = FromDIP(34);
		padding = FromDIP(8);

		wxClientDC dc(this);
		dc.SetFont(Font(TypeRole::BodyMedium));

		int maxWidth = std::max(FromDIP(220), minWidth);
		for (const auto& item : items)
		{
			if (item.type == MenuItem::Type::Separator)
				continue;
			const wxSize textSize = dc.GetTextExtent(item.label);
			const int itemWidth = textSize.x + padding * 2 + rowHeight + FromDIP(12);
			maxWidth = std::max(maxWidth, itemWidth);
		}

		width = maxWidth;
		height = padding;
		rowTops.clear();
		for (const auto& item : items)
		{
			if (item.type == MenuItem::Type::Separator)
			{
				rowTops.push_back(-1);
				height += FromDIP(13);
			}
			else if (item.type == MenuItem::Type::Header)
			{
				rowTops.push_back(height);
				height += headerHeight;
			}
			else
			{
				rowTops.push_back(height);
				height += rowHeight;
			}
		}
		height += padding;

		SetSize(wxSize(width, height));
		ApplyRoundedShape();
	}

	// The popup is a top-level window, so the OS would otherwise apply its own
	// corner rounding and reveal whatever sits behind the square window corners.
	// Clipping the window to the same rounded rectangle that OnPaint draws keeps
	// the menu surface clean on every Windows version.
	void MenuPopup::ApplyRoundedShape()
	{
#ifdef __WXMSW__
		if (!dwmConfigured)
		{
			HWND hwnd = static_cast<HWND>(GetHandle());
			HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
			if (dwm)
			{
				typedef HRESULT(WINAPI* Fn)(HWND, DWORD, LPCVOID, DWORD);
				auto setAttr = reinterpret_cast<Fn>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
				if (setAttr)
				{
					const int policy = 2; // DWMNCRP_DISABLED
					setAttr(hwnd, 2, &policy, sizeof(policy)); // DWMWA_NCRENDERING_POLICY
					const int cornerPref = 1; // DWMWCP_DONOTROUND
					setAttr(hwnd, 33, &cornerPref, sizeof(cornerPref)); // DWMWA_WINDOW_CORNER_PREFERENCE
				}
				FreeLibrary(dwm);
			}

			RECT wr;
			::GetWindowRect(hwnd, &wr);
			POINT pt = { 0, 0 };
			::ClientToScreen(hwnd, &pt);
			shadowMarginX = pt.x - wr.left;
			shadowMarginY = pt.y - wr.top;
			dwmConfigured = true;
		}
#endif

		const int radius = FromDIP(12);
		if (radius <= 0 || width <= 0 || height <= 0)
			return;

		const int steps = 64;
		std::vector<wxPoint> points;
		points.reserve(4 * (steps + 1));

		auto addCorner = [&](double cx, double cy, double startDegrees)
		{
			for (int i = 0; i <= steps; ++i)
			{
				const double angle = Deg(startDegrees + 90.0 * i / steps);
				points.push_back(wxPoint(
					static_cast<int>(std::lround(cx + radius * std::cos(angle))),
					static_cast<int>(std::lround(cy + radius * std::sin(angle)))));
			}
		};

		addCorner(radius, radius, 180.0);
		addCorner(width - radius, radius, 270.0);
		addCorner(width - radius, height - radius, 0.0);
		addCorner(radius, height - radius, 90.0);

		SetShape(wxRegion(static_cast<int>(points.size()), points.data()));
	}

	int MenuPopup::HitTest(int y) const
	{
		for (size_t i = 0; i < rowTops.size(); i++)
		{
			if (rowTops[i] < 0)
				continue;
			const int rowTop = rowTops[i];
			const int rowSize = (items[i].type == MenuItem::Type::Header) ? headerHeight : rowHeight;
			if (y >= rowTop && y < rowTop + rowSize)
				return static_cast<int>(i);
		}
		return -1;
	}

	void MenuPopup::OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
		dc.SetBackground(wxBrush(Colors().surfaceContainerHigh));
		dc.Clear();

		wxGCDC gcdc(dc);
		wxGraphicsContext* gc = gcdc.GetGraphicsContext();
		if (!gc)
			return;

		const wxRect client = GetClientRect();
		DrawRoundedRect(gc, client, FromDIP(12), Colors().surfaceContainerHigh, Colors().outlineVariant);

		for (size_t i = 0; i < items.size(); i++)
		{
			const MenuItem& item = items[i];

			if (item.type == MenuItem::Type::Separator)
			{
				const int y = rowTops[i] + FromDIP(6);
				gc->SetPen(wxPen(Colors().outlineVariant, 1));
				gc->StrokeLine(client.x + padding, y, client.GetRight() - padding, y);
				continue;
			}

			if (item.type == MenuItem::Type::Header)
			{
				gc->SetFont(Font(TypeRole::LabelSmall), Colors().onSurfaceVariant);
				wxDouble tw = 0, th = 0;
				gc->GetTextExtent(item.label, &tw, &th, nullptr, nullptr);
				gc->DrawText(item.label.Upper(), client.x + padding + FromDIP(10),
					rowTops[i] + (headerHeight - th) / 2.0);
				continue;
			}

			const int rowTop = rowTops[i];
			wxRect rowRect(client.x + padding, rowTop, client.width - padding * 2, rowHeight);

			const bool isHovered = (static_cast<int>(i) == hovered) && item.enabled;
			if (isHovered)
			{
				DrawRoundedRect(gc, rowRect, FromDIP(8),
					StateLayer(Colors().surfaceContainerHigh, Colors().onSurface, 0.08), wxNullColour);
			}

			const bool hasIndicator = item.type == MenuItem::Type::Check || item.type == MenuItem::Type::Radio;
			const int iconSize = FromDIP(18);
			const int indicatorX = rowRect.x + FromDIP(12)+ (FromDIP(24) - iconSize) / 2;
			const int centerY = rowTop + rowHeight / 2;

			if (item.type == MenuItem::Type::Check && item.checked)
			{
				DrawIcon(gc, Icon::Check, wxRect(indicatorX, centerY - iconSize / 2, iconSize, iconSize),
					Colors().primary, 2.0);
			}
			else if (item.type == MenuItem::Type::Radio)
			{
				const int r = FromDIP(8);
				gc->SetPen(wxPen(item.checked ? Colors().primary : Colors().outline, 1.6));
				gc->SetBrush(*wxTRANSPARENT_BRUSH);
				gc->DrawEllipse(indicatorX + (iconSize - 2 * r) / 2.0, centerY - r, 2 * r, 2 * r);
				if (item.checked)
				{
					gc->SetPen(*wxTRANSPARENT_PEN);
					gc->SetBrush(wxBrush(Colors().primary));
					gc->DrawEllipse(indicatorX + (iconSize - 2 * r) / 2.0 + r * 0.45, centerY - r * 0.55, r * 1.1, r * 1.1);
				}
			}

			const wxColour textColor = !item.enabled
				? Colors().outline
				: Colors().onSurface;
			gc->SetFont(Font(TypeRole::BodyMedium), textColor);

			wxDouble tw = 0, th = 0;
			gc->GetTextExtent(item.label, &tw, &th, nullptr, nullptr);
			const int textX = hasIndicator
				? rowRect.x + FromDIP(12) + FromDIP(24) + FromDIP(10)
				: rowRect.x + FromDIP(14);
			gc->DrawText(item.label, textX, centerY - th / 2.0);
		}
	}

	void MenuPopup::OnMouseMove(wxMouseEvent& event)
	{
		const int index = HitTest(event.GetY());
		if (index != hovered)
		{
			hovered = index;
			Refresh(false);
		}
	}

	void MenuPopup::OnLeaveWindow(wxMouseEvent&)
	{
		hovered = -1;
		Refresh(false);
	}

	void MenuPopup::OnLeftUp(wxMouseEvent& event)
	{
		Activate(HitTest(event.GetY()));
	}

	void MenuPopup::OnKeyDown(wxKeyEvent& event)
	{
		if (event.GetKeyCode() == WXK_ESCAPE)
		{
			Dismiss();
			return;
		}

		if (event.GetKeyCode() == WXK_DOWN || event.GetKeyCode() == WXK_UP)
		{
			const int direction = event.GetKeyCode() == WXK_DOWN ? 1 : -1;
			int index = hovered;
			for (int step = 0; step < static_cast<int>(items.size()); step++)
			{
				index += direction;
				if (index < 0) index = static_cast<int>(items.size()) - 1;
				if (index >= static_cast<int>(items.size())) index = 0;
				if (items[index].type != MenuItem::Type::Separator &&
					items[index].type != MenuItem::Type::Header &&
					items[index].enabled)
					break;
			}
			hovered = index;
			Refresh(false);
			return;
		}

		if (event.GetKeyCode() == WXK_RETURN)
		{
			Activate(hovered);
			return;
		}

		event.Skip();
	}

	void MenuPopup::Activate(int index)
	{
		if (index < 0 || index >= static_cast<int>(items.size()))
			return;

		MenuItem& item = items[index];
		if (!item.enabled)
			return;

		if (item.type == MenuItem::Type::Check)
		{
			item.checked = !item.checked;
			if (onSelect)
				onSelect(item.id, item.checked);
			Dismiss();
		}
		else if (item.type == MenuItem::Type::Radio)
		{
			item.checked = true;
			if (onSelect)
				onSelect(item.id, true);
			Dismiss();
		}
		else if (item.type == MenuItem::Type::Item)
		{
			if (onSelect)
				onSelect(item.id, false);
			Dismiss();
		}
	}

	void MenuPopup::OnDismiss()
	{
		if (!dismissed)
		{
			dismissed = true;
			Destroy();
		}
	}

	void MenuPopup::PopupAt(const wxPoint& screenPosition)
	{
		wxPoint position = screenPosition;
		int displayIndex = wxDisplay::GetFromPoint(screenPosition);
		if (displayIndex == wxNOT_FOUND)
			displayIndex = 0;
		const wxRect display = wxDisplay(displayIndex).GetClientArea();
		if (position.x + width + shadowMarginX > display.GetRight())
			position.x = display.GetRight() - width - shadowMarginX;
		if (position.y + height + shadowMarginY > display.GetBottom())
			position.y = display.GetBottom() - height - shadowMarginY;
		if (position.x < display.x)
			position.x = display.x;
		if (position.y < display.y)
			position.y = display.y;

		// The window rect includes the DWM shadow margin; compensate so the
		// drawn surface (client area) sits exactly on the anchor.
		position.x -= shadowMarginX;
		position.y -= shadowMarginY;

		SetPosition(position);
		Popup();
	}

	void MenuPopup::SetMinimumWidth(int minimumWidth)
	{
		minWidth = minimumWidth;
		Recompute();
	}

	Select::Select(wxWindow* parent, const wxString& placeholderText)
		: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
		placeholder(placeholderText)
	{
		fieldHeight = FromDIP(40);
		horizontalPadding = FromDIP(14);
		chevronSize = FromDIP(18);
		SetCursor(wxCursor(wxCURSOR_HAND));
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetDoubleBuffered(true);
		SetBackgroundColour(parent ? parent->GetBackgroundColour() : Colors().surface);

		Bind(wxEVT_PAINT, &Select::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
		Bind(wxEVT_ENTER_WINDOW, &Select::OnEnter, this);
		Bind(wxEVT_LEAVE_WINDOW, &Select::OnLeave, this);
		Bind(wxEVT_LEFT_DOWN, &Select::OnLeftDown, this);
		Bind(wxEVT_LEFT_UP, &Select::OnLeftUp, this);
		Bind(wxEVT_MOUSE_CAPTURE_LOST, &Select::OnCaptureLost, this);
		Bind(wxEVT_KEY_DOWN, &Select::OnKeyDown, this);
	}

	void Select::Clear()
	{
		items.Clear();
		selection = -1;
		InvalidateBestSize();
		Refresh(false);
	}

	void Select::Append(const wxString& item)
	{
		items.Add(item);
		InvalidateBestSize();
		Refresh(false);
	}

	void Select::SetSelection(int index)
	{
		if (index >= -1 && index < static_cast<int>(items.size()))
		{
			selection = index;
			Refresh(false);
		}
	}

	wxString Select::GetStringSelection() const
	{
		if (selection >= 0 && selection < static_cast<int>(items.size()))
			return items[selection];
		return wxEmptyString;
	}

	void Select::SetPlaceholder(const wxString& text)
	{
		placeholder = text;
		Refresh(false);
	}

	void Select::OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
		dc.SetBackground(wxBrush(Colors().surfaceContainerLow));
		dc.Clear();

		wxGCDC gcdc(dc);
		wxGraphicsContext* gc = gcdc.GetGraphicsContext();
		if (!gc)
			return;

		const wxRect rect = GetClientRect();
		DrawRoundedRect(gc, rect, FromDIP(8), Colors().surfaceContainerHighest, wxNullColour);
		if (hovered || pressed)
		{
			DrawRoundedRect(gc, rect, FromDIP(8),
				StateLayer(Colors().surfaceContainerHighest, Colors().onSurface, pressed ? 0.12 : 0.06),
				wxNullColour);
		}

		const bool hasSelection = selection >= 0 && selection < static_cast<int>(items.size());
		const wxString label = hasSelection ? items[selection] : placeholder;
		gc->SetFont(Font(TypeRole::BodyMedium), hasSelection ? Colors().onSurface : Colors().onSurfaceVariant);

		wxDouble tw = 0, th = 0;
		gc->GetTextExtent(label, &tw, &th, nullptr, nullptr);
		gc->DrawText(label, rect.x + horizontalPadding, rect.y + (rect.height - th) / 2.0);

		const wxRect chevronRect(
			rect.GetRight() - horizontalPadding - chevronSize + FromDIP(2),
			rect.y + (rect.height - chevronSize) / 2,
			chevronSize,
			chevronSize);
		DrawIcon(gc, Icon::ChevronDown, chevronRect, Colors().onSurfaceVariant, 2.0);
	}

	void Select::OnEnter(wxMouseEvent&)
	{
		hovered = true;
		Refresh(false);
	}

	void Select::OnLeave(wxMouseEvent&)
	{
		hovered = false;
		Refresh(false);
	}

	void Select::OnLeftDown(wxMouseEvent&)
	{
		if (!HasCapture())
			CaptureMouse();
		pressed = true;
		Refresh(false);
	}

	void Select::OnLeftUp(wxMouseEvent& event)
	{
		if (HasCapture())
			ReleaseMouse();
		pressed = false;
		Refresh(false);
		if (GetClientRect().Contains(event.GetPosition()))
			OpenMenu();
	}

	void Select::OnCaptureLost(wxMouseCaptureLostEvent&)
	{
		pressed = false;
		Refresh(false);
	}

	void Select::OnKeyDown(wxKeyEvent& event)
	{
		if (event.GetKeyCode() == WXK_SPACE || event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_DOWN)
		{
			OpenMenu();
			return;
		}
		event.Skip();
	}

	wxSize Select::DoGetBestSize() const
	{
		wxClientDC dc(const_cast<Select*>(this));
		dc.SetFont(Font(TypeRole::BodyMedium));
		int widest = 0;
		for (const auto& item : items)
			widest = std::max(widest, dc.GetTextExtent(item).x);
		widest = std::max(widest, dc.GetTextExtent(placeholder).x);
		const int width = horizontalPadding * 2 + chevronSize + FromDIP(18) + widest;
		return wxSize(width, fieldHeight);
	}

	void Select::OpenMenu()
	{
		using Material::MenuItem;

		std::vector<MenuItem> menuItems;
		for (size_t i = 0; i < items.size(); i++)
		{
			// Material 3 dropdown menus mark the current value with a check.
			menuItems.push_back(MenuItem::Check(static_cast<int>(i) + 1, items[i],
				static_cast<int>(i) == selection));
		}

		if (menuItems.empty())
			return;

		const wxPoint origin = ClientToScreen(wxPoint(0, GetSize().y + FromDIP(4)));
		auto* menu = new MenuPopup(this, menuItems, [this](int id, bool)
		{
			const int index = id - 1;
			if (index >= 0 && index < static_cast<int>(items.size()))
			{
				selection = index;
				Refresh(false);
				EmitChanged();
			}
		});
		menu->SetMinimumWidth(GetSize().x);
		menu->PopupAt(origin);
	}

	void Select::EmitChanged()
	{
		wxCommandEvent event(wxEVT_CHOICE, GetId());
		event.SetEventObject(this);
		event.SetInt(selection);
		ProcessWindowEvent(event);
	}

	SettingsRow::SettingsRow(wxWindow* parent, const wxString& labelText,
		const wxString& subtitleText, Mode mode, bool checked)
		: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
		label(labelText),
		subtitle(subtitleText),
		mode(mode),
		checked(checked)
	{
		rowHeight = subtitle.empty() ? FromDIP(56) : FromDIP(72);
		SetCursor(wxCursor(wxCURSOR_HAND));
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetDoubleBuffered(true);
		SetBackgroundColour(parent ? parent->GetBackgroundColour() : Colors().surfaceContainerLow);

		Bind(wxEVT_PAINT, &SettingsRow::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
		Bind(wxEVT_ENTER_WINDOW, &SettingsRow::OnEnter, this);
		Bind(wxEVT_LEAVE_WINDOW, &SettingsRow::OnLeave, this);
		Bind(wxEVT_LEFT_DOWN, &SettingsRow::OnLeftDown, this);
		Bind(wxEVT_LEFT_UP, &SettingsRow::OnLeftUp, this);
		Bind(wxEVT_MOUSE_CAPTURE_LOST, &SettingsRow::OnCaptureLost, this);
		Bind(wxEVT_KEY_DOWN, &SettingsRow::OnKeyDown, this);
	}

	void SettingsRow::SetChecked(bool value)
	{
		checked = value;
		Refresh(false);
	}

	void SettingsRow::SetSubtitle(const wxString& text)
	{
		subtitle = text;
		rowHeight = subtitle.empty() ? FromDIP(56) : FromDIP(72);
		InvalidateBestSize();
		Refresh(false);
	}

	void SettingsRow::OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
		const wxColour backdrop = GetParent() ? GetParent()->GetBackgroundColour() : Colors().surfaceContainerLow;
		dc.SetBackground(wxBrush(backdrop));
		dc.Clear();

		wxGCDC gcdc(dc);
		wxGraphicsContext* gc = gcdc.GetGraphicsContext();
		if (!gc)
			return;

		const wxRect rect = GetClientRect();
		if (hovered || pressed)
		{
			DrawRoundedRect(gc, rect, FromDIP(12),
				StateLayer(backdrop, Colors().onSurface, pressed ? 0.10 : 0.06), wxNullColour);
		}

		gc->SetFont(Font(TypeRole::BodyLarge), Colors().onSurface);
		wxDouble tw = 0, th = 0;
		gc->GetTextExtent(label, &tw, &th, nullptr, nullptr);
		if (subtitle.empty())
		{
			gc->DrawText(label, rect.x + FromDIP(16), rect.y + (rect.height - th) / 2.0);
		}
		else
		{
			gc->DrawText(label, rect.x + FromDIP(16), rect.y + FromDIP(13));
			gc->SetFont(Font(TypeRole::BodySmall), Colors().onSurfaceVariant);
			gc->GetTextExtent(subtitle, &tw, &th, nullptr, nullptr);
			gc->DrawText(subtitle, rect.x + FromDIP(16), rect.y + FromDIP(36));
		}

		// Trailing control.
		const int controlRight = rect.GetRight() - FromDIP(16);
		const int centerY = rect.y + rect.height / 2;

		if (mode == Mode::Check)
		{
			const int trackW = FromDIP(44);
			const int trackH = FromDIP(24);
			const int trackX = controlRight - trackW;
			const int trackY = centerY - trackH / 2;
			const wxColour trackFill = checked ? Colors().primary : Colors().surfaceContainerHighest;
			const wxColour trackBorder = checked ? wxNullColour : Colors().outline;
			DrawRoundedRect(gc, wxRect(trackX, trackY, trackW, trackH), trackH / 2, trackFill, trackBorder);

			const int thumbR = FromDIP(9);
			const int thumbCx = checked
				? trackX + trackW - thumbR - FromDIP(3)
				: trackX + thumbR + FromDIP(3);
			gc->SetPen(*wxTRANSPARENT_PEN);
			gc->SetBrush(wxBrush(checked ? Colors().onPrimary : Colors().outline));
			gc->DrawEllipse(thumbCx - thumbR, centerY - thumbR, thumbR * 2, thumbR * 2);
		}
		else
		{
			const int r = FromDIP(10);
			const int cx = controlRight - r;
			gc->SetBrush(*wxTRANSPARENT_BRUSH);
			gc->SetPen(wxPen(checked ? Colors().primary : Colors().outline, checked ? 2.2 : 1.6));
			gc->DrawEllipse(cx - r, centerY - r, r * 2, r * 2);
			if (checked)
			{
				const int dot = FromDIP(5);
				gc->SetPen(*wxTRANSPARENT_PEN);
				gc->SetBrush(wxBrush(Colors().primary));
				gc->DrawEllipse(cx - dot, centerY - dot, dot * 2, dot * 2);
			}
		}
	}

	void SettingsRow::OnEnter(wxMouseEvent&)
	{
		hovered = true;
		Refresh(false);
	}

	void SettingsRow::OnLeave(wxMouseEvent&)
	{
		hovered = false;
		Refresh(false);
	}

	void SettingsRow::OnLeftDown(wxMouseEvent&)
	{
		if (!HasCapture())
			CaptureMouse();
		pressed = true;
		Refresh(false);
	}

	void SettingsRow::OnLeftUp(wxMouseEvent& event)
	{
		if (HasCapture())
			ReleaseMouse();
		pressed = false;
		Refresh(false);
		if (GetClientRect().Contains(event.GetPosition()))
		{
			if (mode == Mode::Check)
			{
				checked = !checked;
				Refresh(false);
			}
			else
			{
				checked = true;
				Refresh(false);
			}
			Emit();
		}
	}

	void SettingsRow::OnCaptureLost(wxMouseCaptureLostEvent&)
	{
		pressed = false;
		Refresh(false);
	}

	void SettingsRow::OnKeyDown(wxKeyEvent& event)
	{
		if (event.GetKeyCode() == WXK_SPACE || event.GetKeyCode() == WXK_RETURN)
		{
			if (mode == Mode::Check)
				checked = !checked;
			else
				checked = true;
			Refresh(false);
			Emit();
			return;
		}
		event.Skip();
	}

	wxSize SettingsRow::DoGetBestSize() const
	{
		return wxSize(FromDIP(320), rowHeight);
	}

	void SettingsRow::Emit()
	{
		wxCommandEvent event(mode == Mode::Check ? wxEVT_CHECKBOX : wxEVT_RADIOBUTTON, GetId());
		event.SetEventObject(this);
		event.SetInt(checked ? 1 : 0);
		ProcessWindowEvent(event);
	}

	DeviceCard::DeviceCard(wxWindow* parent, const wxString& name, const wxString& url,
		const wxString& backRes, const wxString& frontRes)
		: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
		name(name),
		url(url),
		backRes(backRes),
		frontRes(frontRes)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetDoubleBuffered(true);
		SetBackgroundColour(parent ? parent->GetBackgroundColour() : Colors().surface);
		Bind(wxEVT_PAINT, &DeviceCard::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
		Bind(wxEVT_ENTER_WINDOW, &DeviceCard::OnEnter, this);
		Bind(wxEVT_LEAVE_WINDOW, &DeviceCard::OnLeave, this);
	}

	void DeviceCard::OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
		const wxColour backdrop = GetParent() ? GetParent()->GetBackgroundColour() : Colors().surface;
		dc.SetBackground(wxBrush(backdrop));
		dc.Clear();

		wxGCDC gcdc(dc);
		wxGraphicsContext* gc = gcdc.GetGraphicsContext();
		if (!gc)
			return;

		const wxRect rect = GetClientRect();
		DrawRoundedRect(gc, rect, FromDIP(16), Colors().surfaceContainerLow,
			hovered ? Colors().outline : Colors().outlineVariant);

		const int centerY = rect.y + rect.height / 2;
		const int iconRadius = FromDIP(20);
		const int iconCx = rect.x + FromDIP(16) + iconRadius;
		gc->SetPen(*wxTRANSPARENT_PEN);
		gc->SetBrush(wxBrush(Colors().secondaryContainer));
		gc->DrawEllipse(iconCx - iconRadius, centerY - iconRadius, iconRadius * 2, iconRadius * 2);
		DrawIcon(gc, Icon::Devices,
			wxRect(iconCx - FromDIP(12), centerY - FromDIP(12), FromDIP(24), FromDIP(24)),
			Colors().onSecondaryContainer, 1.8);

		const int textX = iconCx + iconRadius + FromDIP(14);
		gc->SetFont(Font(TypeRole::TitleSmall), Colors().onSurface);
		wxDouble tw = 0, th = 0;
		gc->GetTextExtent(name, &tw, &th, nullptr, nullptr);
		gc->DrawText(name, textX, centerY - th - FromDIP(2));

		gc->SetFont(Font(TypeRole::BodySmall), Colors().onSurfaceVariant);
		gc->GetTextExtent(url, &tw, &th, nullptr, nullptr);
		gc->DrawText(url, textX, centerY + FromDIP(2));

		gc->SetFont(Font(TypeRole::LabelSmall), Colors().onSurfaceVariant);
		const wxString resolutionText = wxString::Format("Back %s  -  Front %s", backRes, frontRes);
		gc->GetTextExtent(resolutionText, &tw, &th, nullptr, nullptr);
		gc->DrawText(resolutionText, rect.GetRight() - FromDIP(16) - tw, centerY - th / 2.0);
	}

	void DeviceCard::OnEnter(wxMouseEvent&)
	{
		hovered = true;
		Refresh(false);
	}

	void DeviceCard::OnLeave(wxMouseEvent&)
	{
		hovered = false;
		Refresh(false);
	}

	wxSize DeviceCard::DoGetBestSize() const
	{
		return wxSize(FromDIP(560), FromDIP(72));
	}

	EmptyState::EmptyState(wxWindow* parent, Icon icon, const wxString& title,
		const wxString& subtitle)
		: wxControl(parent, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(420), FromDIP(220)),
			wxBORDER_NONE),
		icon(icon),
		title(title),
		subtitle(subtitle)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetDoubleBuffered(true);
		SetBackgroundColour(parent ? parent->GetBackgroundColour() : Colors().surface);
		Bind(wxEVT_PAINT, &EmptyState::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
	}

	void EmptyState::OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
		const wxColour backdrop = GetParent() ? GetParent()->GetBackgroundColour() : Colors().surface;
		dc.SetBackground(wxBrush(backdrop));
		dc.Clear();

		wxGCDC gcdc(dc);
		wxGraphicsContext* gc = gcdc.GetGraphicsContext();
		if (!gc)
			return;

		const wxRect rect = GetClientRect();
		const int iconSize = FromDIP(48);
		DrawIcon(gc, icon,
			wxRect(rect.x + (rect.width - iconSize) / 2, rect.y + FromDIP(28), iconSize, iconSize),
			Colors().onSurfaceVariant, 1.8);

		wxDouble tw = 0, th = 0;
		gc->SetFont(Font(TypeRole::TitleMedium), Colors().onSurface);
		gc->GetTextExtent(title, &tw, &th, nullptr, nullptr);
		gc->DrawText(title, rect.x + (rect.width - tw) / 2.0, rect.y + FromDIP(94));

		gc->SetFont(Font(TypeRole::BodySmall), Colors().onSurfaceVariant);
		gc->GetTextExtent(subtitle, &tw, &th, nullptr, nullptr);
		gc->DrawText(subtitle, rect.x + (rect.width - tw) / 2.0, rect.y + FromDIP(124));
	}

	wxStaticText* Label(wxWindow* parent, const wxString& text, TypeRole role, const wxColour& color)
	{
		auto label = new wxStaticText(parent, wxID_ANY, text);
		label->SetFont(Font(role));
		label->SetForegroundColour(color.IsOk() ? color : Colors().onSurface);
		if (parent)
			label->SetBackgroundColour(parent->GetBackgroundColour());
		return label;
	}

	void StyleChoice(wxChoice* choice)
	{
		if (!choice)
			return;
		choice->SetFont(Font(TypeRole::BodyMedium));
		choice->SetForegroundColour(Colors().onSurface);
		choice->SetBackgroundColour(Colors().surfaceContainerHighest);
		choice->SetMinSize(wxSize(choice->GetMinSize().x, choice->FromDIP(36)));
	}

	void StyleSlider(wxSlider* slider)
	{
		if (!slider)
			return;
		slider->SetBackgroundColour(Colors().surfaceContainerLow);
		slider->SetForegroundColour(Colors().primary);
	}

	void StyleCheckBox(wxCheckBox* checkBox)
	{
		if (!checkBox)
			return;
		checkBox->SetFont(Font(TypeRole::BodyMedium));
		checkBox->SetForegroundColour(Colors().onSurface);
		checkBox->SetBackgroundColour(Colors().surfaceContainerLow);
	}

	void StyleRadioButton(wxRadioButton* radio)
	{
		if (!radio)
			return;
		radio->SetFont(Font(TypeRole::BodyMedium));
		radio->SetForegroundColour(Colors().onSurface);
		radio->SetBackgroundColour(Colors().surfaceContainerLow);
	}

	void StyleTextCtrl(wxTextCtrl* textCtrl)
	{
		if (!textCtrl)
			return;
		textCtrl->SetFont(Font(TypeRole::BodyMedium));
		textCtrl->SetForegroundColour(Colors().onSurface);
		textCtrl->SetBackgroundColour(Colors().surfaceContainerHighest);
	}

	void StyleListView(wxListView* listView)
	{
		if (!listView)
			return;
		listView->SetFont(Font(TypeRole::BodyMedium));
		listView->SetForegroundColour(Colors().onSurface);
		listView->SetBackgroundColour(Colors().surfaceContainerLow);
	}
}
