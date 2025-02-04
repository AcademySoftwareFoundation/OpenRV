//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <assert.h>
#include <iostream>
#include <TwkApp/Menu.h>

namespace TwkApp
{
    using namespace std;

    Menu::StateFunc::StateFunc() {}

    Menu::StateFunc::~StateFunc() {}

    void Menu::Item::copyFrom(const Menu::Item* item)
    {
        assert(item);
        m_title = item->title();
        m_action = item->action() ? item->action()->copy() : 0;
        m_key = item->key();
        m_stateFunc = item->stateFunc() ? item->stateFunc()->copy() : 0;
        m_subMenu = item->subMenu() ? new Menu(item->subMenu()) : 0;
    }

    Menu::Menu(const std::string& name)
        : m_name(name)
    {
    }

    Menu::Menu(const Menu* menu)
    {
        if (menu)
        {
            m_name = menu->name();

            const Items& items = menu->items();

            for (int i = 0; i < items.size(); i++)
            {
                addItem(new Item(items[i]));
            }
        }
    }

    Menu::~Menu() { clear(); }

    Menu::Item* Menu::matchingItem(const Menu::Item* item)
    {
        if (item->title() != "_")
        {
            for (int i = 0; i < m_items.size(); i++)
            {
                if (m_items[i]->title() == item->title())
                {
                    return m_items[i];
                }
            }
        }

        return 0;
    }

    void Menu::merge(const Menu* menu)
    {
        if (!menu)
            return;
        const Items& items = menu->items();

        for (int i = 0; i < items.size(); i++)
        {
            Item* item = items[i];

            if (Item* m = matchingItem(item))
            {
                if (m->subMenu() && item->subMenu())
                //
                //  If existing item has a submenu, merge incoming submenu.
                //
                {
                    m->subMenu()->merge(item->subMenu());
                }
                else if (item->subMenu())
                //
                //  If the incoming item has a submenu, add one to the existing
                //  item
                //
                {
                    m->subMenu(new Menu(item->subMenu()));
                }
                else
                //
                //  Otherwise, copy new item over old one.
                //
                {
                    m->copyFrom(item);
                }
            }
            else
            {
                addItem(new Item(item));
            }
        }
    }

    void Menu::clear()
    {
        for (int i = 0; i < m_items.size(); ++i)
        {
            delete m_items[i];
        }

        m_items.clear();
    }

} // namespace TwkApp
