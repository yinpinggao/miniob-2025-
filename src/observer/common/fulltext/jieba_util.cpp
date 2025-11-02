#include "common/fulltext/jieba_util.h"
#include "common/log/log.h"
#include "cppjieba/Jieba.hpp"
#include "common/utils/private_accessor.h"
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <unordered_set>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

IMPLEMENT_GET_PRIVATE_VAR(KeywordExtractor, cppjieba::KeywordExtractor, stopWords_, std::unordered_set<std::string>)

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
        try {
            // Use default constructor - let cppjieba find dicts using __FILE__ mechanism
            // This is the same approach as official miniob code
            LOG_INFO("Initializing Jieba with default constructor (__FILE__ mechanism)");
            
            // Redirect stderr to suppress limonp errors that cause abort()
            int old_stderr = dup(STDERR_FILENO);
            int null_fd = open("/dev/null", O_WRONLY);
            if (null_fd >= 0) {
                dup2(null_fd, STDERR_FILENO);
                close(null_fd);
            }
            
            // Initialize Jieba with default constructor (uses __FILE__ to locate dicts)
            jieba_ = new cppjieba::Jieba();
            
            // Restore stderr
            if (old_stderr >= 0) {
                dup2(old_stderr, STDERR_FILENO);
                close(old_stderr);
            }
            
            LOG_INFO("Jieba initialized successfully");
            initialized_ = true;
            
            // Load stop words from cppjieba's extractor
            load_stop_words_from_jieba();
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
    
    void load_stop_words_from_jieba() {
        // cppjieba's extractor already has stop words loaded via default constructor
        LOG_INFO("Using cppjieba's built-in stop words");
    }
    
    RC tokenize(const string &text, vector<string> &tokens) {
        tokens.clear();
        if (text.empty()) return RC::SUCCESS;
        if (!initialized_ || !jieba_) {
            LOG_WARN("Jieba not initialized");
            return RC::INTERNAL;
        }
        try {
            // Use Cut() instead of CutForSearch() - same as official code
            jieba_->Cut(text, tokens);
            
            // Remove stop words - same as official code
            auto &stopWords_ = *GET_PRIVATE(cppjieba::KeywordExtractor, &jieba_->extractor, KeywordExtractor, stopWords_);
            tokens.erase(std::remove_if(tokens.begin(),
                                        tokens.end(),
                                        [&stopWords_](const std::string &word) { 
                                            return stopWords_.find(word) != stopWords_.end(); 
                                        }),
                        tokens.end());
            
            return RC::SUCCESS;
        } catch (const std::exception &e) {
            LOG_ERROR("Tokenization failed: %s", e.what());
            return RC::INTERNAL;
        }
    }
private:
    cppjieba::Jieba *jieba_ = nullptr;
    bool initialized_ = false;
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
