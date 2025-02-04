//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <CDL/cdl_utils.h>
#include <TwkMath/Iostream.h>
#include <TwkUtil/PathConform.h>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace
{
    using namespace CDL;
    using namespace boost;
    using namespace TwkMath;

    ColorCorrection readCC(property_tree::ptree::value_type e)
    {
        ColorCorrection cc;
        cc.slope = e.second.get<Vec3f>("SOPNode.Slope", cc.slope);
        cc.offset = e.second.get<Vec3f>("SOPNode.Offset", cc.offset);
        cc.power = e.second.get<Vec3f>("SOPNode.Power", cc.power);
        cc.saturation =
            e.second.get<float>("SatNode.Saturation", cc.saturation);

        return cc;
    }

} // namespace

namespace CDL
{
    using namespace std;
    using namespace boost;
    using namespace TwkMath;
    using namespace TwkUtil;

    bool isCDLFile(const std::string& filepath)
    {
        const string ext = boost::filesystem::extension(filepath);

        //
        //  Add the extensions that we support.
        //

        static const char* exts[] = {".cc", ".ccc", ".cdl", 0};

        for (const char** p = exts; *p; p++)
        {
            if (ext == *p)
                return true;
        }

        return false;
    }

    ColorCorrectionCollection readCDL(string filepath)
    {
        filepath = pathConform(filepath);

        ColorCorrectionCollection collection;
        filebuf fb;
        if (!fb.open(filepath.c_str(), ios::in))
        {
            cerr << "ERROR: Unable to open CDL file: '" << filepath << "'"
                 << endl;
            return collection;
        }

        istream is(&fb);
        property_tree::ptree pt;
        try
        {
            property_tree::read_xml(is, pt);
        }
        catch (property_tree::xml_parser_error& exc)
        {
            cerr << "ERROR: Unable to read XML '" << filepath << "'. "
                 << exc.what() << endl;
            fb.close();
            return collection;
        }

        try
        {
            string seed = "";
            string extension =
                boost::filesystem::path(filepath).extension().string();
            if (extension == ".cc")
            {
                BOOST_FOREACH (const property_tree::ptree::value_type& e, pt)
                {
                    if (e.first == "ColorCorrection")
                    {
                        collection.push_back(readCC(e));
                    }
                }
            }
            else if (extension == ".ccc")
            {
                seed = "ColorCorrectionCollection";
            }
            else if (extension == ".cdl")
            {
                seed = "ColorDecisionList.ColorDecision";
            }
            else
            {
                cerr << "ERROR: Unknown CDL type: '" << filepath << "'" << endl;
                fb.close();
                return collection;
            }

            if (seed != "")
            {
                const property_tree::ptree& kids = pt.get_child(seed);
                BOOST_FOREACH (const property_tree::ptree::value_type& e, kids)
                {
                    if (e.first == "ColorCorrection")
                    {
                        collection.push_back(readCC(e));
                    }
                }
            }
        }
        catch (property_tree::ptree_bad_path& exc)
        {
            cerr << "ERROR: Malformed CDL: " << exc.what() << endl;
            fb.close();
            return collection;
        }
        fb.close();

        return collection;
    }

} // namespace CDL
