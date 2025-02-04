//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// MISSING: setStops (void; QGradient this, "const QGradientStops &" stopPoints)
// MISSING: stops ("QGradientStops"; QGradient this)
//
//  the tuple and array are created in QColorSymbols.cpp

addSymbol(new Function(c, "setStops", setStops, None,
                       // Compiled, QGradient_setStops_,
                       Return, "int", Parameters,
                       new Param(c, "this", "qt.QGradient"),
                       new Param(c, "this", "(double,qt.QColor)[]"), End));

addSymbol(new Function(c, "stops", stops, None,
                       // Compiled, QGradient_stops_,
                       Return, "(double,qt.QColor)[]", Parameters,
                       new Param(c, "this", "qt.QGradient"), End));
