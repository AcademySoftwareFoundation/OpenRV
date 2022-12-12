//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Type patterns.  Patten matching is depth first. '#N and '#M have
//  type "size" or something. The first type pattern determines the
//  meanings of the type/index variables.
//

int 'M, 'R, 'N;
MatrixMR := 'a['M,'R];
MatrixRN := 'b['R,'N];
MatrixRR := 'c['M,'N];

operator: * (MatrixMN; MatrixMR a, MatrixRN b)
{
    MatrixMN m;

    for (int i = 0; i < 'M; i++)
    {
        for (int j = 0; j < 'N; j++)
        {
            'c acc = 0;

            for (int q = 0; q < 'R; q++)
            {
                acc += a[i,q] * b[q,j];
            }

            m[i,j] = acc;
        }
    }

    m;
}

operator: * (MatrixMN; MatrixMR a, MatrixRN b)
{
    MatrixMN m;

    for_index (i, j; m)
    {
        'c acc;

        for_index (q; 4)
        {
            acc += a[i,q] * b[q,j];
        }

        m[i,j] = acc;
    }

    m;
}

row_type float[4,4];
column_type float[4,4];
