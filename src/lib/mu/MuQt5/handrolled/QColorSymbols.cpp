//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

//
//  This is for QGradient

Context::TypeVector types;
types.push_back(c->doubleType());
types.push_back(this);
c->arrayType((const Type*)c->tupleType(types), 1, 0);
