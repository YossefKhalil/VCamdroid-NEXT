#pragma once

#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <wx/popupwin.h>

#include <functional>
#include <vector>

class wxListView;

namespace Material
{
	// Material 3 baseline color roles. The light and dark schemes below are
	// the standard M3 baseline palette (purple seed), so the app follows the
	// same color logic as Material 3 components.
	struct ColorScheme
	{
		wxColour primary;
		wxColour onPrimary;
		wxColour primaryContainer;
		wxColour onPrimaryContainer;

		wxColour secondary;
		wxColour onSecondary;
		wxColour secondaryContainer;
		wxColour onSecondaryContainer;

		wxColour tertiary;
		wxColour onTertiary;
		wxColour tertiaryContainer;
		wxColour onTertiaryContainer;

		wxColour error;
		wxColour onError;
		wxColour errorContainer;
		wxColour onErrorContainer;

		wxColour surface;
		wxColour onSurface;
		wxColour surfaceVariant;
		wxColour onSurfaceVariant;

		// Window / dialog backdrop. Material 3 uses the surface role for
		// full-screen backgrounds; kept as a separate alias for readability.
		wxColour background;

		wxColour surfaceContainerLowest;
		wxColour surfaceContainerLow;
		wxColour surfaceContainer;
		wxColour surfaceContainerHigh;
		wxColour surfaceContainerHighest;

		wxColour outline;
		wxColour outlineVariant;

		wxColour inverseSurface;
		wxColour inverseOnSurface;
		wxColour inversePrimary;

		wxColour shadow;
		wxColour scrim;
		wxColour videoBackground;
	};

	const ColorScheme& Colors();
	bool IsDark();

	// Material 3 type scale, adapted to desktop point sizes.
	enum class TypeRole
	{
		DisplayLarge,
		DisplayMedium,
		DisplaySmall,
		HeadlineLarge,
		HeadlineMedium,
		HeadlineSmall,
		TitleLarge,
		TitleMedium,
		TitleSmall,
		BodyLarge,
		BodyMedium,
		BodySmall,
		LabelLarge,
		LabelMedium,
		LabelSmall
	};

	wxFont Font(TypeRole role);
	wxFont Font(int pointSize, wxFontWeight weight = wxFONTWEIGHT_NORMAL);
	wxString UiFontFace();

	// Material 3 shape scale, in DIPs.
	enum class Shape
	{
		ExtraSmall = 4,
		Small = 8,
		Medium = 12,
		Large = 16,
		ExtraLarge = 28,
		Full = 999
	};

	int Radius(Shape shape);

	wxColour StateLayer(const wxColour& base, const wxColour& layer, double opacity);
	void DrawRoundedRect(wxGraphicsContext* gc, const wxRect& rect, int radius,
		const wxColour& fill, const wxColour& border = wxNullColour, int borderWidth = 1);
	void DrawShadow(wxGraphicsContext* gc, const wxRect& rect, int radius, int elevation);

	enum class Icon
	{
		QrCode,
		Devices,
		Settings,
		MoreVert,
		RotateLeft,
		RotateRight,
		FlipHorizontal,
		FlipVertical,
		ZoomIn,
		ZoomOut,
		Flash,
		SwapCamera,
		Snapshot,
		Adjustments,
		StreamSettings,
		Close,
		Check,
		Info,
		Exit,
		Stats,
		ChevronDown,
		Refresh,
		ArrowBack,
		Gear
	};

	void DrawIcon(wxGraphicsContext* gc, Icon icon, const wxRect& rect,
		const wxColour& color, double strokeWidth = 2.0);

	// Filled Material 3 card with optional level-1 elevation.
	class Card : public wxPanel
	{
	public:
		Card(wxWindow* parent, int padding = 16, Shape shape = Shape::Medium, bool elevated = false);

		wxBoxSizer* Content();
		void SetBackdrop(const wxColour& color);
		void SetFill(const wxColour& color);

	private:
		wxBoxSizer* content;
		int padding;
		int radius;
		bool elevated;
		wxColour backdrop;
		wxColour fill;

		void OnPaint(wxPaintEvent& event);
	};

	// Card with a Material section title and an empty body sizer.
	class Section : public Card
	{
	public:
		Section(wxWindow* parent, const wxString& title, const wxString& subtitle = wxEmptyString);

		wxBoxSizer* Body();

	private:
		wxBoxSizer* body;
	};

	// Circular Material 3 icon button with a state layer.
	class IconButton : public wxControl
	{
	public:
		IconButton(wxWindow* parent, Icon icon, const wxString& tooltip,
			bool primary = false, int size = 40);

		void SetBackdrop(const wxColour& color);
		void SetActive(bool value);
		bool IsActive() const { return active; }

	private:
		Icon icon;
		bool primary;
		bool hovered = false;
		bool pressed = false;
		bool active = false;
		int buttonSize;
		wxColour backdrop;

		void OnPaint(wxPaintEvent& event);
		void OnEnter(wxMouseEvent& event);
		void OnLeave(wxMouseEvent& event);
		void OnLeftDown(wxMouseEvent& event);
		void OnLeftUp(wxMouseEvent& event);
		void OnCaptureLost(wxMouseCaptureLostEvent& event);
		wxSize DoGetBestSize() const override;
		void Emit();
	};

	// Material 3 filled / tonal / outlined / text button with optional icon.
	class PillButton : public wxControl
	{
	public:
		enum class Variant
		{
			Filled,
			Tonal,
			Outlined,
			Text
		};

		PillButton(wxWindow* parent, const wxString& label, Variant variant = Variant::Filled,
			Icon icon = Icon::Info, bool showIcon = false);

		void SetBackdrop(const wxColour& color);

	private:
		wxString label;
		Variant variant;
		Icon icon;
		bool showIcon;
		bool hovered = false;
		bool pressed = false;
		int height;
		int horizontalPadding;
		int iconSize;
		int iconGap;
		wxColour backdrop;

		void OnPaint(wxPaintEvent& event);
		void OnEnter(wxMouseEvent& event);
		void OnLeave(wxMouseEvent& event);
		void OnLeftDown(wxMouseEvent& event);
		void OnLeftUp(wxMouseEvent& event);
		void OnCaptureLost(wxMouseCaptureLostEvent& event);
		wxSize DoGetBestSize() const override;
		void Emit();
	};

	// Material 3 assist / status chip.
	class Chip : public wxControl
	{
	public:
		Chip(wxWindow* parent, const wxString& text, const wxColour& dotColor = wxNullColour,
			bool outlined = true);

		void SetLabelText(const wxString& text);
		void SetDotColor(const wxColour& color);
		void SetTonal(const wxColour& fillColour, const wxColour& textColour,
			const wxColour& dotColor = wxNullColour);

	private:
		wxString text;
		wxColour dot;
		bool outlined;
		wxColour fillColour;
		wxColour textColour;
		int height;
		int horizontalPadding;
		int dotSize;
		int dotGap;

		void OnPaint(wxPaintEvent& event);
		wxSize DoGetBestSize() const override;
	};

	// Material 3 menu item description used by MenuPopup.
	struct MenuItem
	{
		enum class Type
		{
			Item,
			Check,
			Radio,
			Separator,
			Header
		};

		Type type = Type::Item;
		int id = 0;
		wxString label;
		bool checked = false;
		bool enabled = true;

		static MenuItem Item(int id, const wxString& label, bool enabled = true);
		static MenuItem Check(int id, const wxString& label, bool checked, bool enabled = true);
		static MenuItem Radio(int id, const wxString& label, bool checked, bool enabled = true);
		static MenuItem Separator();
		static MenuItem Header(const wxString& label);
	};

	// Material 3 popup menu. Draws its own surface, rows, hover state and
	// check/radio indicators instead of using the native Win32 menu.
	class MenuPopup : public wxPopupTransientWindow
	{
	public:
		MenuPopup(wxWindow* parent, const std::vector<MenuItem>& items,
			std::function<void(int, bool)> onSelect);

		void PopupAt(const wxPoint& screenPosition);
		wxSize GetPopupSize() const { return wxSize(width, height); }
		void SetMinimumWidth(int minimumWidth);

	protected:
		void OnPaint(wxPaintEvent& event);
		void OnMouseMove(wxMouseEvent& event);
		void OnLeftUp(wxMouseEvent& event);
		void OnLeaveWindow(wxMouseEvent& event);
		void OnKeyDown(wxKeyEvent& event);
		void OnDismiss() override;

	private:
		std::vector<MenuItem> items;
		std::function<void(int, bool)> onSelect;
		std::vector<int> rowTops;
		int width = 220;
		int height = 0;
		int rowHeight = 40;
		int headerHeight = 32;
		int padding = 8;
		int minWidth = 220;
		int hovered = -1;
		bool dismissed = false;

		void Recompute();
		int HitTest(int y) const;
		void Activate(int index);
	};

	// Material 3 select / dropdown. Shows the current value and opens a
	// MenuPopup with radio rows for the available options.
	class Select : public wxControl
	{
	public:
		Select(wxWindow* parent, const wxString& placeholder = "Select");

		void Clear();
		void Append(const wxString& item);
		void SetSelection(int index);
		int GetSelection() const { return selection; }
		wxString GetStringSelection() const;
		unsigned int GetCount() const { return static_cast<unsigned int>(items.size()); }
		void SetPlaceholder(const wxString& text);

	private:
		wxArrayString items;
		int selection = -1;
		wxString placeholder;
		bool hovered = false;
		bool pressed = false;
		int fieldHeight = 40;
		int horizontalPadding = 14;
		int chevronSize = 18;

		void OnPaint(wxPaintEvent& event);
		void OnEnter(wxMouseEvent& event);
		void OnLeave(wxMouseEvent& event);
		void OnLeftDown(wxMouseEvent& event);
		void OnLeftUp(wxMouseEvent& event);
		void OnCaptureLost(wxMouseCaptureLostEvent& event);
		void OnKeyDown(wxKeyEvent& event);
		wxSize DoGetBestSize() const override;
		void OpenMenu();
		void EmitChanged();
	};

	// Material 3 settings row with a trailing switch, radio or chevron.
	class SettingsRow : public wxControl
	{
	public:
		enum class Mode
		{
			Check,
			Radio
		};

		SettingsRow(wxWindow* parent, const wxString& label,
			const wxString& subtitle = wxEmptyString,
			Mode mode = Mode::Check, bool checked = false);

		void SetChecked(bool value);
		bool GetChecked() const { return checked; }
		void SetSubtitle(const wxString& text);

	private:
		wxString label;
		wxString subtitle;
		Mode mode;
		bool checked;
		bool hovered = false;
		bool pressed = false;
		int rowHeight;

		void OnPaint(wxPaintEvent& event);
		void OnEnter(wxMouseEvent& event);
		void OnLeave(wxMouseEvent& event);
		void OnLeftDown(wxMouseEvent& event);
		void OnLeftUp(wxMouseEvent& event);
		void OnCaptureLost(wxMouseCaptureLostEvent& event);
		void OnKeyDown(wxKeyEvent& event);
		wxSize DoGetBestSize() const override;
		void Emit();
	};

	// Material 3 card used for one connected device.
	class DeviceCard : public wxControl
	{
	public:
		DeviceCard(wxWindow* parent, const wxString& name, const wxString& url,
			const wxString& backRes, const wxString& frontRes);

	private:
		wxString name;
		wxString url;
		wxString backRes;
		wxString frontRes;
		bool hovered = false;

		void OnPaint(wxPaintEvent& event);
		void OnEnter(wxMouseEvent& event);
		void OnLeave(wxMouseEvent& event);
		wxSize DoGetBestSize() const override;
	};

	// Centred icon + title + subtitle placeholder.
	class EmptyState : public wxControl
	{
	public:
		EmptyState(wxWindow* parent, Icon icon, const wxString& title,
			const wxString& subtitle);

	private:
		Icon icon;
		wxString title;
		wxString subtitle;

		void OnPaint(wxPaintEvent& event);
	};

	wxStaticText* Label(wxWindow* parent, const wxString& text, TypeRole role,
		const wxColour& color = wxNullColour);

	void StyleChoice(wxChoice* choice);
	void StyleSlider(wxSlider* slider);
	void StyleCheckBox(wxCheckBox* checkBox);
	void StyleRadioButton(wxRadioButton* radio);
	void StyleTextCtrl(wxTextCtrl* textCtrl);
	void StyleListView(wxListView* listView);
}
