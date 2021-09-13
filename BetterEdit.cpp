#include "BetterEdit.hpp"
#include "tools/EditorLayerInput/LayerManager.hpp"
#include "tools/AutoSave/Backup/LevelBackupManager.hpp"
#include "tools/CustomKeybinds/KeybindManager.hpp"

using namespace gdmake;
using namespace gdmake::extra;
using namespace gd;
using namespace cocos2d;

log_stream g_obLogStream = log_stream();

bool DSdictHasKey(DS_Dictionary* dict, std::string const& key) {
    return dict->getKey(dict->getIndexOfKey(key.c_str())) == key;
}

log_stream::log_stream() {
    if (std::filesystem::exists(g_sLogfileDat)) {
        try {
            this->type = std::stoi(readFileString(g_sLogfileDat));
        } catch (...) {}
    }
    this->setType(this->type);
    writeFileString(g_sLogfile, "");
}

void log_stream::setType(int t) {
    this->type = t;
    writeFileString(g_sLogfileDat, std::to_string(t));
}

log_stream& log_stream::operator<<(log_end end) {
    if (!this->type)
        return *this;

    auto s = "[ " + timePointAsString(std::chrono::system_clock::now()) + "] " +
        this->output.str() + "\n";

    if (this->type & kLogTypeFile) {
        std::ofstream outfile;
        outfile.open(g_sLogfile, std::ios_base::app);
        outfile << s;
        outfile.close();
    }
    if (this->type & kLogTypeConsole) {
        std::cout << s;
    }

    this->output.str(std::string());

    return *this;
}

log_stream& BetterEdit::log() {
    return g_obLogStream;
}

#define BE_SAVE_Bool(_name_, _get_, _def_) \
    data->setBoolForKey(_name_, _get_ ^ _def_)
#define BE_SAVE_Float(_name_, _get_, _def_) \
    data->setFloatForKey(_name_, _get_)
#define BE_SAVE_Integer(_name_, _get_, _def_) \
    data->setIntegerForKey(_name_, _get_)

#define BE_LOAD_Bool(_name_, _get_, _def_) \
    set##_name_(_get_(#_name_) ^ _def_)
#define BE_LOAD_Integer(_name_, _get_, _def_) \
    set##_name_(_get_(#_name_))
#define BE_LOAD_Float(_name_, _get_, _def_) \
    set##_name_(_get_(#_name_))

#define BE_SAVE_SETTING(__name__, _, _d_, __ctype__, _0, _1, _2, _3) \
    BE_SAVE_##__ctype__##(#__name__, get##__name__(), _d_);
#define BE_LOAD_SETTING(__name__, _, _d_, __ctype__, _0, _1, _2, _3) \
    if (DSdictHasKey(data, #__name__)) \
        BE_LOAD_##__ctype__##(__name__, data->get##__ctype__##ForKey, _d_);
#define BE_DEFAULT_SETTING(__name__, _, __value__, __, ___, _0, _1, _2) \
    set##__name__(__value__);

BetterEdit* g_betterEdit;



bool BetterEdit::init() {
    this->m_pTemplateManager = TemplateManager::sharedState();
    this->m_sFileName = "BetterEdit.dat";

    if (!LayerManager::initGlobal())
        return false;

    // BE_SETTINGS(BE_DEFAULT_SETTING)

    this->setup();

    return true;
}

void BetterEdit::encodeDataTo(DS_Dictionary* data) {
    BetterEdit::log() << "Saving Settings" << log_end();
    STEP_SUBDICT(data, "settings",
        BE_SETTINGS(BE_SAVE_SETTING)
    );

    BetterEdit::log() << "Saving Bools" << log_end();
    STEP_SUBDICT(data, "bools",
        for (auto & [key, val] : m_mSaveBools)
            if (val.global)
                data->setBoolForKey(key.c_str(), *val.global);
    );

    BetterEdit::log() << "Saving Templates" << log_end();
    STEP_SUBDICT(data, "templates",
        m_pTemplateManager->encodeDataTo(data);
    );

    BetterEdit::log() << "Saving Editor Layers" << log_end();
    STEP_SUBDICT(data, "editor-layers",
        LayerManager::get()->encodeDataTo(data);
    );

    BetterEdit::log() << "Saving Presets" << log_end();
    STEP_SUBDICT(data, "presets",
        auto ix = 0u;
        for (auto preset : m_vPresets)
            STEP_SUBDICT(data, ("k" + std::to_string(ix)).c_str(),
                data->setStringForKey("name", preset.name);
                data->setStringForKey("data", preset.data);
                ix++;
            );
    );

    BetterEdit::log() << "Saving Favorites" << log_end();
    std::string favStr = "";
    for (auto fav : m_vFavorites)
        favStr += std::to_string(fav) + ",";
    data->setStringForKey("favorites", favStr);

    BetterEdit::log() << "Saving Backups" << log_end();
    LevelBackupManager::get()->save();

    BetterEdit::log() << "Saving Keybinds" << log_end();
    KeybindManager::get()->save();
}

void BetterEdit::dataLoaded(DS_Dictionary* data) {
    BetterEdit::log() << "Loading Settings" << log_end();
    STEP_SUBDICT_NC(data, "settings",
        BE_SETTINGS(BE_LOAD_SETTING)
    );

    BetterEdit::log() << "Loading Bools" << log_end();
    STEP_SUBDICT_NC(data, "bools",
        for (auto key : data->getAllKeys())
            m_mSaveBools[key] = { nullptr, data->getBoolForKey(key.c_str()) };
    );

    BetterEdit::log() << "Loading Templates" << log_end();
    STEP_SUBDICT_NC(data, "templates",
        m_pTemplateManager->dataLoaded(data);
    );

    BetterEdit::log() << "Loading Editor Layers" << log_end();
    STEP_SUBDICT_NC(data, "editor-layers",
        LayerManager::get()->dataLoaded(data);
    );

    BetterEdit::log() << "Loading Presets" << log_end();
    STEP_SUBDICT_NC(data, "presets",
        for (auto key : data->getAllKeys())
            STEP_SUBDICT_NC(data, key.c_str(),
                m_vPresets.push_back({
                    data->getStringForKey("name"),
                    data->getStringForKey("data")
                });
            );
    );

    BetterEdit::log() << "Loading Favorites" << log_end();
    for (auto fav : stringSplit(data->getStringForKey("favorites"), ","))
        try { this->addFavorite(std::stoi(fav)); } catch (...) {}
}

void BetterEdit::firstLoad() {}


BetterEdit* BetterEdit::sharedState() {
    return g_betterEdit;
}

bool BetterEdit::initGlobal() {
    g_betterEdit = new BetterEdit();

    if (g_betterEdit && g_betterEdit->init())
        return true;

    CC_SAFE_DELETE(g_betterEdit);
    return false;
}


void BetterEdit::showHookConflictMessage() {
    gd::FLAlertLayer::create(
        nullptr,
        "Hook Conflict Detected",
        "OK", nullptr,
        380.0f,
        "It appears that you have other <cp>mods</c> installed which "
        "are <cy>conflicting</c> with <cl>BetterEdit</c>.\n\n"

        "Mods that may have caused this include <co>GroupIDFilter</c>, "
        "<co>Global Clipboard</c>, and other editor mods.\n\n"

        "Please <cr>remove</c> or <cg>load</c> the mods at a different "
        "loading phase. Contact <cy>HJfod#1795</c> for help."
    )->show();
}

