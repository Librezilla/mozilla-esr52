/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Hal.h"

#include <sensorsapi.h>
#include <sensors.h>
#include <portabledevicetypes.h>

#define MEAN_GRAVITY 9.80665
#define DEFAULT_SENSOR_POLL 100

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

static RefPtr<ISensor> sAccelerometer;

class SensorEvent final : public ISensorEvents {
public:
  SensorEvent() : mCount(0) {
  }

  // IUnknown interface

  STDMETHODIMP_(ULONG) AddRef() {
    return InterlockedIncrement(&mCount);
  }

  STDMETHODIMP_(ULONG) Release() {
    ULONG count = InterlockedDecrement(&mCount);
    if (!count) {
      delete this;
      return 0;
    }
    return count;
  }

  STDMETHODIMP QueryInterface(REFIID iid, void** ppv) {
    if (iid == IID_IUnknown) {
      *ppv = static_cast<IUnknown*>(this);
    } else if (iid == IID_ISensorEvents) {
      *ppv = static_cast<ISensorEvents*>(this);
    } else {
      return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
  }

  // ISensorEvents interface

  STDMETHODIMP OnEvent(ISensor *aSensor, REFGUID aId, IPortableDeviceValues *aData) {
    return S_OK;
  }

  STDMETHODIMP OnLeave(REFSENSOR_ID aId) {
    return S_OK;
  }

  STDMETHODIMP OnStateChanged(ISensor *aSensor, SensorState state) {
    return S_OK;
  }

  STDMETHODIMP OnDataUpdated(ISensor *aSensor, ISensorDataReport *aReport) {
    PROPVARIANT v;
    HRESULT hr;
    InfallibleTArray<float> values;

    // X-axis acceleration in g's
    hr = aReport->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_X_G, &v);
    if (FAILED(hr)) {
      return hr;
    }
    values.AppendElement(float(-v.dblVal * MEAN_GRAVITY));

    // Y-axis acceleration in g's
    hr = aReport->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_Y_G, &v);
    if (FAILED(hr)) {
      return hr;
    }
    values.AppendElement(float(-v.dblVal * MEAN_GRAVITY));

    // Z-axis acceleration in g's
    hr = aReport->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_Z_G, &v);
    if (FAILED(hr)) {
      return hr;
    }
    values.AppendElement(float(-v.dblVal * MEAN_GRAVITY));

    return S_OK;
  }

private:
  ULONG mCount;
};

void
EnableSensorNotifications(SensorType aSensor)
{
    return;
}

void
DisableSensorNotifications(SensorType aSensor)
{
}

} // hal_impl
} // mozilla
