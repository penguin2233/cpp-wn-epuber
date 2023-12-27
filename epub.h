#pragma once
// epub.h: contains epub creation functions

struct bookInfo { std::string name; std::string id; std::string author; std::string language; std::string genre; std::string cv_height; std::string cv_width; };
struct chapterInfo { std::string name; std::string id; std::string filename; std::vector<std::string> lines; std::string html; };
struct epubmetadata { std::string contentopf; std::string tocncx; std::string titlepagexhtml; };

// not intended for external call, but allowed
int epubcreate(const char* zipfilename);
int epubdir(const char* dirname);
int epubadd(const char* filename, const char* data, bool compression, int sz);
int epubwrite();

int epubroll(std::string bookname_e, std::vector<chapterInfo> chapters, std::vector<unsigned char> cover_raw, epubmetadata epub_metadata);
epubmetadata epubmeta(bookInfo book, std::vector<chapterInfo> chapters);
std::string epubhtml(bookInfo book, chapterInfo chapter);
