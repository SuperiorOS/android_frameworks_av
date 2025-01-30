/*
 * Copyright 2018, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "Codec2Buffer"
#define ATRACE_TAG  ATRACE_TAG_VIDEO
#include <utils/Log.h>
#include <utils/Trace.h>

#include <android_media_codec.h>

#include <aidl/android/hardware/graphics/common/Cta861_3.h>
#include <aidl/android/hardware/graphics/common/Smpte2086.h>
#include <android-base/no_destructor.h>
#include <android-base/properties.h>
#include <android/hardware/cas/native/1.0/types.h>
#include <android/hardware/drm/1.0/types.h>
#include <hidlmemory/FrameworkUtils.h>
#include <media/hardware/HardwareAPI.h>
#include <media/stagefright/CodecBase.h>
#include <media/stagefright/MediaCodecConstants.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AUtils.h>
#include <media/stagefright/foundation/ColorUtils.h>
#include <mediadrm/ICrypto.h>
#include <nativebase/nativebase.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Fence.h>

#include <C2AllocatorGralloc.h>
#include <C2BlockInternal.h>
#include <C2Debug.h>

#include "Codec2Buffer.h"
#include "Codec2BufferUtils.h"

namespace android {

// Codec2Buffer

bool Codec2Buffer::canCopyLinear(const std::shared_ptr<C2Buffer> &buffer) const {
    if (const_cast<Codec2Buffer *>(this)->base() == nullptr) {
        return false;
    }
    if (!buffer) {
        // Nothing to copy, so we can copy by doing nothing.
        return true;
    }
    if (buffer->data().type() != C2BufferData::LINEAR) {
        return false;
    }
    if (buffer->data().linearBlocks().size() == 0u) {
        // Nothing to copy, so we can copy by doing nothing.
        return true;
    } else if (buffer->data().linearBlocks().size() > 1u) {
        // We don't know how to copy more than one blocks.
        return false;
    }
    if (buffer->data().linearBlocks()[0].size() > capacity()) {
        // It won't fit.
        return false;
    }
    return true;
}

bool Codec2Buffer::copyLinear(const std::shared_ptr<C2Buffer> &buffer) {
    // We assume that all canCopyLinear() checks passed.
    if (!buffer || buffer->data().linearBlocks().size() == 0u
            || buffer->data().linearBlocks()[0].size() == 0u) {
        setRange(0, 0);
        return true;
    }
    C2ReadView view = buffer->data().linearBlocks()[0].map().get();
    if (view.error() != C2_OK) {
        ALOGD("Error while mapping: %d", view.error());
        return false;
    }
    if (view.capacity() > capacity()) {
        ALOGD("C2ConstLinearBlock lied --- it actually doesn't fit: view(%u) > this(%zu)",
                view.capacity(), capacity());
        return false;
    }
    memcpy(base(), view.data(), view.capacity());
    setRange(0, view.capacity());
    return true;
}

void Codec2Buffer::setImageData(const sp<ABuffer> &imageData) {
    mImageData = imageData;
}

// LocalLinearBuffer

bool LocalLinearBuffer::canCopy(const std::shared_ptr<C2Buffer> &buffer) const {
    return canCopyLinear(buffer);
}

bool LocalLinearBuffer::copy(const std::shared_ptr<C2Buffer> &buffer) {
    return copyLinear(buffer);
}

// DummyContainerBuffer

static uint8_t sDummyByte[1] = { 0 };

DummyContainerBuffer::DummyContainerBuffer(
        const sp<AMessage> &format, const std::shared_ptr<C2Buffer> &buffer)
    : Codec2Buffer(format, new ABuffer(sDummyByte, 1)),
      mBufferRef(buffer) {
    setRange(0, buffer ? 1 : 0);
}

std::shared_ptr<C2Buffer> DummyContainerBuffer::asC2Buffer() {
    return mBufferRef;
}

void DummyContainerBuffer::clearC2BufferRefs() {
    mBufferRef.reset();
}

bool DummyContainerBuffer::canCopy(const std::shared_ptr<C2Buffer> &) const {
    return !mBufferRef;
}

bool DummyContainerBuffer::copy(const std::shared_ptr<C2Buffer> &buffer) {
    mBufferRef = buffer;
    setRange(0, mBufferRef ? 1 : 0);
    return true;
}

// LinearBlockBuffer

// static
sp<LinearBlockBuffer> LinearBlockBuffer::Allocate(
        const sp<AMessage> &format, const std::shared_ptr<C2LinearBlock> &block) {
    C2WriteView writeView(block->map().get());
    if (writeView.error() != C2_OK) {
        return nullptr;
    }
    return new LinearBlockBuffer(format, std::move(writeView), block);
}

std::shared_ptr<C2Buffer> LinearBlockBuffer::asC2Buffer() {
    return C2Buffer::CreateLinearBuffer(mBlock->share(offset(), size(), C2Fence()));
}

bool LinearBlockBuffer::canCopy(const std::shared_ptr<C2Buffer> &buffer) const {
    return canCopyLinear(buffer);
}

bool LinearBlockBuffer::copy(const std::shared_ptr<C2Buffer> &buffer) {
    return copyLinear(buffer);
}

LinearBlockBuffer::LinearBlockBuffer(
        const sp<AMessage> &format,
        C2WriteView&& writeView,
        const std::shared_ptr<C2LinearBlock> &block)
    : Codec2Buffer(format, new ABuffer(writeView.data(), writeView.size())),
      mWriteView(writeView),
      mBlock(block) {
}

// ConstLinearBlockBuffer

// static
sp<ConstLinearBlockBuffer> ConstLinearBlockBuffer::Allocate(
        const sp<AMessage> &format, const std::shared_ptr<C2Buffer> &buffer) {
    if (!buffer
            || buffer->data().type() != C2BufferData::LINEAR
            || buffer->data().linearBlocks().size() != 1u) {
        if (!buffer) {
            ALOGD("ConstLinearBlockBuffer::Allocate: null buffer");
        } else {
            ALOGW("ConstLinearBlockBuffer::Allocate: type=%d # linear blocks=%zu",
                  buffer->data().type(), buffer->data().linearBlocks().size());
        }
        return nullptr;
    }
    C2ReadView readView(buffer->data().linearBlocks()[0].map().get());
    if (readView.error() != C2_OK) {
        ALOGW("ConstLinearBlockBuffer::Allocate: readView.error()=%d", readView.error());
        return nullptr;
    }
    return new ConstLinearBlockBuffer(format, std::move(readView), buffer);
}

ConstLinearBlockBuffer::ConstLinearBlockBuffer(
        const sp<AMessage> &format,
        C2ReadView&& readView,
        const std::shared_ptr<C2Buffer> &buffer)
    : Codec2Buffer(format, new ABuffer(
            // NOTE: ABuffer only takes non-const pointer but this data is
            //       supposed to be read-only.
            const_cast<uint8_t *>(readView.data()), readView.capacity())),
      mReadView(readView),
      mBufferRef(buffer) {
}

std::shared_ptr<C2Buffer> ConstLinearBlockBuffer::asC2Buffer() {
    return mBufferRef;
}

void ConstLinearBlockBuffer::clearC2BufferRefs() {
    mBufferRef.reset();
}

// GraphicBlockBuffer

// static
sp<GraphicBlockBuffer> GraphicBlockBuffer::Allocate(
        const sp<AMessage> &format,
        const std::shared_ptr<C2GraphicBlock> &block,
        std::function<sp<ABuffer>(size_t)> alloc) {
    ATRACE_BEGIN("GraphicBlockBuffer::Allocate block->map()");
    C2GraphicView view(block->map().get());
    ATRACE_END();
    if (view.error() != C2_OK) {
        ALOGD("C2GraphicBlock::map failed: %d", view.error());
        return nullptr;
    }

    GraphicView2MediaImageConverter converter(view, format, false /* copy */);
    if (converter.initCheck() != OK) {
        ALOGD("Converter init failed: %d", converter.initCheck());
        return nullptr;
    }
    bool wrapped = true;
    sp<ABuffer> buffer = converter.wrap();
    if (buffer == nullptr) {
        buffer = alloc(converter.backBufferSize());
        if (!converter.setBackBuffer(buffer)) {
            ALOGD("Converter failed to set back buffer");
            return nullptr;
        }
        wrapped = false;
    }
    return new GraphicBlockBuffer(
            format,
            buffer,
            std::move(view),
            block,
            converter.imageData(),
            wrapped);
}

GraphicBlockBuffer::GraphicBlockBuffer(
        const sp<AMessage> &format,
        const sp<ABuffer> &buffer,
        C2GraphicView &&view,
        const std::shared_ptr<C2GraphicBlock> &block,
        const sp<ABuffer> &imageData,
        bool wrapped)
    : Codec2Buffer(format, buffer),
      mView(view),
      mBlock(block),
      mWrapped(wrapped) {
    setImageData(imageData);
}

std::shared_ptr<C2Buffer> GraphicBlockBuffer::asC2Buffer() {
    ATRACE_CALL();
    uint32_t width = mView.width();
    uint32_t height = mView.height();
    if (!mWrapped) {
        (void)ImageCopy(mView, base(), imageData());
    }
    return C2Buffer::CreateGraphicBuffer(
            mBlock->share(C2Rect(width, height), C2Fence()));
}

// GraphicMetadataBuffer
GraphicMetadataBuffer::GraphicMetadataBuffer(
        const sp<AMessage> &format,
        const std::shared_ptr<C2Allocator> &alloc)
    : Codec2Buffer(format, new ABuffer(sizeof(VideoNativeMetadata))),
      mAlloc(alloc) {
    ((VideoNativeMetadata *)base())->pBuffer = nullptr;
}

std::shared_ptr<C2Buffer> GraphicMetadataBuffer::asC2Buffer() {
#ifdef __LP64__
    static std::once_flag s_checkOnce;
    static bool s_is64bitOk {true};
    std::call_once(s_checkOnce, [&](){
        const std::string abi32list =
        ::android::base::GetProperty("ro.product.cpu.abilist32", "");
        if (!abi32list.empty()) {
            int32_t inputSurfaceSetting =
            ::android::base::GetIntProperty("debug.stagefright.c2inputsurface", int32_t(0));
            s_is64bitOk = inputSurfaceSetting != 0;
        }
    });

    if (!s_is64bitOk) {
        ALOGE("GraphicMetadataBuffer does not work in 32+64 system if compiled as 64-bit object"\
              "when debug.stagefright.c2inputsurface is set to 0");
        return nullptr;
    }
#endif

    VideoNativeMetadata *meta = (VideoNativeMetadata *)base();
    ANativeWindowBuffer *buffer = (ANativeWindowBuffer *)meta->pBuffer;
    if (buffer == nullptr) {
        ALOGD("VideoNativeMetadata contains null buffer");
        return nullptr;
    }

    ALOGV("VideoNativeMetadata: %dx%d", buffer->width, buffer->height);
    C2Handle *handle = WrapNativeCodec2GrallocHandle(
            buffer->handle,
            buffer->width,
            buffer->height,
            buffer->format,
            buffer->usage,
            buffer->stride);
    std::shared_ptr<C2GraphicAllocation> alloc;
    c2_status_t err = mAlloc->priorGraphicAllocation(handle, &alloc);
    if (err != C2_OK) {
        ALOGD("Failed to wrap VideoNativeMetadata into C2GraphicAllocation");
        native_handle_close(handle);
        native_handle_delete(handle);
        return nullptr;
    }
    std::shared_ptr<C2GraphicBlock> block = _C2BlockFactory::CreateGraphicBlock(alloc);

    meta->pBuffer = 0;
    // TODO: wrap this in C2Fence so that the component can wait when it
    //       actually starts processing.
    if (meta->nFenceFd >= 0) {
        sp<Fence> fence(new Fence(meta->nFenceFd));
        fence->waitForever(LOG_TAG);
    }
    return C2Buffer::CreateGraphicBuffer(
            block->share(C2Rect(buffer->width, buffer->height), C2Fence()));
}

// ConstGraphicBlockBuffer

// static
sp<ConstGraphicBlockBuffer> ConstGraphicBlockBuffer::Allocate(
        const sp<AMessage> &format,
        const std::shared_ptr<C2Buffer> &buffer,
        std::function<sp<ABuffer>(size_t)> alloc) {
    if (!buffer
            || buffer->data().type() != C2BufferData::GRAPHIC
            || buffer->data().graphicBlocks().size() != 1u) {
        ALOGD("C2Buffer precond fail");
        return nullptr;
    }
    ATRACE_BEGIN("ConstGraphicBlockBuffer::Allocate block->map()");
    std::unique_ptr<const C2GraphicView> view(std::make_unique<const C2GraphicView>(
            buffer->data().graphicBlocks()[0].map().get()));
    ATRACE_END();
    std::unique_ptr<const C2GraphicView> holder;

    GraphicView2MediaImageConverter converter(*view, format, false /* copy */);
    if (converter.initCheck() != OK) {
        ALOGD("Converter init failed: %d", converter.initCheck());
        return nullptr;
    }
    bool wrapped = true;
    sp<ABuffer> aBuffer = converter.wrap();
    if (aBuffer == nullptr) {
        aBuffer = alloc(converter.backBufferSize());
        if (!converter.setBackBuffer(aBuffer)) {
            ALOGD("Converter failed to set back buffer");
            return nullptr;
        }
        wrapped = false;
        converter.copyToMediaImage();
        // We don't need the view.
        holder = std::move(view);
    }
    return new ConstGraphicBlockBuffer(
            format,
            aBuffer,
            std::move(view),
            buffer,
            converter.imageData(),
            wrapped);
}

// static
sp<ConstGraphicBlockBuffer> ConstGraphicBlockBuffer::AllocateEmpty(
        const sp<AMessage> &format,
        std::function<sp<ABuffer>(size_t)> alloc) {
    int32_t width, height;
    if (!format->findInt32("width", &width)
            || !format->findInt32("height", &height)) {
        ALOGD("format had no width / height");
        return nullptr;
    }
    int32_t colorFormat = COLOR_FormatYUV420Flexible;
    int32_t bpp = 12;  // 8(Y) + 2(U) + 2(V)
    if (format->findInt32(KEY_COLOR_FORMAT, &colorFormat)) {
        if (colorFormat == COLOR_FormatYUVP010) {
            bpp = 24;  // 16(Y) + 4(U) + 4(V)
        }
    }
    sp<ABuffer> aBuffer(alloc(align(width, 16) * align(height, 16) * bpp / 8));
    if (aBuffer == nullptr) {
        ALOGD("%s: failed to allocate buffer", __func__);
        return nullptr;
    }
    return new ConstGraphicBlockBuffer(
            format,
            aBuffer,
            nullptr,
            nullptr,
            nullptr,
            false);
}

ConstGraphicBlockBuffer::ConstGraphicBlockBuffer(
        const sp<AMessage> &format,
        const sp<ABuffer> &aBuffer,
        std::unique_ptr<const C2GraphicView> &&view,
        const std::shared_ptr<C2Buffer> &buffer,
        const sp<ABuffer> &imageData,
        bool wrapped)
    : Codec2Buffer(format, aBuffer),
      mView(std::move(view)),
      mBufferRef(buffer),
      mWrapped(wrapped) {
    setImageData(imageData);
}

std::shared_ptr<C2Buffer> ConstGraphicBlockBuffer::asC2Buffer() {
    return mBufferRef;
}

void ConstGraphicBlockBuffer::clearC2BufferRefs() {
    mView.reset();
    mBufferRef.reset();
}

bool ConstGraphicBlockBuffer::canCopy(const std::shared_ptr<C2Buffer> &buffer) const {
    if (mWrapped || mBufferRef) {
        ALOGD("ConstGraphicBlockBuffer::canCopy: %swrapped ; buffer ref %s",
                mWrapped ? "" : "not ", mBufferRef ? "exists" : "doesn't exist");
        return false;
    }
    if (!buffer) {
        // Nothing to copy, so we can copy by doing nothing.
        return true;
    }
    if (buffer->data().type() != C2BufferData::GRAPHIC) {
        ALOGD("ConstGraphicBlockBuffer::canCopy: buffer precondition unsatisfied");
        return false;
    }
    if (buffer->data().graphicBlocks().size() == 0) {
        return true;
    } else if (buffer->data().graphicBlocks().size() != 1u) {
        ALOGD("ConstGraphicBlockBuffer::canCopy: too many blocks");
        return false;
    }

    ATRACE_BEGIN("ConstGraphicBlockBuffer::canCopy block->map()");
    GraphicView2MediaImageConverter converter(
            buffer->data().graphicBlocks()[0].map().get(),
            // FIXME: format() is not const, but we cannot change it, so do a const cast here
            const_cast<ConstGraphicBlockBuffer *>(this)->format(),
            true /* copy */);
    ATRACE_END();
    if (converter.initCheck() != OK) {
        ALOGD("ConstGraphicBlockBuffer::canCopy: converter init failed: %d", converter.initCheck());
        return false;
    }
    if (converter.backBufferSize() > capacity()) {
        ALOGD("ConstGraphicBlockBuffer::canCopy: insufficient capacity: req %u has %zu",
                converter.backBufferSize(), capacity());
        return false;
    }
    return true;
}

bool ConstGraphicBlockBuffer::copy(const std::shared_ptr<C2Buffer> &buffer) {
    if (!buffer || buffer->data().graphicBlocks().size() == 0) {
        setRange(0, 0);
        return true;
    }

    GraphicView2MediaImageConverter converter(
            buffer->data().graphicBlocks()[0].map().get(), format(), true /* copy */);
    if (converter.initCheck() != OK) {
        ALOGD("ConstGraphicBlockBuffer::copy: converter init failed: %d", converter.initCheck());
        return false;
    }
    sp<ABuffer> aBuffer = new ABuffer(base(), capacity());
    if (!converter.setBackBuffer(aBuffer)) {
        ALOGD("ConstGraphicBlockBuffer::copy: set back buffer failed");
        return false;
    }
    setRange(0, aBuffer->size());  // align size info
    converter.copyToMediaImage();
    setImageData(converter.imageData());
    mBufferRef = buffer;
    return true;
}

// EncryptedLinearBlockBuffer

EncryptedLinearBlockBuffer::EncryptedLinearBlockBuffer(
        const sp<AMessage> &format,
        const std::shared_ptr<C2LinearBlock> &block,
        const sp<IMemory> &memory,
        int32_t heapSeqNum)
    // TODO: Using unsecurePointer() has some associated security pitfalls
    //       (see declaration for details).
    //       Either document why it is safe in this case or address the
    //       issue (e.g. by copying).
    : Codec2Buffer(format, new ABuffer(memory->unsecurePointer(), memory->size())),
      mBlock(block),
      mMemory(memory),
      mHeapSeqNum(heapSeqNum) {
}

std::shared_ptr<C2Buffer> EncryptedLinearBlockBuffer::asC2Buffer() {
    return C2Buffer::CreateLinearBuffer(mBlock->share(offset(), size(), C2Fence()));
}

void EncryptedLinearBlockBuffer::fillSourceBuffer(
        hardware::drm::V1_0::SharedBuffer *source) {
    BufferChannelBase::IMemoryToSharedBuffer(mMemory, mHeapSeqNum, source);
}

void EncryptedLinearBlockBuffer::fillSourceBuffer(
        hardware::cas::native::V1_0::SharedBuffer *source) {
    ssize_t offset;
    size_t size;

    mHidlMemory = hardware::fromHeap(mMemory->getMemory(&offset, &size));
    source->heapBase = *mHidlMemory;
    source->offset = offset;
    source->size = size;
}

bool EncryptedLinearBlockBuffer::copyDecryptedContent(
        const sp<IMemory> &decrypted, size_t length) {
    C2WriteView view = mBlock->map().get();
    if (view.error() != C2_OK) {
        return false;
    }
    if (view.size() < length) {
        return false;
    }
    memcpy(view.data(), decrypted->unsecurePointer(), length);
    return true;
}

bool EncryptedLinearBlockBuffer::copyDecryptedContentFromMemory(size_t length) {
    return copyDecryptedContent(mMemory, length);
}

native_handle_t *EncryptedLinearBlockBuffer::handle() const {
    return const_cast<native_handle_t *>(mBlock->handle());
}

void EncryptedLinearBlockBuffer::getMappedBlock(
        std::unique_ptr<MappedBlock> * const mappedBlock) const {
    if (mappedBlock) {
        mappedBlock->reset(new EncryptedLinearBlockBuffer::MappedBlock(mBlock));
    }
    return;
}

EncryptedLinearBlockBuffer::MappedBlock::MappedBlock(
        const std::shared_ptr<C2LinearBlock> &block) : mView(block->map().get()) {
}

bool EncryptedLinearBlockBuffer::MappedBlock::copyDecryptedContent(
        const sp<IMemory> &decrypted, size_t length) {
    if (mView.error() != C2_OK) {
        return false;
    }
    if (mView.size() < length) {
        ALOGE("View size(%d) less than decrypted length(%zu)",
                mView.size(), length);
        return false;
    }
    memcpy(mView.data(), decrypted->unsecurePointer(), length);
    mView.setOffset(mView.offset() + length);
    return true;
}

EncryptedLinearBlockBuffer::MappedBlock::~MappedBlock() {
    mView.setOffset(0);
}

using ::aidl::android::hardware::graphics::common::Cta861_3;
using ::aidl::android::hardware::graphics::common::Smpte2086;

namespace {

class GrallocBuffer {
public:
    GrallocBuffer(const C2Handle *const handle) : mBuffer(nullptr) {
        GraphicBufferMapper& mapper = GraphicBufferMapper::get();

        // Unwrap raw buffer handle from the C2Handle
        native_handle_t *nh = UnwrapNativeCodec2GrallocHandle(handle);
        if (!nh) {
            ALOGE("handle is not compatible to any gralloc C2Handle types");
            return;
        }
        // Import the raw handle so IMapper can use the buffer. The imported
        // handle must be freed when the client is done with the buffer.
        status_t status = mapper.importBufferNoValidate(
                nh,
                &mBuffer);

        if (status != OK) {
            ALOGE("Failed to import buffer. Status: %d.", status);
            return;
        }

        // TRICKY: UnwrapNativeCodec2GrallocHandle creates a new handle but
        //         does not clone the fds. Thus we need to delete the handle
        //         without closing it.
        native_handle_delete(nh);
    }

    ~GrallocBuffer() {
        GraphicBufferMapper& mapper = GraphicBufferMapper::get();
        if (mBuffer) {
            // Free the imported buffer handle. This does not release the
            // underlying buffer itself.
            mapper.freeBuffer(mBuffer);
        }
    }

    buffer_handle_t get() const { return mBuffer; }
    operator bool() const { return (mBuffer != nullptr); }
private:
    buffer_handle_t mBuffer;
};

}  // namspace

c2_status_t GetHdrMetadataFromGralloc4Handle(
        const C2Handle *const handle,
        std::shared_ptr<C2StreamHdrStaticMetadataInfo::input> *staticInfo,
        std::shared_ptr<C2StreamHdrDynamicMetadataInfo::input> *dynamicInfo) {
    c2_status_t err = C2_OK;
    GraphicBufferMapper& mapper = GraphicBufferMapper::get();
    GrallocBuffer buffer(handle);
    if (!buffer) {
        // Gralloc4 not supported; nothing to do
        return err;
    }
    if (staticInfo) {
        ALOGV("Grabbing static HDR info from gralloc metadata");
        staticInfo->reset(new C2StreamHdrStaticMetadataInfo::input(0u));
        memset(&(*staticInfo)->mastering, 0, sizeof((*staticInfo)->mastering));
        (*staticInfo)->maxCll = 0;
        (*staticInfo)->maxFall = 0;

        std::optional<Smpte2086> smpte2086;
        status_t status = mapper.getSmpte2086(buffer.get(), &smpte2086);
        if (status != OK || !smpte2086) {
            err = C2_CORRUPTED;
        } else {
            if (smpte2086) {
                  (*staticInfo)->mastering.red.x    = smpte2086->primaryRed.x;
                  (*staticInfo)->mastering.red.y    = smpte2086->primaryRed.y;
                  (*staticInfo)->mastering.green.x  = smpte2086->primaryGreen.x;
                  (*staticInfo)->mastering.green.y  = smpte2086->primaryGreen.y;
                  (*staticInfo)->mastering.blue.x   = smpte2086->primaryBlue.x;
                  (*staticInfo)->mastering.blue.y   = smpte2086->primaryBlue.y;
                  (*staticInfo)->mastering.white.x  = smpte2086->whitePoint.x;
                  (*staticInfo)->mastering.white.y  = smpte2086->whitePoint.y;

                  (*staticInfo)->mastering.maxLuminance = smpte2086->maxLuminance;
                  (*staticInfo)->mastering.minLuminance = smpte2086->minLuminance;
            }
        }

        std::optional<Cta861_3> cta861_3;
        status = mapper.getCta861_3(buffer.get(), &cta861_3);
        if (status != OK || !cta861_3) {
            err = C2_CORRUPTED;
        } else {
            if (cta861_3) {
                  (*staticInfo)->maxCll   = cta861_3->maxContentLightLevel;
                  (*staticInfo)->maxFall  = cta861_3->maxFrameAverageLightLevel;
            }
        }
    }

    if (err != C2_OK) {
        staticInfo->reset();
    }

    if (dynamicInfo) {
        ALOGV("Grabbing dynamic HDR info from gralloc metadata");
        dynamicInfo->reset();
        std::optional<std::vector<uint8_t>> vec;
        status_t status = mapper.getSmpte2094_40(buffer.get(), &vec);
        if (status != OK || !vec) {
            dynamicInfo->reset();
            err = C2_CORRUPTED;
        } else {
            if (vec) {
                *dynamicInfo = C2StreamHdrDynamicMetadataInfo::input::AllocShared(
                      vec->size(), 0u, C2Config::HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40);
                memcpy((*dynamicInfo)->m.data, vec->data(), vec->size());
            }
        }
    }

    return err;
}

c2_status_t SetMetadataToGralloc4Handle(
        android_dataspace_t dataSpace,
        const std::shared_ptr<const C2StreamHdrStaticMetadataInfo::output> &staticInfo,
        const std::shared_ptr<const C2StreamHdrDynamicMetadataInfo::output> &dynamicInfo,
        const C2Handle *const handle) {
    c2_status_t err = C2_OK;
    GraphicBufferMapper& mapper = GraphicBufferMapper::get();
    GrallocBuffer buffer(handle);
    if (!buffer) {
        // Gralloc4 not supported; nothing to do
        return err;
    }
    // Use V0 dataspaces for Gralloc4+
    if (android::media::codec::provider_->dataspace_v0_partial()) {
        ColorUtils::convertDataSpaceToV0(dataSpace);
    }
    status_t status = mapper.setDataspace(buffer.get(), static_cast<ui::Dataspace>(dataSpace));
    if (status != OK) {
       err = C2_CORRUPTED;
    }
    if (staticInfo && *staticInfo) {
        ALOGV("Setting static HDR info as gralloc metadata");
        std::optional<Smpte2086> smpte2086 = Smpte2086{
            {staticInfo->mastering.red.x, staticInfo->mastering.red.y},
            {staticInfo->mastering.green.x, staticInfo->mastering.green.y},
            {staticInfo->mastering.blue.x, staticInfo->mastering.blue.y},
            {staticInfo->mastering.white.x, staticInfo->mastering.white.y},
            staticInfo->mastering.maxLuminance,
            staticInfo->mastering.minLuminance,
        };
        if (0.0 <= smpte2086->primaryRed.x && smpte2086->primaryRed.x <= 1.0
                && 0.0 <= smpte2086->primaryRed.y && smpte2086->primaryRed.y <= 1.0
                && 0.0 <= smpte2086->primaryGreen.x && smpte2086->primaryGreen.x <= 1.0
                && 0.0 <= smpte2086->primaryGreen.y && smpte2086->primaryGreen.y <= 1.0
                && 0.0 <= smpte2086->primaryBlue.x && smpte2086->primaryBlue.x <= 1.0
                && 0.0 <= smpte2086->primaryBlue.y && smpte2086->primaryBlue.y <= 1.0
                && 0.0 <= smpte2086->whitePoint.x && smpte2086->whitePoint.x <= 1.0
                && 0.0 <= smpte2086->whitePoint.y && smpte2086->whitePoint.y <= 1.0
                && 0.0 <= smpte2086->maxLuminance && 0.0 <= smpte2086->minLuminance) {
            status = mapper.setSmpte2086(buffer.get(), smpte2086);
            if (status != OK) {
                err = C2_CORRUPTED;
            }
        }
        std::optional<Cta861_3> cta861_3 = Cta861_3{
            staticInfo->maxCll,
            staticInfo->maxFall,
        };
        if (0.0 <= cta861_3->maxContentLightLevel && 0.0 <= cta861_3->maxFrameAverageLightLevel) {
            status = mapper.setCta861_3(buffer.get(), cta861_3);
            if (status != OK) {
                err = C2_CORRUPTED;
            }
        }
    }
    if (dynamicInfo && *dynamicInfo && dynamicInfo->flexCount() > 0) {
        ALOGV("Setting dynamic HDR info as gralloc metadata");
        if (dynamicInfo->m.type_ == C2Config::HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40) {
            std::optional<std::vector<uint8_t>> smpte2094_40 = std::vector<uint8_t>();
            smpte2094_40->resize(dynamicInfo->flexCount());
            memcpy(smpte2094_40->data(), dynamicInfo->m.data, dynamicInfo->flexCount());

            status = mapper.setSmpte2094_40(buffer.get(), smpte2094_40);
            if (status != OK) {
                err = C2_CORRUPTED;
            }
        } else {
            err = C2_BAD_VALUE;
        }
    }

    return err;
}

}  // namespace android
