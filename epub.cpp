// epub.cpp: contains epub creation functions
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <zip.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "epub_strings.h"
#include "epub.h"

std::string htmlbodyescape(std::string in) { // need to do this better
	std::string p = in;
	size_t h = 0;
	h = p.find("&", h);
	while (h != std::string::npos) {
		p.insert(h + 1, "amp;");
		h++;
		h = p.find("&", h);
	} h = 0;
	h = p.find("<", h);
	while (h != std::string::npos) {
		p.erase(h, 1);
		p.insert(h, "&lt;");
		h = p.find("<", h);
	} h = 0;
	h = p.find(">", h);
	while (h != std::string::npos) {
		p.erase(h, 1);
		p.insert(h, "&gt;");
		h = p.find(">", h);
	} // h = 0;
	return p;
}

zip_t* epubzip_f;

int epubcreate(const char* zipfilename) {
	int err = 0;
	epubzip_f = zip_open(zipfilename, ZIP_CREATE | ZIP_TRUNCATE, &err);
	if (epubzip_f == NULL) {
		zip_error_t ziperr;
		zip_get_error(epubzip_f);
		std::cout << "create zip fail " << zip_error_strerror(&ziperr) << '\n';
		return -1;
	} return 0;
}

int epubdir(const char* dirname) {
	if (epubzip_f == NULL) { std::cout << "zip file not open.\n"; return -1; }
	zip_int64_t err = zip_dir_add(epubzip_f, "META-INF", ZIP_FL_ENC_UTF_8);
	if (err == -1) {
		zip_error_t ziperr;
		zip_get_error(epubzip_f);
		std::cout << "create dir in zip fail " << zip_error_strerror(&ziperr) << '\n';
		return -1;
	}
	return 0;
}

int epubadd(const char* filename, const char* data, bool compression = true, int sz = 0) {
	if (epubzip_f == NULL) { std::cout << "zip file not open.\n"; return -1; }
	if (sz == 0) sz = strlen(data);
	zip_source_t* zf = zip_source_buffer(epubzip_f, data, sz, 0);
	zip_int64_t index = zip_file_add(epubzip_f, filename, zf, ZIP_FL_ENC_UTF_8);
	if (index == -1) {
		zip_error_t ziperr;
		zip_get_error(epubzip_f);
		std::cout << "add file to zip fail " << zip_error_strerror(&ziperr) << '\n';
		return -1;
	}
	if (!compression) zip_set_file_compression(epubzip_f, index, ZIP_CM_STORE, 0);
	return 0;
}

int epubwrite() {
	int err = zip_close(epubzip_f);
	if (err != 0) {
		zip_error_t* ziperr = zip_get_error(epubzip_f);
		std::cout << "write zip fail " << zip_error_strerror(ziperr) << '\n';
		return -1;
	} return 0;
}

int epubroll(std::string bookname_e, std::vector<chapterInfo> chapters, std::vector<unsigned char> cover_raw, epubmetadata epub_metadata) {
	// roll zip
	std::string zipfilename = bookname_e + ".epub";
	epubcreate(zipfilename.c_str());
	epubadd("mimetype", epubmimetype, false);
	epubdir("META-INF");
	epubadd("META-INF/container.xml", epubcontainerxml);
	epubadd("stylesheet.css", epubstylesheetcss);
	for (size_t i = 0; i < chapters.size(); i++) {
		epubadd(chapters[i].filename.c_str(), chapters[i].html.c_str());
	}
	epubadd("content.opf", epub_metadata.contentopf.c_str());
	epubadd("titlepage.xhtml", epub_metadata.titlepagexhtml.c_str());
	epubadd("toc.ncx", epub_metadata.tocncx.c_str());
	epubadd("cover.jpg", (char*)&cover_raw[0], true, cover_raw.size());

	epubwrite();
	return 0;
}

epubmetadata epubmeta(bookInfo book, std::vector<chapterInfo> chapters) {
	// this function use epub structure and metadata from wikipedia and calibre
	// content.opf
	std::string contentopf = epubcontentopf;
	// spine
	std::string itemRefs = "";
	for (size_t i = 0; i < chapters.size(); i++) {
		std::string navpoint = "\n    <itemref idref=\"ch" + chapters[i].id + "\"/>";
		itemRefs.append(navpoint);
	}
	contentopf.insert(973, itemRefs);
	// manifest
	std::string items = "";
	for (size_t i = 0; i < chapters.size(); i++) {
		std::string navpoint = "\n    <item id=\"ch" + chapters[i].id +"\" href=\"" + chapters[i].filename + "\" media-type=\"application/xhtml+xml\"/>";
		items.append(navpoint);
	}
	contentopf.insert(651, items);
	// date
	time_t curtime = time(NULL);
	char* timestr = new char[33];
	// std::strftime(timestr, 33, "%FT%TZ", std::gmtime(&curtime));
#if (defined(_MSC_VER) && _MSC_VER >= 1900)
	tm tmtime;
	gmtime_s(&tmtime, &curtime);
	std::strftime(timestr, 33, "%Y-%m-%d", &tmtime);
#else
	std::strftime(timestr, 33, "%Y-%m-%d", std::gmtime(&curtime));
#endif
	contentopf.insert(573, timestr);
	delete[] timestr;
	// bkp
	contentopf.insert(542, bkpstring);
	// uuid
	std::string uuidstr = "";
#ifdef _WIN32
	UUID uuid;
	UuidCreate(&uuid);
	wchar_t* uuidstr_ww = new wchar_t[39];
	StringFromGUID2((GUID)uuid, uuidstr_ww, 39);
	std::wstring uuidstr_w = uuidstr_ww;
	uuidstr = std::string(uuidstr_w.begin(), uuidstr_w.end());
	uuidstr.erase(0, 1); uuidstr.pop_back(); // remove { and }
#else
	#warning "no uuid gen, using c593ed25-1fa6-4180-9bb2-3379680b6913"
	uuidstr = "c593ed25-1fa6-4180-9bb2-3379680b6913";
#endif
	contentopf.insert(490, uuidstr);
	// creator
	contentopf.insert(426, book.author);
	contentopf.insert(409, book.author);
	// language
	contentopf.insert(365, book.language);
	// title
	contentopf.insert(336, book.name);

	// toc.ncx
	// navMap
	std::string tocncx = epubtocncx;
	std::string navLabels = "";
	for (size_t i = 0; i < chapters.size(); i++) {
		std::string navpoint = epubnavpoint;
		std::stringstream navpoint_index_ss;
		navpoint_index_ss << 2 + i; // start from playorder 2, 1 is cover
		std::string navpoint_index = navpoint_index_ss.str();
		navpoint.insert(111, chapters[i].filename);
		navpoint.insert(65, htmlbodyescape(chapters[i].name));
		navpoint.insert(31, navpoint_index);
		navpoint.insert(18, "ch" + navpoint_index);
		navLabels.append(navpoint);
	}
	tocncx.insert(594, navLabels);
	tocncx.insert(386, book.name);
	tocncx.insert(250, bkpstring);
	tocncx.insert(165, uuidstr);
	
	// titlepage.xhtml
	std::string titlepagexhtml = epubtitlepagexhtml;
	titlepagexhtml.insert(653, book.cv_height);
	titlepagexhtml.insert(643, book.cv_width);
	titlepagexhtml.insert(583, book.cv_height);
	titlepagexhtml.insert(582, book.cv_width);

	return epubmetadata{ contentopf, tocncx, titlepagexhtml };
}

std::string epubhtml(bookInfo book, chapterInfo chapter) {
	// this function use html from calibre
	// html
	std::string html = "<?xml version='1.0' encoding='utf-8'?>\n<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<head>\n<title></title>\n<link rel=\"stylesheet\" type=\"text/css\" href=\"stylesheet.css\"/>\n</head>\n<body>\n";
	html.insert(97, htmlbodyescape(book.name));
	// h1
	html.append("<h1>" + htmlbodyescape(chapter.name) + "</h1>\n");
	// p
	int br = 0;
	for (size_t i = 0; i < chapter.lines.size(); i++) {
		if (chapter.lines[i] != "") {
			if (br > 3) { // 4 newline = 1 pagebreak
				html.append("<p class=\"scenebreak\">" + htmlbodyescape(chapter.lines[i]) + "</p>\n");
				br = 0;
			}
			else { html.append("<p>" + htmlbodyescape(chapter.lines[i]) + "</p>\n"); br = 0; }
		}
		else br++;
	}
	html.append("\n</body>\n</html>\n");
	return html;
}
