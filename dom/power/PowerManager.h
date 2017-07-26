/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef mozilla_dom_power_PowerManager_h
#define mozilla_dom_power_PowerManager_h

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIDOMWindow.h"
#include "nsWeakReference.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/MozPowerManagerBinding.h"

class nsPIDOMWindowInner;

namespace mozilla {
class ErrorResult;

namespace dom {

class PowerManager final : public nsIDOMMozWakeLockListener
                         , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(PowerManager)
  NS_DECL_NSIDOMMOZWAKELOCKLISTENER

  nsresult Init(nsPIDOMWindowInner* aWindow);
  nsresult Shutdown();

  static already_AddRefed<PowerManager> CreateInstance(nsPIDOMWindowInner*);

  // WebIDL
  nsPIDOMWindowInner* GetParentObject() const
  {
    return mWindow;
  }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;
  void AddWakeLockListener(nsIDOMMozWakeLockListener* aListener);
  void RemoveWakeLockListener(nsIDOMMozWakeLockListener* aListener);
  void GetWakeLockState(const nsAString& aTopic, nsAString& aState,
                        ErrorResult& aRv);

private:
  ~PowerManager() {}

  nsCOMPtr<nsPIDOMWindowInner> mWindow;
  nsTArray<nsCOMPtr<nsIDOMMozWakeLockListener> > mListeners;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_power_PowerManager_h
