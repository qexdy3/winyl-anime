/*  This file is part of Winyl Player source code.
    Copyright (C) 2008-2018, Alex Kras. <winylplayer@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "resource.h"
#include "DlgSkin.h"
#include "FileSystem.h"
#include "FileDialogEx.h"

// A deliberately small editor for unpacked skins. Originals are kept beside
// the modified files; editing never changes a packed skin.
class SkinEditor : public DialogEx
{
public:
	SkinEditor(const std::wstring& root, bool russian) : root(root), russian(russian) {}
	bool changed = false;
private:
	std::wstring root;
	bool russian;
	std::vector<std::wstring> images;
	std::vector<size_t> visibleImages;
	ExImage preview;

	// Categories are based on common skin folder names. Unusual skins are
	// still fully accessible through the "All images" category.
	int ImageCategory(const std::wstring& path)
	{
		std::wstring lower = path;
		for (auto& c : lower) if (c >= L'A' && c <= L'Z') c += L'a' - L'A';
		if (lower.find(L"button") != std::wstring::npos || lower.find(L"switch") != std::wstring::npos) return 2;
		if (lower.find(L"scroll") != std::wstring::npos || lower.find(L"trackbar") != std::wstring::npos || lower.find(L"volumebar") != std::wstring::npos || lower.find(L"slider") != std::wstring::npos) return 3;
		if (lower.find(L"cover") != std::wstring::npos) return 4;
		if (lower.find(L"icons") != std::wstring::npos || lower.find(L"rating") != std::wstring::npos || lower.find(L"expand") != std::wstring::npos) return 5;
		if (lower.find(L"background") != std::wstring::npos || lower.find(L"wallpaper") != std::wstring::npos || lower.find(L"back.") != std::wstring::npos) return 1;
		return 6;
	}

	int SelectedImage()
	{
		int row = (int)::SendDlgItemMessageW(thisWnd, IDC_SKIN_IMAGES, LB_GETCURSEL, 0, 0);
		if (row < 0 || (size_t)row >= visibleImages.size()) return -1;
		return (int)visibleImages[row];
	}

	void UpdatePreview()
	{
		int index = SelectedImage();
		preview.Clear();
		::EnableWindow(::GetDlgItem(thisWnd, IDC_SKIN_REPLACE), index >= 0);
		bool hasBackup = index >= 0 && FileSystem::Exists(root + images[index] + L".winyl-original");
		::EnableWindow(::GetDlgItem(thisWnd, IDC_SKIN_RESTORE), hasBackup);
		if (index >= 0)
		{
			::SetDlgItemTextW(thisWnd, IDC_SKIN_IMAGE_INFO, images[index].c_str());
			RECT rc = {};
			::GetClientRect(::GetDlgItem(thisWnd, IDC_SKIN_IMAGE_PREVIEW), &rc);
			preview.ThumbnailFromFile(root + images[index], nullptr, rc.right - rc.left, rc.bottom - rc.top);
		}
		else ::SetDlgItemTextW(thisWnd, IDC_SKIN_IMAGE_INFO, russian ? L"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0438\u0437\u043e\u0431\u0440\u0430\u0436\u0435\u043d\u0438\u0435 \u0434\u043b\u044f \u043f\u0440\u043e\u0441\u043c\u043e\u0442\u0440\u0430." : L"Select an image to preview it.");
		::InvalidateRect(::GetDlgItem(thisWnd, IDC_SKIN_IMAGE_PREVIEW), NULL, TRUE);
	}

	void ShowCategory()
	{
		int category = (int)::SendDlgItemMessageW(thisWnd, IDC_SKIN_CATEGORY, CB_GETCURSEL, 0, 0);
		::SendDlgItemMessageW(thisWnd, IDC_SKIN_IMAGES, LB_RESETCONTENT, 0, 0);
		visibleImages.clear();
		HWND list = ::GetDlgItem(thisWnd, IDC_SKIN_IMAGES);
		HDC dc = ::GetDC(list);
		HGDIOBJ oldFont = ::SelectObject(dc, (HFONT)::SendMessageW(list, WM_GETFONT, 0, 0));
		int extent = 0;
		for (size_t i = 0; i < images.size(); ++i)
		{
			if (category != 0 && ImageCategory(images[i]) != category) continue;
			visibleImages.push_back(i);
			::SendDlgItemMessageW(thisWnd, IDC_SKIN_IMAGES, LB_ADDSTRING, 0, (LPARAM)images[i].c_str());
			SIZE size = {};
			::GetTextExtentPoint32W(dc, images[i].c_str(), (int)images[i].size(), &size);
			extent = std::max(extent, (int)size.cx + 8);
		}
		::SelectObject(dc, oldFont);
		::ReleaseDC(list, dc);
		::SendDlgItemMessageW(thisWnd, IDC_SKIN_IMAGES, LB_SETHORIZONTALEXTENT, extent, 0);
		UpdatePreview();
	}

	void Scan(const std::wstring& relative)
	{
		WIN32_FIND_DATAW fd = {};
		HANDLE h = ::FindFirstFileW((root + relative + L"*").c_str(), &fd);
		if (h == INVALID_HANDLE_VALUE) return;
		do {
			std::wstring name = fd.cFileName;
			if (name == L"." || name == L".." || (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) Scan(relative + name + L"\\");
			else {
				std::wstring ext = PathEx::ExtFromFile(name);
				for (auto& c : ext) if (c >= L'A' && c <= L'Z') c += L'a' - L'A';
				if (ext == L"png" || ext == L"bmp" || ext == L"jpg" || ext == L"jpeg") images.push_back(relative + name);
			}
		} while (::FindNextFileW(h, &fd));
		::FindClose(h);
	}

	bool Backup(const std::wstring& path)
	{
		std::wstring backup = path + L".winyl-original";
		if (::CopyFileW(path.c_str(), backup.c_str(), TRUE)) return true;
		return ::GetLastError() == ERROR_FILE_EXISTS && !(::GetFileAttributesW(backup.c_str()) & FILE_ATTRIBUTE_DIRECTORY);
	}

	std::wstring BackgroundFile()
	{
		int target = (int)::SendDlgItemMessageW(thisWnd, IDC_SKIN_TARGET, CB_GETCURSEL, 0, 0);
		return root + (target == 1 ? L"Library\\Library.xml" : L"Playlist\\Playlist.xml");
	}

	void LoadBackground()
	{
		pugi::xml_document doc;
		if (!doc.load_file(BackgroundFile().c_str(), pugi::parse_full | pugi::parse_ws_pcdata, pugi::encoding_utf8)) {
			::SetDlgItemTextW(thisWnd, IDC_SKIN_COLOR, L"");
			return;
		}
		int target = (int)::SendDlgItemMessageW(thisWnd, IDC_SKIN_TARGET, CB_GETCURSEL, 0, 0);
		auto bg = doc.child(target == 1 ? "Library" : "Playlist").child("Background");
		int mode = bg.attribute("Contain").as_bool() ? 3 : bg.attribute("Cover").as_bool() ? 2 : bg.attribute("Stretch").as_bool() ? 1 : 0;
		::SendDlgItemMessageW(thisWnd, IDC_SKIN_MODE, CB_SETCURSEL, mode, 0);
		std::wstring color = UTF::UTF16(bg.attribute("Color").as_string("000000"));
		::SetDlgItemTextW(thisWnd, IDC_SKIN_COLOR, color.c_str());
	}

	void SaveBackground()
	{
		wchar_t color[16] = {};
		::GetDlgItemTextW(thisWnd, IDC_SKIN_COLOR, color, 16);
		if (wcslen(color) != 6 || wcsspn(color, L"0123456789abcdefABCDEF") != 6) {
			::MessageBoxW(thisWnd, L"Enter a six-digit RGB color, for example 100b10.", L"Skin editor", MB_ICONWARNING); return;
		}
		std::wstring file = BackgroundFile();
		pugi::xml_document doc;
		if (!doc.load_file(file.c_str(), pugi::parse_full | pugi::parse_ws_pcdata, pugi::encoding_utf8)) {
			::MessageBoxW(thisWnd, L"Could not read the background XML file.", L"Skin editor", MB_ICONERROR); return;
		}
		int target = (int)::SendDlgItemMessageW(thisWnd, IDC_SKIN_TARGET, CB_GETCURSEL, 0, 0);
		auto bg = doc.child(target == 1 ? "Library" : "Playlist").child("Background");
		if (!bg) { ::MessageBoxW(thisWnd, L"This skin has no background element for this view.", L"Skin editor", MB_ICONWARNING); return; }
		auto attr = bg.attribute("Color");
		if (!attr) attr = bg.append_attribute("Color");
		attr.set_value(UTF::UTF8(color).c_str());
		bg.remove_attribute("Stretch"); bg.remove_attribute("Cover"); bg.remove_attribute("Contain");
		int mode = (int)::SendDlgItemMessageW(thisWnd, IDC_SKIN_MODE, CB_GETCURSEL, 0, 0);
		if (mode > 0 && mode < 4) bg.append_attribute(mode == 1 ? "Stretch" : mode == 2 ? "Cover" : "Contain").set_value("1");
		wchar_t temp[MAX_PATH] = {};
		std::wstring dir = PathEx::PathFromFile(file);
		if (!Backup(file) || !::GetTempFileNameW(dir.c_str(), L"wny", 0, temp)) {
			::MessageBoxW(thisWnd, L"Could not create backup or temporary file. Check folder permissions.", L"Skin editor", MB_ICONERROR); return;
		}
		bool ok = doc.save_file(temp, "", pugi::format_raw, pugi::encoding_utf8) && ::MoveFileExW(temp, file.c_str(), MOVEFILE_REPLACE_EXISTING);
		if (!ok) { ::DeleteFileW(temp); ::MessageBoxW(thisWnd, L"Could not save background settings.", L"Skin editor", MB_ICONERROR); return; }
		changed = true;
	}

	void ImageAction(bool restore)
	{
		int index = SelectedImage();
		if (index < 0) return;
		std::wstring dest = root + images[index], source;
		if (restore) source = dest + L".winyl-original";
		else {
			FileDialogEx dlg;
			FileDialogEx::FILE_TYPES types[] = {{L"Images", L"*.png;*.bmp;*.jpg;*.jpeg"}};
			dlg.SetFileTypes(types, 1);
			if (!dlg.DoModalFile(thisWnd)) return;
			source = dlg.GetFile();
			ExImage test;
			if (!test.LoadFile(source)) { ::MessageBoxW(thisWnd, L"The selected file is not a readable image.", L"Skin editor", MB_ICONERROR); return; }
			if (!Backup(dest)) { ::MessageBoxW(thisWnd, L"Could not back up the original image.", L"Skin editor", MB_ICONERROR); return; }
		}
		if (!::CopyFileW(source.c_str(), dest.c_str(), FALSE)) { ::MessageBoxW(thisWnd, L"Could not replace image or no original backup exists.", L"Skin editor", MB_ICONERROR); return; }
		changed = true;
		UpdatePreview();
	}

	INT_PTR DialogProc(HWND, UINT msg, WPARAM wp, LPARAM lp) override
	{
		if (msg == WM_INITDIALOG) {
			if (russian) {
				::SetWindowTextW(thisWnd, L"\u0420\u0435\u0434\u0430\u043a\u0442\u043e\u0440 \u0441\u043a\u0438\u043d\u0430");
				::SetDlgItemTextW(thisWnd, IDC_SKIN_CATEGORY_LABEL, L"1. \u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u043a\u0430\u0442\u0435\u0433\u043e\u0440\u0438\u044e:");
				::SetDlgItemTextW(thisWnd, IDC_SKIN_IMAGE_LABEL, L"2. \u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0438\u0437\u043e\u0431\u0440\u0430\u0436\u0435\u043d\u0438\u0435:");
				::SetDlgItemTextW(thisWnd, IDC_SKIN_PREVIEW_GROUP, L"\u041f\u0440\u0435\u0434\u043f\u0440\u043e\u0441\u043c\u043e\u0442\u0440");
				::SetDlgItemTextW(thisWnd, IDC_SKIN_REPLACE, L"\u0417\u0430\u043c\u0435\u043d\u0438\u0442\u044c...");
				::SetDlgItemTextW(thisWnd, IDC_SKIN_RESTORE, L"\u0412\u043e\u0441\u0441\u0442\u0430\u043d\u043e\u0432\u0438\u0442\u044c");
				::SetDlgItemTextW(thisWnd, IDC_SKIN_BACKUP_LABEL, L"\u0418\u0437\u043c\u0435\u043d\u0435\u043d\u0438\u044f \u0441\u043e\u0445\u0440\u0430\u043d\u044f\u044e\u0442\u0441\u044f \u0441\u0440\u0430\u0437\u0443. \u0421\u043e\u0437\u0434\u0430\u044e\u0442\u0441\u044f \u0440\u0435\u0437\u0435\u0440\u0432\u043d\u044b\u0435 \u043a\u043e\u043f\u0438\u0438.");
				::SetDlgItemTextW(thisWnd, IDC_SKIN_BACKGROUND_GROUP, L"\u041d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0438 \u0444\u043e\u043d\u0430 \u043f\u043b\u0435\u0439\u043b\u0438\u0441\u0442\u0430 / \u0431\u0438\u0431\u043b\u0438\u043e\u0442\u0435\u043a\u0438");
				::SetDlgItemTextW(thisWnd, IDC_SKIN_COLOR_LABEL, L"\u0426\u0432\u0435\u0442 (RRGGBB):");
				::SetDlgItemTextW(thisWnd, IDC_SKIN_BACKGROUND_SAVE, L"\u0421\u043e\u0445\u0440\u0430\u043d\u0438\u0442\u044c");
				::SetDlgItemTextW(thisWnd, IDCANCEL, L"\u0417\u0430\u043a\u0440\u044b\u0442\u044c");
			}
			Scan(L"");
			std::sort(images.begin(), images.end());
			const wchar_t* categories[] = {L"All images", L"Backgrounds", L"Buttons and switches", L"Sliders and scrollbars", L"Covers", L"Icons and ratings", L"Other"};
			const wchar_t* categoriesRu[] = {L"\u0412\u0441\u0435 \u0438\u0437\u043e\u0431\u0440\u0430\u0436\u0435\u043d\u0438\u044f", L"\u0424\u043e\u043d\u044b", L"\u041a\u043d\u043e\u043f\u043a\u0438 \u0438 \u043f\u0435\u0440\u0435\u043a\u043b\u044e\u0447\u0430\u0442\u0435\u043b\u0438", L"\u041f\u043e\u043b\u0437\u0443\u043d\u043a\u0438 \u0438 \u043f\u0440\u043e\u043a\u0440\u0443\u0442\u043a\u0430", L"\u041e\u0431\u043b\u043e\u0436\u043a\u0438", L"\u0417\u043d\u0430\u0447\u043a\u0438 \u0438 \u043e\u0446\u0435\u043d\u043a\u0438", L"\u0414\u0440\u0443\u0433\u043e\u0435"};
			for (size_t i = 0; i < 7; ++i) ::SendDlgItemMessageW(thisWnd, IDC_SKIN_CATEGORY, CB_ADDSTRING, 0, (LPARAM)(russian ? categoriesRu[i] : categories[i]));
			::SendDlgItemMessageW(thisWnd, IDC_SKIN_CATEGORY, CB_SETCURSEL, 1, 0);
			ShowCategory();
			::SendDlgItemMessageW(thisWnd, IDC_SKIN_TARGET, CB_ADDSTRING, 0, (LPARAM)(russian ? L"\u041f\u043b\u0435\u0439\u043b\u0438\u0441\u0442" : L"Playlist"));
			::SendDlgItemMessageW(thisWnd, IDC_SKIN_TARGET, CB_ADDSTRING, 0, (LPARAM)(russian ? L"\u0411\u0438\u0431\u043b\u0438\u043e\u0442\u0435\u043a\u0430" : L"Library"));
			const wchar_t* modes[] = {L"Crop", L"Stretch", L"Cover", L"Contain"};
			const wchar_t* modesRu[] = {L"\u041e\u0431\u0440\u0435\u0437\u043a\u0430", L"\u0420\u0430\u0441\u0442\u044f\u043d\u0443\u0442\u044c", L"\u0417\u0430\u043f\u043e\u043b\u043d\u0438\u0442\u044c", L"\u0412\u043f\u0438\u0441\u0430\u0442\u044c"};
			for (size_t i = 0; i < 4; ++i) ::SendDlgItemMessageW(thisWnd, IDC_SKIN_MODE, CB_ADDSTRING, 0, (LPARAM)(russian ? modesRu[i] : modes[i]));
			::SendDlgItemMessageW(thisWnd, IDC_SKIN_TARGET, CB_SETCURSEL, 0, 0);
			LoadBackground(); return TRUE;
		}
		if (msg == WM_COMMAND) {
			switch (LOWORD(wp)) {
			case IDCANCEL: EndDialog(IDCANCEL); return TRUE;
			case IDC_SKIN_REPLACE: ImageAction(false); return TRUE;
			case IDC_SKIN_RESTORE: ImageAction(true); return TRUE;
			case IDC_SKIN_BACKGROUND_SAVE: SaveBackground(); return TRUE;
			case IDC_SKIN_TARGET: if (HIWORD(wp) == CBN_SELCHANGE) LoadBackground(); return TRUE;
			case IDC_SKIN_CATEGORY: if (HIWORD(wp) == CBN_SELCHANGE) ShowCategory(); return TRUE;
			case IDC_SKIN_IMAGES: if (HIWORD(wp) == LBN_SELCHANGE) UpdatePreview(); return TRUE;
			}
		}
		if (msg == WM_DRAWITEM && wp == IDC_SKIN_IMAGE_PREVIEW) {
			DRAWITEMSTRUCT* item = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
			::FillRect(item->hDC, &item->rcItem, (HBRUSH)(COLOR_WINDOW + 1));
			if (preview.IsValid()) {
				int x = (item->rcItem.right - item->rcItem.left - preview.Width()) / 2;
				int y = (item->rcItem.bottom - item->rcItem.top - preview.Height()) / 2;
				preview.Draw(item->hDC, x, y);
			}
			return TRUE;
		}
		return FALSE;
	}
};

DlgSkin::DlgSkin()
{

}

DlgSkin::~DlgSkin()
{
	if (imListSkin)
		::ImageList_Destroy(imListSkin);
}

INT_PTR DlgSkin::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		OnInitDialog();
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			OnBnClickedOK();
			return TRUE;
		case IDCANCEL:
			OnBnClickedCancel();
			EndDialog(LOWORD(wParam));
			return TRUE;
		case IDC_SKIN_EDIT:
			if (selectedSkin < 0) return TRUE;
			if (skinNames[selectedSkin].second) {
				::MessageBoxW(thisWnd, L"Extract the .wzp skin into a folder in Skin first, then reopen this dialog.", L"Skin editor", MB_ICONINFORMATION);
				return TRUE;
			}
			{
				SkinEditor editor(programPath + L"Skin\\" + skinNames[selectedSkin].first + L"\\", lang->GetLanguage() == L"Russian");
				editor.ModalDialog(thisWnd, IDD_DLGSKINEDITOR);
				if (editor.changed && skinNames[selectedSkin].first == curSkinName && !curSkinPack)
					OnBnClickedOK();
			}
			return TRUE;
		case IDC_SKIN_INSTALL_FILE:
			InstallSkin(false);
			return TRUE;
		case IDC_SKIN_INSTALL_FOLDER:
			InstallSkin(true);
			return TRUE;
		}
		return TRUE;
	case WM_DRAWITEM:
		OnDrawItem((int)wParam, (LPDRAWITEMSTRUCT)lParam);
		return TRUE;
	case WM_NOTIFY:
		OnNotify(wParam, lParam);
		return TRUE;
	}

	return FALSE;
}

void DlgSkin::OnInitDialog()
{
	::SetWindowText(thisWnd, lang->GetLine(Lang::SkinDialog, 0));
	::SetDlgItemText(thisWnd, IDCANCEL, lang->GetLine(Lang::SkinDialog, 1));
	::SetDlgItemText(thisWnd, IDOK, lang->GetLine(Lang::SkinDialog, 2)); 
	::SetDlgItemText(thisWnd, IDC_STATIC_SELECTSKIN, lang->GetLine(Lang::SkinDialog, 3));
	::SetDlgItemText(thisWnd, IDC_STATIC_PREVIEW2, lang->GetLine(Lang::SkinDialog, 4));
	::SetDlgItemText(thisWnd, IDC_STATIC_INFO, lang->GetLine(Lang::SkinDialog, 5));
	::SetDlgItemText(thisWnd, IDC_STATIC_AUTHOR, lang->GetLine(Lang::SkinDialog, 6));
	::SetDlgItemText(thisWnd, IDC_STATIC_VERSION, lang->GetLine(Lang::SkinDialog, 7));
	::SetDlgItemText(thisWnd, IDC_STATIC_DESC, lang->GetLine(Lang::SkinDialog, 8));
	if (lang->GetLanguage() == L"Russian") {
		::SetDlgItemTextW(thisWnd, IDC_SKIN_EDIT, L"\u0418\u0437\u043c\u0435\u043d\u0438\u0442\u044c...");
		::SetDlgItemTextW(thisWnd, IDC_SKIN_INSTALL_FILE, L"\u0423\u0441\u0442\u0430\u043d\u043e\u0432\u0438\u0442\u044c...");
		::SetDlgItemTextW(thisWnd, IDC_SKIN_INSTALL_FOLDER, L"\u0418\u0437 \u043f\u0430\u043f\u043a\u0438...");
	}

	listSkin = ::GetDlgItem(thisWnd, IDC_LIST_SKIN);

	// Assign styles and the theme to ListView (Vista/7 selection style like in explorer)
	ListView_SetExtendedListViewStyleEx(listSkin, 0, LVS_EX_FULLROWSELECT|LVS_EX_INFOTIP|LVS_EX_DOUBLEBUFFER);
	::SetWindowTheme(listSkin, L"explorer", NULL);

	// Load icon
	imListSkin = ::ImageList_Create(16, 16, ILC_COLOR32, 0, 0);
	if (imListSkin)
	{
		HICON icon = (HICON)::LoadImageW(::GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_ICON_SKIN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

		::ImageList_AddIcon(imListSkin, icon);
		::DestroyIcon(icon);
	
		ListView_SetImageList(listSkin, imListSkin, LVSIL_SMALL);
	}

	CRect rcList;
	::GetClientRect(listSkin, rcList);
	
	LVCOLUMN column = {};
	column.mask = LVCF_WIDTH;
	column.cx = rcList.Width() - ::GetSystemMetrics(SM_CXVSCROLL);
	ListView_InsertColumn(listSkin, 0, &column);

	// Search for subfolders in Skin folder
	std::wstring path = programPath;
	path += L"Skin";
	path.push_back('\\');

	FileSystem::Find find(path);

	while (find.Next())
	{
		if (find.IsDirectory())
		{
			skinNames.push_back(std::make_pair(find.GetFileName(), 0));
		}
		else
		{
			std::wstring ext = PathEx::ExtFromFile(find.GetFileName());
			if (ext == L"wzp")
			{
				std::wstring skinName = PathEx::NameFromFile(find.GetFileName());
				skinNames.push_back(std::make_pair(skinName, 1));
			}
		}
	}

	SetWindowRedraw(listSkin, FALSE);

	for (std::size_t i = 0, size = skinNames.size(); i < size; ++i)
	{
		if (curSkinName == skinNames[i].first &&
			curSkinPack == !!skinNames[i].second)
			selectedSkin = (int)i;

		LVITEM item = {};
		item.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		item.iItem = 0;
		item.iSubItem = 0;
		item.iImage = 0;
		item.lParam = i;

		std::wstring skinName = skinNames[i].first;
		if (skinNames[i].second == 0)
			skinName.push_back('*');

		item.pszText = const_cast<wchar_t*>(skinName.c_str());

		ListView_InsertItem(listSkin, &item);
	}

	SetWindowRedraw(listSkin, TRUE);

	// Select the current skin

	LVFINDINFO fi = {};
	fi.flags = LVFI_PARAM;
	fi.lParam = (LPARAM)selectedSkin;

	int current = ListView_FindItem(listSkin, -1, &fi);
	if (current > -1)
	{
		LVITEM item = {};
		item.iItem = current;
		item.mask = LVIF_PARAM;
		ListView_GetItem(listSkin, &item);

		selectedSkin = (int)item.lParam;

		ListView_SetSelectionMark(listSkin, current);
		ListView_SetItemState(listSkin, current, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

		ListView_EnsureVisible(listSkin, current, FALSE);
	}
}

// Copy only into a newly created theme directory; never follow links or
// overwrite an existing skin. A failed copy is left for the user to inspect.
static bool CopySkinFolder(const std::wstring& source, const std::wstring& target, unsigned depth)
{
	if (depth > 32) return false;
	WIN32_FIND_DATAW fd = {};
	HANDLE find = ::FindFirstFileW((source + L"\\*").c_str(), &fd);
	if (find == INVALID_HANDLE_VALUE) return false;
	bool ok = true;
	do {
		std::wstring name = fd.cFileName;
		if (name == L"." || name == L"..") continue;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) { ok = false; break; }
		std::wstring from = source + L"\\" + name, to = target + L"\\" + name;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (!::CreateDirectoryW(to.c_str(), NULL) || !CopySkinFolder(from, to, depth + 1)) { ok = false; break; }
		} else if (!::CopyFileW(from.c_str(), to.c_str(), TRUE)) { ok = false; break; }
	} while (::FindNextFileW(find, &fd));
	if (ok && ::GetLastError() != ERROR_NO_MORE_FILES) ok = false;
	::FindClose(find);
	return ok;
}

void DlgSkin::InstallSkin(bool folder)
{
	FileDialogEx dialog;
	if (folder) {
		dialog.SetFolderTitleXP(L"Select the unpacked skin folder");
		if (!dialog.DoModalFolder(thisWnd)) return;
	} else {
		FileDialogEx::FILE_TYPES types[] = {{L"Winyl skin package (*.wzp)", L"*.wzp"}};
		dialog.SetFileTypes(types, 1);
		if (!dialog.DoModalFile(thisWnd)) return;
	}
	std::wstring source = dialog.GetFile();
	while (source.size() > 3 && (source.back() == L'\\' || source.back() == L'/')) source.pop_back();
	std::wstring name = PathEx::FileFromPath(source);
	if (folder) {
		DWORD attr = ::GetFileAttributesW(source.c_str());
		if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY) || (attr & FILE_ATTRIBUTE_REPARSE_POINT) || !FileSystem::Exists(source + L"\\Skin.xml") || !FileSystem::Exists(source + L"\\SkinInfo.xml")) {
			::MessageBoxW(thisWnd, L"Select a skin folder containing Skin.xml and SkinInfo.xml (not a ZIP file).", L"Install skin", MB_ICONWARNING); return;
		}
	} else {
		std::wstring ext = PathEx::ExtFromFile(name);
		for (auto& c : ext) if (c >= L'A' && c <= L'Z') c += L'a' - L'A';
		if (ext != L"wzp") { ::MessageBoxW(thisWnd, L"Select a .wzp file. Extract ZIP files and use Add folder instead.", L"Install skin", MB_ICONWARNING); return; }
		name = PathEx::NameFromFile(name);
		ZipFile zip;
		XmlFile info, window;
		if (!zip.OpenFile(source) || !info.LoadEx(L"SkinInfo.xml", &zip) || !window.LoadEx(L"Skin.xml", &zip)) {
			::MessageBoxW(thisWnd, L"This is not a supported Winyl skin package.", L"Install skin", MB_ICONWARNING); return;
		}
	}
	if (name.empty() || name == L"." || name == L"..") return;
	std::wstring target = programPath + L"Skin\\" + name + (folder ? L"" : L".wzp");
	// Do not replace either form of an existing skin with the same name.
	if (FileSystem::Exists(programPath + L"Skin\\" + name) || FileSystem::Exists(programPath + L"Skin\\" + name + L".wzp")) {
		::MessageBoxW(thisWnd, L"A skin with this name already exists. The existing skin was not changed.", L"Install skin", MB_ICONWARNING); return;
	}
	bool ok = folder ? (::CreateDirectoryW(target.c_str(), NULL) && CopySkinFolder(source, target, 0)) : !!::CopyFileW(source.c_str(), target.c_str(), TRUE);
	if (!ok) {
		::MessageBoxW(thisWnd, L"Could not install the skin. Check permissions and free space. A partially copied folder may remain in Skin.", L"Install skin", MB_ICONERROR); return;
	}
	int skinIndex = (int)skinNames.size();
	skinNames.push_back(std::make_pair(name, folder ? 0 : 1));
	std::wstring label = name + (folder ? L"*" : L"");
	LVITEM item = {};
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	item.iItem = 0; item.iImage = 0; item.lParam = skinIndex;
	item.pszText = const_cast<wchar_t*>(label.c_str());
	int row = ListView_InsertItem(listSkin, &item);
	if (row >= 0) {
		ListView_SetItemState(listSkin, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(listSkin, row, FALSE);
	}
	::MessageBoxW(thisWnd, L"Skin installed. Select Apply to use it.", L"Install skin", MB_ICONINFORMATION);
}

void DlgSkin::OnBnClickedCancel()
{

}

void DlgSkin::OnBnClickedOK()
{
	if (skinEngine != L"1.6")
		MessageBox::Show(thisWnd, lang->GetLine(Lang::Message, 2), lang->GetLine(Lang::Message, 5), MessageBox::Icon::Information);
	else
	{
		if (selectedSkin == -1)
			return;

		WindowEx::BeginWaitCursor();

		LRESULT result = ::SendMessage(thisParentWnd, UWM_CHANGESKIN, (WPARAM)skinNames[selectedSkin].second, (LPARAM)skinNames[selectedSkin].first.c_str());
		
		WindowEx::EndWaitCursor();
		::BringWindowToTop(thisWnd);

		if (!result)
			MessageBox::Show(thisWnd, lang->GetLine(Lang::Message, 1), lang->GetLine(Lang::Message, 6), MessageBox::Icon::Warning);
		else
		{
			curSkinName = skinNames[selectedSkin].first;
			curSkinPack = !!skinNames[selectedSkin].second;
		}
	}
}

void DlgSkin::OnNotify(WPARAM wParam, LPARAM lParam)
{
	NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);

	if (nmhdr->code == LVN_ITEMCHANGED)
	{
		NMLISTVIEW* nmlv = reinterpret_cast<NMLISTVIEW*>(lParam);

		if (!(nmlv->uNewState & LVIS_FOCUSED))
			return;

		selectedSkin = (int)nmlv->lParam;

		if (skinNames[selectedSkin].second == 0)
			LoadSkinInfo(skinNames[selectedSkin].first, NULL);
		else
		{
			ZipFile zipFile;
			LoadSkinInfo(skinNames[selectedSkin].first, &zipFile);
		}
	}
	else if (nmhdr->code == NM_DBLCLK)
	{
		NMITEMACTIVATE* nmia = reinterpret_cast<NMITEMACTIVATE*>(lParam);

		if (nmia->iItem != -1)
		{
			OnBnClickedOK();
			EndDialog(IDOK);
		}
	}
}

void DlgSkin::LoadSkinInfo(const std::wstring& folder, ZipFile* zipFile)
{
	std::wstring path = programPath;
	path += L"Skin";
	path.push_back('\\');
	path += folder;

	std::wstring file;
	if (zipFile == NULL)
	{
		path.push_back('\\');
		file = path + L"SkinInfo.xml";
	}
	else
	{
		zipFile->OpenFile(path + L".wzp");
		file = L"SkinInfo.xml";
	}

	std::wstring author;
	std::wstring version;
	std::wstring info;
	std::wstring preview;

	XmlFile xmlFile;

	if (xmlFile.LoadEx(file, zipFile))
	{	
		XmlNode xmlMain = xmlFile.RootNode().FirstChild("SkinInfo");

		if (xmlMain)
		{
			skinEngine = xmlMain.Attribute16("SkinEngine");

			XmlNode xmlAuthor = xmlMain.FirstChild("Author");
			if (xmlAuthor)
				author = xmlAuthor.Attribute16("Line");

			XmlNode xmlVersion = xmlMain.FirstChild("Version");
			if (xmlVersion)
				version = xmlVersion.Attribute16("Line");

			XmlNode xmlInfo = xmlMain.FirstChild("Info");
			if (xmlInfo)
				info = xmlInfo.Attribute16("Line");

			XmlNode xmlPreview = xmlMain.FirstChild("Preview");
			if (xmlPreview)
				preview = xmlPreview.Attribute16("File");
		}
	}

	::SetDlgItemText(thisWnd, IDC_TEXT_AUTHOR, author.c_str());
	::SetDlgItemText(thisWnd, IDC_TEXT_VERSION, version.c_str());
	::SetDlgItemText(thisWnd, IDC_TEXT_INFO, info.c_str());

	if (zipFile == NULL)
		LoadPreview(path + preview, NULL);
	else
		LoadPreview(preview, zipFile);
}

void DlgSkin::LoadPreview(const std::wstring& file, ZipFile* zipFile)
{
	HWND wndPreview = ::GetDlgItem(thisWnd, IDC_STATIC_PREVIEW);

	CRect rc;
	::GetClientRect(wndPreview, rc);

	imPreview.ThumbnailFromFile(file, zipFile, rc.Width(), rc.Height());

	if (imPreview.IsValid())
	{
		if (!::IsWindowVisible(wndPreview))
			::ShowWindow(wndPreview, SW_SHOW);
		else
			::InvalidateRect(wndPreview, NULL, TRUE);
	}
	else
	{
		if (::IsWindowVisible(wndPreview))
			::ShowWindow(wndPreview, SW_HIDE);
	}
}

void DlgSkin::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nIDCtl == IDC_STATIC_PREVIEW)
	{
		imPreview.Draw(lpDrawItemStruct->hDC, 0, 0);
	}
}
