// Dylan Bernier and Mihai Muntean
//Cssc4062 and Cssc4061
//570 Spring 24
//Assigment #2, XE Two Pass Assembler
//Program name Main.cpp

#include <bits/stdc++.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <array>

using namespace std;
map<string, pair<int, string>> OPTAB;  // Operation code table: mnemonic -> (format, opcode)
map<string, int> SYMTAB;               // Symbol table: label -> address
vector<pair<int, vector<string>>> prog; // Program storage: (location counter, [label, opcode, operand])
map<string, string> REGS;              // Register table: register name -> register number
vector<pair<int, string>> obcode;      // Object code storage: (location counter, object code)
// GLobal variables
string programName;
int LOCCTR = 0, base = 0, pc = 0, indx = 0;
bool checkpc = 0;

// Calculates the word size for storage directives
int wrdsize(int i)
{
    string s = prog[i].second[2] ;
    return s.length()/ 2 ;
}

// Calculates the byte size for storage directives
int bytsize(int i)
{
    string adr = prog[i].second[2];
    if (adr[0] == 'C')
    {

        string s = adr.substr(2, adr.size() - 3);
        return s.size();
    }
    else if (adr[0] == 'X')
        return 1;
}
// Converts a hex string to an integer
int hextoint(string hexstring)
{
    int number = (int)strtol(hexstring.c_str(), NULL, 16);
    return number;
}
// Converts an integer to a hex string with given number of digits
string inttohex(int x, int b)
{
    string s;
    stringstream sstream;
    sstream << setfill('0') << setw(b) << hex << (int)x;
    s = sstream.str();
    sstream.clear();
    for (int i = 0; i < s.length(); i++)
        if (s[i] >= 97)
            s[i] -= 32;
    return s;
}
// Converts binary flags to a hex string
string bintohex(bool a, bool b, bool c, bool d)
{
    string s;
    int sum = 0;
    sum += (int)d * 1;
    sum += (int)c * 2;
    sum += (int)b * 4;
    sum += (int)a * 8;
    s = inttohex(sum, 1);
    return s;
}
// Determines the address for format 3 instructions based on PC or base relative addressing
string readdr(string res)
{
    int x = hextoint(res);
    if (x - pc > -256 && x - pc < 4096)
    {
        checkpc = 1;
        return inttohex(x - pc, 3);
    }
    else
    {
        checkpc = 0;
        return inttohex(x - base, 3);
    }
}
// Removes trailing spaces from a string
void rtrim(std::string &str) {
    while (!str.empty() && isspace(str.back())) {
        str.pop_back();
    }
}
// Constructs the object code for format 2 instructions
string format2(int i)
{
    string s, r1, r2 = "A", result; // Default to register A if no second register
    s = prog[i].second[2];
    int j;
    for (j = 0; j < s.size() && s[j] != ','; j++)
        ;
    r1 = s.substr(0, j);
    if (j < s.size()){
        r2 = s.substr(j + 1, s.size() - j - 1);
    }    
    result = OPTAB[prog[i].second[1]].second;
    result += REGS[r1];
    result += REGS[r2];
    return result;
}
// Constructs the object code for format 3 instructions
string format3(int i)
{
    string adr = prog[i].second[2], res1, res2, res3;
    bool flags[6] = {}, dr = 0;
    int no = 0;
    // Check for Literal
    if (adr[0] == '=')
    {
        int j = 0;
            int i = 0;
            while(i < adr.size() && adr[i] != '\'') 
            i++;
            j = i;
            j++;
            while(j < adr.size() && adr[j] != '\'')
            j++;
            adr = adr.substr(i+1, j-i-1);
    }
    if (adr[adr.size() - 1] == 'X' && adr[adr.size() - 2] == ',')
    {
        flags[2] = 1;
        adr = adr.substr(0, adr.size() - 2);
    }
    //Check for Immediate addressing
    if (adr[0] == '#')
    {
        flags[1] = 1;
        adr = adr.substr(1, adr.size() - 1);
        if (SYMTAB.find(adr) != SYMTAB.end())
        {
            res2 = inttohex(SYMTAB[adr], 3);
        }
        else
        {
            res2 = adr;
            dr = 1;
        }
        no = 1;
    }
    //Check for Indirect addressing
    else if (adr[0] == '@')
    {
        flags[0] = 1;
        adr = adr.substr(1, adr.size() - 1);
        no = 2;
        int z = SYMTAB[adr], j;
        for (j = 0; j < prog.size(); j++)
            if (prog[j].first == z)
                break;
        res2 = adr;
        adr = inttohex(prog[j].first, 3);
        if (prog[j].second[1] != "WORD" && prog[j].second[1] != "BYTE" && prog[j].second[1] != "RESW" && prog[j].second[1] != "RESB")
        {
            adr = prog[j].second[2];
            z = SYMTAB[adr];
            for (j = 0; j < prog.size(); j++)
                if (prog[j].second[0] == res2)
                    break;
            adr = prog[j].second[2];
            res2 = inttohex(SYMTAB[adr], 3);
        }
        else
        {
            res2 = adr;
        }
    }
    /*else if (adr[0] == '=')
    {
        adr = adr.substr(3, adr.size() - 4);
        dr = 1;
    }*/
    // Normal case
    else
    {
        res2 = inttohex(SYMTAB[adr], 3);
        flags[0] = 1;
        flags[1] = 1;
        no = 3;
    }
    if (dr != 1 && adr != "*")
    {

        res2 = readdr(res2);
        res2 = res2.substr(res2.size() - 3, 3);
        flags[4] = checkpc;
        flags[3] = !checkpc;
    }
    if (flags[2] == 1)
    {
        res2 = inttohex(hextoint(res2) - indx, 3);
    }
    // Resizing res2 if necessary
    rtrim(res2);
    while (res2.size() < 3)
        res2 = "0" + res2;
    res3 = OPTAB[prog[i].second[1]].second;
    res3 = inttohex(hextoint(res3) + no, 2) + bintohex(flags[2], flags[3], flags[4], flags[5]) + res2;
    return res3;
}
// Constructs the object code for format 4 instructions
string format4(int bb, int adr)
{
    string z = prog[bb].second[2], te = prog[bb].second[1], TA = "", obcode;
    int no = 0;
    // Gathering nixbpe flags
    bool nixbpe[6] = {0, 0, 0, 0, 0, 0};
    nixbpe[0] = (z[0] == '@');
    nixbpe[1] = (z[0] == '#');
    if (nixbpe[0] == nixbpe[1])
    {
        nixbpe[0] = !nixbpe[0];
        nixbpe[1] = !nixbpe[1];
    }
    nixbpe[2] = (z[z.length() - 1] == 'X' && z[z.length() - 2] == ',') ? 1 : 0;
    nixbpe[3] = 0;
    nixbpe[4] = 0;
    nixbpe[5] = 1;
    // Resizing symbol if needed
    if (z[0] == '@' || z[0] == '#'){
        z = z.substr(1, z.length() - 1);
    }
    if (z[z.length() - 1] == 'X' && z[z.length() - 2] == ','){
 
        z = z.substr(0, z.length() - 2);
    }
    // Normal case
    if (nixbpe[0] == 1 && nixbpe[1] == 1)
    {
        for (const auto& symbol : SYMTAB) {
                string s = inttohex(adr, 5);
                
                for (int i = 0; i < prog.size(); i++)
                {
                    if (inttohex(prog[i].first, 5) == s)
                    {
                        if (nixbpe[2] == 0)
                            TA = s;
                        else
                            TA = inttohex(hextoint(s) + indx, 5);
                    }
                }
        }
        no = 3;
    }
    // Checking for Indirect addressing
    else if (nixbpe[0] == 1 && nixbpe[1] == 0 && nixbpe[2] == 0)
    {
       for (const auto& symbol : SYMTAB) {
                string s = to_string(adr);
                for (int i = 0; i < prog.size(); i++)
                    if (to_string(prog[i].first) == s)
                    {
                        s = prog[i].second[2];
                        for (int j = 0; i < prog.size(); j++)
                        if (to_string(prog[j].first) == s)
                            TA = prog[j].second[2];
                    }
       }
        no = 2;
    }
    // Checking for immediate addressing
    else if (nixbpe[0] == 0 && nixbpe[1] == 1)
    {
        for (const auto& symbol : SYMTAB) {
                if (z[0] < 65)
                    TA = inttohex(stoi(z), 5);
                else
                    TA = inttohex(adr, 5);
        }
        no = 1;
    }

    string res3 = OPTAB[prog[bb].second[1].substr(1, prog[bb].second[1].size() - 1)].second;

    res3 = inttohex(hextoint(res3) + no, 2) + bintohex(nixbpe[2], nixbpe[3], nixbpe[4], nixbpe[5]) + TA;
    return res3;
}
// Reads a line from the input file and breaks into 3 parts
void getInput(string l, string *a, string *b, string *c) {
    string x, y, z;

    if (l[0] == ' ')
        x = "*";
    else {
        int j = 0;
        for (j; j < l.size() && l[j] != ' '; j++);
        x = l.substr(0, j);
    }

    if (x.size() + 1 < l.size()) {
        l = l.substr(x.size() + 1);
    } else {
        l = ""; // Reset l to prevent out of range access if x is at the end or nearly at the end of l
    }

    int i = 0;
    while (i < l.size() && l[i] == ' ') i++;
    l = l.substr(i);

    int k = 0;
    for (k; k < l.size() && l[k] != ' '; k++);
    y = l.substr(0, k);
    if (y.size() + 1 < l.size()) {
        l = l.substr(y.size() + 1);
    } else {
        l = ""; // Similar fix for y
    }

    i = 0;
    while (i < l.size() && l[i] == ' ') i++;
    l = l.substr(i);
    z = l.empty() || l[0] == ' ' ? "*" : l;

    *a = x;
    *b = y;
    *c = z;
}


int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1; // Exit the program if no filename is provided
    }

    std::string filename = argv[1]; // Take the filename from the command line arguments
    std::ifstream input1(filename);

    if (input1.fail()) {
        std::cerr << "File '" << filename << "' not found." << std::endl;
        return 1; // Exit the program if the file cannot be opened
    }
    
    freopen("instructions.txt", "r", stdin);  // opens the instructions file for OPTAB
    string x, y, z;
    int a;
    for (int i = 0; i < 59; i++)
    {
        cin >> x >> a >> z;
        OPTAB[x] = {a, z};
    }
    freopen("registers.txt", "r", stdin);
    for (int i = 0; i < 9; i++)
    {
        cin >> x >> y;
        REGS[x] = y; // maps every register name with it's number
    }
    vector<std::string> LITs;
    string l;
    ifstream input(filename) ;
    // Reads input and skips comment lines
    getline(input, l);
    getInput(l, &x, &y, &z);
    while(x[0] == '.')
    {
        getline(input, l);
        getInput(l, &x, &y, &z);
    }
    programName = x; // save program name to be on top of symtab
    // Pass 1
    while (getline(input, l))
    {
        getInput(l, &x, &y, &z);
        if (x[0] == '.')
            continue;
        prog.push_back({LOCCTR, vector<string>()});
        prog.back().second.push_back(x);
        prog.back().second.push_back(y);
        prog.back().second.push_back(z);
        if (x != "*")
        {
            SYMTAB.emplace(x, LOCCTR);
        }
        else if (x == "*" && y[0] == '=') // Check for Literal
        {
            int j = 0;
            int i = 0;
            while(i < y.size() && y[i] != '\'') 
            i++;
            j = i;
            j++;
            while(j < y.size() && y[j] != '\'')
            j++;
            string myLit = y.substr(i+1, j-i-1);
            LITs.push_back(myLit);
            SYMTAB.emplace(myLit, LOCCTR);
        }
        // Checking each part of the broken up line from the input to determine what format the line is
        if (y == "RESW")
        {
            LOCCTR += stoi(z) * 3;
        }
        else if (y == "RESB")
        {
            LOCCTR += stoi(z);
        }
        else if (y == "WORD")
        {
            LOCCTR += wrdsize(prog.size() - 1);
        }
        else if (y == "BYTE")
        {
            LOCCTR += bytsize(prog.size() - 1);
        }
        else if (y[0] == '+')
        {
            LOCCTR += 4;
        }
        else if (OPTAB.find(y) != OPTAB.end())
        {
            LOCCTR += OPTAB[y].first;
        }
        
    }

    ofstream symFile(filename + ".st");
    if (!symFile.is_open()) {
        cerr << "Error: Could not open file '" << filename << "' for writing." << endl;
    
    }
    else {
    // Writing the SYMTAB to file
        symFile << "SYMTAB=====================" << "\n";
        symFile << "Symbol:" << " " << "Location:" << " " << "Length" << "\n";
        symFile << programName << "     " << hex << uppercase << setfill('0') << setw(6) << 0000 << "     " << LOCCTR << "\n";
        for (const auto& symbol : SYMTAB) {
            bool skip = false;
            for (const auto& item : LITs) {
                if (symbol.first == programName || symbol.first == item) {
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                symFile << symbol.first << "     " << hex << uppercase << setfill('0') << setw(6) << symbol.second << "\n";
            }
        }

        symFile << "\n";
        symFile << "LTTAB=====================" << "\n";
         symFile << "Literal:" << " " << "Location:" << "\n";
        for (const auto& item : LITs) {
            if (SYMTAB.count(item)) {
                symFile << item << "     " << hex << uppercase << setfill('0') << setw(6) << SYMTAB[item] << "\n";
            }
        }

        symFile.close();
    }
    // Pass 2
    for (int i = 0; i < prog.size(); i++)
    {
        string s;
        int j;
        for (j = i; j < prog.size(); j++)
        {
            if (prog[j].first != prog[i].first)
                break;
        }
        // Checking each part of the broken up line from the input to determine what format the line is
        pc = prog[j].first;
        string s2 = prog[i].second[1];
        int format = OPTAB[prog[i].second[1]].first;
        if (format == 1)
        {
            s = OPTAB[prog[i].second[1]].second;
        }
        else if (format == 2)
        {
            s = format2(i);
        }

        else if (format == 3)
        {
            s = format3(i);
        }
        else if (s2[0] == '+')
        {
            string tt = prog[i].second[2]; // Resizing tt if necessary
            for (const auto& symbol : SYMTAB) {
                
                if (tt[0] == '#' || tt[0] == '&'){
                    tt.erase(0,1);       
                }
                if (tt[0] == '=')
                {
                    int j = 0;
                    int i = 0;
                    while(i < tt.size() && tt[i] != '\'') 
                    i++;
                    j = i;
                    j++;
                    while(j < tt.size() && tt[j] != '\'')
                    j++;
                    tt = tt.substr(i+1, j-i-1);
                }
                rtrim(tt);
                if(symbol.first == tt){
                    int a = symbol.second;
                    s = format4(i,a);
                }
            }
        }
        if (prog[i].second[1] == "BASE")
        {
            base = SYMTAB[prog[i].second[2]];            
        }
        if (prog[i].second[2] == "LDX")
        {
            x = SYMTAB[prog[i].second[2]];
        }
        if (prog[i].second[1] == "NOBASE")
        {
            base = 0;
        }
        else if (prog[i].second[1] == "BYTE")
        {
            string adr = prog[i].second[2];
            s = adr.substr(2, adr.size() - 3);
            string s3 = "";
            for (int j = 0; j < s.size(); j++)
            {
                s3 += inttohex(s[j], 2);
            }
            if (adr[0] == 'C')
                s = s3;
        }
        prog[i].second.push_back(s);
    }
    // Creating the listing file
    freopen((filename + ".l").c_str(), "w", stdout);
    for (int i = 0; i < prog.size(); i++)
    {
        if(prog[i].second[0] == "*")
        {
            if(prog[i].second[2].length()==8)
                cout << inttohex(prog[i].first, 4)<<"\t\t" << prog[i].second[1] << "\t" << prog[i].second[2] << "  " << prog[i].second[3] << endl;
                else if(prog[i].second[2] == "*")
                    cout << inttohex(prog[i].first, 4)<<"\t\t" << prog[i].second[1] << "\t" << " " << "\t  " << prog[i].second[3] << endl;
            else
                cout << inttohex(prog[i].first, 4)<<"\t\t" << prog[i].second[1] << "\t" << prog[i].second[2] << "\t  " << prog[i].second[3] << endl;

        }
        else
        cout << inttohex(prog[i].first, 4) << "\t" << prog[i].second[0] << "\t" << prog[i].second[1] << "\t" << prog[i].second[2] << "\t  " << prog[i].second[3] << endl;

        if (i == 0)
            programName = prog[i].second[0];
        else if (prog[i].second[3].length() != 0)
            obcode.push_back({prog[i].first, prog[i].second[3]});

    }
    freopen("/dev/tty", "a", stdout);

}
