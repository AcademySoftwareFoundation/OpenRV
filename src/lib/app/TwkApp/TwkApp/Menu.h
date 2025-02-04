//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__Menu__h__
#define __TwkApp__Menu__h__
#include <TwkApp/Action.h>
#include <string>
#include <vector>

namespace TwkApp
{

    //
    //  class Menu
    //
    //  Menu and Menu::Item are abstactions of some unknown UI toolkit
    //  menu. You can use a menu tree to represent things like the
    //  application menu in cocoa or a pop-up menu. In the Documnet class,
    //  for example, a document menu is defined. Its up to the UI toolkit
    //  implementation to make this menu and keep it working correctly.
    //

    class Menu
    {
    public:
        //
        //  A Menu item
        //

        class Item;

        class StateFunc
        {
        public:
            StateFunc();
            virtual ~StateFunc();
            virtual int state() = 0;
            virtual StateFunc* copy() const = 0;
            virtual bool error() const = 0;
        };

        class Item
        {
        public:
            Item(const std::string& title, Action* action,
                 const std::string& key = "", StateFunc* stateFunc = 0,
                 Menu* subMenu = 0)
                : m_title(title)
                , m_action(action)
                , m_key(key)
                , m_stateFunc(stateFunc)
                , m_subMenu(subMenu)
            {
            }

            Item(const std::string& title, Action* action, Menu* subMenu)
                : m_title(title)
                , m_action(action)
                , m_key("")
                , m_stateFunc(0)
                , m_subMenu(subMenu)
            {
            }

            ~Item()
            {
                delete m_action;
                delete m_subMenu;
                delete m_stateFunc;
            }

            Item(const Item* i) { copyFrom(i); }

            Item(const Item& i) { copyFrom(&i); }

            const std::string& title() const { return m_title; }

            std::string& title() { return m_title; }

            const Action* action() const { return m_action; }

            Action* action() { return m_action; }

            const std::string& key() const { return m_key; }

            std::string& key() { return m_key; }

            const StateFunc* stateFunc() const { return m_stateFunc; }

            StateFunc* stateFunc() { return m_stateFunc; }

            const Menu* subMenu() const { return m_subMenu; }

            Menu* subMenu() { return m_subMenu; }

            void subMenu(Menu* m) { m_subMenu = m; }

            void copyFrom(const Item*);

        private:
            std::string m_title;
            Action* m_action;
            std::string m_key;
            StateFunc* m_stateFunc;
            Menu* m_subMenu;
        };

        typedef std::vector<Item*> Items;

        Menu(const std::string& name);
        Menu(const Menu*);
        ~Menu();

        const std::string& name() const { return m_name; }

        const Items& items() const { return m_items; }

        void addItem(Item* i) { m_items.push_back(i); }

        void merge(const Menu*);

        void clear();

    private:
        Item* matchingItem(const Item*);

    private:
        std::string m_name;
        Items m_items;
    };

} // namespace TwkApp

#endif // __TwkApp__Menu__h__
