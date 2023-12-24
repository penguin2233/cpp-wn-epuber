#pragma once
// epub.h: contains epub creation functions

struct bookInfo { std::string name; std::string id; std::string author; std::string language; std::string genre; };
struct chapterInfo { std::string name; std::string id; std::string filename; };

int epubroll(std::string bookname_e, std::vector<chapterInfo> chapters);
int epubmeta(bookInfo book, std::vector<chapterInfo> chapters);
void epubcover(std::string html);
void epubhtml(std::vector<std::string> txts, std::string headertitle, std::string htmlfilename, std::string title);
void epubcss();
