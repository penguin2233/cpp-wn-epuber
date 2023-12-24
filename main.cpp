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
	std::string f = "input.html";
	if (argc < 2 || argc > 3) {
		std::cout << "usage: wn-epuber <url to webnovel read page>\n";
		std::cout << "OR: wn-epuber -f <path to html file>\n";
		return 0;
	}
	else if (argc == 2) {
		std::cout << "downloading  " << argv[1] << '\n';
		std::string url = argv[1];
		std::string h = "";
		if (http_get(argv[1], &h) == -1) return -1;
		std::ofstream hf("input.html");
		hf << h;
		hf.close();
	}
	else if (argc == 3) {
		if (strcmp("-f", argv[1]) != 0) { std::cout << "bad usage\n"; return 0; }
		std::cout << "opening file " << argv[2] << '\n';
		f = argv[2];
	}

	std::vector<chapterInfo> chapters;
top:
	std::ifstream inputf(f);
	std::string html = "";
	while (inputf.good()) {
		std::string buf = "";
		std::getline(inputf, buf);
		html.append(buf);
	}
	inputf.close();

	// enum chapters
	//size_t bcl_begin = html.find("\"@type\": \"BreadcrumbList\",");
	//size_t bcl_end = html.find(']', bcl_begin);
	//std::string breadcrumblist_s = html.substr(bcl_begin, bcl_end - bcl_begin);
	//std::vector<std::string> chapters_r = bfjson::parseArrayOfUnnamedObjects(breadcrumblist_s);
	//std::string genre = bfjson::findSingleElement(chapters_r[0], "name", true);
	//std::vector<chapter> chapters;
	//for (size_t i = 2; i < chapters_r.size(); i++) { // ignore index0, that is the genre, ignore index1, that is foobar
	//	chapter h;
	//	h.name = bfjson::findSingleElement(chapters_r[i], "name", true);
	//	h.url = bfjson::findSingleElement(chapters_r[i], "item", true);
	//	chapters.push_back(h);
	//}

	// chapInfo
	size_t json_begin = html.find("var chapInfo=") + 14;
	size_t json_end_rough = html.find(",\"isAuth\":", json_begin);
	std::string json = html.substr(json_begin, json_end_rough - json_begin);
	size_t json_end = json.find_last_of('}') + 1;
	json = json.substr(0, json_end);
	json.push_back(']'); json.push_back('}'); json.push_back('}');
	// book info
	std::string bkInfo = bfjson::findSingleJSONObject(json, "bookInfo");
	std::string chapInfo = bfjson::findSingleJSONObject(json, "chapterInfo");
	std::string bookId = unescape(bfjson::findSingleElement(bkInfo, "bookId"));
	std::string bookname = unescape(bfjson::findSingleElement(bkInfo, "bookName"));
	std::string author = unescape(bfjson::findSingleElement(bkInfo, "authorName"));
	std::string language = bfjson::findSingleElement(bkInfo, "languageName");
	std::string genre = bfjson::findSingleElement(bkInfo, "categoryName");
	// current chapter
	std::string curChapterId = bfjson::findSingleElement(chapInfo, "chapterId");
	std::string curChapterName = unescape(bfjson::findSingleElement(chapInfo, "chapterName"));
	// next chapter
	std::string nextChapterUrl = "";
	std::string nextChapterId = bfjson::findSingleElement(chapInfo, "nextChapterId");
	std::string nextChapterName = unescape(bfjson::findSingleElement(chapInfo, "nextChapterName"));
	if (nextChapterId != "-1") {
		// https://www.webnovel.com/book/a-mistake-i-made-got-me-a-girlfriend-(girlxgirl)_14108808705366405/chapter-1-next-morning_39190231278213964
		std::stringstream nextUrl_ss;
		nextUrl_ss << "https://www.webnovel.com/book/" << escape(bookname) << '_' << bookId << '/' << escape(nextChapterName) << '_' << nextChapterId;
		nextChapterUrl = nextUrl_ss.str();
	}
	bookInfo book = { bookname, bookId, author, language, genre };
	chapterInfo chapter = { curChapterName, curChapterId, curChapterId + ".html" };
	chapters.push_back(chapter);

	std::string contents_s = bfjson::findSingleArray(json, "contents");
	std::vector<std::string> contents = bfjson::parseArrayOfUnnamedObjects(contents_s);
	std::vector<std::string> lines;
	for (size_t i = 0; i < contents.size(); i++) {
		std::string text = unescape(bfjson::findSingleElement(contents[i], "content"));
		// std::cout << text << '\n';
		lines.push_back(text);
	}

	epubhtml(lines, curChapterName, curChapterId + ".html", bookname);

	std::string txtfilename = escape(bookname) + '-'  + escape(curChapterName) + ".txt";
	std::ofstream outputf(txtfilename);
	for (size_t i = 0; i < lines.size(); i++) {
		outputf << lines[i] << '\n';
	}
	outputf.close();

	if (nextChapterId != "-1") {
		std::cout << "downloading  " << nextChapterUrl << '\n';
		std::string h;
		if (http_get(nextChapterUrl, &h) == -1) return -1;
		std::ofstream hf("input.html");
		hf << h;
		hf.close();
		goto top;
	}

	// download cover
	std::vector<unsigned char> cover_raw;
	// https://book-pic.webnovel.com/bookcover/13911677005221505
	http_get("book-pic.webnovel.com/bookcover/" + book.id, &cover_raw);
	std::ofstream cover_f("cover.jpg", std::ios::binary);
	cover_f.write((char*)&cover_raw[0], cover_raw.size());
	cover_f.close();
	
	epubcss();
	// epubhtml(lines, curChapterName, curChapterId + ".html", bookname);
	epubmeta(book, chapters);
	epubroll(escape(bookname), chapters);

	return 0;
}