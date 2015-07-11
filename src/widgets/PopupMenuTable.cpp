/**********************************************************************

Audacity: A Digital Audio Editor

TrackControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../Audacity.h"
#include "PopupMenuTable.h"

PopupMenuTable::Menu::~Menu()
{
   Disconnect();
}

void PopupMenuTable::Menu::Extend(PopupMenuTable *pTable)
{
   for (const PopupMenuTable::Entry *pEntry = &*pTable->Get().begin();
      pEntry->IsValid(); ++pEntry) {
      switch (pEntry->type) {
      case PopupMenuTable::Entry::Item:
      {
         this->Append(pEntry->id, pEntry->caption());
         this->pParent->Connect
            (pEntry->id, wxEVT_COMMAND_MENU_SELECTED,
            pEntry->func, NULL, pTable);
         break;
      }
      case PopupMenuTable::Entry::RadioItem:
      {
         this->AppendRadioItem(pEntry->id, pEntry->caption());
         this->pParent->Connect
            (pEntry->id, wxEVT_COMMAND_MENU_SELECTED,
            pEntry->func, NULL, pTable);
         break;
      }
      case PopupMenuTable::Entry::CheckItem:
      {
         this->AppendCheckItem(pEntry->id, pEntry->caption());
         this->pParent->Connect
            (pEntry->id, wxEVT_COMMAND_MENU_SELECTED,
            pEntry->func, NULL, pTable);
         break;
      }
      case PopupMenuTable::Entry::Separator:
         this->AppendSeparator();
         break;
      case PopupMenuTable::Entry::SubMenu:
      {
         PopupMenuTable *const subTable = pEntry->subTable;
         wxMenu *subMenu = BuildMenu(this->pParent, subTable, pUserData);
         this->AppendSubMenu(subMenu, pEntry->caption());
      }
      default:
         break;
      }
   }

   pTable->InitMenu(this, pUserData);
}

void PopupMenuTable::Menu::DisconnectTable(PopupMenuTable *pTable)
{
   for (const PopupMenuTable::Entry *pEntry = &*pTable->Get().begin();
      pEntry->IsValid(); ++pEntry) {
      if (pEntry->IsItem())
         pParent->Disconnect(pEntry->id, wxEVT_COMMAND_MENU_SELECTED,
         pEntry->func, NULL, pTable);
      else if (pEntry->IsSubMenu())
         // recur
         DisconnectTable(pEntry->subTable);
   }

   pTable->DestroyMenu();
}

void PopupMenuTable::Menu::Disconnect()
{
   std::vector<PopupMenuTable*>::const_iterator it = tables.begin(), end = tables.end();
   for (; it != end; ++it) {
      PopupMenuTable *pTable = *it;
      DisconnectTable(pTable);
   }
}

// static
PopupMenuTable::Menu *PopupMenuTable::BuildMenu
(wxEvtHandler *pParent, PopupMenuTable *pTable, void *pUserData)
{
   // Rebuild as needed each time.  That makes it safe in case of language change.
   Menu *const theMenu = new Menu(pParent, pUserData);
   theMenu->Extend(pTable);
   return theMenu;
}
