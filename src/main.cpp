#include <Geode/Geode.hpp>
#include <Geode/Bindings.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <fstream>
#include <vector>
using namespace geode::prelude;
struct LanguageMetadata {
    std::string filename;
    std::string englishName;
    std::string localName;
    std::string twoLetterId;
};
std::string getCustomTranslation(const std::string& key);
namespace LanguageEngine {
    static std::vector<LanguageMetadata> getAllLanguages() {
        std::vector<LanguageMetadata> list;
        auto configDir = Mod::get()->getConfigDir();
        if (!ghc::filesystem::exists(configDir)) {
            ghc::filesystem::create_directories(configDir);
        }
        for (auto& entry : ghc::filesystem::directory_iterator(configDir)) {
            if (entry.path().extension() == ".json") {
              try {
                std::ifstream file(entry.path());
                    
                // Natively unwraps the matjson result wrapper directly into 'data'!
                  GEODE_UNWRAP_INTO(auto data, matjson::parse(file));
                    
                    LanguageMetadata meta;
                    meta.filename = entry.path().filename().string();
                    
                    auto engVal = data["lang-name-en"];
                    meta.englishName = engVal.isString() ? engVal.asString().value() : "Unknown";
                    
                    auto locVal = data["lang-name-local"];
                    meta.localName = locVal.isString() ? locVal.asString().value() : "Unknown";
                    
                    meta.twoLetterId = entry.path().stem().string();
                    list.push_back(meta);
                } catch(...) {}
            }
        }
        return list;
    }
    static void createNewLanguageFile(const std::string& enName, const std::string& locName, const std::string& id) {
        auto configDir = Mod::get()->getConfigDir();
        auto jsonPath = configDir / (id + ".json");
        matjson::Value defaultData;
        defaultData["lang-name-en"] = enName;
        defaultData["lang-name-local"] = locName;
        matjson::Value keysObj;
        keysObj["online-create-button"] = "Create";
        keysObj["online-daily-level-button"] = "Daily";
        keysObj["online-gauntlet-button"] = "Gauntlets";
        defaultData["keys"] = keysObj;
        std::ofstream file(jsonPath);
        file << defaultData.dump(matjson::TAB_INDENTATION);
    }
}
class TranslationEditorPopup : public FLAlertLayer, public TextInputDelegate {
protected:
    LanguageMetadata m_meta;
    std::string m_currentKey = "online-daily-level-button";
    CCTextInputNode* m_inputField = nullptr;
    bool init(LanguageMetadata meta) {
        if (!FLAlertLayer::init(nullptr, meta.englishName.c_str(), "", "Save & Close", nullptr, 360.f, false, 240.f, 1.f)) return false;
        m_meta = meta;
        auto layer = CCLayer::create();
        this->addChild(layer);
        auto keyLabel = CCLabelBMFont::create(("Editing Key: " + m_currentKey).c_str(), "goldFont.fnt");
        keyLabel->setPosition({285, 220});
        keyLabel->setScale(0.5f);
        layer->addChild(keyLabel);
        auto inputBg = CCScale9Sprite::create("square02_small.png");
        inputBg->setContentSize({260, 40});
        inputBg->setPosition({285, 160});
        inputBg->setOpacity(100);
        layer->addChild(inputBg);
        m_inputField = CCTextInputNode::create(240.f, 30.f, "Enter Translation...", "bigFont.fnt");
        m_inputField->setPosition({285, 160});
        m_inputField->setDelegate(this);
        layer->addChild(m_inputField);
        auto menu = CCMenu::create();
        menu->setPosition({285, 100});
        layer->addChild(menu);
        auto testBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("Next Key", "goldFont.fnt", "GJ_button_01.png"), this, menu_selector(TranslationEditorPopup::onNextKey));
        menu->addChild(testBtn);
        return true;
    }
    void onNextKey(CCObject*);
public:
    static TranslationEditorPopup* create(LanguageMetadata meta) {
        auto ret = new TranslationEditorPopup();
        if (ret && ret->init(meta)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};
class LanguageManagerPopup : public FLAlertLayer {
protected:
    CCMenu* m_listMenu = nullptr;
    std::vector<LanguageMetadata> m_cachedLanguages;
    int m_selectedIndex = -1;
    bool init() {
        if (!FLAlertLayer::init(nullptr, "Language Manager", "", "Close", nullptr, 400.f, false, 280.f, 1.f)) return false;
        auto layer = CCLayer::create();
        this->addChild(layer);
        m_listMenu = CCMenu::create();
        m_listMenu->setPosition({220, 140});
        layer->addChild(m_listMenu);
        auto sideMenu = CCMenu::create();
        sideMenu->setPosition({420, 140});
        layer->addChild(sideMenu);
        auto newBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("New", "goldFont.fnt", "GJ_button_05.png"), this, menu_selector(LanguageManagerPopup::onNewLanguageClick));
        sideMenu->addChild(newBtn);
        auto editBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("Edit Text", "goldFont.fnt", "GJ_button_01.png"), this, menu_selector(LanguageManagerPopup::onEditTranslationClick));
        editBtn->setPosition({0, -50});
        sideMenu->addChild(editBtn);
        refreshList();
        return true;
    }
    void refreshList();
    void onSelectRow(CCObject* sender);
    void onNewLanguageClick(CCObject*);
    void onEditTranslationClick(CCObject*);
public:
    static LanguageManagerPopup* create() {
        auto ret = new LanguageManagerPopup();
        if (ret && ret->init()) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};
class NewLanguagePopup : public FLAlertLayer, public TextInputDelegate {
protected:
    CCTextInputNode* m_enInput;
    CCTextInputNode* m_locInput;
    CCTextInputNode* m_idInput;
    LanguageManagerPopup* m_parent;
    bool init(LanguageManagerPopup* parent) {
        if (!FLAlertLayer::init(nullptr, "Create Language", "", "Cancel", nullptr, 320.f, false, 260.f, 1.f)) return false;
        m_parent = parent;
        auto layer = CCLayer::create();
        this->addChild(layer);
        m_enInput = CCTextInputNode::create(200.f, 30.f, "English Name", "chatFont.fnt");
        m_enInput->setPosition({285, 210});
        layer->addChild(m_enInput);
        m_locInput = CCTextInputNode::create(200.f, 30.f, "Local Name", "chatFont.fnt");
        m_locInput->setPosition({285, 170});
        layer->addChild(m_locInput);
        m_idInput = CCTextInputNode::create(200.f, 30.f, "Two-Letter ID", "chatFont.fnt");
        m_idInput->setPosition({285, 130});
        layer->addChild(m_idInput);
        auto menu = CCMenu::create();
        menu->setPosition({285, 80});
        layer->addChild(menu);
        auto confirmBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("Confirm", "goldFont.fnt", "GJ_button_01.png"), this, menu_selector(NewLanguagePopup::onConfirm));
        menu->addChild(confirmBtn);
        return true;
    }
    void onConfirm(CCObject*);
public:
    static NewLanguagePopup* create(LanguageManagerPopup* parent) {
        auto ret = new NewLanguagePopup();
        if (ret && ret->init(parent)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};
void TranslationEditorPopup::onNextKey(CCObject*) {
    if (m_inputField) {
        // FIXED: Explicitly convert the text field input to a primitive string to satisfy matjson
        std::string text = m_inputField->getString();
        if (!text.empty()) {
            auto configDir = Mod::get()->getConfigDir();
            auto path = configDir / m_meta.filename;
            try {
                std::ifstream readFile(path);
                auto data = matjson::parse(readFile);
                data["keys"][m_currentKey] = text;
                std::ofstream writeFile(path);
                writeFile << data.dump(matjson::TAB_INDENTATION);
                FLAlertLayer::create("Success", "Translation saved locally!", "OK")->show();
            } catch(...) {}
        }
    }
}
void LanguageManagerPopup::refreshList() {
    m_listMenu->removeAllChildren();
    m_cachedLanguages = LanguageEngine::getAllLanguages();
    float yOffset = 80.0f;
    for (size_t i = 0; i < m_cachedLanguages.size(); ++i) {
        std::string labelText = m_cachedLanguages[i].englishName + " (" + m_cachedLanguages[i].localName + ")";
        auto itemBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create(labelText.c_str(), "bigFont.fnt", "GJ_button_02.png"), this, menu_selector(LanguageManagerPopup::onSelectRow));
        itemBtn->setTag(static_cast<int>(i));
        itemBtn->setPosition({0, yOffset});
        m_listMenu->addChild(itemBtn);
        yOffset -= 40.0f;
    }
}
void LanguageManagerPopup::onSelectRow(CCObject* sender) {
    m_selectedIndex = sender->getTag();
    FLAlertLayer::create("Selected", ("Selected Profile: " + m_cachedLanguages[m_selectedIndex].englishName).c_str(), "OK")->show();
}
void LanguageManagerPopup::onNewLanguageClick(CCObject*) {
    NewLanguagePopup::create(this)->show();
}
void LanguageManagerPopup::onEditTranslationClick(CCObject*) {
    if (m_selectedIndex == -1) {
        FLAlertLayer::create("Selection Required", "Please click an item from the list layout first!", "OK")->show();
        return;
    }
    TranslationEditorPopup::create(m_cachedLanguages[m_selectedIndex])->show();
}
void NewLanguagePopup::onConfirm(CCObject*) {
    // FIXED: Unpacked directly to standard primitives to clear template constraints completely
    std::string en = m_enInput->getString();
    std::string loc = m_locInput->getString();
    std::string id = m_idInput->getString();
    if(en.empty() || loc.empty() || id.empty()) {
        FLAlertLayer::create("Error", "All parameter inputs are required!", "OK")->show();
        return;
    }
    LanguageEngine::createNewLanguageFile(en, loc, id);
    this->onClose(nullptr);
    LanguageMetadata newMeta = { id + ".json", en, loc, id };
    TranslationEditorPopup::create(newMeta)->show();
}
std::string getCustomTranslation(const std::string& key) {
    ghc::filesystem::path configDir = Mod::get()->getConfigDir();
    ghc::filesystem::path jsonPath = configDir / "en.json";
    try {
        for (auto& entry : ghc::filesystem::directory_iterator(configDir)) {
            if (entry.path().extension() == ".json") {
                jsonPath = entry.path();
                break;
            }
        }
        if (!ghc::filesystem::exists(jsonPath)) {
            return key;
        }
        std::ifstream file(jsonPath);
        
        // Clean macro unwrapping block
        GEODE_UNWRAP_INTO(auto data, matjson::parse(file));
        
        if (data.contains("keys") && data["keys"].contains(key)) {
            auto val = data["keys"][key];
            if (val.isString()) {
                return val.asString().value();
            }
        }
    } catch (...) {
        return key;
    }
    return key;
}

class $modify(MyCreatorLayer, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;
        auto menu = this->getChildByID("creator-buttons-menu");
        if (!menu) return true;
        auto versusBtn = menu->getChildByID("versus-button");
        auto buttonContainer = cocos2d::CCNode::create();
        buttonContainer->setContentSize({ 50.f, 60.f });
        buttonContainer->setAnchorPoint({ 0.5f, 0.5f });
        auto iconSprite = cocos2d::CCSprite::create("logo-transparent.png");
        if (!iconSprite) {
            iconSprite = cocos2d::CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        }
        iconSprite->setScale(0.0878925f);
        iconSprite->setPosition({ 25.f, 35.f });
        buttonContainer->addChild(iconSprite);
        auto textLabel = cocos2d::CCLabelBMFont::create("Languages", "bigFont.fnt");
        textLabel->setPosition({ 25.f, 5.f });
        textLabel->setScale(0.42f);
        buttonContainer->addChild(textLabel);
        auto langButton = CCMenuItemSpriteExtra::create(buttonContainer, this, menu_selector(MyCreatorLayer::onLanguageMenuClick));
        langButton->setID("mpo-languages-editor-button");
        if (versusBtn) {
            int versusIndex = menu->getChildren()->indexOfObject(versusBtn);
            menu->addChild(langButton);
            menu->reorderChild(langButton, versusIndex + 1);
        } else {
            menu->addChild(langButton);
        }
        menu->updateLayout();
        if (auto dailyBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(menu->getChildByID("daily-level-button"))) {
            if (auto label = typeinfo_cast<CCLabelBMFont*>(dailyBtn->getChildByIDRecursive("label"))) {
                label->setString(getCustomTranslation("online-daily-level-button").c_str());
            }
        }
        if (auto gauntletBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(menu->getChildByID("gauntlet-button"))) {
            if (auto label = typeinfo_cast<CCLabelBMFont*>(gauntletBtn->getChildByIDRecursive("label"))) {
                label->setString(getCustomTranslation("online-gauntlet-button").c_str());
            }
        }
        return true;
    }
    void onLanguageMenuClick(cocos2d::CCObject* sender) {
        LanguageManagerPopup::create()->show();
    }
};