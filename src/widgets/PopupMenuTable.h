/**********************************************************************

Audacity: A Digital Audio Editor

PopupMenuTable.h

Paul Licameli

This file defines PopupMenuTable, which inherits from wxEventHandler,

associated macros simplifying the population of tables,

and PopupMenuTable::Menu which is buildable from one or more such
tables, and automatically attaches and detaches the event handlers.

**********************************************************************/

#ifndef __AUDACITY_POPUP_MENU_TABLE__
#define __AUDACITY_POPUP_MENU_TABLE__

class wxCommandEvent;
class wxString;
#include <vector>
#include <wx/event.h>
#include <wx/menu.h>

#include "../TranslatableStringArray.h"

class PopupMenuTable;

struct PopupMenuTableEntry
{
   enum Type { Item, RadioItem, CheckItem, Separator, SubMenu, Invalid };

   Type type;
   int id;
   wxString caption_;
   wxObjectEventFunction func;
   PopupMenuTable *subTable;

   PopupMenuTableEntry(Type type_, int id_, wxString caption__,
      wxObjectEventFunction func_, PopupMenuTable *subTable_)
      : type(type_)
      , id(id_)
      , caption_(caption__)
      , func(func_)
      , subTable(subTable_)
   {}
   wxString caption() const { return wxGetTranslation(caption_); }

   bool IsItem() const { return type == Item || type == RadioItem || type == CheckItem; }
   bool IsSubMenu() const { return type == SubMenu; }
   bool IsValid() const { return type != Invalid; }
};

class PopupMenuTable : public TranslatableArray< std::vector<PopupMenuTableEntry> >
{
public:
   typedef PopupMenuTableEntry Entry;

   class Menu
      : public wxMenu
   {
      friend class PopupMenuTable;

      Menu(wxEvtHandler *pParent_, void *pUserData_)
         : pParent(pParent_), tables(), pUserData(pUserData_) {}

   public:
      virtual ~Menu();

      void Extend(PopupMenuTable *pTable);
      void DisconnectTable(PopupMenuTable *pTable);
      void Disconnect();

   private:
      wxEvtHandler *pParent;
      std::vector<PopupMenuTable*> tables;
      void *pUserData;
   };

   // Called when the menu is about to pop up.
   // Your chance to enable and disable items.
   virtual void InitMenu(Menu *pMenu, void *pUserData) = 0;

   // Called when menu is destroyed.
   virtual void DestroyMenu() = 0;

   // Optional pUserData gets passed to the InitMenu routines of tables.
   // No memory management responsibility is assumed by this function.
   static Menu *BuildMenu
      (wxEvtHandler *pParent, PopupMenuTable *pTable, void *pUserData = NULL);
};

/*
The following macros make it easy to attach a popup menu to a window.

Exmple of usage:

In class MyHandler (maybe in the private section),
which inherits from PopupMenuHandler,

DECLARE_POPUP_MENU(MyHandler);
virtual void InitMenu(Menu *pMenu, void *pUserData);
virtual void DestroyMenu();

Then in MyHandler.cpp,

void MyHandler::InitMenu(Menu *pMenu, void *pUserData)
{
// Remember pUserData, enable or disable menu items
}

void MyHandler::DestroyMenu()
{
// Do cleanup
}

BEGIN_POPUP_MENU(MyHandler)
POPUP_MENU_ITEM(OnCutSelectedTextID,     _("Cu&t"),          OnCutSelectedText)
// etc.
END_POPUP_MENU()

where OnCutSelectedText is a (maybe private) member function of MyHandler.

Elswhere,

MyHandler myHandler;
std::unique_ptr<wxMenu> pMenu(PopupMenuTable::BuildMenu(pParent, &myHandler));

// Optionally:
OtherHandler otherHandler;
pMenu->Extend(&otherHandler);

pParent->PopupMenu(pMenu.get(), event.m_x, event.m_y);

That's all!
*/

#define DECLARE_POPUP_MENU(HandlerClass) \
      virtual void Populate();

#define BEGIN_POPUP_MENU(HandlerClass) \
void HandlerClass::Populate() { \
   typedef HandlerClass My; \

#define POPUP_MENU_ITEM(id, string, memFn) \
   mContents.push_back( \
      Entry(Entry::Item, (id), (string), ((wxObjectEventFunction)(wxEventFunction) \
                      (wxCommandEventFunction)(&My::memFn)), NULL));

#define POPUP_MENU_RADIO_ITEM(id, string, memFn) \
   mContents.push_back( \
      Entry(Entry::RadioItem, (id), (string), ((wxObjectEventFunction)(wxEventFunction) \
                      (wxCommandEventFunction)(&My::memFn)), NULL));

#define POPUP_MENU_CHECK_ITEM(id, string, memFn) \
   mContents.push_back( \
      Entry(Entry::CheckItem, (id), (string), ((wxObjectEventFunction)(wxEventFunction) \
                      (wxCommandEventFunction)(&My::memFn)), NULL));

// classname names a class that derives from MenuTable and defines Instance()
#define POPUP_MENU_SUB_MENU(id, string, classname) \
   mContents.push_back( \
      Entry(Entry::SubMenu, (id), (string), NULL, &classname::Instance()));

#define POPUP_MENU_SEPARATOR() \
   mContents.push_back( \
      Entry(Entry::Separator, -1, wxT(""), NULL, NULL));

#define END_POPUP_MENU() \
   mContents.push_back( \
      Entry(Entry::Invalid, -1, wxT(""), NULL, NULL)); \
} \

#endif
