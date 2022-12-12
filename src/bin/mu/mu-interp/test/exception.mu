//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

function: print_trace (void; exception exc)
{
    string[] trace = exc.backtrace();

    for (int i=0; i < trace.size(); i++)
    {
        if (i < 100) print(" ");
        if (i < 10) print(" ");
        print(i + ": " + trace[i] + "\n");
    }
}

try
{
    try
    {
	try
	{
	    try
	    {
		throw exception("the thing that was thrown!");
	    }
	    catch (exception e)
	    {
		print("caught this: ");
		print(e);
		print("\n");
                print_trace(e);
                print("about to rethrow\n");
		throw; // rethrow
	    }
	}
	catch (exception e)
	{
	    print("outside caught this: ");
	    print(e);
	    print("\n");
            print_trace(e);
	    throw;
	}
	catch (...)
	{
	    print("caught something I shouldn't have\n");
	}
    }
    catch (...)
    {
	print("successfully caught whatever\n");
	throw;
    }
}
catch (exception e)
{
    print("after rethrow of unknown caught: ");
    print(e);
    print("\n");
    print_trace(e);
}
