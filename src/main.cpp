#include <Geode/Geode.hpp>
#include <Geode/Bindings.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <fstream>
#include <vector>

using namespace geode::prelude;

// Forward declarations of UI classes
class LanguageManagerPopup;
class NewLanguagePopup;
class TranslationEditorPopup;

// Structure to track languages dynamically loaded from file headers
struct LanguageMetadata {
    std::string filename; // e.g. "es.json"
    std::string englishName;
    std::string localName;
    std::string twoLetterId;
};

// ==========================================
// DATA ENGINE UTILITIES
// ==========================================
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
                    auto data = matjson::parse(file);
                    
                    LanguageMetadata meta;
                    meta.filename = entry.path().filename().string();
                    meta.englishName = data["lang-name-en"].asString().unwrap_or("Unknown");
                    meta.localName = data["lang-name-local"].asString().unwrap_or("Unknown");
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

// ==========================================
// POPUP 3: THE TRANSLATION KEY/VALUE EDITOR
// ==========================================
class TranslationEditorPopup : public FLAlertLayer, public TextInputDelegate {
protected:
    LanguageMetadata m_meta;
    std::string m_currentKey = "online-daily-level-button"; // Default editing item template
    CCTextInputNode* m_inputField = nullptr;
    CCLabelBMFont::create* m_displayLabel = nullptr;

    bool init(LanguageMetadata meta) {
        m_meta = meta;
        if (!FLAlertLayer::init(nullptr, m_meta.englishName.c_str(), "", "Save & Close", nullptr, 360.f, false, 240.f, 1.f)) {
            return false;
        }

        auto layer = CCLayer::create();
        this->addChild(layer);

        // Header Indicator
        auto keyLabel = CCLabelBMFont::create(("Editing Key: " + m_currentKey).c_str(), "goldFont.fnt");
        keyLabel->setPosition({285, 220});
        keyLabel->setScale(0.5f);
        layer->addChild(keyLabel);

        // Input Field BG
        auto inputBg = CCScale9Sprite::create("square02_small.png");
        inputBg->setContentSize({260, 40});
        inputBg->setPosition({285, 160});
        inputBg->setOpacity(100);
        layer->addChild(inputBg);

        // Interactive Text Input Controller
        m_inputField = CCTextInputNode::create(240.f, 30.f, "Enter Translation...", "bigFont.fnt");
        m_inputField->setPosition({285, 160});
        m_inputField->setDelegate(this);
        layer->addChild(m_inputField);

        // Quick Selector Row
        auto menu = CCMenu::create();
        menu->setPosition({285, 100});
        layer->addChild(menu);

        auto testBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Next Key", "goldFont.fnt", "GJ_button_01.png"),
            this,
            menu_selector(TranslationEditorPopup::onNextKey)
        );
        menu->addChild(testBtn);

        return true;
    }

    void onNextKey(CCObject*) {
        // Core functionality to cycle fields dynamically can be scale-mapped here
        if (m_inputField) {
            auto text = m_inputField->getString();
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

public:
    static TranslationEditorPopup* create(LanguageMetadata meta) {
        auto ret = new TranslationEditorPopup();
        if (ret && ret->init(meta)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ==========================================
// POPUP 2: CREATION PROFILE OVERLAY
// ==========================================
class NewLanguagePopup : public FLAlertLayer, public TextInputDelegate {
protected:
    CCTextInputNode* m_enInput;
    CCTextInputNode* m_locInput;
    CCTextInputNode* m_idInput;
    LanguageManagerPopup* m_parent;

    bool init(LanguageManagerPopup* parent) {
        m_parent = parent;
        if (!FLAlertLayer::init(nullptr, "Create Language", "", "Cancel", nullptr, 320.f, false, 260.f, 1.f)) {
            return false;
        }

        auto layer = CCLayer::create();
        this->addChild(layer);

        // English Name Field Setup
        m_enInput = CCTextInputNode::create(200.f, 30.f, "English Name (e.g. French)", "chatFont.fnt");
        m_enInput->setPosition({285, 210});
        layer->addChild(m_enInput);

        // Local Name Field Setup
        m_locInput = CCTextInputNode::create(200.f, 30.f, "Local Name (e.g. Français)", "chatFont.fnt");
        m_locInput->setPosition({285, 170});
        layer->addChild(m_locInput);

        // Two-letter code field setup
        m_idInput = CCTextInputNode::create(200.f, 30.f, "Two-Letter ID (e.g. fr)", "chatFont.fnt");
        m_idInput->setPosition({285, 130});
        layer->addChild(m_idInput);

        // Confirmation Button
        auto menu = CCMenu::create();
        menu->setPosition({285, 80});
        layer->addChild(menu);

        auto confirmBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Confirm", "goldFont.fnt", "GJ_button_01.png"),
            this,
            menu_selector(NewLanguagePopup::onConfirm)
        );
        menu->addChild(confirmBtn);

        return true;
    }

    void onConfirm(CCObject*) {
        std::string en = m_enInput->getString();
        std::string loc = m_locInput->getString();
        std::string id = m_idInput->getString();

        if(en.empty() || loc.empty() || id.empty()) {
            FLAlertLayer::create("Error", "All parameter inputs are required!", "OK")->show();
            return;
        }

        LanguageEngine::createNewLanguageFile(en, loc, id);
        
        // Success routing callback
        this->onClose(nullptr);
        
        LanguageMetadata newMeta = { id + ".json", en, loc, id };
        TranslationEditorPopup::create(newMeta)->show();
    }

public:
    static NewLanguagePopup* create(LanguageManagerPopup* parent) {
        auto ret = new NewLanguagePopup();
        if (ret && ret->init(parent)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ==========================================
// POPUP 1: ROOT LANGUAGE DIRECTORY MANAGER
// ==========================================
class LanguageManagerPopup : public FLAlertLayer {
protected:
    CCMenu* m_listMenu = nullptr;
    std::vector<LanguageMetadata> m_cachedLanguages;
    int m_selectedIndex = -1;

    bool init() {
        if (!FLAlertLayer::init(nullptr, "Language Manager", "", "Close", nullptr, 400.f, false, 280.f, 1.f)) {
            return false;
        }

        auto layer = CCLayer::create();
        this->addChild(layer);

        // Dynamic Menu Column Anchor Frame
        m_listMenu = CCMenu::create();
        m_listMenu->setPosition({220, 140});
        layer->addChild(m_listMenu);

        // Action Toolbar Side Anchor Menu Container
        auto sideMenu = CCMenu::create();
        sideMenu->setPosition({420, 140});
        layer->addChild(sideMenu);

        // "New" Button initialization
        auto newBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("New", "goldFont.fnt", "GJ_button_05.png"),
            this,
            menu_selector(LanguageManagerPopup::onNewLanguageClick)
        );
        sideMenu->addChild(newBtn);

        // "Edit Translation" Button initialization
        auto editBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Edit Text", "goldFont.fnt", "GJ_button_01.png"),
            this,
            menu_selector(LanguageManagerPopup::onEditTranslationClick)
        );
        editBtn->setPosition({0, -50});
        sideMenu->addChild(editBtn);

        refreshList();
        return true;
    }

    void refreshList() {
        m_listMenu->removeAllChildren();
        m_cachedLanguages = LanguageEngine::getAllLanguages();

        float yOffset = 80.0f;
for (size_t i = 0; i < m_cachedLanguages.size(); ++i)
	{
		std::string labelText = m_cachedLanguages[i].englishName + " (" + m_cachedLanguages[i].localName + ")";auto itemBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create(labelText.c_str(), "bigFont.fnt", "GJ_button_02.png"),this,menu_selector(LanguageManagerPopup::onSelectRow));itemBtn->setTag(static_cast(i));itemBtn->setPosition({0, yOffset});m_listMenu->addChild(itemBtn);yOffset -= 40.0f;}}void onSelectRow(CCObject* sender) {m_selectedIndex = sender->getTag();FLAlertLayer::create("Selected", ("Selected Profile: " + m_cachedLanguages[m_selectedIndex].englishName).c_str(), "OK")->show();}void onNewLanguageClick(CCObject*) {NewLanguagePopup::create(this)->show();}void onEditTranslationClick(CCObject*) {if (m_selectedIndex == -1) {FLAlertLayer::create("Selection Required", "Please click an item from the list layout first!", "OK")->show();return;}TranslationEditorPopup::create(m_cachedLanguages[m_selectedIndex])->show();}public:static LanguageManagerPopup* create() {auto ret = new LanguageManagerPopup();if (ret && ret->init()) {ret->autorelease();return ret;
	}
	CC_SAFE_DELETE(ret);return nullptr;}};// ==========================================// CORE INTERACTION HOOK// ==========================================
// ==========================================
// CORE INTERACTION HOOK - STREAMLINED FIX
// ==========================================
class $modify(MyCreatorLayer, CreatorLayer) {
    bool init() {
        // 1. Initialize the base game layer first
        if (!CreatorLayer::init()) return false;

        // 2. Locate the main grid button container layout
        auto menu = this->getChildByID("creator-buttons-menu");
        if (!menu) return true; // Safety check

        // 3. Find the Versus button to use as our layout reference point
        auto versusBtn = menu->getChildByID("versus-button");

        // 4. Create a bounding base container to hold our text/icon vertical stack
        auto buttonContainer = cocos2d::CCNode::create();
        buttonContainer->setContentSize({ 50.f, 60.f }); // Uniform tile slot dimensions
        buttonContainer->setAnchorPoint({ 0.5f, 0.5f });

        // 5. Load your high-resolution 512x512 logo-transparent.png artwork
        auto iconSprite = cocos2d::CCSprite::create("logo-transparent.png");
        if (!iconSprite) {
            iconSprite = cocos2d::CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        }
        
        iconSprite->setScale(0.0878925f); 
        iconSprite->setPosition({ 25.f, 35.f }); // Centers the icon in the upper half of the tile
        buttonContainer->addChild(iconSprite);

        // 6. Generate the bottom text label in the official gold menu font
        auto textLabel = cocos2d::CCLabelBMFont::create("Languages", "goldFont.fnt");
        textLabel->setPosition({ 25.f, 5.f }); 
        textLabel->setScale(0.42f); 
        buttonContainer->addChild(textLabel);

        // 7. Wrap the composite layout container into an interactive, bouncy button
        auto langButton = CCMenuItemSpriteExtra::create(
            buttonContainer,
            this,
            menu_selector(MyCreatorLayer::onLanguageMenuClick)
        );
        langButton->setID("mpo-languages-editor-button");

        // 8. Insert our new button dynamically into the layout array safely
        if (versusBtn) {
            int versusIndex = menu->getChildren()->indexOfObject(versusBtn);
            menu->addChild(langButton);
            menu->reorderChild(langButton, versusIndex + 1);
        } else {
            menu->addChild(langButton);
        }

        // 9. Force the grid to recalculate its spacing metrics pixel-perfectly
        menu->updateLayout();

        // 10. DYNAMICALLY APPLY LANGUAGE OVERRIDES SAFELY USING GEODE IDS
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

