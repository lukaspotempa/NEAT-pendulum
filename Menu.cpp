#include "Menu.hpp"
#include "Constants.hpp"

Menu::Menu(const sf::Font& font) : m_font(font) {
    m_background.setFillColor(Constants::COLOR_ZINC_800);
    m_background.setOutlineColor(Constants::COLOR_ZINC_600);
    m_background.setOutlineThickness(2.f);
    
    m_itemHighlight.setFillColor(sf::Color(Constants::COLOR_EMERALD_600.r, 
                                           Constants::COLOR_EMERALD_600.g,
                                           Constants::COLOR_EMERALD_600.b, 150));
}

void Menu::addItem(const std::string& label, std::function<void()> action) {
    MenuItem item;
    item.label = label;
    item.isToggle = false;
    item.action = action;
    m_items.push_back(item);
    updateLayout();
}

void Menu::addToggleItem(const std::string& label, bool initialState, std::function<void(bool)> action) {
    MenuItem item;
    item.label = label;
    item.isToggle = true;
    item.toggleState = initialState;
    item.toggleAction = action;
    m_items.push_back(item);
    updateLayout();
}

void Menu::handleKeyPressed(sf::Keyboard::Key key) {
    if (!m_visible) return;
    
    if (key == sf::Keyboard::Key::Up) {
        m_selectedIndex = (m_selectedIndex - 1 + static_cast<int>(m_items.size())) % static_cast<int>(m_items.size());
    }
    else if (key == sf::Keyboard::Key::Down) {
        m_selectedIndex = (m_selectedIndex + 1) % static_cast<int>(m_items.size());
    }
    else if (key == sf::Keyboard::Key::Enter) {
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
            auto& item = m_items[m_selectedIndex];
            if (item.isToggle) {
                item.toggleState = !item.toggleState;
                if (item.toggleAction) {
                    item.toggleAction(item.toggleState);
                }
            } else {
                if (item.action) {
                    item.action();
                }
            }
            updateLayout();
        }
    }
}

void Menu::update() {
    
}

void Menu::setToggleState(size_t index, bool state) {
    if (index < m_items.size() && m_items[index].isToggle) {
        m_items[index].toggleState = state;
        updateLayout();
    }
}

void Menu::updateLayout() {
    m_texts.clear();
    
    float totalHeight = m_padding * 2 + m_items.size() * m_itemHeight;
    
    m_background.setSize({ m_menuWidth, totalHeight });
    m_background.setPosition({
        (Constants::WINDOW_WIDTH - m_menuWidth) / 2.f,
        (Constants::WINDOW_HEIGHT - totalHeight) / 2.f
    });
    
    m_itemHighlight.setSize({ m_menuWidth - 10.f, m_itemHeight - 4.f });
    
    for (size_t i = 0; i < m_items.size(); ++i) {
        std::string displayLabel = m_items[i].label;
        if (m_items[i].isToggle) {
            displayLabel += m_items[i].toggleState ? "  [ON]" : "  [OFF]";
        }
        
        sf::Text text(m_font, displayLabel, 20);
        text.setFillColor(Constants::COLOR_ZINC_300);
        
        float x = m_background.getPosition().x + m_padding;
        float y = m_background.getPosition().y + m_padding + i * m_itemHeight + 5.f;
        text.setPosition({ x, y });
        
        m_texts.push_back(text);
    }
}

void Menu::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_visible) return;
    
    target.draw(m_background, states);
    
    // Draw highlight
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
        float x = m_background.getPosition().x + 5.f;
        float y = m_background.getPosition().y + m_padding + m_selectedIndex * m_itemHeight;
        m_itemHighlight.setPosition({ x, y });
        target.draw(m_itemHighlight, states);
    }
    
    // Draw text items
    for (const auto& text : m_texts) {
        target.draw(text, states);
    }
}
