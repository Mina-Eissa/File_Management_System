#pragma once
#include<bits/stdc++.h>
using namespace std;
vector<char> intToBytes(int paramInt)
{
    vector<char> arrayOfByte(4);
    for (int i = 0; i < 4; i++)
        arrayOfByte[3 - i] = (paramInt >> (i * 8));
    return arrayOfByte;
}
int BytesToint(vector<char> BYTE) {
    reverse(BYTE.begin(), BYTE.end());
    int ret;
    char bytes[4];
    for (int i = 0; i < 4; i++)
        bytes[i] = BYTE[i];
    memcpy(&ret, bytes, sizeof(int));
    return ret;
}