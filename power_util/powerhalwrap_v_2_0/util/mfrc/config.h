#include <stdlib.h>

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <mutex>
#include <atomic>
#include <unistd.h>
#include <sys/stat.h>

namespace MTK::MFRC {

class WL {
public:

    WL(char const* const* paths) {
        load_config(paths);
    };

    const std::string &get(const std::string& cat, const std::string& key) {
        static std::string empty = "";

        auto& vstr = gets(cat, key);
        return vstr.size() ? vstr.back() : empty;
    }
    const std::vector<std::string> &gets(const std::string& cat, const std::string& key) {
        std::lock_guard<std::mutex> config_lock(config_mutex);
        auto& vstr = config[cat][key];

        return vstr;
    }

    const char *get_setting(const char* category, const char* key) {

        return get(category? category : "", key? key : "").c_str();
    }

protected:

    std::mutex config_mutex;
    std::map<std::string, std::map<std::string, std::vector<std::string> > > config;

private:

    enum CFG_TOKEN_TYPE {
        IGNORE,
        CATEGORY,
        KEYVALUE,
    };

    class CFG_TOKEN {
    public:
        CFG_TOKEN_TYPE type = IGNORE;
        char str[128] {};
        char val[128] {};
    };

    void load_config(char const* const* paths);

    CFG_TOKEN process_line(const std::string&);
};

static const char *trim(char* line) {
    char *l = line;

    while (*l == ' ' || *l == '\t') l++;

    for (int i = strlen(l); i-- > 0;) {
        switch (l[i]) {
        case ' ': case '\n': case '\r': case '\t':
            l[i] = 0;
            break;
        default:
            goto go_next;
        }
    }
go_next:

    int len = strlen(l);

    for (int i = 0; i <= len; ++i)
        line[i] = l[i];
    return line;
}

struct load_guard
{
    std::mutex        mutex;
    std::atomic<int>  loaded;
};

static load_guard wl_load_guard;

void WL::load_config(const char * const *paths) {
    std::lock_guard<std::mutex> config_lock(config_mutex);

    const char * const*p = paths;
    std::fstream fs;

    do {
        fs.open(*p, std::fstream::in);
    } while (!fs.is_open() && *(++p) != 0);

    if (!fs.is_open()) {
        return;
    }

    char line[256];
    char category[256] = "default";

    while (fs.getline(line, 256)) {
        CFG_TOKEN tok = process_line(trim(line));

        switch (tok.type) {
        case CATEGORY:
            if(snprintf(category, sizeof(category), "%s", tok.str) < 0) ALOGE("config.h snprintf error: %d", __LINE__);
            break;
        case KEYVALUE:
            config[category][tok.str].push_back(tok.val);
            break;
        case IGNORE:
        default:
            /* do nothing */
            break;
        }
    }
    fs.close();

}

WL::CFG_TOKEN WL::process_line(const std::string& str) {
    CFG_TOKEN ret;
    const char *l = str.c_str();
    int len = strlen(l);

    if (l[0] == '[' && l[len-1] == ']') {

        ret.type = CATEGORY;
        if(snprintf(ret.str, sizeof(ret.str), "%s", l+1) < 0) ALOGE("config.h snprintf error: %d", __LINE__);
        ret.str[len-2 >= 0? len-2: 0] = 0;

    } else if (isalpha(l[0]) || l[0] == '/') {
        ret.type = KEYVALUE;
        auto sign_equal = str.find("=");

        if (sign_equal != std::string::npos) {
            if(snprintf(ret.str, sizeof(ret.str), "%s", str.substr(0, sign_equal).c_str()) < 0) ALOGE("config.h snprintf error: %d", __LINE__);;
            trim(ret.str);
            if(snprintf(ret.val, sizeof(ret.val), "%s", str.substr(sign_equal+1).c_str()) < 0) ALOGE("config.h snprintf error: %d", __LINE__);
            trim(ret.val);
        } else {
            if(snprintf(ret.str, sizeof(ret.str), "%s", str.c_str()) < 0) ALOGE("config.h snprintf error: %d", __LINE__);
            trim(ret.str);
            if(snprintf(ret.val, sizeof(ret.val), "1") < 0) ALOGE("config.h snprintf error: %d", __LINE__);
        }
    }
    return ret;
}


} // namespace MTK::MFRC
