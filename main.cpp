#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include "bfjson.h"
#include "epub.h"
#include "net.h"

std::string unescape(std::string in) {
	std::string p = in;
	for (size_t h = 0; h < p.size(); h++) {
		if (p[h] == '\\') {
			if (p[h + 1] == 'r' || p[h + 1] == 'n') { p.erase(h, 2); h--; }
			else p.erase(h, 1);
		}
	}
	return p;
}

std::string escape(std::string in) {
	std::string p = in;
	for (size_t h = 0; h < p.size(); h++) {
		if (p[h] == ' ') p.replace(h, 1, 1, '-');
		else if (64 < p[h] && p[h] < 91) p[h] = std::tolower(p[h]);
		else if (p[h] == ':' || p[h] == '?') { p.erase(h, 1); h--; }
	}
	return p;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cout << "usage: wn-epuber <url to webnovel read page>\n";
		return 0;
	}

	std::cout << "Downloading " << argv[1] << '\n';
	std::string url = argv[1];
	std::string html = "";
	if (http_get(url, &html) != 0) return -1;

	// parse book
	int counter = 0;
	bookInfo book;
	std::vector<chapterInfo> chapters;
top:
	// chapInfo (loop)
	size_t json_begin = html.find("var chapInfo=") + 14;
	size_t json_end_rough = html.find(",\"isAuth\":", json_begin);
	std::string json = html.substr(json_begin, json_end_rough - json_begin);
	size_t json_end = json.find_last_of('}') + 1;
	json = json.substr(0, json_end);
	json.push_back(']'); json.push_back('}'); json.push_back('}');
	// bookInfo
	if (counter == 0) {
		std::string bkInfo = bfjson::findSingleJSONObject(json, "bookInfo");
		book = {
			unescape(bfjson::findSingleElement(bkInfo, "bookName")),
			bfjson::findSingleElement(bkInfo, "bookId"),
			unescape(bfjson::findSingleElement(bkInfo, "authorName")),
			bfjson::findSingleElement(bkInfo, "languageName"),
			bfjson::findSingleElement(bkInfo, "categoryName")
		};
	}
	// chapterInfo
	std::string chapInfo = bfjson::findSingleJSONObject(json, "chapterInfo");
	// current chapter
	std::string curChapterId = bfjson::findSingleElement(chapInfo, "chapterId");
	std::string curChapterName = unescape(bfjson::findSingleElement(chapInfo, "chapterName"));
	std::string contents_s = bfjson::findSingleArray(json, "contents");
	std::vector<std::string> contents = bfjson::parseArrayOfUnnamedObjects(contents_s);
	std::vector<std::string> lines;
	for (size_t i = 0; i < contents.size(); i++) {
		std::string text = unescape(bfjson::findSingleElement(contents[i], "content"));
		// std::cout << text << '\n';
		lines.push_back(text);
	}
	chapterInfo curChapter = { unescape(curChapterName), curChapterId, unescape(curChapterId) + ".html", lines };
	curChapter.html = epubhtml(book, curChapter);
	chapters.push_back(curChapter); counter++;
	// next chapter
	std::string nextChapterId = bfjson::findSingleElement(chapInfo, "nextChapterId");
	if (nextChapterId != "-1") {
		std::string nextChapterName = unescape(bfjson::findSingleElement(chapInfo, "nextChapterName"));
		// https://www.webnovel.com/book/a-mistake-i-made-got-me-a-girlfriend-(girlxgirl)_14108808705366405/chapter-1-next-morning_39190231278213964
		std::string nextUrl = "https://www.webnovel.com/book/" + escape(book.name) + '_' + book.id + '/' + escape(nextChapterName) + '_' + nextChapterId;
		std::cout << "Downloading " << nextUrl << '\n';
		html.clear();
		if (http_get(nextUrl, &html) != 0) return -1;
		goto top;
	} // else == done

	// download cover
	std::cout << "\nDownloading cover.\n";
	std::vector<unsigned char> cover_raw;
	// https://book-pic.webnovel.com/bookcover/13911677005221505
	std::string cover_url = "book-pic.webnovel.com/bookcover/" + book.id;
	http_get(cover_url, &cover_raw);
	std::ofstream cover_f("cover.jpg", std::ios::binary);
	cover_f.write((char*)&cover_raw[0], cover_raw.size());
	cover_f.close();
	// find cover image dimensions
	size_t im_loc = html.find(cover_url) - 71;
	std::string im_json = html.substr(im_loc, html.find("}", im_loc) - im_loc + 1);
	book.cv_height = bfjson::findSingleElement(im_json, "height");
	book.cv_width = bfjson::findSingleElement(im_json, "width");

	std::cout << "Parsed " << counter << " chapters.\nCreating EPUB...\n";

	epubcss();
	epubmeta(book, chapters);
	epubroll(escape(book.name), chapters);

	std::cout << "Finished.\n";

	return 0;
}