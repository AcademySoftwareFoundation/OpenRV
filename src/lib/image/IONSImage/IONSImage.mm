//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#include <IONSImage/IONSImage.h>
#include <iostream>
#include <string>
#include <stl_ext/string_algo.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Iostream.h>
#include <TwkFB/Operations.h>
#include <TwkFB/Exception.h>
#include <objc/objc-class.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSBitmapImageRep.h>
#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/NSString.h>
#import <Foundation/NSArray.h>
#import <Foundation/NSData.h>
#import <Foundation/NSEnumerator.h>
#import <Foundation/NSDictionary.h>

namespace TwkFB {
using namespace std;
using namespace TwkMath;

static bool manageMemory = false;

struct FormatDescription
{
    const char* ext;
    const char* desc;
};

static FormatDescription formats[] =
{ 
    {"bmp", "Windows Bitmap"},
    {"pdf", "Portable Document Format"},
    {"pict", "Apple PICT Image"},
    {"pic",  "Pixar Image"},
    {"eps",  "Encapsulated Postscript"},
    {"epi",  "Encapsulated Postscript Interchange"},
    {"epsi",  "Encapsulated Postscript Interchange"},
    {"epsf",  "Encapsulated Postscript"},
    {"ps",  "Postscript"},
    {"fax",  "FAX Dump"},
    {"ico",  "Icon"},
    {"cur",  "Windows Cursor"},
    {"gif",  "Graphics Interchange Format"},
    {"fpx",  "FlashPix"},
    {"fpix",  "FlashPix"},
    {"pnt",  "MacPaint"},
    {"pntg",  "MacPaint"},
    {"mac",  "MacPaint"},
    {"psd",  "PhotoShop"},
    {"targa",  "TARGA"},
    {"tga",  "TARGA"},
    {"qtif",  "Quicktime Image"},
    {"qti",  "Quicktime Image"},
    {"mrw",  "Minolta Photo Raw"},
    {"icns",  "OS X Icons"},
    {"orf",  "Olympus Photo Raw"},
    {"raf",  "Fuji Photo Raw"},
    {"crw",  "Canon Photo Raw"},
    {"nef",  "Nikon Electronic Format"},
    {"srf",  "SONY Photo Raw"},
    {"dng",  "Adobe Digital Negative"},
    {"cr2",  "Canon Photo Raw"},
    {"hdr",  "Radiance Map File"},
    {"xbm",  "X-Windows Bitmap"},
    {"dcr",  "Kodak Photo Raw"},
    {"pct",  "Apple PICT Image"},
{0, 0}
};

void IONSImage::useLocalMemoryPool()
{
    manageMemory = true;
}

IONSImage::IONSImage() : FrameBufferIO("NSImage", "o") // after OIIO (n)
{
    NSAutoreleasePool *pool = manageMemory ? [[NSAutoreleasePool alloc] init] : nil;
    unsigned int cap = ImageRead;

    NSArray* array = [NSImage imageUnfilteredFileTypes];
    StringPairVector codecs;
    
    for (int i=0; i < [array count]; i++)
    {
        string s = [[array objectAtIndex: i] UTF8String];
        if (s[0] >= 'A' && s[0] <= 'Z' || s[0] == '\'') continue;

        const char* desc = "";
        
        for (FormatDescription* d = formats; d->ext; d++)
        {
            if (s == d->ext) { desc = d->desc; break; }
        }

        addType(s, desc, cap, codecs);
    }

    if (pool) [pool release];
}

IONSImage::~IONSImage() {}

string
IONSImage::about() const
{ 
    return "NSImage";
}

static NSImage*
readNSImage(const char *name, bool getreps=true)
{
    NSString *aFile = [[NSString alloc] initWithUTF8String: name];
    NSImage *image  = [[NSImage alloc] initWithContentsOfFile: aFile];
    return image;
}

void
IONSImage::getImageInfo(const std::string& filename, FBInfo& fbi) const
{
    NSAutoreleasePool *pool = manageMemory ? [[NSAutoreleasePool alloc] init] : nil;

    if (NSImage* image = readNSImage(filename.c_str()))
    {
        //NSBitmapImageRep* rep = [NSBitmapImageRep 
                                    //imageRepWithData: [image TIFFRepresentation]];
	NSBitmapImageRep *rep = [[image representations] objectAtIndex:0];
	NSSize size         = [rep size];
        fbi.numChannels     = [rep samplesPerPixel];
        fbi.width           = [rep pixelsWide];
        fbi.height          = [rep pixelsHigh];

        NSString *colorSpace = [rep colorSpaceName];
        fbi.proxy.newAttribute("NSImage/colorSpaceName", std::string([colorSpace UTF8String]));

        [image autorelease];

        switch ([rep bitsPerSample])
        {
          case 1:
              fbi.dataType = FrameBuffer::BIT;
              break;
          case 8: 
              fbi.dataType = FrameBuffer::UCHAR; 
              break;
          case 16: 
              fbi.dataType = FrameBuffer::USHORT; 
              break;
          case 32: 
              fbi.dataType = FrameBuffer::FLOAT; 
              break;
          default:
              if (pool) [pool release];
              TWK_THROW_STREAM(Exception, "NSImage: Sorry, unsupported bit depth: "
                               << [rep bitsPerSample]);
        }

        if (pool) [pool release];
    }
    else
    {
        if (pool) [pool release];
        TWK_THROW_STREAM(UnsupportedException, "NSImage: not supported");
    }
}


void
IONSImage::readImage(FrameBuffer& fb,
                     const std::string& filename,
                     const ReadRequest& request) const
{
    NSAutoreleasePool *pool = manageMemory ? [[NSAutoreleasePool alloc] init] : nil;

    if (NSImage* image = readNSImage(filename.c_str()))
    {
        //
        //  This will convert non TIFF-like formats to a TIFF like format
        //

        NSBitmapImageRep* rep = [NSBitmapImageRep 
                                    imageRepWithData: [image TIFFRepresentation]];

        [image autorelease];

	NSSize size             = [rep size];
	unsigned char* buffer   = [rep bitmapData];
	int samples             = [rep samplesPerPixel];
        int bbs                 = [rep bitsPerSample];
        size_t rowSize          = [rep bytesPerRow];
        size_t w                = [rep pixelsWide];
        size_t h                = [rep pixelsHigh];

        switch (bbs)
        {
          case 1:
          case 8: 
              fb.restructure(w, h, 0, samples, FrameBuffer::UCHAR ); 
              break;
          case 16: 
              fb.restructure(w, h, 0, samples, FrameBuffer::USHORT ); 
              break;
          case 32: 
              fb.restructure(w, h, 0, samples, FrameBuffer::FLOAT ); 
              break;
          default:
              TWK_THROW_STREAM(UnsupportedException,
                               "Sorry, unsupported bit depth, trying to read file "
                               << filename);
        }

	if ([rep isPlanar])
        {
            TWK_THROW_STREAM(UnsupportedException,
                             "Planar images are not yet supported, reading "
                             << filename);
        }
        else
        {
            typedef unsigned char byte;

            //
            //  Copy the scanlines skipping padding if any in nsimage rep
            //

            for (int row=0; row < h; row++)
            {
                memcpy(fb.scanline<byte>(h-row-1), 
                       buffer + (row * rowSize),
                       fb.scanlineSize());
            }
        }
        
        NSString *colorSpace = [rep colorSpaceName];
        fb.newAttribute("NSImage/ColorSpaceName", std::string([colorSpace UTF8String]));

	NSArray* ireps = [image representations];
	id item = [ireps objectAtIndex: 0];

	if ([item isKindOfClass: [NSBitmapImageRep class]])
        {
#if 1
            if (NSDictionary* d = [item valueForProperty: NSImageEXIFData])
            {
                NSEnumerator *e = [d keyEnumerator];
                NSString* key;
                
                while (key = [e nextObject]) 
                {
                    id val = [d objectForKey: key];
                    //Class c = [val class];
                    //cout << "key is " << [key UTF8String]
                         //<< "for val of type " << c->name << endl;

                    if ([val isKindOfClass: [NSNumber class]])
                    {
                        NSString* sv = [val stringValue];
                        fb.newAttribute(std::string([key UTF8String]),
                                        std::string([sv UTF8String]));
                    }
                    else if ([val isKindOfClass: [NSString class]])
                    {
                        NSString* sv = val;
                        fb.newAttribute(std::string([key UTF8String]),
                                        std::string([sv UTF8String]));
                    }
                    else if ([val isKindOfClass: [NSArray class]])
                    {
                        NSArray* a = val;
                        NSString* v0 = [[a objectAtIndex: 0] stringValue];
                        NSString* v1 = [[a objectAtIndex: 1] stringValue];
                        string v = [v0 UTF8String];
                        v += ".";
                        v += [v1 UTF8String];

                        fb.newAttribute(std::string([key UTF8String]), v);
                    }
                }
            }
#endif
        }
    }

    if (pool) [pool release];
}


void
IONSImage::writeImage(const FrameBuffer& img,
                      const std::string& filename,
                      const WriteRequest& request) const
{
    throw UnsupportedException();
}


}  //  End namespace TwkFB
