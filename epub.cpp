// epub.cpp: contains epub creation functions
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <zip.h>

#ifdef _WIN32
#include <direct.h>
#include <Windows.h>
#endif

#include "epub_strings.h"
#include "epub.h"

int epubroll(std::string bookname_e, std::vector<chapterInfo> chapters) {
	// roll zip
	int err = 0;
	std::string zipfilename = bookname_e + ".epub";
	zip_t* epubzip_f = zip_open(zipfilename.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
	if (epubzip_f == NULL) {
		zip_error_t ziperr;
		zip_error_init_with_code(&ziperr, err);
		std::cout << "create zip fail " << zip_error_strerror(&ziperr) << '\n';
		return -1;
	}

	zip_source_t* mimetype_zf = zip_source_file(epubzip_f, "mimetype", 0, -1);
	zip_file_add(epubzip_f, "mimetype", mimetype_zf, ZIP_FL_ENC_UTF_8);
	zip_set_file_compression(epubzip_f, 0, ZIP_CM_STORE, 0);
	zip_dir_add(epubzip_f, "META-INF", ZIP_FL_ENC_UTF_8);
#ifdef _WIN32
	zip_source_t* containerxml_zf = zip_source_file(epubzip_f, "META-INF\\container.xml", 0, -1);
#else
	zip_source_t* containerxml_zf = zip_source_file(epubzip_f, "META-INF/container.xml", 0, -1);
#endif
	zip_file_add(epubzip_f, "META-INF/container.xml", containerxml_zf, ZIP_FL_ENC_UTF_8);

	// zip_source_t* indexhtml_zf = zip_source_file(epubzip_f, "index.html", 0, -1);
	// zip_file_add(epubzip_f, "index.html", indexhtml_zf, ZIP_FL_ENC_UTF_8);
	for (size_t i = 0; i < chapters.size(); i++) {
		zip_source_t* html_zf = zip_source_file(epubzip_f, chapters[i].filename.c_str(), 0, -1);
		zip_file_add(epubzip_f, chapters[i].filename.c_str(), html_zf, ZIP_FL_ENC_UTF_8);
	}

	zip_source_t* contentopf_zf = zip_source_file(epubzip_f, "content.opf", 0, -1);
	zip_file_add(epubzip_f, "content.opf", contentopf_zf, ZIP_FL_ENC_UTF_8);
	zip_source_t* titlepagexhtml_zf = zip_source_file(epubzip_f, "titlepage.xhtml", 0, -1);
	zip_file_add(epubzip_f, "titlepage.xhtml", titlepagexhtml_zf, ZIP_FL_ENC_UTF_8);
	zip_source_t* tocncx_zf = zip_source_file(epubzip_f, "toc.ncx", 0, -1);
	zip_file_add(epubzip_f, "toc.ncx", tocncx_zf, ZIP_FL_ENC_UTF_8);
	zip_source_t* stylesheetcss_zf = zip_source_file(epubzip_f, "stylesheet.css", 0, -1);
	zip_file_add(epubzip_f, "stylesheet.css", stylesheetcss_zf, ZIP_FL_ENC_UTF_8);
	zip_source_t* coverjpg_zf = zip_source_file(epubzip_f, "cover.jpg", 0, -1);
	zip_file_add(epubzip_f, "cover.jpg", coverjpg_zf, ZIP_FL_ENC_UTF_8);

	err = zip_close(epubzip_f);
	if (err != 0) {
		zip_error_t* ziperr = zip_get_error(epubzip_f);
		std::cout << "write zip fail " << zip_error_strerror(ziperr) << '\n';
		return -1;
	}
	return 0;
}

int epubmeta(bookInfo book, std::vector<chapterInfo> chapters) {
	// this function use epub structure and metadata from wikipedia and calibre
	// mimetype
	std::ofstream mimetype("mimetype");
	mimetype << "application/epub+zip";
	mimetype.close();

	// META-INF/container.xml
#ifdef _WIN32
	_mkdir("META-INF");
	std::ofstream containerxml("META-INF\\container.xml");
#else 
	mkdir("META-INF", 0755);
	std::ofstream containerxml("META-INF/container.xml");
#endif
	containerxml << epubcontainerxml;
	containerxml.close();

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
	// write out
	std::ofstream contentopf_f("content.opf");
	contentopf_f << contentopf;
	contentopf_f.close();

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
		navpoint.insert(65, chapters[i].name);
		navpoint.insert(31, navpoint_index);
		navpoint.insert(18, "ch" + navpoint_index);
		navLabels.append(navpoint);
	}
	tocncx.insert(594, navLabels);
	tocncx.insert(386, book.name);
	tocncx.insert(250, bkpstring);
	tocncx.insert(165, uuidstr);
	std::ofstream tocncx_f("toc.ncx");
	tocncx_f << tocncx;
	tocncx_f.close();
	
	// titlepage.xhtml
	std::ofstream titlepagexhtml_f("titlepage.xhtml");
	titlepagexhtml_f << epubtitlepagexhtml;
	titlepagexhtml_f.close();

	return 0;
}

void epubcover(std::string html) {
	return;
}

void epubcss() {
	// this function use css from calibre
	std::ofstream stylesheetcss("stylesheet.css");
	stylesheetcss << epubstylesheetcss;
	stylesheetcss.close();
	return;
}

std::string htmlbodyescape(std::string in) {
	std::string p = in;
	size_t h = 0;
	h = p.find("&", h);
	while (h != std::string::npos) {
		p.insert(h + 1, "amp;");
		h++;
		h = p.find("&", h);
	}
	return p;
}

void epubhtml(std::vector<std::string> txts, std::string headertitle, std::string htmlfilename, std::string title) {
	// this function use html from calibre
	std::vector<std::string> lines = txts;
	// html
	std::string html = "<?xml version='1.0' encoding='utf-8'?>\n<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<head>\n<title></title>\n<link rel=\"stylesheet\" type=\"text/css\" href=\"stylesheet.css\"/>\n</head>\n<body>\n";
	html.insert(97, htmlbodyescape(title));
	int br = 0;
	// h1
	html.append("<h1>" + htmlbodyescape(headertitle) + "</h1>\n");
	// p
	for (size_t i = 0; i < lines.size(); i++) {
		if (lines[i] != "") {
			if (br > 3) { // 4 newline = 1 pagebreak
				html.append("<p class=\"scenebreak\">" + htmlbodyescape(lines[i]) + "</p>\n");
				br = 0;
			}
			else { html.append("<p>" + htmlbodyescape(lines[i]) + "</p>\n"); br = 0; }
		}
		else br++;
	}
	html.append("\n</body>\n</html>\n");

	std::ofstream html_f(htmlfilename);
	html_f << html;
	html_f.close();
	return;
}
