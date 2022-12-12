//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Cliche OO class example
//

class: Shape
{
    method: name (string;);
    method: isPiecewiseLinear (bool;);
    method: outputName (void;);
}

class: Rectangle : Shape
{
    method: name (string;) { "Rectangle"; }
    method: isPiecewiseLinear (bool;) { return true; }
    method: outputName (void;) { print("Rectangle\n"); }
}

class: Square : Rectangle 
{
    method: name (string;) { "Square"; }
    method: outputName (void;) { print("Square\n"); }
}

class: Ellipse : Shape
{
    method: name (string;) { "Ellipse"; }
    method: isPiecewiseLinear (bool;) { return false; }
    method: outputName (void;) { print("Ellipse\n"); }
}

class: Circle : Ellipse
{
    method: name (string;) { "Circle"; }
    method: outputName (void;) { print("Circle\n"); }
}

Shape circle    = Circle();
Shape ellipse   = Ellipse();
Shape square    = Square();
Shape rectangle = Rectangle();
Shape shape     = Shape();


assert(circle.name() == "Circle");
assert(ellipse.name() == "Ellipse");
assert(square.name() == "Square");
assert(rectangle.name() == "Rectangle");
assert(rectangle.isPiecewiseLinear() == true);
assert(circle.isPiecewiseLinear() == false);
assert(square.isPiecewiseLinear() == true);
assert(ellipse.isPiecewiseLinear() == false);


//
//  Make sure it throws on the unimplemented method
//

bool worked = false;
try { shape.outputName(); } catch (...) { worked = true; }
assert(worked);

