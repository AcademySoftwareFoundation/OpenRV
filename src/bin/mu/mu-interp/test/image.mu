//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
Color         := vector float[4];
AttributePair := (string,string);
Attributes    := [AttributePair];

union: Pixels
{
    FloatPixels float[] | 
    HalfPixels half[] |
    ShortPixels short[] | 
    BytePixels byte[] 
}


class: ImagePlane
{
    ChannelNames  := string[];

    Pixels       _pixels;
    int          _width;
    int          _height;
    int          _scanlineSize;
    int          _depth;
    ChannelNames _channelNames;

    method: index (int; float ndcx, float ndcy)
    {
        let x = int(ndcx * float(_width)),
            y = int(ndcy * float(_height));

        y * _scanlineSize + x * _channelNames.size();
    }

    method: channelValue (float; int ch, float ndcx, float ndcy)
    {
        let i = index(ndcx, ndcy) + ch;
        
        case (_pixels)
        {
            FloatPixels p -> { return p[i]; }
            HalfPixels p -> { return float(p[i]); }
            ShortPixels p -> { return float(p[i]); }
            BytePixels p -> { return float(p[i]) / 255.0; }
        }
    }
}

ImagePlanes := ImagePlane[];
EncodedPlanes := (string, byte[])[];

class: Image
{
    Attributes   _attributes;
    ImagePlanes  _planes;
}

class: EncodedImage
{
    Attributes      _attributes;
    EncodedPlanes   _planes;
}

class: RGBASampler
{
    Channel := (ImagePlane, int);

    Channel    _R;
    Channel    _G;
    Channel    _B;
    Channel    _A;
    Channel    _Y;
    Image      _image;
    
    method: RGBASampler (RGBASampler; Image i)
    {
        _image = i;

        for_each (p; i._planes) 
            for_index (i; p._channelNames)
            {
                let n = p._channelNames[i],
                    c = (p, i);

                if (n == "R") _R = c;
                else if (n == "G") _G = c;
                else if (n == "B") _B = c;
                else if (n == "A") _A = c;
                else if (n == "Y") _Y = c;
            }

        assert(_R neq nil || _Y neq nil);
        if (_R neq nil) assert(_B neq nil);
    }

    operator: [] (Color; RGBASampler this, float ndcx, float ndcy)
    {
        if (_G neq nil)
        {
            return Color(_R._0.channelValue(ndcx, ndcy, _R._1),
                         _G._0.channelValue(ndcx, ndcy, _G._1),
                         _B._0.channelValue(ndcx, ndcy, _B._1),
                         _A._0.channelValue(ndcx, ndcy, _A._1));
        }
    }
}


