//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <iostream>
using namespace std;

int ack(int m, int n)
{
    return m == 0 ? (n + 1)
                  : (n == 0 ? ack(m - 1, 1) : ack(m - 1, ack(m, n - 1)));
}

int main(int argc, const char** argv)
{
    cout << ack(atoi(argv[1]), atoi(argv[2])) << endl;
}
