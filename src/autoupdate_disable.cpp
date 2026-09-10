#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <string>
#include "steam-tweak-tool/autoupdate_disable.hpp"
#include "steam-tweak-tool/utility.hpp"

using namespace std;

AutoUpdateDisabler::AutoUpdateDisabler() {}

bool AutoUpdateDisabler::checkUpdateBehaviour(const string &buffer)
{
    return buffer.find("AutoUpdateBehavior") != string::npos;
}

string AutoUpdateDisabler::replaceUpdateBehaviour()
{
    return "\t\"AutoUpdateBehavior\"\t\t\"1\"";
}

bool AutoUpdateDisabler::checkStateFlags(const string &buffer)
{
    return buffer.find("StateFlags") != string::npos;
}

string AutoUpdateDisabler::replaceStateFlags()
{
    return "\t\"StateFlags\"\t\t\"516\"";
}

bool AutoUpdateDisabler::iterateSteamApps(const string &steamAppsDirectory)
{
    int modifiedCount = 0;
    FileUtility fileUtility;

    // Reuse getAcfID + sortAcfID so appmanifest files are processed in sorted
    // app id order, consistent with option 3, and only appmanifests are touched.
    vector<int> acfIds = fileUtility.getAcfID(steamAppsDirectory);
    fileUtility.sortAcfID(acfIds);

    for (const int &id : acfIds)
    {
        string outPath = "appmanifest_" + to_string(id) + ".acf";
        string filePath = steamAppsDirectory + "/" + outPath;

        try
        {
            ifstream file(filePath);
            if (!file)
            {
                throw runtime_error("Failed to open file for reading: " + outPath);
            }
            stringstream buffer;
            string line;

            while (getline(file, line))
            {
                if (checkUpdateBehaviour(line))
                {
                    buffer << replaceUpdateBehaviour() << endl;
                }
                else if (checkStateFlags(line))
                {
                    buffer << replaceStateFlags() << endl;
                }
                else
                {
                    buffer << line << endl;
                }
            }

            file.close();

            // Temporarily remove the read-only attribute (if set) so the file
            // can be written, then restore it afterwards.
            bool wasReadOnly = fileUtility.isFileReadOnly(filePath);
            bool clearedReadOnly = wasReadOnly && fileUtility.setFileReadOnly(filePath, false);

            ofstream file_of(filePath);
            bool opened = static_cast<bool>(file_of);
            if (opened)
            {
                file_of << buffer.str();
                file_of.close();
            }

            if (clearedReadOnly)
            {
                if (!fileUtility.setFileReadOnly(filePath, true))
                {
                    cout << ">Warning: could not restore read-only attribute: " << outPath << endl;
                }
            }

            if (!opened)
            {
                throw runtime_error("Failed to open file for writing: " + outPath);
            }

            cout << ">Modified: " << outPath << endl;
            if (clearedReadOnly)
            {
                cout << ">Read-only attribute temporarily removed to allow modification, then restored for: " << outPath << endl;
            }
            modifiedCount++;
        }
        catch (const exception &e)
        {
            cout << ">Error: " << e.what() << endl;
            cout << ">Close Steam and try again" << endl;
        }
    }

    cout << ">Auto-update disabled for " << modifiedCount << " games" << endl;
    cout << endl;
    return true;
}
