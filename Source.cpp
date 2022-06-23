#define _CRT_SECURE_NO_WARNINGS
#include<cstdlib>
#include<windows.h>
#include<sstream>
#include<map>
#include<iostream>
#include<vector>
#include<string>
#include<bitset>
#include<fstream>
#include"byte.h"
#include"Fat_Table.h"
#include"Virtual_Disk.h"
#ifdef _WIN32
#include <direct.h>
// MSDN recommends against using getcwd & chdir names
#define cwd _getcwd
#define cd _chdir
#else
#include "unistd.h"
#define cwd getcwd
#define cd chdir
#endif
Virtual_Disk VDisk;
string originPath, CurrentPath;
using namespace std;
bool validFile(string Word) {
    return Word.size() >= 4 && Word.substr(Word.size() - 4, 4) == ".txt";
}
void to_lower(string& s) {
    for (auto& i : s) {
        if (i >= 'A' && i <= 'Z')
            i += 32;
    }
}
/////////////////////////////////////////////( Directory_Entry )///////////////////////////////////////////////////////////
class Directory_Entry {
public:
    char File_name[11] = { 0 },
        File_empty[12],
        File_attribute;
    int First_cluster,
        File_size;
    Directory_Entry();
    Directory_Entry(string name, char attr, int F_cluster);
    vector<char> getByte();
    Directory_Entry getDirectory_Entry(vector<char> data);
};
std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}
Directory_Entry::Directory_Entry() {}
Directory_Entry::Directory_Entry(string name, char attr, int F_cluster) {
    if (attr==0x0) {
        int i = 0;
        for (i; i <= min((int)name.size() - 5, 6); i++)
            File_name[i] = name[i];

        for (int pos = name.size() - 4; pos < name.size(); pos++)
            File_name[i++] = name[pos];
        File_attribute = 0x0;
        File_size = 0;
    }
    else {
        for (int i = 0; i < min(11, (int)name.size()); i++)
            File_name[i] = name[i];
        File_attribute = 0x10;
        File_size = 0;
    }
    First_cluster = F_cluster;
    for (int i = 0; i < 12; i++)
        File_empty[i] = '0';
}
vector<char> Directory_Entry::getByte() {
    vector<char>data(32);
    for (int i = 0; i < 11; i++)
        data[i] = File_name[i];
    data[11] = File_attribute;
    for (int i = 12; i < 24; i++)
        data[i] = File_empty[i % 12];
    vector<char>tmp = intToBytes(First_cluster);
    for (int i = 24; i < 28; i++)
        data[i] = tmp[i % 24];
    tmp.clear();
    tmp = intToBytes(File_size);
    for (int i = 28; i < 32; i++)
        data[i] = tmp[i % 28];
    return data;
}
Directory_Entry Directory_Entry::getDirectory_Entry(vector<char> data) {
    Directory_Entry ret;
    for (int i = 0; i < 11; i++)
        ret.File_name[i] = data[i];
    ret.File_attribute = data[11];
    for (int i = 12; i < 24; i++)
        ret.File_empty[i % 12] = data[i];
    vector<char>tmp(4);
    for (int i = 24; i < 28; i++)
        tmp[i % 24] = data[i];
    ret.First_cluster = BytesToint(tmp);
    for (int i = 28; i < 32; i++)
        tmp[i % 28] = data[i];
    ret.File_size = BytesToint(tmp);
    return ret;
}
//////////////////////////////////////////////( Directory )////////////////////////////////////////////////////////////////
class Directory : public Directory_Entry {
public:
    vector<Directory_Entry> Directory_Table;
    Directory* Parent;
    Directory();
    Directory(string name, char attr, int F_cluster, Directory* parent);
    void update_Content(Directory_Entry New);
    void deleteDirectory();
    void readDirectory();
    void writeDirectory();
    int searchDirectory(string name);
    void operator=(const Directory& obj);
};
Directory::Directory() {
    Parent = new Directory();
}
Directory::Directory(string name, char attr, int F_cluster, Directory* parent) :Directory_Entry(name, attr, F_cluster) {
    if (parent != nullptr)
        this->Parent = parent;
}
int Directory::searchDirectory(string name) {
    for (int i = 0; i < this->Directory_Table.size(); i++) {
        if (this->Directory_Table[i].File_name == name)
            return i;
    }
    return -1;
}
void Directory::update_Content(Directory_Entry New) {
    readDirectory();
    int ndx = searchDirectory(string(New.File_name));
    if (ndx != -1) {
        Directory_Table.erase(Directory_Table.begin() + ndx);
        Directory_Table.insert(Directory_Table.begin() + ndx, New);
    }
    writeDirectory();
}
void Directory::deleteDirectory() {
    VDisk.Fat.get_Fat_Table();
    int ndx = this->First_cluster, next = VDisk.Fat.getNext(ndx);
    do {
        VDisk.Fat.setNext(ndx, 0);
        ndx = next;
        if (ndx != -1)
            next = VDisk.Fat.getNext(ndx);
    } while (ndx != -1 && ndx != 0);
    if (Parent != nullptr) {
        Parent->readDirectory();
        int ndx = Parent->searchDirectory(string(this->File_name));
        if (ndx != -1)
        {
            Parent->Directory_Table.erase(Parent->Directory_Table.begin() + ndx);
            Parent->writeDirectory();
        }
    }
    VDisk.Fat.write_Fat_Table();
}
void Directory::readDirectory() {
    if (this->First_cluster != 0) {
        VDisk.Fat.get_Fat_Table();
        vector<vector<char>>blocks;
        int Fatndx = this->First_cluster, next = VDisk.Fat.getNext(Fatndx);
        if (Fatndx != 0) {
            next = VDisk.Fat.getNext(Fatndx);
            do {
                blocks.push_back(VDisk.read_Block(Fatndx));
                Fatndx = next;
                if (Fatndx != -1)
                    next = VDisk.Fat.getNext(Fatndx);
            } while (Fatndx != -1 && Fatndx != 0);
        }
        vector<char>Directory_Tabledata;
        for (auto it : blocks) {
            for (auto i : it)
                Directory_Tabledata.push_back(i);
        }
        vector<char>Directory_Entrydata;
        Directory_Table.clear();
        for (int i = 0; i < Directory_Tabledata.size(); i++) {
            Directory_Entrydata.push_back(Directory_Tabledata[i]);
            if (Directory_Entrydata.size() == 32) {
                if (isalpha(Directory_Entrydata[0])|| Directory_Entrydata[0]=='.')
                    Directory_Table.push_back(this->getDirectory_Entry(Directory_Entrydata));
                Directory_Entrydata.clear();
            }
        }
        VDisk.Fat.write_Fat_Table();
    }
}
void Directory::writeDirectory() {
    VDisk.Fat.get_Fat_Table();
    vector<char>Directory_Entrydata;
    vector<char>Directory_Tabledata;
    int Fatndx = -1, lastndx = -1;
    for (int i = 0; i < Directory_Table.size(); i++) {
        Directory_Entrydata = Directory_Table[i].getByte();
        for (int j = 0; j < 32; j++) {
            Directory_Tabledata.push_back(Directory_Entrydata[j]);
        }
    }

    int numOfrequiredclusters = (Directory_Tabledata.size() + 1023) / 1024;
    if (numOfrequiredclusters <= VDisk.Fat.getAvailable_Clusters()) {
        if (this->First_cluster != 0)
            Fatndx = this->First_cluster;
        else
            Fatndx = this->First_cluster = VDisk.Fat.getAvailableCluster();
        vector<vector<char>>blocks(numOfrequiredclusters);
        for (int i = 0; i < numOfrequiredclusters; i++) {
            for (int j = i * 1024, pos = 0; j < min((int)Directory_Tabledata.size(), (i + 1) * 1024); j++, pos++)
                blocks[i].push_back(Directory_Tabledata[j]);
        }
        vector<char>Empty_again(1024, '#');
        VDisk.write_Block(Empty_again, Fatndx);
        lastndx = Fatndx;
        for (int i = 0; i < blocks.size(); i++) {
            VDisk.write_Block(blocks[i], Fatndx);
            if (lastndx != Fatndx) {
                VDisk.Fat.setNext(lastndx, Fatndx);
            }
            lastndx = Fatndx;
            Fatndx = VDisk.Fat.getAvailableCluster();
        }
        VDisk.Fat.setNext(lastndx, -1);
        VDisk.Fat.write_Fat_Table();
    }
}
void Directory::operator=(const Directory& obj) {
    for (int i = 0; i < 11; i++)
        this->File_name[i] = obj.File_name[i];
    this->File_attribute = obj.File_attribute;
    for (int i = 0; i < 12; i++)
        this->File_empty[i] = 0;
    this->File_size = obj.File_size;
    this->First_cluster = obj.First_cluster;
    this->Directory_Table = obj.Directory_Table;
    this->Parent = obj.Parent;
}
/////////////////////////////////////////////( File_Entry )/////////////////////////////////////////////////////////////////
class File_Entry :public Directory_Entry {
public:
    Directory* Parent;
    string Content;
    File_Entry();
    File_Entry(string name, char attr, int F_cluster, Directory* parent);
    void writeFile();
    void readFile();
    void deleteFile();
    void updateContent(string New);
};
File_Entry::File_Entry() {
    Parent = new Directory();
}
File_Entry::File_Entry(string name, char attr, int F_cluster, Directory* parent) :Directory_Entry(name, attr, F_cluster) {
    Content = "";
    if (parent != nullptr)
        Parent = parent;
    VDisk.Fat.get_Fat_Table();
}
void File_Entry::writeFile() {
    VDisk.Fat.get_Fat_Table();
    int Fatndx = First_cluster, lastndx = -1;
    int numOfrequiredclusters = (Content.size() + 1023) / 1024;
    if (numOfrequiredclusters <= VDisk.Fat.getAvailable_Clusters()) {
        if (First_cluster != 0)
            Fatndx = First_cluster;
        else
            Fatndx = First_cluster = VDisk.Fat.getAvailableCluster();
    }

    vector<vector<char>>blocks(numOfrequiredclusters);
    for (int i = 0; i < Content.size(); i++) {
        blocks[i / 1024].push_back(Content[i]);
    }

    lastndx = Fatndx;
    for (int i = 0; i < blocks.size(); i++) {
        VDisk.write_Block(blocks[i], Fatndx);
        if (lastndx != Fatndx) {
            VDisk.Fat.setNext(lastndx, Fatndx);
        }
        lastndx = Fatndx;
        Fatndx = VDisk.Fat.getAvailableCluster();
    }
    VDisk.Fat.setNext(lastndx, -1);
    VDisk.Fat.write_Fat_Table();
}
void File_Entry::readFile() {
    if (First_cluster != 0) {
        Content = "";
        VDisk.Fat.get_Fat_Table();
        vector<vector<char>>blocks;
        int Fatndx = First_cluster, next = VDisk.Fat.getNext(Fatndx);
        if (Fatndx != 0) {
            next = VDisk.Fat.getNext(Fatndx);
            do {
                blocks.push_back(VDisk.read_Block(Fatndx));
                Fatndx = next;
                if (Fatndx != -1)
                    next = VDisk.Fat.getNext(Fatndx);
            } while (Fatndx != -1 && Fatndx != 0);
        }
        for (auto it : blocks) {
            int ok = 1;
            for (auto i : it)
            {
                if (i == '\0') {
                    ok = 0;
                    break;
                }
                Content += i;
            }
            if (!ok)
                break;
        }
        VDisk.Fat.write_Fat_Table();
    }
}
void File_Entry::deleteFile() {
    if (this->First_cluster != 0) {
        VDisk.Fat.get_Fat_Table();
        int ndx = this->First_cluster, next = VDisk.Fat.getNext(ndx);
        do {
            VDisk.Fat.setNext(ndx, 0);
            ndx = next;
            if (ndx != -1)
                next = VDisk.Fat.getNext(ndx);
        } while (ndx != -1 && ndx != 0);
    }
    if (Parent != nullptr) {
        Parent->readDirectory();
        int ndx = Parent->searchDirectory(this->File_name);
        if (ndx != -1)
        {
            Parent->Directory_Table.erase(Parent->Directory_Table.begin() + ndx);
            Parent->writeDirectory();
        }
    }
    VDisk.Fat.write_Fat_Table();
}
void File_Entry::updateContent(string New) {
    Content = New;
    File_size = Content.size();
    writeFile();
    Parent->writeDirectory();
}
/////////////////////////////////////////////( instructions )//////////////////////////////////////////////////////////////
Directory* root, * CurrentDir;
class Help {
private:
    map<string, string>mp;
public:
    Help() {
        mp["cd"] = "Displays the name of or changes the current directory.\n" + string(15, ' ') + "If the argument is not present, report the current directory.\n" + string(15, ' ') + "If the directory does not exist an appropriate error should be reported.";
        mp["cls"] = "Clear the screen";
        mp["quit"] = "Terminate the program";
        mp["md"] = "Creates a directory.";
        mp["import"] = "import text file(s) from your computer";
        mp["export"] = "export text file(s) to your computer";
        mp["type"] = "Displays the contents of a text file.";
        mp["copy"] = "Copies one or more files to another location.";
        mp["rename"] = "Renames a file or files.";
        mp["del"] = "Deletes one or more files.";
        mp["dir"] = "Displays a list of files and subdirectories in a directory.";
        mp["rd"] = "Removes a directory.";
    }
    void print(string s) {
        if (s.empty()) {
            cout << "\n";
            for (auto command : mp) {
                cout << command.first;
                cout << string(13 - command.first.size(), ' ');
                cout << "- " << command.second << endl;
            }
        }
        else {
            if (mp.find(s) == mp.end()) {
                cout << "'" << s << "'" << " is not recognized as an internal or external command,\n";
                cout << "operable program or batch file.\n";
            }
            else {
                cout << s << " -" << mp[s] << endl;
            }
            cout << endl;
        }
    }
};
class instruction {
public:
    int exportFile(string fileName, string outPath) {
        int ndx = CurrentDir->searchDirectory(fileName);
        if (ndx == -1) {
            // file not exist
            return 0;
        }
        int check = changeDirectory(outPath);
        if (check == -1) {
            // path doesn't exist;
            return -1;
        }
        int check2 = fileExist(fileName);
        if (check2) {
            // file duplicate
            return -2;
        }
        File_Entry* Tmp = new File_Entry(CurrentDir->Directory_Table[ndx].File_name,
            CurrentDir->Directory_Table[ndx].File_attribute,
            CurrentDir->Directory_Table[ndx].First_cluster,
            CurrentDir);
        Tmp->readFile();
        makeRealFile(fileName, Tmp->Content);
        changeDirectory(originPath);
    }
    int validback(string word) {
        int ctr = 0, n = word.size();
        if (n < 2)
            return 0;
        for (int i = 0; i < n - 2; i += 3)
        {
            if (word[i] != '.' || word[i + 1] != '.' || word[i + 2] != '\\')
                return 0;
            ctr++;
        }
        if (word[n - 2] != '.' || word[n - 1] != '.')
            return 0;
        ctr++;
        return ctr;
    }
    int MakeDir(string Word,bool ok) {
        CurrentDir->readDirectory();
        int ndx = CurrentDir->searchDirectory(Word);
        if (ndx == -1) {
            VDisk.Fat.get_Fat_Table();
            int F_Cluster = VDisk.Fat.getAvailableCluster();
            if (F_Cluster != -1)
            {
                VDisk.Fat.setNext(F_Cluster, -1);
                VDisk.Fat.write_Fat_Table();
                if (ok) {
                    CurrentDir->Directory_Table.push_back(Directory_Entry(Word, 0x0, F_Cluster));
                }
                else {
                    CurrentDir->Directory_Table.push_back(Directory_Entry(Word, 0x10, F_Cluster));
                }
                CurrentDir->writeDirectory();
                Directory* Tmp = CurrentDir;
                while (Tmp->Parent != nullptr) {
                    Tmp->Parent->update_Content(Tmp->getDirectory_Entry(Tmp->getByte()));
                    Tmp = Tmp->Parent;
                }
                //cout << VDisk.Fat.getNext(CurrentDir->First_cluster) << "\n";
            }
            else
                return -1;
            return 1;
        }
        return 0;
    }
    int RemoveDir(string Word) {
        int ndx = CurrentDir->searchDirectory(Word);
        if (ndx != -1) {
            if (CurrentDir->Directory_Table[ndx].File_attribute == 0x0) {
                return 0;
            }
            Directory* Tmp = new Directory(CurrentDir->Directory_Table[ndx].File_name,
                CurrentDir->Directory_Table[ndx].File_attribute,
                CurrentDir->Directory_Table[ndx].First_cluster,
                CurrentDir);
            Tmp->readDirectory();
            if (Tmp->Directory_Table.size() > 0)
                return -1;
            string s;
            do {
                cout << "Are you sure that you want to delete " << Word << " , Please enter y for yes or n for no: ";
                cin >> s;
                to_lower(s);
            } while (s != "y" && s != "n");
            if (s == "y")
                Tmp->deleteDirectory();
            cin.ignore();
            return 1;
        }
        return 0;

    }
    int RemoveFile(string Word) {
        int ndx = CurrentDir->searchDirectory(Word);
        if (ndx != -1) {
            if (CurrentDir->Directory_Table[ndx].File_attribute == 0x10) {
                return 0;
            }
            File_Entry* Tmp = new File_Entry(CurrentDir->Directory_Table[ndx].File_name,
                CurrentDir->Directory_Table[ndx].File_attribute,
                CurrentDir->Directory_Table[ndx].First_cluster,
                CurrentDir);
            Tmp->readFile();
            string s;
            do {
                cout << "Are you sure that you want to delete " << Word << " , Please enter y for yes or n for no: ";
                cin >> s;
                to_lower(s);
            } while (s != "y" && s != "n");
            if (s == "y")
                Tmp->deleteFile();
            cin.ignore();
            return 1;
        }
        return 0;

    }
    int Rename(string Old, string New) {
        if (validFile(Old) ^ validFile(New))
            return -1;
        int exist = CurrentDir->searchDirectory(Old), notexist = CurrentDir->searchDirectory(New);
        if (exist != -1 && notexist == -1) {
            Directory_Entry Tmp = Directory_Entry(New,
                CurrentDir->Directory_Table[exist].File_attribute,
                CurrentDir->Directory_Table[exist].First_cluster);
            CurrentDir->Directory_Table[exist] = Tmp;
            CurrentDir->writeDirectory();
            return 1;
        }
        else if (exist == -1) {
            return -1;
        }
        else if (notexist != -1) {
            return -2;
        }
        return 0;
    }
    int changeDirectory(string& word) {
        char buf[260];
        for (int i = 0; i < 260; i++) {
            buf[i] = 0;
        }
        for (int i = 0; i < (int)word.size(); i++) {
            buf[i] = word[i];
        }
        int x = cd(buf);
        return x;
    }
    bool ChangeDir(string Word) {
        int cnt = validback(Word);
        if (cnt != 0) {
            while (cnt--) {
                if (CurrentPath != "M:") {
                    CurrentDir = CurrentDir->Parent;
                    while (CurrentPath.back() != '\\')CurrentPath.pop_back();
                    CurrentPath.pop_back();
                }
            }
            return true;
        }
        else {
            int ndx = CurrentDir->searchDirectory(Word);
            bool ok = Word.find(".txt") != string::npos && Word.find(".txt") == Word.size() - 4;
            if (ndx != -1 && !ok) {
                CurrentDir = new Directory(string(CurrentDir->Directory_Table[ndx].File_name),
                    CurrentDir->Directory_Table[ndx].File_attribute,
                    CurrentDir->Directory_Table[ndx].First_cluster,
                    CurrentDir);
                CurrentDir->readDirectory();
                CurrentPath += '\\';
                CurrentPath += Word;
                return true;
            }
            return false;
        }
    }
    bool fileExist(const string& name) {
        if (FILE* file = fopen(name.c_str(), "r")) {
            fclose(file);
            return true;
        }
        else {
            return false;
        }
    }
    bool importFile(string outPath) {
        string path = outPath;
        string fileName;
        while (outPath.back() != '\\') {
            fileName += outPath.back();
            outPath.pop_back();
        }
        outPath.pop_back();
        reverse(fileName.begin(), fileName.end());
        int check = changeDirectory(outPath);
        if (check == -1) {
            return 0;
        }
        if (fileExist(fileName)) {
            string New_content = "";
            fstream F;
            F.open(fileName);
            changeDirectory(originPath);
            int Maked = MakeDir(fileName,1);
            CurrentDir->readDirectory();
            int ndx = CurrentDir->searchDirectory(fileName);
            if (ndx != -1)
            {
                File_Entry* Tmp = new File_Entry(CurrentDir->Directory_Table[ndx].File_name,
                    CurrentDir->Directory_Table[ndx].File_attribute,
                    CurrentDir->Directory_Table[ndx].First_cluster,
                    CurrentDir);
                Tmp->readFile();
                Tmp->Content = "";
                while (getline(F, New_content)) {
                    
                    Tmp->Content += New_content;
                    CurrentDir->Directory_Table[ndx].File_size += New_content.size();
                    Tmp->Content += '\n';
                    New_content.clear();
                }
                Tmp->Content += '\0';
                Tmp->writeFile();
                CurrentDir->writeDirectory();
            }
            F.close();
            return Maked;
        }
        return 0;
    }
    bool typeFile(string word) {
    int ndx = CurrentDir->searchDirectory(word);
    if (ndx != -1) {
        File_Entry* Tmp = new File_Entry(CurrentDir->Directory_Table[ndx].File_name,
            CurrentDir->Directory_Table[ndx].File_attribute,
            CurrentDir->Directory_Table[ndx].First_cluster,
            CurrentDir);
        Tmp->readFile();
        cout << endl << word << endl << endl;
        for (auto it : Tmp->Content)
        {
            if (it == '\n')
                cout << "\n";
            else
                cout << it;
        }
        cout << endl;
        return 1;
    }
    return 0;
}
    void ShowDir() {
        cout << " Directory of " << CurrentPath << "\n\n";
        int ctrDir = 0, ctrFile = 0, sumSize = 0;
        CurrentDir->readDirectory();
        for (auto it : CurrentDir->Directory_Table) {
            string s = string(8, ' ');
            if (it.File_attribute == 0x0)
            {
                int ndx = CurrentDir->searchDirectory(string(it.File_name));
                File_Entry* Tmp = new File_Entry(CurrentDir->Directory_Table[ndx].File_name,
                    CurrentDir->Directory_Table[ndx].File_attribute,
                    CurrentDir->Directory_Table[ndx].First_cluster,
                    CurrentDir);
                Tmp->readFile();
                int sz = 0;
                for (auto i : Tmp->Content) {
                    if (i == '\0')break;
                    sz++;
                }
                string siz = to_string(sz);
                s += siz;
                s += string(8 - siz.size(), ' ');
                sumSize += sz;
                ctrFile++;
            }
            else {
                s += "<DIR>   ";
                ctrDir++;
            }
            cout << s << string(it.File_name) << "\n";
        }
        cout << string(8, ' ') << ctrFile << " File(s)   " << sumSize << " bytes\n";
        cout << string(8, ' ') << ctrDir << " Dir(s)    " << VDisk.Fat.getAvailable_Clusters() * 1024 << " bytes free\n";
    }
    void makeRealFile(string fileName, string content) {
        ofstream MyFile(fileName);
        for (auto it : content)
            MyFile << it;
        MyFile.close();
    }
    void quit() {
    exit(0);
    }
    void clearSreen() {
    system("cls");
} 
    string getCurrentPath() {
        char buf[260];
        string s = cwd(buf, 260);
        return s;
    }
};
////////////////////////////////////////////////////( Main )/////////////////////////////////////////

int main() {
    VDisk.initialize();
    instruction I;
    root = CurrentDir = new Directory("M:", 0x10, 5, nullptr);
    root->writeDirectory();
    originPath = I.getCurrentPath();
    string s, command;
    CurrentPath = root->File_name;
    while (true) {
        cout << CurrentPath << ">";
        getline(cin, s);
        stringstream line(s);
        line >> command;
        to_lower(command);
        string word = "";
        line >> word;
        if (command.empty())
            continue;
        if (command == "help") {
            Help h1;
            to_lower(s);
            h1.print(word);
        }
        else if (command == "quit") {
            I.quit();
        }
        else if (command == "cls") {
            if (!word.empty()) {
                cout << "Error: cls command syntax is\n cls \n function: Clear the screen";
            }
            I.clearSreen();
        }
        else if (command == "cd") {
            if (word.empty()) {
                cout << CurrentPath << endl;
            }
            else {
                if (!I.ChangeDir(word))
                    cout << "Error : this path \" " << word << " \" is not exist!\n";
            }
        }
        else if (command == "md") {
            if (word.empty()) {
                cout << "Error: md command syntax is\n";
                cout << "md [directory]\n";
                cout << "[directory] can be a new directory name or fullpath of a new directory\n";
                cout << "Creates a directroy\n";
            }
            else {
                int check = I.MakeDir(word,0);
                if (check == -1) {
                    cout << "Error : Not Enough sapce\n";
                }
                else if (check == 0) {
                    cout << "Error : This directory already exist!\n";
                }
            }
        }
        else if (command == "dir") {
            I.ShowDir();
        }
        else if (command == "rd") {
            if (word.empty()) {
                cout << "Error: md command syntax is\n";
                cout << "rd [directory]\n";
                cout << "[directory] can be a new directory name or fullpath of a new directory\n";
                cout << "Creates a directroy\n";
            }
            else {
                int check = I.RemoveDir(word);
                if (check == 0) {
                    cout << "Error : This directory \" " << word << " \" doesn't exist\n";
                }
                else if (check == -1) {
                    cout << "Error : This directory \" " << word << " \" is not empty\n";
                }
            }
        }
        else if (command == "del") {
            if (word.empty()) {
                cout << "ERROR:\nDeletes one or more files.\nNOTE: it confirms the user choice to delete the file before deleting\n";
                cout << "del command syntax is\n";
                cout << " del [dirFile]+\n+ after [dirfile] represent that you can pass more than file name (or fullpath of file) or directory name (or fullpath of directory)\n";
                cout << "[dirfile] can be file name (or fullpath of file) or directory name (or fullpath of directory)";
            }
            else
            {
                int check = I.RemoveFile(word);
                if (check == 0) {
                    cout << "Error : This File \"" << word << "\" doesn't exist!\n";
                }
            }
        }
        else if (command == "rename") {
            string word2;
            line >> word2;
            int check = I.Rename(word, word2);
            if (check == -1)
                cout << "This file: \"" << word << "\" doesn't exist in your disk\n";
            else if (check == -2)
                cout << "Error: A Duplicate file name exists!\n";
        }
        else if (command == "import") {
            int check = I.importFile(word);
            if(!check)
                cout << "This file: \"" << word << "\" doesn't exist in your disk\n";
            I.changeDirectory(originPath);
        }
        else if (command == "export") {
            string word2;
            line >> word2;
            int check = I.exportFile(word, word2);
            if (check == 0)
                cout << "This file: \"" << word << "\" doesn't exist in your disk\n";
            else if (check == -1)
                cout << "Error : path invalid\n";
            else if (check == -2)
                cout << "Error : file dublication\n";
            I.changeDirectory(originPath);
        }
        else if (command == "type") {
            int check = I.typeFile(word);
            if(!check)
                cout << "This file: \"" << word << "\" doesn't exist in your disk\n";
        }
        else {
            //if command word not exist
            cout << "'" << command << "'" << " is not recognized as an internal or external command,\n";
            cout << "operable program or batch file.\n";
        }
        command.clear();
    }
    return 0;
}