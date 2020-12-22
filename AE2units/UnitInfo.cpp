#include <iostream>
#include <sstream>
#include <string>

#include "endl.hpp"
#include "UnitInfo.hpp"

extern "C" {
	#include "../utils/utils.h"
}

class UnitInfo::Impl {
public:
	unsigned short moveRange = 0;

	short minAttack = 0;
	short maxAttack = 0;

	short defense = 0;

	unsigned short maxAttackRange = 0;
	unsigned short minAttackRange = 0;

	short price = 0;

	typedef std::pair<short, short> charpos;
	std::vector<charpos> charPos;

	std::set<unsigned short> properties;
};

UnitInfo::UnitInfo(): impl(std::make_unique<Impl>()) {
}

UnitInfo::~UnitInfo() {
}

// read unit data from a .bin file
std::ifstream& UnitInfo::read_bin(std::ifstream& inputStream) {
	unsigned char c1, c2;

	// section 1: basic information
	impl->moveRange = inputStream.get();
	impl->minAttack = inputStream.get();
	impl->maxAttack = inputStream.get();
	impl->defense = inputStream.get();
	impl->maxAttackRange = inputStream.get();
	impl->minAttackRange = inputStream.get();
	c1 = inputStream.get();
	c2 = inputStream.get();
	impl->price = static_cast<short>(fourBytesToUInt32(0, 0, c1, c2));

	// section 2: fight animation
	unsigned int numChars = inputStream.get();
	impl->charPos.resize(numChars);
	for (auto& charPos: impl->charPos) {
		charPos.first = inputStream.get();
		charPos.second = inputStream.get();
	}

	// section 3: unit properties
	unsigned int numProperties = inputStream.get();
	impl->properties.clear();
	for (unsigned int j = 0; j < numProperties; ++j) {
		unsigned short property = inputStream.get();
		impl->properties.emplace(property);
	}

	return inputStream;
}

/* Print the unit data to a .unit file.
	Sample format (archer.unit):
	==============================
	MoveRange 5
	Attack 50 55
	Defence 5
	AttackRange 2 1
	Cost 250

	CharCount 5

	CharPos 0 30 63
	CharPos 1 30 101
	CharPos 2 8 80
	CharPos 3 8 121
	CharPos 4 8 41

	HasProperty 6
	==============================
*/
std::ostream& operator<<(std::ostream& outputStream, const UnitInfo& unit) {
	// section 1: basic information
	outputStream << UnitKey::moveRange << " " << unit.impl->moveRange << endl;
	outputStream << UnitKey::attack << " "
		<< unit.impl->minAttack << " " << unit.impl->maxAttack << endl;
	outputStream << UnitKey::defense << " " << unit.impl->defense << endl;
	outputStream << UnitKey::attackRange << " "
		<< unit.impl->maxAttackRange << " "
		<< unit.impl->minAttackRange << endl;
	outputStream << UnitKey::price << " " << unit.impl->price << endl;

	// section 2: fight animation information
	unsigned int numChars = unit.impl->charPos.size();
	outputStream << endl;
	outputStream << UnitKey::charCount << " " << numChars << endl;
	if (numChars > 0) {
		outputStream << endl;
		for (unsigned int i = 0; i < numChars; ++i) {
			const auto& coord = unit.impl->charPos.at(i);
			outputStream << UnitKey::charPos << " " << i << " "
				<< coord.first << " " << coord.second << endl;
		}
	}

	// section 3: unit properties
	if (!unit.impl->properties.empty()) {
		outputStream << endl;
		unsigned int i = 0;
		for (const auto& property: unit.impl->properties) {
			outputStream << "HasProperty " << property;
			if (i < unit.impl->properties.size() - 1) {
				outputStream << endl;
			}
			++i;
		}
	}

	return outputStream;
}

// read unit data from a .unit file
// see the comments before operator<< overloading for the file structure
std::istream& operator>>(std::istream& inputStream, UnitInfo& unit) {

	unit.impl->properties.clear();

	while (!inputStream.eof()) {
		// get line and key
		std::string line, key;
		std::getline(inputStream, line);
		std::istringstream lineStream(line);
		lineStream >> key;

		// section 1: basic information
		if (key == UnitKey::moveRange) {
			lineStream >> unit.impl->moveRange;
		}
		else if (key == UnitKey::attack) {
			lineStream >> unit.impl->minAttack;
			lineStream >> unit.impl->maxAttack;
		}
		else if (key == UnitKey::defense) {
			lineStream >> unit.impl->defense;
		}
		else if (key == UnitKey::attackRange) {
			lineStream >> unit.impl->maxAttackRange;
			lineStream >> unit.impl->minAttackRange;
		}
		else if (key == UnitKey::price) {
			lineStream >> unit.impl->price;
		}

		// section 2: fight animation
		if (key == UnitKey::charCount) {
			unsigned int numChars = 0;
			lineStream >> numChars;
			unit.impl->charPos.resize(numChars);

			// process each CharPos line
			unsigned int j = 0;
			for (; j < numChars; ++j) {
				std::string line;
				do {
					std::getline(inputStream, line);
				} while (line.empty() && !inputStream.eof());

				std::istringstream lineStream(line);
				lineStream >> key;
				if (key == UnitKey::charPos) {
					auto& charPos = unit.impl->charPos.at(j);
					short n;
					lineStream >> n >> charPos.first >> charPos.second;
				}
				else {
					throw std::ifstream::failure("ERROR: Bad data encountered when processing " + UnitKey::charPos);
				}
			}
		}

		// section 3: unit properties
		if (key == UnitKey::hasProperty) {
			unsigned short property;
			lineStream >> property;
			unit.impl->properties.emplace(property);
		}
	}

	return inputStream;
}

// write unit data to a .bin file
std::ofstream& UnitInfo::write_bin(std::ofstream& outputStream) const {

	unsigned char c1, c2, c3, c4;

	// section 1: basic info
	outputStream.put(static_cast<char>(impl->moveRange));
	outputStream.put(static_cast<char>(impl->minAttack));
	outputStream.put(static_cast<char>(impl->maxAttack));
	outputStream.put(static_cast<char>(impl->defense));
	outputStream.put(static_cast<char>(impl->maxAttackRange));
	outputStream.put(static_cast<char>(impl->minAttackRange));
	uInt32ToFourBytes(static_cast<unsigned short>(impl->price), &c1, &c2, &c3, &c4);
	outputStream.put(c3);
	outputStream.put(c4);

	// section 2: fight animation
	outputStream.put(static_cast<char>(impl->charPos.size()));
	for (const auto& charPos: impl->charPos) {
		outputStream.put(static_cast<char>(charPos.first));
		outputStream.put(static_cast<char>(charPos.second));
	}

	// section 3: unit properties
	outputStream.put(static_cast<char>(impl->properties.size()));
	for (const auto& property: impl->properties) {
		outputStream.put(property);
	}

	return outputStream;
}
