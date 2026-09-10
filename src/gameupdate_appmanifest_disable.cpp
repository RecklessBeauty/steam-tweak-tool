#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include "steam-tweak-tool/gameupdate_appmanifest_disable.hpp"
#include "steam-tweak-tool/utility.hpp"

using namespace std;
namespace fs = std::filesystem;

GameUpdateDisabler::GameUpdateDisabler() {}

// Collect every appmanifest_<id>.acf across all library folders, sorted by app id.
// Reuses FileUtility::getAcfID() and FileUtility::sortAcfID() from utility.cpp.
vector<pair<int, string>> GameUpdateDisabler::collectAppManifests(const vector<string> &libraryPaths)
{
    FileUtility fileUtility;
    vector<int> combinedAcfIds;

    for (const string &lib : libraryPaths)
    {
        string sAppsPath = lib + "/steamapps";
        if (fs::exists(sAppsPath))
        {
            try
            {
                vector<int> ids = fileUtility.getAcfID(sAppsPath);
                combinedAcfIds.insert(combinedAcfIds.end(), ids.begin(), ids.end());
            }
            catch (const exception &)
            {
                // Skip libraries that cannot be read
            }
        }
    }

    // sortAcfID sorts numerically and removes duplicates in place
    fileUtility.sortAcfID(combinedAcfIds);

    vector<pair<int, string>> manifests;
    for (const int &id : combinedAcfIds)
    {
        for (const string &lib : libraryPaths)
        {
            string candidate = lib + "/steamapps/appmanifest_" + to_string(id) + ".acf";
            if (fs::exists(candidate))
            {
                manifests.push_back(make_pair(id, candidate));
                break;
            }
        }
    }

    return manifests;
}

void GameUpdateDisabler::printManifestList(const vector<pair<int, string>> &manifests)
{
    FileUtility fileUtility;
    cout << ">Appmanifest files found:" << endl;
    int number = 1;
    for (const auto &manifest : manifests)
    {
        fs::path path(manifest.second);
        bool readOnly = fileUtility.isFileReadOnly(manifest.second);
        cout << ">" << number << ". ["
             << (readOnly ? "READ-ONLY" : "WRITABLE")
             << "] " << path.filename().string()
             << "  (" << path.parent_path().string() << ")" << endl;
        number++;
    }
    cout << ">0. Back to main menu" << endl;
}

void GameUpdateDisabler::run(const vector<string> &libraryPaths)
{
    FileUtility fileUtility;

    while (true)
    {
        vector<pair<int, string>> manifests = collectAppManifests(libraryPaths);
        if (manifests.empty())
        {
            cout << ">There are no games in your steamapps folders" << endl;
            return;
        }

        cout << endl;
        printManifestList(manifests);
        cout << ">Select the number of the appmanifest file: " << endl;
        cout << ">";
        string input;
        if (!getline(cin, input))
        {
            return;
        }
        cout << endl;

        if (input == "0")
        {
            return;
        }

        int choice = 0;
        try
        {
            choice = stoi(input);
        }
        catch (const exception &)
        {
            cout << ">Invalid input" << endl;
            continue;
        }

        if (choice < 1 || choice > static_cast<int>(manifests.size()))
        {
            cout << ">Invalid input" << endl;
            continue;
        }

        string filePath = manifests[choice - 1].second;
        string filename = fs::path(filePath).filename().string();

        bool currentlyReadOnly = fileUtility.isFileReadOnly(filePath);
        cout << ">" << filename << " is currently: "
             << (currentlyReadOnly ? "READ-ONLY" : "WRITABLE") << endl;
        cout << ">Enter 1 to disable game updates (set read-only)" << endl;
        cout << ">Enter 2 to enable game updates (remove read-only)" << endl;
        cout << ">Enter 0 to cancel" << endl;
        cout << ">";
        string action;
        if (!getline(cin, action))
        {
            continue;
        }
        cout << endl;

        if (action == "0")
        {
            cout << ">Cancelled" << endl;
        }
        else if (action == "1" || action == "2")
        {
            bool wantReadOnly = (action == "1");
            // Always re-apply the operation, even if the file already appears to
            // be in the requested state.
            if (fileUtility.setFileReadOnly(filePath, wantReadOnly))
            {
                bool nowReadOnly = fileUtility.isFileReadOnly(filePath);
                cout << ">" << filename << " is now "
                     << (nowReadOnly ? "READ-ONLY (game updates disabled)"
                                     : "WRITABLE (game updates enabled)") << endl;
            }
            else
            {
                cout << ">Error: could not change attributes for " << filename << endl;
                cout << ">Try running this tool as administrator" << endl;
            }
        }
        else
        {
            cout << ">Invalid input" << endl;
        }
        // Loop back: the list is shown again with fresh states.
    }
}
