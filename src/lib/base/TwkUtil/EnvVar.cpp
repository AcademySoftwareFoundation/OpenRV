///*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/
// WARNING : this library is NOT thread-safe

//==============================================================================
// EXTERNAL DECLARATIONS
//==============================================================================

#include <TwkUtil/EnvVar.h>

#include <assert.h>

namespace TwkUtil
{

    //==============================================================================
    // LOCAL CLASS EnvVarRegistryImp
    //==============================================================================

    //------------------------------------------------------------------------------
    //
    EnvVarRegistryImp::EnvVarRegistryImp() {}

    //------------------------------------------------------------------------------
    //
    EnvVarRegistryImp::~EnvVarRegistryImp() {}

    //------------------------------------------------------------------------------
    //
    EnvVarRegistryImp& EnvVarRegistryImp::getInstance()
    {
        // Note: the registry cannot be static since static
        //       uninit order is random and can cause
        //       crash at software exit. We create this
        //       on the heap and the object is leaked.
        static EnvVarRegistryImp* registry = NULL;
        if (!registry)
        {
            registry = new EnvVarRegistryImp();
        }
        return *registry;
    }

    //------------------------------------------------------------------------------
    //
    void EnvVarRegistryImp::add(BaseRegEnvVar* var)
    {
        _vars.push_back(var);
#if DEBUG_ENV_VAR_CORRUPTION
        const int numVars = _vars.size();
        printf("-----------------START-----------------------\n");
        for (int i = 0; i < numVars; i++)
        {
            BaseRegEnvVar* var2 = _vars[i];
            printf("%d VAR %s\n", i, var2->getName());
        }
        printf("-----------------END-----------------------\n");
#endif
    }

    //------------------------------------------------------------------------------
    //
    void EnvVarRegistryImp::remove(BaseRegEnvVar* var)
    {
        Container::iterator it = std::find(_vars.begin(), _vars.end(), var);
        if (it != _vars.end())
        {
            _vars.erase(it);
        }
    }

    int EnvVarRegistryImp::getNumElements() const
    {
        return static_cast<int>(_vars.size());
    }

    BaseRegEnvVar* EnvVarRegistryImp::getEnvVar(int index) const
    {
        assert(index < static_cast<int>(_vars.size()) && index >= 0);
        return _vars[index];
    }

    std::vector<int> EnvVarRegistryImp::search(const char* searchChr,
                                               int searchType, bool exact)
    {
        std::vector<int> foundList;
        const std::string searchStr = searchChr != NULL ? searchChr : "";
        std::string uppedSearchStr = searchStr;

        // Empty string? I don't think so
        if (searchStr.empty())
        {
            return foundList;
        }

        // We only need an uppercase version in non exact mode
        if (!exact)
        {
            // Lets make an uppercase version of our searchStr
            std::transform(searchStr.begin(), searchStr.end(),
                           uppedSearchStr.begin(),
                           EnvVarUtils::myToUpperFunctor);
        }

        char value[BaseEnvVar::MAX_STR_VALUE_LENGTH];
        for (unsigned int i = 0; i < _vars.size(); i++)
        {
            getFieldValueAsStr(value, searchType, i);
            std::string fieldValue = value;
            if (!exact)
            {
                // Lets make an uppercase version of our name
                std::transform(fieldValue.begin(), fieldValue.end(),
                               fieldValue.begin(),
                               EnvVarUtils::myToUpperFunctor);
            }
            if (
                // non exact search, search for substrings, case insensitive
                (!exact
                 && (fieldValue.find(uppedSearchStr) != std::string::npos))
                ||

                // exact search, look for perfect match, case sensitive
                (fieldValue == searchStr))
            {
                foundList.push_back(i);
            }
        }
        return foundList;
    }

    //------------------------------------------------------------------------------
    //
    void EnvVarRegistryImp::getFieldValueAsStr(char* val, int searchType,
                                               int envVarNum) const
    {
        if (!val)
            return;

        BaseRegEnvVar* var = _vars[envVarNum];

        val[0] = '\0';
        if (!var)
            return;

        switch (searchType)
        {
        case BaseEnvVar::EV_NAME:
            snprintf(val, BaseEnvVar::MAX_STR_VALUE_LENGTH, "%s",
                     var->getName());
            break;
        case BaseEnvVar::EV_VALUE:
            var->getValueAsStr(val, BaseEnvVar::MAX_STR_VALUE_LENGTH);
            break;
        case BaseEnvVar::EV_DEF_VALUE:
            var->getDefaultValueAsStr(val, BaseEnvVar::MAX_STR_VALUE_LENGTH);
            break;
        case BaseEnvVar::EV_FILE_NAME:
            snprintf(val, BaseEnvVar::MAX_STR_VALUE_LENGTH, "%s",
                     var->getFileName());
            break;
        default:
            // unknown type assert
            assert(0);
        }
    }

    //------------------------------------------------------------------------------
    //
    inline void EnvVarRegistryImp::dump(std::ostream& os) const
    {
        for (Container::const_iterator it = _vars.begin(); it != _vars.end();
             ++it)
        {

            using namespace std;

            BaseRegEnvVar* var = *it;

            os << endl;
            os << *var;
        }
    }

    //------------------------------------------------------------------------------
    //
    void EnvVarRegistryImp::dumpHtml(std::ostream& os) const
    {

        using namespace std;

        // Beginning of html file.
        os << "<html>" << endl
           << "<body>" << endl
           << "<table border=1>" << endl
           << "<tr>" << endl
           << "<th colspan=3>" << "Environnement Variables" << "</th>" << endl
           << "</tr>" << endl
           << "<tr>" << endl
           << "<td align=center>" << "<font size=+1>" << "Name" << "</font>"
           << "</td>"
           << "<td align=center>" << "<font size=+1>" << "Value" << "</font>"
           << "</td>"
           << "<td align=center>" << "<font size=+1>" << "File" << "</font>"
           << "</td>" << endl
           << "</tr>" << endl;

        // body of html file.
        for (Container::const_iterator it = _vars.begin(); it != _vars.end();
             ++it)
        {

            using namespace std;

            os << "<tr>" << endl
               << "<td align=left>" << (*it)->getName() << "</td>";
            char value[BaseEnvVar::MAX_STR_VALUE_LENGTH] = "(null)";
            (*it)->getValueAsStr(value, BaseEnvVar::MAX_STR_VALUE_LENGTH);
            os << "<td align=center>" << value << "</td>"
               << "<td align=left>" << (*it)->getFileName() << "</td>" << endl
               << "</tr>" << endl;
        }

        // Ending of html file.
        os << "</table>" << endl << "</body>" << endl << "</html>";
    }

} // end namespace TwkUtil
