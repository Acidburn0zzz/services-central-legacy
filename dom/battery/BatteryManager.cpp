/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mounir Lamouri <mounir.lamouri@mozilla.com> (Original Author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "BatteryManager.h"
#include "nsIDOMClassInfo.h"
#include "Constants.h"

DOMCI_DATA(BatteryManager, mozilla::dom::battery::BatteryManager)

namespace mozilla {
namespace dom {
namespace battery {

NS_IMPL_CYCLE_COLLECTION_CLASS(BatteryManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BatteryManager,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnLevelChangeListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnChargingChangeListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BatteryManager,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnLevelChangeListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnChargingChangeListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BatteryManager)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBatteryManager)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BatteryManager)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BatteryManager, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BatteryManager, nsDOMEventTargetHelper)

BatteryManager::BatteryManager()
  : mLevel(kDefaultLevel)
  , mCharging(kDefaultCharging)
{
}

BatteryManager::~BatteryManager()
{
  if (mListenerManager) {
    mListenerManager->Disconnect();
  }
}

NS_IMETHODIMP
BatteryManager::GetCharging(bool* aCharging)
{
  *aCharging = mCharging;

  return NS_OK;
}

NS_IMETHODIMP
BatteryManager::GetLevel(float* aLevel)
{
  *aLevel = mLevel;

  return NS_OK;
}

NS_IMETHODIMP
BatteryManager::GetOnlevelchange(nsIDOMEventListener** aOnlevelchange)
{
  return GetInnerEventListener(mOnLevelChangeListener, aOnlevelchange);
}

NS_IMETHODIMP
BatteryManager::SetOnlevelchange(nsIDOMEventListener* aOnlevelchange)
{
  return RemoveAddEventListener(NS_LITERAL_STRING("levelchange"),
                                mOnLevelChangeListener, aOnlevelchange);
}

NS_IMETHODIMP
BatteryManager::GetOnchargingchange(nsIDOMEventListener** aOnchargingchange)
{
  return GetInnerEventListener(mOnChargingChangeListener, aOnchargingchange);
}

NS_IMETHODIMP
BatteryManager::SetOnchargingchange(nsIDOMEventListener* aOnchargingchange)
{
  return RemoveAddEventListener(NS_LITERAL_STRING("chargingchange"),
                                mOnChargingChangeListener, aOnchargingchange);
}

} // namespace battery
} // namespace dom
} // namespace mozilla
