//******************************************************************************
// Copyright (c) 2007 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#import <Foundation/Foundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <DarwinBundle/DarwinBundle.h>
#include <sstream>
#include <TwkUtil/File.h>
#include <TwkUtil/Base64.h>
#include <TwkExc/Exception.h>
#include <stl_ext/string_algo.h>
#include <iostream>
#include <sys/stat.h>
#include <AppKit/NSPasteboard.h>
#include <errno.h>
#include <sys/sysctl.h>
#include <boost/algorithm/string.hpp>

using namespace boost;

void (*sessionFromUrlPointer)(std::string) = 0;
void (*putUrlOnMacPasteboardPointer)(std::string, std::string) = 0;
void (*registerEventHandlerPointer)() = 0;

void
putUrlOnMacPasteboard (std::string url, std::string title)
{
    NSString *titleString = [[NSString alloc] initWithUTF8String: title.c_str()];
    NSString *urlString = [[NSString alloc] initWithUTF8String: url.c_str()];
    NSString *fixedUrlString = (NSString *) CFURLCreateStringByAddingPercentEscapes(NULL, (CFStringRef) urlString, NULL, NULL, kCFStringEncodingUTF8);
    
    NSURL *nsurl = [NSURL URLWithString: fixedUrlString];

    if (nsurl == nil) 
    {
        std::cerr << "ERROR: malformed url: '" << url << "'" << std::endl;
        return;
    }

    const NSString* kUTTypeURLName = @"public.url-name";
    const NSString* kUTTypeURL = @"public.url";
    const NSString* kUTTypePlainText = @"public.utf8-plain-text";

    NSPasteboard* pb = [NSPasteboard generalPasteboard];

    [pb declareTypes:[NSArray arrayWithObjects:
            NSURLPboardType, kUTTypeURLName, kUTTypeURL, kUTTypePlainText, nil] owner:nil];
    [nsurl writeToPasteboard:pb];
    [pb setString:titleString forType:(NSString*)kUTTypeURLName];
    [pb setString:fixedUrlString forType:(NSString*)kUTTypeURL];
    [pb setString:fixedUrlString forType:(NSString*)kUTTypePlainText];

    std::cerr << "INFO: URL on pasteboard: '" << title << "' (" << url << ")" << std::endl;
}

@interface UrlHandler: NSObject {
}
-(void) getUrl:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent;
@end

@implementation UrlHandler
- (void) getUrl:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
    NSString *url = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
    //  std::cerr << "INFO: url received from apple event '" << [url cString] << "'" << std::endl;

    if (sessionFromUrlPointer) (*sessionFromUrlPointer)([url UTF8String]);
}
@end

UrlHandler *urlHandler = 0;

namespace TwkApp {
using namespace TwkUtil;
using namespace std;

struct BundlePaths
{
    NSAutoreleasePool* pool;
    NSString*          appName;
    NSBundle*          bundle;
    NSUserDefaults*    defaults;
    NSString*          plugins;
    NSArray*           supportArray;
    NSArray*           homeSupport;
};

static void
mkpath(NSString* path)
{
    NSArray* array = [path pathComponents];
    int n = [array count];
    NSString* ppath = [array objectAtIndex: 0];
    
    for (int i = 1; i < n; i++)
    {
        ppath = [ppath stringByAppendingPathComponent: [array objectAtIndex: i]];
        const char* utf8 = [ppath UTF8String]; 

        if (!fileExists(utf8)) mkdir(utf8, 0777);
    }
}

static void
registerEventHandler()
{
    if (urlHandler)
    {
        //  cerr << "INFO: registering rvlink url handler" << endl;
        //
        //  Note: we _must_ remove previous handler here.  It will
        //  be the internal Qt one as of 4.6, and doesn't work
        //  (conflates url open with file open and mangles the url).
        //  Apple docs say that setEventHandler removes previous
        //  handler, but that appears to be false.
        //
        [[NSAppleEventManager sharedAppleEventManager] removeEventHandlerForEventClass:kInternetEventClass andEventID:kAEGetURL];

        [[NSAppleEventManager sharedAppleEventManager] setEventHandler:urlHandler 
                andSelector:@selector(getUrl:withReplyEvent:) forEventClass:kInternetEventClass 
                andEventID:kAEGetURL];
    }
}

DarwinBundle::DarwinBundle(const FileName& appName,
                           size_t majv,
                           size_t minv,
                           size_t revn,
                           bool protocolHandler,
                           bool sandboxed,
                           bool inheritedSandbox) 
    : Bundle(appName, majv, minv, revn, sandboxed, inheritedSandbox)
{
    m_bundle = new BundlePaths();
    m_bundle->pool = [[NSAutoreleasePool alloc] init];
    m_bundle->appName = [[NSString alloc] initWithUTF8String: appName.c_str()];
    m_bundle->bundle = [NSBundle mainBundle];
    m_bundle->defaults = [NSUserDefaults standardUserDefaults];

    bool hasSandboxedEnabled = sandboxed || inheritedSandbox;

    ostringstream str;
    str << majv << "." << minv << "." << revn;
    setEnvVar("TWK_APP_VERSION", str.str().c_str(), 1);

    NSString* versionStr = [[NSString alloc] initWithUTF8String: str.str().c_str()];

    //
    //  Make sure ~/Library/Application Support/RV exists
    //

    NSString* userSupport = [[NSString alloc] initWithUTF8String: getenv("HOME")];
    if (hasSandboxedEnabled)
    {
        userSupport = [userSupport stringByAppendingPathComponent: @"Library"];
        userSupport = [userSupport stringByAppendingPathComponent: @"Containers"];
        userSupport = [userSupport stringByAppendingPathComponent: @"com.tweaksoftware.crank"];
        userSupport = [userSupport stringByAppendingPathComponent: @"Data"];
    }
    userSupport = [userSupport stringByAppendingPathComponent: @"Library"];
    userSupport = [userSupport stringByAppendingPathComponent: @"Application Support"];
    userSupport = [userSupport stringByAppendingPathComponent: @"RV"];
    m_userSupportPath = [userSupport UTF8String];
        
    if (!fileExists([userSupport UTF8String]))
    {
        mkpath(userSupport);
    }

    const char* rvSupportEnv = hasSandboxedEnabled ? NULL : getenv("RV_SUPPORT_PATH");
    const char* twkSupportEnv = hasSandboxedEnabled ? NULL : getenv("TWK_SUPPORT_PATH");

    if (const char* c = rvSupportEnv ? rvSupportEnv : twkSupportEnv)
    {
        vector<string> tokens;
        stl_ext::tokenize(tokens, c, ":");

        NSMutableArray* m = [[NSMutableArray alloc] init];

        for (size_t i=0; i < tokens.size(); i++)
        {
            [m addObject: [NSString stringWithUTF8String: tokens[i].c_str()]];
        }

        [m addObject: [m_bundle->bundle builtInPlugInsPath]];

        m_bundle->supportArray = m;
    }
    else
    {
        NSMutableArray* m = [[NSMutableArray alloc] initWithArray:
                            NSSearchPathForDirectoriesInDomains(
                                 NSApplicationSupportDirectory,
                                 NSAllDomainsMask,
                                 YES)];

        for (size_t i = 0; i < [m count]; i++)
        {
            NSString* s = [m objectAtIndex: i];
            [m replaceObjectAtIndex: i withObject: 
                   [s stringByAppendingPathComponent: m_bundle->appName]];
        }

        [m addObject: [m_bundle->bundle builtInPlugInsPath]];
        m_bundle->supportArray = m;
    }

    for (int i=0; i < [m_bundle->supportArray count]; i++)
    {
        NSString* path = [m_bundle->supportArray objectAtIndex: i];

        if (fileExists([path UTF8String]))
        {
            NSString* mu = [path stringByAppendingPathComponent: @"Mu"];
            NSString* py = [path stringByAppendingPathComponent: @"Python"];
            NSString* imgformats = [path stringByAppendingPathComponent: @"ImageFormats"];
            NSString* nodes = [path stringByAppendingPathComponent: @"Nodes"];
            NSString* oiioplugs = [path stringByAppendingPathComponent: @"OIIO"];
            NSString* output = [path stringByAppendingPathComponent: @"Output"];
            NSString* libraries = [path stringByAppendingPathComponent: @"MediaLibrary"];
            NSString* movformats = [path stringByAppendingPathComponent: @"MovieFormats"];
            NSString* supportfiles = [path stringByAppendingPathComponent: @"SupportFiles"];
            NSString* configfiles = [path stringByAppendingPathComponent: @"ConfigFiles"];
            NSString* lib = [path stringByAppendingPathComponent: @"lib"];
            NSString* packages = [path stringByAppendingPathComponent: @"Packages"];
            NSString* profiles = [path stringByAppendingPathComponent: @"Profiles"];

            if (!hasSandboxedEnabled && !fileExists([packages UTF8String]))
            {
                mkpath(packages);
                mkpath(mu);
                mkpath(imgformats);
                mkpath(oiioplugs);
                mkpath(output);
                mkpath(libraries);
                mkpath(movformats);
                mkpath(supportfiles);
                mkpath(configfiles);
                mkpath(lib);
                mkpath(profiles);
            }

            addPathToEnvVar("TWK_FB_PLUGIN_PATH", [imgformats UTF8String]);
            addPathToEnvVar("TWK_NODE_PLUGIN_PATH", [nodes UTF8String]);
            addPathToEnvVar("TWK_MOVIE_PLUGIN_PATH", [movformats UTF8String]);
            addPathToEnvVar("TWK_OUTPUT_PLUGIN_PATH", [output UTF8String]);
            addPathToEnvVar("TWK_PROFILE_PLUGIN_PATH", [profiles UTF8String]);
            addPathToEnvVar("MU_MODULE_PATH", [mu UTF8String]);
            addPathToEnvVar("PYTHONPATH", [py UTF8String]);
            addPathToEnvVar("OIIO_LIBRARY_PATH", [oiioplugs UTF8String]);
            addPathToEnvVar("TWK_MEDIA_LIBRARY_PLUGIN_PATH", [libraries UTF8String]);
            addPathToEnvVar("PYTHONPATH", [libraries UTF8String]);
        }
    }

    [m_bundle->defaults removeVolatileDomainForName: NSArgumentDomain];
    [m_bundle->defaults setObject: @"NO" forKey: @"NSTreatUnknownArgumentsAsOpen"];

    addPathToEnvVar("TWK_APP_SUPPORT_PATH", supportPath());

    if (protocolHandler)
    {
        //
        //  rvlink protocol handling
        //

        //  Regsiter event handler
        //
        //  cerr << "INFO: registering rvlink url handler" << endl;
        urlHandler = [[UrlHandler alloc] init];
        registerEventHandler();
        registerEventHandlerPointer = registerEventHandler;

        //  Make mac pasteboard copy available.
        //
        putUrlOnMacPasteboardPointer = putUrlOnMacPasteboard;
    }

    //
    //  Non-deprecated way to get the OS minor version
    //

    vector<char> charbuffer(256);
    size_t size = charbuffer.size();
    int ret = sysctlbyname("kern.osrelease", &charbuffer[0], &size, NULL, 0);

    vector<string> buffer;
    string r = &charbuffer[0];
    algorithm::split(buffer, r, is_any_of(string(".")), token_compress_on);

    size_t minor = std::atoi(buffer[0].c_str()) - 4;
    
    ostringstream minorStr;
    minorStr << minor;

    setEnvVar("RV_OS_VERSION_MAJOR", "10");
    setEnvVar("RV_OS_VERSION_MINOR", minorStr.str());
    setEnvVar("RV_OS_VERSION_REVISION", "0");
}

DarwinBundle::~DarwinBundle()
{
    [m_bundle->pool release];
    delete m_bundle;
}

void
DarwinBundle::registerHandler()
{
    //
    //  Claim to be the default protocol handler
    //
    NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
    OSStatus result = LSSetDefaultHandlerForURLScheme((CFStringRef)@"rvlink", (CFStringRef)bundleID);

    //TODO: Check result for errors
}

Bundle::Path 
DarwinBundle::top()
{
    return Path([[m_bundle->bundle bundlePath] UTF8String]);
}

Bundle::Path 
DarwinBundle::resourcePath()
{
    return Path([[m_bundle->bundle resourcePath] UTF8String]);
}

Bundle::Path 
DarwinBundle::builtInPluginPath()
{
    return Path([[m_bundle->bundle builtInPlugInsPath] UTF8String]);
}

Bundle::Path 
DarwinBundle::resource(const FileName& name, const FileName& type)
{
    NSString* s = [[NSString alloc] initWithUTF8String: name.c_str()];
    NSString* t = [[NSString alloc] initWithUTF8String: type.c_str()];
    NSString* p = [m_bundle->bundle pathForResource: s ofType: t];
    return p ? Path([p UTF8String]) : "";
}

Bundle::Path
DarwinBundle::executableFile(const FileName& name)
{
    NSString* s = [[NSString alloc] initWithUTF8String: name.c_str()];
    NSString* p = [m_bundle->bundle pathForAuxiliaryExecutable: s];
    return p ? Path([p UTF8String]) : "";
}

Bundle::Path
DarwinBundle::application(const FileName& name)
{
    return resource(name, "app");
}

Bundle::PathVector 
DarwinBundle::fileInSupportPath(const DirName& name)
{
    PathVector paths;
    NSString* s  = [[NSString alloc] initWithUTF8String: name.c_str()];
    NSString* mp = [[NSString alloc] initWithUTF8String: majorVersionDir().c_str()];
    NSString* vp = [[NSString alloc] initWithUTF8String: versionDir().c_str()];

    //
    //  Specific version (e.g., Library/Application Support/App/3.6/name)
    //

    for (int i=0; i < [m_bundle->supportArray count]; i++)
    {
        paths.push_back([[[[m_bundle->supportArray objectAtIndex: i] 
                             stringByAppendingPathComponent: vp]
                             stringByAppendingPathComponent: s]
                            UTF8String]);
    }

    //
    //  Major version (e.g., Library/Application Support/App/3/name)
    //

    for (int i=0; i < [m_bundle->supportArray count]; i++)
    {
        paths.push_back([[[[m_bundle->supportArray objectAtIndex: i] 
                             stringByAppendingPathComponent: mp]
                             stringByAppendingPathComponent: s]
                            UTF8String]);
    }

    //
    //  Generic (e.g., Library/Application Support/App/name)
    //

    for (int i=0; i < [m_bundle->supportArray count]; i++)
    {
        paths.push_back([[[m_bundle->supportArray objectAtIndex: i] 
                             stringByAppendingPathComponent: s]
                            UTF8String]);
    }

    return paths;
}

Bundle::PathVector
DarwinBundle::supportPath()
{
    PathVector paths;

    for (int i=0; i < [m_bundle->supportArray count]; i++)
    {
        paths.push_back([[m_bundle->supportArray objectAtIndex: i] UTF8String]);
    }

    return paths;
}

Bundle::Path 
DarwinBundle::defaultLicense(const FileName& licFileName,
                             const FileName& type)
{
    return "/Library/Application Support/RV/" + licFileName + "." + type;
}

Bundle::PathVector
DarwinBundle::licenseFiles(const FileName& licFileName,
                           const FileName& type)
{
    FileName f = licFileName;
    f += ".";
    f += type;
    PathVector paths = fileInSupportPath(f);
    Path appfile = resource(licFileName, type);
    paths.push_back(appfile);
    Path dpath = defaultLicense(licFileName, type);
    if (find(paths.begin(), paths.end(), dpath) == paths.end()) paths.push_back(dpath);
    return paths;
}


Bundle::Path 
DarwinBundle::rcfile(const FileName& rcfileName, 
                     const FileName& type,
                     const EnvVar& rcenv)
{
    FileName rc = rcfileName;
    rc += ".";
    rc += type;

    FileName dotFile = "."; 
    dotFile += rc;

    NSString* dotString = [[NSString alloc] initWithUTF8String: dotFile.c_str()];
    NSString* rcString  = [[NSString alloc] initWithUTF8String: rcfileName.c_str()];

    NSString* home = NSHomeDirectory();
    NSString* homeInit = [home stringByAppendingPathComponent: dotString];
    NSFileManager* fm = [NSFileManager defaultManager];

    if (const char* e = getenv(rcenv.c_str()))
    {
        return e;
    }

    if ([fm fileExistsAtPath: homeInit])
    {
        const char* initPath = [homeInit UTF8String];
        return initPath;
    }

    Path binit = resource(rcfileName, type);
    NSString* bs = [[NSString alloc] initWithUTF8String: binit.c_str()];

    if ([fm fileExistsAtPath: bs])
    {
        const char* initPath = [bs UTF8String];
        return initPath;
    }

    return "";
}


Bundle::PathVector 
DarwinBundle::pluginPath(const DirName& pluginType)
{
    return fileInSupportPath(pluginType);
}

Bundle::PathVector 
DarwinBundle::scriptPath(const DirName& scriptType)
{
    return fileInSupportPath(scriptType);
}

Bundle::Path
DarwinBundle::appPluginPath(const DirName& pluginType)
{
    Path p = builtInPluginPath();
    ostringstream str;
    str << p;
    if (p[p.size()-1] != '/') str << "/";
    str << pluginType;
    return str.str();
}

Bundle::Path 
DarwinBundle::appScriptPath(const DirName& scriptType)
{
    return appPluginPath(scriptType);
}

Bundle::Path 
DarwinBundle::userCacheDir()
{
    NSFileManager* fm = [NSFileManager defaultManager];

    NSError* errorReturn;

    NSURL* url = [fm URLForDirectory: NSCachesDirectory
                            inDomain: NSUserDomainMask
                   appropriateForURL: nil
                              create: YES
                               error: nil];

    if ([url isFileURL] == YES)
    {
        const char* utf8 = [[url path] UTF8String]; 
        return Path(utf8);
    }

    return Path();
}

void
DarwinBundle::setEnvVar(const EnvVar& var, const Path& value, bool force)
{
    if (!getenv(var.c_str()) || force)
    {
        setenv(var.c_str(), value.c_str(), 1);
    }
}

void
DarwinBundle::addPathToEnvVar(const EnvVar& var, const PathVector& value)
{
    ostringstream str;
    const char* envvar = getenv(var.c_str());
    if (envvar) str << envvar;

    for (int i=0; i < value.size(); i++)
    {
        if (i || envvar) str << ":";
        str << value[i];
    }

    setEnvVar(var, str.str(), true);
}

void
DarwinBundle::addPathToEnvVar(const EnvVar& var, const Path& value, bool toHead)
{
    ostringstream str;
    const char* envvar = getenv(var.c_str());

    if (toHead)
    {
        str << value;
        if (envvar) str << ":" << envvar;
    }
    else
    {
        if (envvar) str << envvar << ":";
        str << value;
    }
    setEnvVar(var, str.str(), true);
}

void
DarwinBundle::removePSN(int &argc, char* argv[], vector<char*>& nargv)
{
    int eatArgs = 0;
    
    for (int i=0; i < argc; i++)
    {
        if (eatArgs > 0)
        {
            eatArgs--;
        }
        else if (!strncmp(argv[i], "-psn", 4))
        {
        }
        else if (!strncmp(argv[i], "-NS", 3))
        {
            eatArgs = 1;
        }
        else
        {
            nargv.push_back(argv[i]);
        }
    }
}

Bundle::FileName
DarwinBundle::crashLogFile()
{
    string lfile = [m_bundle->appName UTF8String];
    lfile += ".crash.log";
    return lfile;
}

Bundle::FileName
DarwinBundle::crashLogDirectory()
{
    return "Library/Logs/CrashReporter/";
}

Bundle::Path
DarwinBundle::userPluginPath(const DirName& pluginType)
{
    return m_userSupportPath + "/" + pluginType;
}

Bundle::FileAccessPermission
DarwinBundle::permissionForFileAccess(const Path& filePath, bool readonly) const
{
    if (isSandboxed())
    {
        NSString* nsFilePath = [NSString stringWithUTF8String: filePath.c_str()];
        NSURL* url = [NSURL fileURLWithPath: nsFilePath];
        NSError* error = NULL;

        NSURLBookmarkCreationOptions options = NSURLBookmarkCreationWithSecurityScope | 
            (readonly ? NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess : 0) | 
            //NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess |
            NSURLBookmarkCreationPreferFileIDResolution;

        NSData* data = [url bookmarkDataWithOptions: options
                            includingResourceValuesForKeys: nil 
                                      relativeToURL: nil 
                                              error: &error];

        if (error)
        {
            NSString* desc = [error localizedDescription];
            ostringstream err;
            err << "ERROR: " << [desc UTF8String];
            cout << err.str() << endl;
            TWK_THROW_EXC_STREAM(err.str());
        }

        const void* bytes      = [data bytes];
        NSUInteger sizeInBytes = [data length];

        return TwkUtil::id64Encode((const char*)bytes, sizeInBytes);
    }
    else
    {
        FileAccessPermission permission;
        return permission;
    }

}

Bundle::AccessObject 
DarwinBundle::beginFileAccessWithPermission(const FileAccessPermission& permission) const
{
    if (isSandboxed())
    {
        if (permission.empty()) return AccessObject(0);

        vector<char> decodeData;
        TwkUtil::id64Decode(permission, decodeData);

        NSData* data = [NSData dataWithBytes: &decodeData.front()
                                      length: decodeData.size()];

        NSError* error = NULL;
        BOOL stale = NO;

        NSURLBookmarkResolutionOptions options = NSURLBookmarkResolutionWithSecurityScope;

        NSURL* outURL = [NSURL URLByResolvingBookmarkData: data
                                                  options: options
                                            relativeToURL: nil 
                                      bookmarkDataIsStale: &stale 
                                                    error: &error];

        if (error)
        {
            NSString* desc = [error localizedDescription];
            ostringstream err;
            err << "ERROR: bookmark resolve failed: " << [desc UTF8String];
            cout << err.str() << endl;
            TWK_THROW_EXC_STREAM(err.str());
        }
        else if (!outURL)
        {
            // throw
            TWK_THROW_EXC_STREAM("ERROR: Unable to resolve bookmark data to URL");
        }

        NSURL* fileURL = outURL;
        [fileURL retain];
        [fileURL startAccessingSecurityScopedResource];
        return AccessObject(fileURL);
    }
    else
    {
        return AccessObject(0x1);
    }
}

void 
DarwinBundle::endFileAccessWithPermission(AccessObject aobj) const
{
    if (isSandboxed() && aobj != AccessObject(0))
    {
        NSURL* fileURL = (NSURL*)aobj;
        [fileURL stopAccessingSecurityScopedResource];
        [fileURL release];
    }
}

Bundle::Path
DarwinBundle::userHome()
{
    return Path([NSHomeDirectory() UTF8String]);
}

Bundle::Path
DarwinBundle::userMovies()
{
    return Path([[NSHomeDirectory() stringByAppendingPathComponent: @"Movies"] UTF8String]);
}

Bundle::Path
DarwinBundle::userMusic()
{
    return Path([[NSHomeDirectory() stringByAppendingPathComponent: @"Music"] UTF8String] );
}

Bundle::Path
DarwinBundle::userPictures()
{
    return Path([[NSHomeDirectory() stringByAppendingPathComponent: @"Pictures"] UTF8String] );
}

} // TwkApp
