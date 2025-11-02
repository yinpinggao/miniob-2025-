#include "common/fulltext/jieba_util.h"
#include "common/log/log.h"
#include "cppjieba/Jieba.hpp"
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <unordered_set>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

// Get the directory where the executable is located
static string get_executable_dir() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        string exe_path(path);
        size_t last_slash = exe_path.find_last_of('/');
        if (last_slash != string::npos) {
            return exe_path.substr(0, last_slash);
        }
    }
    return "";
}

static string get_dict_path(const char* filename) {
    // Priority 1: Environment variable
    const char* dict_dir = std::getenv("JIEBA_DICT_DIR");
    if (dict_dir) {
        string path = string(dict_dir) + "/" + filename;
        std::ifstream test_file(path);
        if (test_file.good()) return path;
    }
    
    // Priority 2: Try paths relative to executable location
    string exe_dir = get_executable_dir();
    if (!exe_dir.empty()) {
        const char* exe_relative_paths[] = {
            "/../dict/",                                      // Relative to bin/
            "/../../deps/3rd/usr/local/dict/",               // Standard install location
            "/../../deps/3rd/usr/local/share/cppjieba/dict/",
            "/../../deps/3rd/cppjieba/dict/",                // Source location
            NULL
        };
        for (int i = 0; exe_relative_paths[i] != NULL; i++) {
            string test_path = exe_dir + exe_relative_paths[i] + "jieba.dict.utf8";
            std::ifstream test_file(test_path);
            if (test_file.good()) {
                return exe_dir + exe_relative_paths[i] + filename;
            }
        }
    }
    
    // Priority 3: Try standard paths relative to current directory
    const char* possible_paths[] = {
        "deps/3rd/usr/local/dict/",                      // From project root - OFFICIAL STANDARD PATH
        "deps/3rd/usr/local/share/cppjieba/dict/",       // From project root (actual install)
        "deps/3rd/cppjieba/dict/",                       // From project root (source location)
        "../dict/",                                      // Relative to bin/ (created by build.sh)
        "../../deps/3rd/usr/local/dict/",               // From bin/ - standard install
        "../../deps/3rd/usr/local/share/cppjieba/dict/",
        "../../deps/3rd/cppjieba/dict/",
        "../../../deps/3rd/usr/local/dict/",
        "../../../deps/3rd/usr/local/share/cppjieba/dict/",
        "../../../deps/3rd/cppjieba/dict/",
        "./dict/",
        "/root/miniob/deps/3rd/cppjieba/dict/",
        NULL
    };
    
    for (int i = 0; possible_paths[i] != NULL; i++) {
        string test_path = string(possible_paths[i]) + "jieba.dict.utf8";
        std::ifstream test_file(test_path);
        if (test_file.good()) {
            return string(possible_paths[i]) + filename;
        }
    }
    
    // If all fail, log error and return default
    LOG_WARN("Cannot find Jieba dictionary files, using default path: ../dict/");
    return string("../dict/") + filename;
}

class JiebaUtil::Impl {
public:
    Impl() {
        // Verify all dictionary files exist before initializing
        const char* dict_files[] = {
            "jieba.dict.utf8",
            "hmm_model.utf8",
            "user.dict.utf8",
            "idf.utf8",
            "stop_words.utf8",
            NULL
        };
        
        bool all_dicts_exist = true;
        for (int i = 0; dict_files[i] != NULL; i++) {
            string dict_path = get_dict_path(dict_files[i]);
            std::ifstream test_file(dict_path);
            if (!test_file.good()) {
                LOG_ERROR("Dictionary file not found: %s", dict_path.c_str());
                all_dicts_exist = false;
            }
        }
        
        if (!all_dicts_exist) {
            LOG_ERROR("Not all dictionary files found, Jieba initialization failed");
            jieba_ = nullptr;
            initialized_ = false;
            return;
        }
        
        try {
            string jieba_dict = get_dict_path("jieba.dict.utf8");
            string hmm_model = get_dict_path("hmm_model.utf8");
            string user_dict = get_dict_path("user.dict.utf8");
            string idf_dict = get_dict_path("idf.utf8");
            string stop_words = get_dict_path("stop_words.utf8");
            
            LOG_INFO("Initializing Jieba with dicts:");
            LOG_INFO("  jieba.dict: %s", jieba_dict.c_str());
            LOG_INFO("  hmm_model: %s", hmm_model.c_str());
            LOG_INFO("  user.dict: %s", user_dict.c_str());
            LOG_INFO("  idf: %s", idf_dict.c_str());
            LOG_INFO("  stop_words: %s", stop_words.c_str());
            
            // Redirect stderr to suppress limonp errors that cause abort()
            int old_stderr = dup(STDERR_FILENO);
            int null_fd = open("/dev/null", O_WRONLY);
            if (null_fd >= 0) {
                dup2(null_fd, STDERR_FILENO);
                close(null_fd);
            }
            
            // Initialize Jieba (may trigger abort() in limonp)
            jieba_ = new cppjieba::Jieba(jieba_dict, hmm_model, user_dict, idf_dict, stop_words);
            
            // Restore stderr
            if (old_stderr >= 0) {
                dup2(old_stderr, STDERR_FILENO);
                close(old_stderr);
            }
            
            LOG_INFO("Jieba initialized successfully");
            initialized_ = true;
            
            // Load stop words
            load_stop_words();
        } catch (const std::exception &e) {
            LOG_ERROR("Failed to initialize Jieba: %s", e.what());
            if (jieba_) {
                delete jieba_;
                jieba_ = nullptr;
            }
            initialized_ = false;
        } catch (...) {
            LOG_ERROR("Failed to initialize Jieba: unknown exception");
            if (jieba_) {
                delete jieba_;
                jieba_ = nullptr;
            }
            initialized_ = false;
        }
    }
    ~Impl() { 
        if (jieba_) {
            try {
                delete jieba_;
            } catch (...) {
                // Ignore exceptions during cleanup
            }
        }
    }
    
    void load_stop_words() {
        std::ifstream ifs(get_dict_path("stop_words.utf8"));
        if (!ifs.is_open()) {
            LOG_WARN("Failed to open stop_words file");
            return;
        }
        std::string line;
        while (std::getline(ifs, line)) {
            if (!line.empty()) {
                stop_words_.insert(line);
            }
        }
        ifs.close();
        LOG_INFO("Loaded %zu stop words", stop_words_.size());
    }
    
    bool is_stop_word(const string &word) const {
        return stop_words_.find(word) != stop_words_.end();
    }
    
    RC tokenize(const string &text, vector<string> &tokens) {
        tokens.clear();
        if (text.empty()) return RC::SUCCESS;
        if (!initialized_ || !jieba_) {
            LOG_WARN("Jieba not initialized");
            return RC::INTERNAL;
        }
        try {
            vector<string> all_tokens;
            jieba_->CutForSearch(text, all_tokens);
            
            // Step 1: Filter stop words and collect unique tokens in order
            vector<string> ordered_tokens;
            std::unordered_set<string> seen;
            for (const auto &token : all_tokens) {
                if (!is_stop_word(token) && !token.empty() && seen.find(token) == seen.end()) {
                    ordered_tokens.push_back(token);
                    seen.insert(token);
                }
            }
            
            // Step 2: Remove tokens that are substrings of other tokens
            // Create a copy sorted by length (descending) for efficient substring checking
            vector<string> sorted_by_length = ordered_tokens;
            std::sort(sorted_by_length.begin(), sorted_by_length.end(), 
                     [](const string &a, const string &b) {
                         return a.length() > b.length();
                     });
            
            // Find all tokens that should be kept (not substrings of longer tokens)
            std::unordered_set<string> tokens_to_keep;
            for (const auto &token : sorted_by_length) {
                bool is_substring_of_kept = false;
                // Check if this token is a substring of any already-kept longer token
                for (const auto &kept_token : tokens_to_keep) {
                    // Since sorted_by_length is sorted by length descending,
                    // kept_token is always >= token in length
                    if (kept_token.length() > token.length() && 
                        kept_token.find(token) != string::npos) {
                        is_substring_of_kept = true;
                        break;
                    }
                }
                if (!is_substring_of_kept) {
                    tokens_to_keep.insert(token);
                }
            }
            
            // Step 3: Output in original order, only keeping selected tokens
            for (const auto &token : ordered_tokens) {
                if (tokens_to_keep.find(token) != tokens_to_keep.end()) {
                    tokens.push_back(token);
                }
            }
            
            return RC::SUCCESS;
        } catch (const std::exception &e) {
            LOG_ERROR("Tokenization failed: %s", e.what());
            return RC::INTERNAL;
        }
    }
private:
    cppjieba::Jieba *jieba_ = nullptr;
    bool initialized_ = false;
    std::unordered_set<string> stop_words_;  // Stop words set
};

JiebaUtil::JiebaUtil() : impl_(new Impl()) {}
JiebaUtil::~JiebaUtil() { delete impl_; }
JiebaUtil &JiebaUtil::instance() { static JiebaUtil instance; return instance; }
RC JiebaUtil::tokenize(const string &text, vector<string> &tokens) {
    return impl_->tokenize(text, tokens);
}
string JiebaUtil::format_tokens_as_json(const vector<string> &tokens) {
    if (tokens.empty()) return "[]";
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "\"" << tokens[i] << "\"";
    }
    oss << "]";
    return oss.str();
}
