#include "../BetterEdit.hpp"
#include <Geode/modify/EditorUI.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/EditButtonBar.hpp>
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

struct $modify(EditorUI) {
    CCNode* m_viewModeBtn;

    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel))
            return false;
        
        auto winSize = CCDirector::get()->getWinSize();
        
        // make a bit space for new style menu since the old one is a tiny bit cramped
        if (auto side = getChildOfType<CCSprite>(this, 1)) {
            side->setPositionX(96.f);
        }
        for (auto& child : CCArrayExt<CCNode>(m_pChildren)) {
            if (auto bar = typeinfo_cast<EditButtonBar*>(child)) {
                bar->setPositionX(bar->getPositionX() + 5.f);
            }
        }

        if (auto menu = this->getChildByID("toolbar-categories-menu")) {
            m_fields->m_viewModeBtn = CCMenuItemSpriteExtra::create(
                CCNode::create(), this, menu_selector(EditorUI::toggleMode)
            );
            m_fields->m_viewModeBtn->setID("view-button"_spr);
            m_fields->m_viewModeBtn->setTag(4);
            menu->addChild(m_fields->m_viewModeBtn);

            this->updateModeSprites();

            menu->setContentSize({ 90.f, 90.f });
            menu->setPositionX(47.f);
            menu->setLayout(RowLayout::create()
                ->setCrossAxisOverflow(false)
                ->setGrowCrossAxis(true)
            );
        }

        return true;
    }

    void toggleMode(CCObject* sender) {
        if (!sender) return;
        auto tag = sender->getTag();
        if (m_selectedMode != tag) {
            m_selectedMode = tag;
            this->resetUI();
            this->updateModeSprites();
        }
    }

    void updateModeSprite(CCNode* node, int tag, const char* spr) {
        // resetUI resets the button sprites so we may have to change them back 
        // to squarish ones again
        if (node) {
            auto bg = m_selectedMode == tag ? "GJ_button_02.png" : "GJ_button_01.png";
            auto btn = static_cast<CCMenuItemSpriteExtra*>(node);
            if (auto bspr = typeinfo_cast<ButtonSprite*>(btn->getNormalImage())) {
                // remove GD's texture
                bspr->setTexture(nullptr);
                bspr->setTextureRect({ 0, 0, 0, 0 });
                bspr->updateBGImage(bg);
            }
            else {
                btn->setNormalImage(ButtonSprite::create(
                    CCSprite::createWithSpriteFrameName(spr),
                    0x32, true, 0x32, bg, .6f
                ));
            }
        }
    }

    void updateModeSprites() {
        this->updateModeSprite(m_buildModeBtn, 2, "tab-create.png"_spr);
        this->updateModeSprite(m_deleteModeBtn, 1, "tab-delete.png"_spr);
        this->updateModeSprite(m_editModeBtn, 3, "tab-edit.png"_spr);
        this->updateModeSprite(m_fields->m_viewModeBtn, 4, "tab-view.png"_spr);
    }
};