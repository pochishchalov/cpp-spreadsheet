#include <limits>
#include <iostream>

#include "common.h"
#include "formula.h"
#include "tests.h"

using namespace std;

enum Command { CLEAR, SET, PRINT };

string ParseCommand() {
	char ch;
	cin >> ch;
	string result;
	result += ch;
	while (cin.get(ch) && !isspace(ch)) {
		result += ch;
	}
	cin.putback(ch);
	return result;
}

string ParseText() {
	char ch;
	cin >> ch;
	string result;
	if (ch == '\"') {
		while (cin.get(ch) && ch != '\"') {
			result += ch;
		}
		if (ch != '\"') {
			throw invalid_argument("\'");
		}
	}
	else {
		throw invalid_argument("\'");
	}
	return result;
}

Command GetCommonCommand(string& command) {
	if (command == "clear"s) {
		return CLEAR;
	}
	else if (command == "set"s) {
		return SET;
	}
	else if (command == "print"s) {
		return PRINT;
	}
	else {
		throw invalid_argument(command);
	}
}

void PrintInstructions() {
	cout << "Common spreadsheet commands:\n"s;
	cout << "--------------------------------------------------------------------------\n"s;
	cout << "  set"s << "       Sets a value in a new or existing cell.\n"s 
		 << "            Input format: set 'cell position' \"cell contents\"\n"s
		 << "            Example: set A1 \"=5 + 10\"\n"s;
	cout << "--------------------------------------------------------------------------\n"s;
	cout << "  print"s << "     Prints the contents of the specified cell or the entire table.\n"s
		 << "            Input format for print specified cell: print 'cell position'\n"s
		 << "            Additional commands:\n"s
		 << "        -v"s << "  Prints a table showing the values in the cells\n"s
	     << "        -t"s << "  Prints a table with text in the cells\n"s;
	cout << "--------------------------------------------------------------------------\n"s;
	cout << "  clear"s << "     Clears the cell value.\n"s
		 << "            Input format : clear 'cell position'\n"s;
	cout << "--------------------------------------------------------------------------\n"s;
	cout << "  quite"s << "     Exit the program.\n"s;
	cout << "--------------------------------------------------------------------------\n"s;
}


int main() {
    //test::RunTests();
	auto sheet = CreateSheet();
	while (true) {
		string command = ParseCommand();
		if (command == "quite"s) {
			return 0;
		}
		if (command == "help"s) {
			PrintInstructions();
			continue;
		}
		try{
			auto main_command = GetCommonCommand(command);
			command = ParseCommand();
			if (command.size() < 2) {
				throw invalid_argument(command);
			}
			switch (main_command)
			{
			case CLEAR:
			{
				sheet->ClearCell(Position::FromString(command));
				break;
			}
			case SET:
			{
				auto pos = Position::FromString(command);
				string text = ParseText();
				sheet->SetCell(pos, text);
				break;
			}
			case PRINT:
			{
				if (command[0] == '-') {
					auto table_size = sheet->GetPrintableSize();
					switch (command[1])
					{
					case 't':
					{
						sheet->PrintTexts(std::cout);
						break;
					}
					case 'v':
					{
						sheet->PrintValues(std::cout);
						break;
					}
					default:
					{
						throw invalid_argument(command);
					}
					}
				}
				else {
					auto cell = sheet->GetCell(Position::FromString(command));
					std::cout << "Value: "s << cell->GetValue() << "; "s
						<< "Text: "s << cell->GetText() << std::endl;
				}
				break;
			}
			default:
				break;
			}
		}
		catch (invalid_argument& arg) {
			if (*arg.what() == '\"') {
				cerr << "error, missing quote"s << endl;
			}
			else {
				cerr << '\'' << arg.what() << "' is not a spreadsheet command, see 'help'"s << endl;
			}
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
		}
		catch (InvalidPositionException) {
			cerr << "error: invalid position"s << endl;
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
		}
		catch (FormulaException) {
			cerr << "error: invalid formula"s << endl;
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
		}
		catch (CircularDependencyException) {
			cerr << "error: circular dependency"s << endl;
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
		}
		catch (...) {
			cerr << "unknown exception"s << endl;
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
		}
	}
}
