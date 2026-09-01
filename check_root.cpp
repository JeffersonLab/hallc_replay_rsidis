#include <iostream>
#include <filesystem>
#include <fstream>
#include <TFile.h>
#include <TTree.h>
#include <map>
#include <vector>
#include <string>

namespace fs = std::filesystem;

struct ReplayCheck {
    std::vector<std::string> trees;
    std::map<std::string, std::vector<std::string>> branches;
};

// -------------------- utility helpers --------------------
bool make_read_only(const fs::path& file_path) {
    try {
        fs::permissions(file_path,
            fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read,
            fs::perm_options::replace);
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error making file read-only: " << e.what() << std::endl;
        return false;
    }
}

bool is_file_open(const fs::path& file_path) {
    std::ifstream file(file_path, std::ios::in);
    return !file.is_open();
}

// -------------------- core ROOT validation --------------------
bool check_root_file(const std::string& file_path, const ReplayCheck& check) {
    TFile file(file_path.c_str(), "READ");
    if (file.IsZombie()) {
        std::cerr << "Error: File is a zombie!" << std::endl;
        return false;
    }

    for (const auto& tree_name : check.trees) {
        TTree* tree = nullptr;
        file.GetObject(tree_name.c_str(), tree);
        if (!tree) {
            std::cerr << "Error: TTree '" << tree_name << "' not found!" << std::endl;
            return false;
        }

        auto it = check.branches.find(tree_name);
        if (it != check.branches.end()) {
            for (const auto& branch : it->second) {
                if (!tree->GetBranch(branch.c_str())) {
                    std::cerr << "Error: Branch '" << branch
                              << "' missing in TTree '" << tree_name << "'!" << std::endl;
                    return false;
                }
            }
        }
    }

    file.Close();
    return true;
}

// -------------------- mode definitions --------------------
ReplayCheck get_replay_check(const std::string& mode) {
    ReplayCheck rc;

    if (mode == "COIN_PROD") {
        rc.trees = {"E", "T", "TSP", "TSH", "TSHelP", "TSHelH"};
        rc.branches = {
 	    {"E", {"evnum", "ecHMS_Angle","ecSHMS_Angle"}},
            {"T", {"H.cal.etottracknorm", "H.gtr.dp","P.cal.etottracknorm", "P.gtr.dp"}},
            {"TSP", {"evNumber"}},
            {"TSH", {"H.pEL_HI.scaler", "H.pEL_HI.scalerRate"}},
            {"TSHelP", {"P.BCM1_Hel.scaler", "P.BCM1_Hel.scalerRate"}},
            {"TSHelH", {"H.BCM1_Hel.scaler", "H.BCM1_Hel.scalerRate"}}
        };
    }
    else if (mode == "HMS_PROD") {
        rc.trees = {"E", "T", "TSH"};
        rc.branches = {
            {"E", {"evnum", "ecHMS_Angle"}},
            {"T", {"H.cal.etottracknorm", "H.gtr.dp"}},
            {"TSH", {"H.pEL_HI.scaler", "H.pEL_HI.scalerRate"}}
        };
    }
    else if (mode == "SHMS_PROD") {
        rc.trees = {"E", "T", "TSP"};
        rc.branches = {
            {"E", {"evnum", "ecSHMS_Angle"}},
            {"T", {"P.cal.etottracknorm", "P.gtr.dp"}},
            {"TSP", {"P.pEL_HI.scaler", "P.pEL_HI.scalerRate"}}
        };
    }
    else if (mode == "HEEP_PROD") {
        rc.trees = {"E", "T", "TSP", "TSH"};
        rc.branches = {
 	    {"E", {"evnum", "ecHMS_Angle","ecSHMS_Angle"}},
            {"T", {"H.cal.etottracknorm", "H.gtr.dp","P.cal.etottracknorm", "P.gtr.dp"}},
            {"TSP", {"evNumber"}},
            {"TSH", {"H.pEL_HI.scaler", "H.pEL_HI.scalerRate"}},
            {"TSHelP", {"P.BCM1_Hel.scaler", "P.BCM1_Hel.scalerRate"}},
            {"TSHelH", {"H.BCM1_Hel.scaler", "H.BCM1_Hel.scalerRate"}}
        };
    }
    else {
        throw std::runtime_error("Unknown replay mode: " + mode);
    }

    return rc;
}

// -------------------- main --------------------
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: check_root <output_directory> <mode>" << std::endl;
        return 6;
    }

    fs::path output_dir = argv[1];
    std::string mode = argv[2];
    fs::path root_file_path;
    int root_file_count = 0;

    for (const auto& entry : fs::directory_iterator(output_dir)) {
        if (entry.path().extension() == ".root") {
            root_file_path = entry.path();
            ++root_file_count;
        }
    }

    if (root_file_count == 0) {
        std::cerr << "Error: No .root file found in " << output_dir << std::endl;
        return 7;
    } else if (root_file_count > 1) {
        std::cerr << "Error: Multiple .root files found in " << output_dir << std::endl;
        return 8;
    }

    if (is_file_open(root_file_path)) {
        std::cerr << "Error: File " << root_file_path << " is currently open." << std::endl;
        return 10;
    }

    if (!make_read_only(root_file_path)) {
        return 11;
    }

    ReplayCheck check;
    try {
        check = get_replay_check(mode);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 12;
    }

    if (!check_root_file(root_file_path.string(), check)) {
        fs::path volatile_path = "/volatile/hallc/c-rsidis/cmorean/failed_jobs" / root_file_path.filename();
        try {
            fs::copy_file(root_file_path, volatile_path, fs::copy_options::overwrite_existing);
            if (!fs::exists(volatile_path)) {
                std::cerr << "Error: File copy operation failed." << std::endl;
                return 9;
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error copying file: " << e.what() << std::endl;
            return 9;
        }
    }

    std::cout << "File checked successfully for mode " << mode << std::endl;
    return 0;
}


// Do the following to compile in tch shell
//g++ -O3 -std=c++17 check_root.cpp -o check_root `root-config --cflags --libs`
