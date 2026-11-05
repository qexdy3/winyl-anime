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
	explicit SkinEditor(const std::wstring& root) : root(root) {}
	bool changed = false;
private:
	std::wstring root;
	std::vector<std::wstring> images;

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
		int index = (int)::SendDlgItemMessageW(thisWnd, IDC_SKIN_IMAGES, LB_GETCURSEL, 0, 0);
		if (index < 0 || (size_t)index >= images.size()) return;
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
	}

	INT_PTR DialogProc(HWND, UINT msg, WPARAM wp, LPARAM) override
	{
		if (msg == WM_INITDIALOG) {
			Scan(L"");
			std::sort(images.begin(), images.end());
			for (const auto& image : images) ::SendDlgItemMessageW(thisWnd, IDC_SKIN_IMAGES, LB_ADDSTRING, 0, (LPARAM)image.c_str());
			::SendDlgItemMessageW(thisWnd, IDC_SKIN_TARGET, CB_ADDSTRING, 0, (LPARAM)L"Playlist");
			::SendDlgItemMessageW(thisWnd, IDC_SKIN_TARGET, CB_ADDSTRING, 0, (LPARAM)L"Library");
			const wchar_t* modes[] = {L"Crop", L"Stretch", L"Cover", L"Contain"};
			for (const auto mode : modes) ::SendDlgItemMessageW(thisWnd, IDC_SKIN_MODE, CB_ADDSTRING, 0, (LPARAM)mode);
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
			}
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
				SkinEditor editor(programPath + L"Skin\\" + skinNames[selectedSkin].first + L"\\");
				editor.ModalDialog(thisWnd, IDD_DLGSKINEDITOR);
				if (editor.changed && skinNames[selectedSkin].first == curSkinName && !curSkinPack)
					OnBnClickedOK();
			}
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
