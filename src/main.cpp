#include <Geode/Geode.hpp>
#include <Geode/Bindings.hpp>
#include <Geode/modify/CreatorLayer.hpp>

using namespace geode::prelude;

// Custom Class for your Language Selection Window
class LanguageSelectionPopup : public FLAlertLayer {
protected:
    bool init() {
        if (!FLAlertLayer::init(nullptr, "Languages", "", "Close", nullptr, 320.f, false, 220.f, 1.f)) {
            return false;
        }

        auto layer = cocos2d::CCLayer::create();
        this->addChild(layer);

        // Add an informative label inside the window
        auto label = cocos2d::CCLabelBMFont::create("Select a Custom Language:", "bigFont.fnt");
        label->setPosition({285, 210});
        label->setScale(0.5f);
        layer->addChild(label);

        // Create a menu grid layout area to add language selection buttons later
        auto menu = cocos2d::CCMenu::create();
        menu->setPosition({285, 140});
        layer->addChild(menu);

        return true;
    }

public:
    static LanguageSelectionPopup* create() {
        auto ret = new LanguageSelectionPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// Hooking into the Online Creator Layer menu
class $modify(MyCreatorLayer, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;

        // Locating the main button menu container grid
        auto menu = this->getChildByID("creator-buttons-menu");
        if (!menu) return true; 

        // Use a generic button sprite frame from the game texture files
        auto buttonSprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");

        auto langButton = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(MyCreatorLayer::onLanguageMenuClick)
        );

        langButton->setID("mpo-languages-button");
        menu->addChild(langButton);
        menu->updateLayout();

        return true;
    }

    void onLanguageMenuClick(CCObject* sender) {
        auto popup = LanguageSelectionPopup::create();
        if (popup) {
            popup->show();
        }
    }
};
