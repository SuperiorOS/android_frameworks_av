/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_COMPANION_VIRTUALCAMERA_VIRTUALCAMERACAPTURERESULT_H
#define ANDROID_COMPANION_VIRTUALCAMERA_VIRTUALCAMERACAPTURERESULT_H

#include <chrono>
#include <cstdint>
#include <cstring>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "Exif.h"
#include "GLES/gl.h"
#include "VirtualCameraCaptureRequest.h"
#include "VirtualCameraDevice.h"
#include "VirtualCameraRenderThread.h"
#include "VirtualCameraSessionContext.h"
#include "aidl/android/hardware/camera/device/CameraMetadata.h"

namespace android {
namespace companion {
namespace virtualcamera {

// Construct the Metadata for the Capture result based on the request
// settings, timestamp and reported sensore size
::aidl::android::hardware::camera::device::CameraMetadata
createCaptureResultMetadata(std::chrono::nanoseconds timestamp,
                            const RequestSettings& requestSettings,
                            Resolution reportedSensorSize);

}  // namespace virtualcamera
}  // namespace companion
}  // namespace android

#endif  // ANDROID_COMPANION_VIRTUALCAMERA_VIRTUALCAMERACAPTURERESULT_H