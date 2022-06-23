#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include<bits/stdc++.h>
#include<iostream>
#include<vector>
#include<string>
#include<bitset>
#include<fstream>
#include"Fat_Table.h"
using namespace std;
class Virtual_Disk {
public:
	fstream Current_File;
	Fat_Table Fat;
	void initialize();
	void write_Block(vector<char>data, int index);
	vector<char> read_Block(int index);
};
void Virtual_Disk::initialize() {
	Fat.initialize();
	Fat.get_Fat_Table();
}
void Virtual_Disk::write_Block(vector<char>data, int index) {
	if (index == -1)
		return;
	while (data.size() < 1024)
		data.push_back(0);

	Current_File.open("file.txt");
	Current_File.seekg(index * 1024, ios::beg);
	for (int i = 0; i < 1024; i++) {
		Current_File << data[i];
	}
	Current_File.close();
}
vector<char> Virtual_Disk::read_Block(int index) {
	vector<char>data(1024);
	fstream Current_File;
	Current_File.open("file.txt");
	Current_File.seekg(index * 1024, ios::beg);
	for (int i = 0; i < 1024; i++) {
		Current_File >> data[i];
	}
	Current_File.close();
	return data;
}
