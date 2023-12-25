#pragma once
// epub.h: contains epub creation functions

struct bookInfo { std::string name; std::string id; std::string author; std::string language; std::string genre; std::string cv_height; std::string cv_width; };
struct chapterInfo { std::string name; std::string id; std::string filename; std::vector<std::string> lines; std::string html; };

int epubroll(std::string bookname_e, std::vector<chapterInfo> chapters);
int epubmeta(bookInfo book, std::vector<chapterInfo> chapters);
std::string epubhtml(bookInfo book, chapterInfo chapter);
void epubcss();
