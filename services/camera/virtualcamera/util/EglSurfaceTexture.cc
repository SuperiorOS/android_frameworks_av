/*
 * Copyright (C) 2023 The Android Open Source Project
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

// #define LOG_NDEBUG 0
#include <chrono>

#include "utils/Timers.h"
#define LOG_TAG "EglSurfaceTexture"

#include <GLES/gl.h>
#include <com_android_graphics_libgui_flags.h>
#include <gui/BufferQueue.h>
#include <gui/GLConsumer.h>
#include <gui/IGraphicBufferProducer.h>
#include <hardware/gralloc.h>

#include <cstdint>

#include "EglSurfaceTexture.h"
#include "EglUtil.h"

namespace android {
namespace companion {
namespace virtualcamera {
namespace {

// Maximal number of buffers producer can dequeue without blocking.
constexpr int kBufferProducerMaxDequeueBufferCount = 64;

}  // namespace

EglSurfaceTexture::EglSurfaceTexture(const uint32_t width, const uint32_t height)
    : mWidth(width), mHeight(height) {
  glGenTextures(1, &mTextureId);
  if (checkEglError("EglSurfaceTexture(): glGenTextures")) {
    ALOGE("Failed to generate texture");
    return;
  }

#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_CONSUMER_BASE_OWNS_BQ)
  mGlConsumer = sp<GLConsumer>::make(mTextureId, GLConsumer::TEXTURE_EXTERNAL,
                                     false, false);
  mGlConsumer->setName(String8("VirtualCameraEglSurfaceTexture"));
  mGlConsumer->setDefaultBufferSize(mWidth, mHeight);
  mGlConsumer->setConsumerUsageBits(GRALLOC_USAGE_HW_TEXTURE);
  mGlConsumer->setDefaultBufferFormat(AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420);

  mSurface = mGlConsumer->getSurface();
  mSurface->setMaxDequeuedBufferCount(kBufferProducerMaxDequeueBufferCount);
#else
  BufferQueue::createBufferQueue(&mBufferProducer, &mBufferConsumer);
  // Set max dequeue buffer count for producer to maximal value to prevent
  // blocking when dequeuing input buffers.
  mBufferProducer->setMaxDequeuedBufferCount(
      kBufferProducerMaxDequeueBufferCount);
  mGlConsumer = sp<GLConsumer>::make(
      mBufferConsumer, mTextureId, GLConsumer::TEXTURE_EXTERNAL, false, false);
  mGlConsumer->setName(String8("VirtualCameraEglSurfaceTexture"));
  mGlConsumer->setDefaultBufferSize(mWidth, mHeight);
  mGlConsumer->setConsumerUsageBits(GRALLOC_USAGE_HW_TEXTURE);
  mGlConsumer->setDefaultBufferFormat(AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420);

  mSurface = sp<Surface>::make(mBufferProducer);
#endif  // COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_CONSUMER_BASE_OWNS_BQ)
}

EglSurfaceTexture::~EglSurfaceTexture() {
  if (mTextureId != 0) {
    glDeleteTextures(1, &mTextureId);
  }
}

sp<Surface> EglSurfaceTexture::getSurface() {
  return mSurface;
}

sp<GraphicBuffer> EglSurfaceTexture::getCurrentBuffer() {
  return mGlConsumer->getCurrentBuffer();
}

void EglSurfaceTexture::setFrameAvailableListener(
    const wp<ConsumerBase::FrameAvailableListener>& listener) {
  mGlConsumer->setFrameAvailableListener(listener);
}

bool EglSurfaceTexture::waitForNextFrame(const std::chrono::nanoseconds timeout) {
  return mSurface->waitForNextFrame(mGlConsumer->getFrameNumber(),
                                    static_cast<nsecs_t>(timeout.count()));
}

std::chrono::nanoseconds EglSurfaceTexture::getTimestamp() {
  return std::chrono::nanoseconds(mGlConsumer->getTimestamp());
}

GLuint EglSurfaceTexture::updateTexture() {
  int previousFrameId;
  int framesAdvance = 0;
  // Consume buffers one at the time.
  // Contrary to the code comments in GLConsumer, the GLConsumer acquires
  // next queued buffer (not the most recently queued buffer).
  while (true) {
    previousFrameId = mGlConsumer->getFrameNumber();
    mGlConsumer->updateTexImage();
    int currentFrameId = mGlConsumer->getFrameNumber();
    if (previousFrameId == currentFrameId) {
      // Frame number didn't change after updating the texture,
      // this means we're at the end of the queue and current attached
      // buffer is the most recent buffer.
      break;
    }

    framesAdvance++;
    previousFrameId = currentFrameId;
  }
  ALOGV("%s: Advanced %d frames", __func__, framesAdvance);
  return mTextureId;
}

GLuint EglSurfaceTexture::getTextureId() const {
  return mTextureId;
}

std::array<float, 16> EglSurfaceTexture::getTransformMatrix() {
  std::array<float, 16> matrix;
  mGlConsumer->getTransformMatrix(matrix.data());
  return matrix;
}

uint32_t EglSurfaceTexture::getWidth() const {
  return mWidth;
}

uint32_t EglSurfaceTexture::getHeight() const {
  return mHeight;
}

}  // namespace virtualcamera
}  // namespace companion
}  // namespace android
