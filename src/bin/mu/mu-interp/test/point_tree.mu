//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
require math;

module: point_tree
{

Vec         := vector float[3];
Scalar      := float;
IndexArray  := int[];
PointArray  := Vec[];
SortFunc    := (int; int, int);

//----------------------------------------------------------------------
//
//  Slice
//

class: Slice
{
    IndexArray  _array;
    int         _start;
    int         _end;

    operator: []   (int; Slice this, int index) { _array[_start+index]; }
    function: size (int; Slice this) { _end - _start; }

    method: Slice(Slice; IndexArray a, int start, int end)
    {
        _array = a;
        _start = start;
        _end = end;
    }

    method: Slice (Slice; Slice other, int start, int end)
    {
        _array = other._array;
        _start = other._start + start;
        _end = other._start + start + (end - start);
    }
}


//----------------------------------------------------------------------
//
//  nth_element
//  for the moment just sort the whole array
//

function: nth_sort(void; IndexArray a, int lo, int hi, int nth, SortFunc comp)
{
    if (lo < hi) 
    {
        let l = lo,
            h = hi,
            p = a[hi];

        do 
        {
            while (l < h && comp(a[l], p) <= 0) l++;
            while (h > l && comp(a[h], p) >= 0) h--;

            if (l < h) 
            {
                let t = a[l];
                a[l] = a[h];
                a[h] = t;
            }

        } while (l < h);

        let t = a[l];
        a[l] = a[hi];
        a[hi] = t;

        if (nth >= lo && nth <= l-1) nth_sort(a, lo, l-1, nth, comp);
        else if (nth >= l+1 && nth <= hi) nth_sort(a, l+1, hi, nth, comp);
    }
}

function: nth_element (int; Slice slice, int nth, SortFunc comp)
{
    nth_sort(slice._array, slice._start, slice._end - 1, nth + slice._start, comp);
    return slice[nth];
}

function: nth_element (int; IndexArray array, int nth, SortFunc comp)
{
    nth_sort(array, 0, array.size() - 1, nth, comp);
    return array[nth];
}

//----------------------------------------------------------------------
//
//  BBox
//

class: BBox 
{ 
    Vec min; 
    Vec max; 

    function: makeEmpty (void; BBox this) { min = float.max; max = -min; }

    function: extend (void; BBox this, Vec p)
    {
        if (p.x < min.x) min.x = p.x; if (p.x > max.x) max.x = p.x;
        if (p.y < min.y) min.y = p.y; if (p.y > max.y) max.y = p.y;
        if (p.z < min.z) min.z = p.z; if (p.z > max.z) max.z = p.z;
    }

    function: majorAxis (int; BBox this)
    {
        Vec v = max - min;
        if v.x > v.y then (if (v.x > v.z) then 0 else 2) else (if (v.y > v.z) then 1 else 2);
    }

    function: center (Vec; BBox this) { (max + min) / 2.0; }
    method: BBox (BBox;) { min = float.max; max = float.min; }
    method: BBox (BBox; BBox other) { min = other.min; max = other.max; }
};


//----------------------------------------------------------------------
//
//  Point Tree
//

class: PointTree
{
    class: Node
    {
        Scalar _splitValue;
        int    _dimension;
        BBox   _box;
        Node   _right;
        Node   _left;
        Slice  _indices;

        function: hasLeft (bool; Node this) { return !(_left eq nil); }
        function: hasRight (bool; Node this) { return !(_right eq nil); }
        function: isLeaf (bool; Node this) { !this.hasLeft() && !this.hasRight(); }

        method: Node (Node;) { this; }
    }

    CompareFunc := (int;int,int);

    Node             _root;
    BBox             _bbox;
    IndexArray       _indices;
    PointArray       _points;
    CompareFunc[3]   _compFuncs;

    function: comp (int; int a, int b, int dim, PointArray points)
    {
        points[a][dim] - points[b][dim];
    }

    function: build (PointTree; PointTree this, PointArray points)
    {
        _points  = points;
        _bbox    = BBox();
        _root    = Node();
        _indices = IndexArray();
        _indices.resize(points.size());

        for (int i=0; i < _indices.size(); i++) _indices[i] = i;
        for_each (p; points) _bbox.extend(p);

        // curry the comp function
        for_each (i; int[]{0,1,2}) _compFuncs[i] = comp(,,i,points);

        split(this, _root, Slice(_indices, 0, _indices.size()), _bbox, 0);
        this;
    }

    method: PointTree (PointTree; Vec[] points)
    {
        _root = nil;
        _bbox = nil;
        _indices = nil;
        _points = nil;
        _compFuncs = PointTree.CompareFunc[3]();
        this.build(points);
    }

    function: split (void; 
                     PointTree this,
                     Node node,
                     Slice slice,
                     BBox bbox,
                     int depth)
    {
        if (slice.size() <= 8)
        {
            node._indices = slice;
        }
        else
        {
            let dimension   = bbox.majorAxis();
            node._dimension = dimension;
            
            //
            //  Why can't I make one big let statement instead of 7
            //  separate ones?
            //

            let bsplit      = bbox.center()[dimension],
                s           = slice.size(),
                leftSize    = s / 2,
                rightSize   = s - leftSize,
                rslice      = Slice(slice, leftSize, s),
                lslice      = Slice(slice, 0, leftSize),
                nth         = nth_element(slice, leftSize, _compFuncs[dimension]);
            
            node._splitValue = _points[nth][dimension];

            if (leftSize > 0)
            {
                BBox newBox = BBox(bbox);
                newBox.max[dimension] = bsplit;
                node._left = Node();
                split(this, node._left, lslice, newBox, depth+1);
            }

            if (rightSize > 0)
            {
                BBox newBox = BBox(bbox);
                newBox.min[dimension] = bsplit;
                node._right = Node();
                split(this, node._right, rslice, newBox, depth+1);
            }
        }
    }

    function: _intersect (IndexArray; PointTree this, 
                          Node n, IndexArray a, Vec p, Scalar r)
    {
        if (n.isLeaf())
        {
            for (int i=0, s=n._indices.size(); i < s; i++)
            {
                int index = n._indices[i];
                float m = mag(_points[index] - p);
                if (m <= r) a.push_back(index);
            }
        }
        else
        {
            if ((p[n._dimension] - r) < n._splitValue && n.hasLeft())
            {
                _intersect(this, n._left, a, p, r);
            }

            if ((p[n._dimension] + r) >= n._splitValue && n.hasRight())
            {
                _intersect(this, n._right, a, p, r);
            }
        }

        a;
    }

    function: intersect (IndexArray; PointTree this, Vec point, Scalar radius)
    {
        _intersect(this, _root, IndexArray(), point, radius);
    }
}


//----------------------------------------------------------------------

function: test_nth_element (void;)
{
    int[] a = {12, 17, 2, 19, 4, 6, 10, 15, 7, 18, 8, 20, 0, 
               3, 16, 11, 5, 9, 1, 14, 13};

    print("a is " + string(a) + "\n");
    
    for (int i=0; i < a.size(); i++)
    {
        int[] acopy = int[](a);
        let nth = nth_element(acopy, i, \: (int;int a, int b) {a - b;});
        print("nth " + i + " is " + nth + "  " + string(acopy) + "\n");
    }
}

function: slice_test (void;)
{
    int[] a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    let s = Slice(Slice(a, 2, 8), 2, 4);
    for (int i=0; i < s.size(); i++) print("s["+i+"] = " + s[i] + "\n");
}

function: bbox_test (void;)
{
    BBox bbox;
    bbox.makeEmpty();
    bbox.extend(Vec(1,0,0));
    bbox.extend(Vec(0,1,0));
    bbox.extend(Vec(0,0,1));
    bbox.extend(Vec(-1,0,0));
    bbox.extend(Vec(0,-1,0));
    bbox.extend(Vec(0,0,-1));
    print(string(bbox) + "\n");
}

//test_nth_element();
//slice_test();
//bbox_test();
};

