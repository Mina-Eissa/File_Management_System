#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<vector>
#include<string>
#include<bitset>
#include<fstream>
#include"byte.h"
using namespace std;
class Fat_Table {
public:
	int* Table;
	Fat_Table();
	void initialize();
	void write_Fat_Table();
	vector<int> get_Fat_Table();
	void update_Fat_table();
	void print_Fat_Table();
	int getAvailable_Clusters();
	int getAvailableCluster();
	int getNext(int index);
	void setNext(int index, int Value);
};

Fat_Table::Fat_Table() {
	Table = new int[1024];
}
void Fat_Table::initialize() {
	for (int i = 0; i < 5; i++)
		Table[i] = -1;
	for (int i = 5; i < 1024; i++)
		Table[i] = 0;
}
void Fat_Table::write_Fat_Table() {
	fstream CurrentFile;
	CurrentFile.open("file.txt");
	CurrentFile.seekg(1024, ios::beg);
	for (int i = 0; i < 1024; i++) {
		vector<char>data = intToBytes(Table[i]);
		for (auto it : data)
			CurrentFile << it;
	}
	CurrentFile.close();
}
vector<int> Fat_Table::get_Fat_Table() {
	vector<int>Values(1024);
	fstream CurrentFile;
	CurrentFile.open("file.txt");
	CurrentFile.seekg(1024, ios::beg);
	for (int i = 0; i < 4 * 1024; i++) {
		vector<char>data;
		char c;
		CurrentFile >> c;
		data.push_back(c);
		if (data.size() == 4) {
			Values[i / 4] = Table[i / 4] = BytesToint(data);
			data.clear();
		}
	}
	CurrentFile.close();
	return Values;
}
void Fat_Table::update_Fat_table() {}
void Fat_Table::print_Fat_Table() {
	for (int i=0;i<1024;i++)
		cout << Table[i] << " ";
	cout << "\n";
	cout << endl;
}
int Fat_Table::getAvailable_Clusters() {
	int Count_Zeros = 0;
	for (int i = 0; i < 1024; i++)
		Count_Zeros += Table[i] == 0;
	return Count_Zeros;
}
int Fat_Table::getAvailableCluster() {
	for (int i = 0; i < 1024; i++)
		if (Table[i] == 0)
			return i;
	return -1;
}
int Fat_Table::getNext(int index) {
	return Table[index];
}
void Fat_Table::setNext(int index, int Value) {
	Table[index] = Value;
}