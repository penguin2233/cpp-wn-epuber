#pragma once

// cpp-bfjson.h : Defines the functions for the static library.
// C++ Brute Force JSON Library
// Copyright (C) 2022 penguin2233 (penguin2233.gq)

// Version 20231217 - fixes for json from webnovels (better handle string type in compact json)

#include <string>
#include <vector>
#include <algorithm>

namespace bfjson
{
	inline std::string findSingleElement(std::string json, std::string element, bool removeQuotes = true) {
		std::string ret = "";
		size_t loc = 0;
		while (true) {
			loc = json.find("\"" + element + "\":", loc);
			if (loc == std::string::npos) {
				return ""; // element not found
			}
			// check that we are in the main json, not inside an array or another object
			if (std::count(json.begin(), json.begin() + loc, '{') - std::count(json.begin(), json.begin() + loc, '}') > 1) { loc++; continue; }
			else break;
		}
		bool compact = true, string = false;
		size_t locEnd = 0;
		if (json[loc + element.size() + 3] == ' ') { // not compact json
			compact = false; 
			if (json[loc + element.size() + 4] == '\"') {
				string = true; locEnd = json.find(',', loc); loc++;// locEnd = json.find("\", \"", loc);
			} else locEnd = json.find(',', loc);
		}
		else {
			if (json[loc + element.size() + 3] == '\"') {
				string = true; locEnd = json.find("\",\"", loc);
			} else locEnd = json.find(',', loc);
		}
		if (locEnd == std::string::npos) {
			// high chance this is a singular json object
			locEnd = json.find('}', loc);
			if (locEnd == std::string::npos) {
				return ""; // invalid json
			}
		}
		if (compact) 
			ret = json.substr(loc + element.size() + 4, locEnd - (loc + element.size() + 4));
		else ret = json.substr(loc + element.size() + 3, locEnd - (loc + element.size() + 3));
		if (ret[0] == '\"' && removeQuotes) {
			ret = ret.substr(1, ret.size() - 2);
		}
		return ret;
	}

	inline std::string findSingleArray(std::string json, std::string element) {
		std::string ret = "";
		size_t loc = 0;
		while (true) {
			loc = json.find("\"" + element + "\"", loc);
			if (loc == std::string::npos) return ""; // couldn't find element
			else {
				size_t loc2 = loc + 1 + element.size() + 1 + 1;
				if (json[loc2] != '[') {
					if (json[loc2 + 1] == '[') loc2 = loc2 + 1;
					else continue; // not an array, keep searching
				}
				size_t openSqCt = 0, lastSqOp = 0, closeSqCt = 0, lastSqCloseLoc = 0;
				for (size_t i = loc2 + 1; i < json.size(); i++) {
					if (json[i] == '[') { openSqCt++; lastSqOp = i; }
					else if (json[i] == ']') {
						if (lastSqOp == 0 || (closeSqCt == openSqCt && lastSqOp != 0)) { lastSqCloseLoc = i; break; }
						else closeSqCt++;
					}
				}
				if (lastSqCloseLoc == loc2 + 1 || lastSqCloseLoc == 0) return ""; // empty array
				return json.substr(loc2, lastSqCloseLoc - (loc2 - 1));
			}
		}
	}

	inline std::string findSingleJSONObject(std::string json, std::string element) {
		std::string ret = "";
		size_t loc = 0;
		while (true) {
			loc = json.find("\"" + element + "\"", loc);
			if (loc == std::string::npos) return ""; // couldn't find element
			else {
				if (std::count(json.begin(), json.begin() + loc, '{') - std::count(json.begin(), json.begin() + loc, '}') > 1) { loc++; continue; }
				size_t loc2 = loc + 1 + element.size() + 1 + 1;
				if (json[loc2] == '{') loc2++;
				else if (json[loc2 + 1] == '{') loc2 = loc2 + 2;
				else { loc++; continue; } // not an object, keep searching
				size_t openSqCt = 0, lastSqOp = 0, closeSqCt = 0, lastSqCloseLoc = 0;
				for (size_t i = loc2; i < json.size(); i++) {
					if (json[i] == '{') { openSqCt++; lastSqOp = i; }
					else if (json[i] == '}') {
						if (lastSqOp == 0 || (closeSqCt == openSqCt && lastSqOp != 0)) { lastSqCloseLoc = i; break; }
						else closeSqCt++;
					}
				}
				if (lastSqCloseLoc == loc2 || lastSqCloseLoc == 0) return ""; // empty array
				loc2--;
				return json.substr(loc2, lastSqCloseLoc - (loc2 - 1));
			}
		}
	}

	inline std::vector<std::string> parseArrayOfArrays(std::string json, std::string element) {
		std::vector<std::string> ret;
		std::string arrays = bfjson::findSingleArray(json, element);
		if (arrays == "") return ret; // no array found
		size_t loc = 0;
		while (true) {
			loc = arrays.find('[', loc);
			if (loc == std::string::npos) break; // couldn't find array start (or no more array start left)
			size_t openSqCt = 0, lastSqOp = 0, closeSqCt = 0, lastSqCloseLoc = 0;
			for (size_t i = loc + 1; i < arrays.size(); i++) {
				if (arrays[i] == '[') { openSqCt++; lastSqOp = i; }
				else if (arrays[i] == ']') {
					if (lastSqOp == 0 || (closeSqCt == openSqCt && lastSqOp != 0)) { lastSqCloseLoc = i; break; }
					else closeSqCt++;
				}
			}
			ret.push_back(arrays.substr(loc, lastSqCloseLoc - (loc - 1)));
			loc = lastSqCloseLoc;
		}
		return ret;
	}

	inline std::vector<std::string> parseArray(std::string json) {
		std::vector<std::string> ret;
		if (json == "[]" || json[0] != '[' || json[json.size() - 1] != ']') return ret; // not an array
		size_t loc = 1;
		while (true) {
			if (json[loc] == ' ') loc++;
			size_t loc2 = json.find(',', loc);
			if (loc2 == std::string::npos) { ret.push_back(json.substr(loc, json.find(']') - loc)); break; }
			else { ret.push_back(json.substr(loc, loc2 - loc)); loc = loc2 + 1; }
		}
		return ret;
	}

	inline std::vector<std::string> parseArrayOfUnnamedObjects(std::string json) {
		std::vector<std::string> ret;
		size_t loc = 0;
		while (true) {
			loc = json.find('{', loc);
			if (loc == std::string::npos) break; // couldn't find object start (or no more object start left)
			size_t openCrCt = 0, lastCrOp = 0, closeCrCt = 0, lastCrCloseLoc = 0;
			for (size_t i = loc + 1; i < json.size(); i++) {
				if (json[i] == '{') { openCrCt++; lastCrOp = i; }
				else if (json[i] == '}') {
					if (lastCrOp == 0 || (closeCrCt == openCrCt && lastCrOp != 0)) { lastCrCloseLoc = i; break; }
					else closeCrCt++;
				}
			}
			ret.push_back(json.substr(loc, lastCrCloseLoc - (loc - 1)));
			loc = lastCrCloseLoc;
		}
		return ret;
	}
}
