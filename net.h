#pragma once
// net.h: header file for net.cpp
int http_get(std::string url, std::vector<unsigned char>* out); // bin mode
int http_get(std::string url, std::string* out); // str mode
