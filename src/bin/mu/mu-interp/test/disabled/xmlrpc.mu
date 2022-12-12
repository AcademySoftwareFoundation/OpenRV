//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use io;
use encoding;

union: Value
{
      Nil
    | Int int 
    | String string 
    | Bool bool 
    | Double float 
    | DateTime (int,int,int,int,int,int)
    | Binary byte[]
    | Struct [(string, Value)]
    | EmptyArray
    | Array [Value]
}

function: outputValue (void; ostream o, Value v)
{
    case (v)
    {
        Nil         -> { print(o, "</nil>"); }
        Int i       -> { print(o, "<int>%d</int>" % i); }
        String s    -> { print(o, "<string>%s</string>" % s); }
        Bool b      -> { print(o, "<boolean>%d</booleanl>" % (if b then 1 else 0)); }
        Double f    -> { print(o, "<double>%g</double>" % f); }
        EmptyArray  -> { print(o, "<array><data></data></array>"); }
        DateTime d  -> { print("<dateTime.iso8601>%04%02%02T%02:%02:%02" % d); }

        Struct s -> 
        { 
            print(o, "<struct>");

            for_each (p; s) 
            {
                print(o, "<member><name>%s</name><value>" % p._0);
                outputValue(o, p._1);
                print(o, "</value></member>");
            }

            print(o, "</struct>");
        }

        Array a ->
        {
            print(o, "<array><data>");
            for_each (e; a) outputValue(o, e);
            print(o, "</data></array>");
        }

        Binary b -> 
        {
            print(o, "<base64>");
            print(o, utf8_to_string(to_base64(b)));
            print(o, "</base64>");
        }
    }
}

function: outputParamList (void; ostream o, [Value] vlist)
{
    print(o, "<params>");

    for_each (v; vlist)
    {
        print(o, "<param>");
        outputValue(o, v);
        print(o, "</param>");
    }

    print(o, "</params>");
}

function: outputMethod (void; ostream o, string methodName, [Value] paramList)
{
    print(o, "<methodCall><methodName>%s</methodName>" % methodName);
    outputParamList(o, paramList);
    print(o, "</methodCall>");
}

function: xmlrpc (void; 
                  string name, 
                  [Value] params,
                  (void; Value) rvalFunc)
{
//     httpPost("https://rvdemo.shotgridstudio.com/api3_preview/",
//              [("Content-Type", "text/xml")],
//              "",
//              "xmlrpc-return",
//              "xmlrpc-authenticate",
//              "xmlrpc-error");
    ;
}


Member := (string, Value);
Param  := Value;

use Value;

let p1   = Struct( [ ("script_name", String("rv") ),
                     ("script_key", String("4b1676497a208c845b12f5c6734daf9d6e7c6274")) ] );

let p2   = Struct( [ ("paging", 
                      Struct( [("current_path", Int(1)),
                               ("entities_per_page", Int(500))] )),

                      ("filters", 
                       Struct( [("conditions", EmptyArray),
                                ("logical_operator", String("and"))] )) 

                     ] );

osstream str;
outputMethod(str, "read", [p1, p2]);
print("str = ");
print(string(str));
print("\n");

