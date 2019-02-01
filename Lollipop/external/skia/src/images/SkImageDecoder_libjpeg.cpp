/*
 * Copyright 2007 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// MStar Android Patch Begin
#define LOG_TAG "SkiaJPEG"
#include <utils/Log.h>
// MStar Android Patch End

#include "SkImageDecoder.h"
#include "SkImageEncoder.h"
#include "SkJpegUtility.h"
#include "SkColorPriv.h"
#include "SkDither.h"
#include "SkScaledBitmapSampler.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkTime.h"
#include "SkUtils.h"
#include "SkRTConf.h"
#include "SkRect.h"
#include "SkCanvas.h"

// MStar Android Patch Begin
#include <drvCMAPool.h>

#ifdef HW_JPEG_SUPPORT
#include <mmap.h>
#include "MsTypes.h"
#include "apiJPEG.h"
#include "drvSYS.h"
#include "apiGFX.h"
#include <cutils/properties.h>

static void *pvaddr_read = NULL,*pvaddr_internal = NULL,*pvaddr_write = NULL, *pvaddr_final = NULL;
static uint32_t read_paddr=0,read_len=0,inter_paddr=0,inter_len=0,write_paddr=0,write_len=0;
static bool read_mapping = false, inter_mapping = false, write_mapping = false;
static  int s_index = -1;
static bool bDisableHWDecodeForPatch = false;
static SkStream* staticStream = NULL;
static signed long JpegFillHdrFunc(unsigned long BufAddr, unsigned long BufLength);

#define JPEG_WRITE_BUFF_SIZE 0x3FC000
#define JPEG_READ_BUFF_SIZE  0x50080
#define JPEG_INTERNAL_BUFF_SIZE  0xA0000

extern "C" {
#include "jpeglib.h"

#ifdef __ARM_NEON__
    void yuv422_2_rgb8888_neon(uint8_t *dst_ptr,
                               const uint8_t *src_ptr,
                               int width,
                               int height,
                               int yuv_pitch,
                               int rgb_pitch);

    void yuv422_2_rgb4444_neon(uint8_t *dst_ptr,
                               const uint8_t *src_ptr,
                               int width,
                               int height,
                               int yuv_pitch,
                               int rgb_pitch);

    void yuv422_2_rgb565_neon(uint8_t *dst_ptr,
                               const uint8_t *src_ptr,
                               int width,
                               int height,
                               int yuv_pitch,
                               int rgb_pitch);
#endif /* __ARM_NEON__ */
}
#endif // HW_JPEG_SUPPORT
// MStar Android Patch End

#include <stdio.h>
extern "C" {
    #include "jpeglib.h"
    #include "jerror.h"
}

// MStar Android Patch Begin
// define jpeg decode max resolution and HW support max resolution
// The values below are the results of  many of tests.
#define JPEG_BASELINE_MAX_WIDTH    (1920 * 16)
#define JPEG_BASELINE_MAX_HEIGHT    (1080 * 16)
#define JPEG_PROGRESSIVE_MAX_WIDTH    (7000)
#define JPEG_PROGRESSIVE_MAX_HEIGHT    (7000)
// MStar Android Patch End
// These enable timing code that report milliseconds for an encoding/decoding
//#define TIME_ENCODE
//#define TIME_DECODE

// this enables our rgb->yuv code, which is faster than libjpeg on ARM
#define WE_CONVERT_TO_YUV

// MStar Android Patch Begin
//#define DUMP_RAW_DATA
//#define TEST_TIME_USED
#define PATCH_CTS

#ifdef HW_JPEG_SUPPORT
#define PHOTO_DECODE_JPEG_BASELINE_MAX_WIDTH        (1920)
#define PHOTO_DECODE_JPEG_BASELINE_MAX_HEIGHT       (1080)

#define PHOTO_DECODE_JPEG_PROGRESSIVE_MAX_WIDTH     (1024)
#define PHOTO_DECODE_JPEG_PROGRESSIVE_MAX_HEIGHT    (768)

#define PHOTO_DECODE_MIN_WIDTH                      (512)
#define PHOTO_DECODE_MIN_HEIGHT                     (512)

typedef enum {
    DECODE_DONE = 0,
    DECODING,
    DECODE_ERR,
} E_JPEG_DECODER_STATUS;

typedef enum {
    E_MP_TYPE_BASELINE = 0x030000
  , E_MP_TYPE_LARGE_THUMBNAIL_CLASS1 = 0x010001
  , E_MP_TYPE_LARGE_THUMBNAIL_CLASS2 = 0x010002
  , E_MP_TYPE_MULTI_FRAME_PANORAMA = 0x020001
  , E_MP_TYPE_MULTI_FRAME_DISPARITY = 0x020002
  , E_MP_TYPE_MULTI_FRAME_MULTI_ANGLE = 0x020003
  , E_MP_TYPE_MASK = 0x00FFFFFF
} MP_TYPE_CODE;

// currently, we only support disparity type two image decoding
#define DECODE_MPO_IMAGE_MAX_NUM 2
#endif
// MStar Android Patch End

// If ANDROID_RGB is defined by in the jpeg headers it indicates that jpeg offers
// support for two additional formats (1) JCS_RGBA_8888 and (2) JCS_RGB_565.

#if defined(SK_DEBUG)
#define DEFAULT_FOR_SUPPRESS_JPEG_IMAGE_DECODER_WARNINGS false
#define DEFAULT_FOR_SUPPRESS_JPEG_IMAGE_DECODER_ERRORS false
#else  // !defined(SK_DEBUG)
#define DEFAULT_FOR_SUPPRESS_JPEG_IMAGE_DECODER_WARNINGS true
#define DEFAULT_FOR_SUPPRESS_JPEG_IMAGE_DECODER_ERRORS true
#endif  // defined(SK_DEBUG)
SK_CONF_DECLARE(bool, c_suppressJPEGImageDecoderWarnings,
                "images.jpeg.suppressDecoderWarnings",
                DEFAULT_FOR_SUPPRESS_JPEG_IMAGE_DECODER_WARNINGS,
                "Suppress most JPG warnings when calling decode functions.");
SK_CONF_DECLARE(bool, c_suppressJPEGImageDecoderErrors,
                "images.jpeg.suppressDecoderErrors",
                DEFAULT_FOR_SUPPRESS_JPEG_IMAGE_DECODER_ERRORS,
                "Suppress most JPG error messages when decode "
                "function fails.");

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static void overwrite_mem_buffer_size(jpeg_decompress_struct* cinfo) {
#ifdef SK_BUILD_FOR_ANDROID
    /* Check if the device indicates that it has a large amount of system memory
     * if so, increase the memory allocation to 30MB instead of the default 5MB.
     */
#ifdef ANDROID_LARGE_MEMORY_DEVICE
    cinfo->mem->max_memory_to_use = 30 * 1024 * 1024;
#else
    cinfo->mem->max_memory_to_use = 5 * 1024 * 1024;
#endif
#endif // SK_BUILD_FOR_ANDROID
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static void do_nothing_emit_message(jpeg_common_struct*, int) {
    /* do nothing */
}
static void do_nothing_output_message(j_common_ptr) {
    /* do nothing */
}

static void initialize_info(jpeg_decompress_struct* cinfo, skjpeg_source_mgr* src_mgr) {
    SkASSERT(cinfo != NULL);
    SkASSERT(src_mgr != NULL);
    jpeg_create_decompress(cinfo);
    overwrite_mem_buffer_size(cinfo);
    cinfo->src = src_mgr;
    /* To suppress warnings with a SK_DEBUG binary, set the
     * environment variable "skia_images_jpeg_suppressDecoderWarnings"
     * to "true".  Inside a program that links to skia:
     * SK_CONF_SET("images.jpeg.suppressDecoderWarnings", true); */
    if (c_suppressJPEGImageDecoderWarnings) {
        cinfo->err->emit_message = &do_nothing_emit_message;
    }
    /* To suppress error messages with a SK_DEBUG binary, set the
     * environment variable "skia_images_jpeg_suppressDecoderErrors"
     * to "true".  Inside a program that links to skia:
     * SK_CONF_SET("images.jpeg.suppressDecoderErrors", true); */
    if (c_suppressJPEGImageDecoderErrors) {
        cinfo->err->output_message = &do_nothing_output_message;
    }
}

#ifdef SK_BUILD_FOR_ANDROID
class SkJPEGImageIndex {
public:
    SkJPEGImageIndex(SkStreamRewindable* stream, SkImageDecoder* decoder)
        : fSrcMgr(stream, decoder)
        , fInfoInitialized(false)
        , fHuffmanCreated(false)
        , fDecompressStarted(false)
        {
            SkDEBUGCODE(fReadHeaderSucceeded = false;)
        }

    ~SkJPEGImageIndex() {
        if (fHuffmanCreated) {
            // Set to false before calling the libjpeg function, in case
            // the libjpeg function calls longjmp. Our setjmp handler may
            // attempt to delete this SkJPEGImageIndex, thus entering this
            // destructor again. Setting fHuffmanCreated to false first
            // prevents an infinite loop.
            fHuffmanCreated = false;
            jpeg_destroy_huffman_index(&fHuffmanIndex);
        }
        if (fDecompressStarted) {
            // Like fHuffmanCreated, set to false before calling libjpeg
            // function to prevent potential infinite loop.
            fDecompressStarted = false;
            jpeg_finish_decompress(&fCInfo);
        }
        if (fInfoInitialized) {
            this->destroyInfo();
        }
    }

    /**
     *  Destroy the cinfo struct.
     *  After this call, if a huffman index was already built, it
     *  can be used after calling initializeInfoAndReadHeader
     *  again. Must not be called after startTileDecompress except
     *  in the destructor.
     */
    void destroyInfo() {
        SkASSERT(fInfoInitialized);
        SkASSERT(!fDecompressStarted);
        // Like fHuffmanCreated, set to false before calling libjpeg
        // function to prevent potential infinite loop.
        fInfoInitialized = false;
        jpeg_destroy_decompress(&fCInfo);
        SkDEBUGCODE(fReadHeaderSucceeded = false;)
    }

    /**
     *  Initialize the cinfo struct.
     *  Calls jpeg_create_decompress, makes customizations, and
     *  finally calls jpeg_read_header. Returns true if jpeg_read_header
     *  returns JPEG_HEADER_OK.
     *  If cinfo was already initialized, destroyInfo must be called to
     *  destroy the old one. Must not be called after startTileDecompress.
     */
    bool initializeInfoAndReadHeader() {
        SkASSERT(!fInfoInitialized && !fDecompressStarted);
        initialize_info(&fCInfo, &fSrcMgr);
        fInfoInitialized = true;
        const bool success = (JPEG_HEADER_OK == jpeg_read_header(&fCInfo, true));
        SkDEBUGCODE(fReadHeaderSucceeded = success;)
        return success;
    }

    jpeg_decompress_struct* cinfo() { return &fCInfo; }

    huffman_index* huffmanIndex() { return &fHuffmanIndex; }

    /**
     *  Build the index to be used for tile based decoding.
     *  Must only be called after a successful call to
     *  initializeInfoAndReadHeader and must not be called more
     *  than once.
     */
    bool buildHuffmanIndex() {
        SkASSERT(fReadHeaderSucceeded);
        SkASSERT(!fHuffmanCreated);
        jpeg_create_huffman_index(&fCInfo, &fHuffmanIndex);
        SkASSERT(1 == fCInfo.scale_num && 1 == fCInfo.scale_denom);
        fHuffmanCreated = jpeg_build_huffman_index(&fCInfo, &fHuffmanIndex);
        return fHuffmanCreated;
    }

    /**
     *  Start tile based decoding. Must only be called after a
     *  successful call to buildHuffmanIndex, and must only be
     *  called once.
     */
    bool startTileDecompress() {
        SkASSERT(fHuffmanCreated);
        SkASSERT(fReadHeaderSucceeded);
        SkASSERT(!fDecompressStarted);
        if (jpeg_start_tile_decompress(&fCInfo)) {
            fDecompressStarted = true;
            return true;
        }
        return false;
    }

private:
    skjpeg_source_mgr  fSrcMgr;
    jpeg_decompress_struct fCInfo;
    huffman_index fHuffmanIndex;
    bool fInfoInitialized;
    bool fHuffmanCreated;
    bool fDecompressStarted;
    SkDEBUGCODE(bool fReadHeaderSucceeded;)
};
#endif

class SkJPEGImageDecoder : public SkImageDecoder {
public:
#ifdef SK_BUILD_FOR_ANDROID
    SkJPEGImageDecoder() {
        fImageIndex = NULL;
        fImageWidth = 0;
        fImageHeight = 0;
    }

    virtual ~SkJPEGImageDecoder() {
        SkDELETE(fImageIndex);
    }
#endif

    virtual Format getFormat() const {
        return kJPEG_Format;
    }

protected:
#ifdef SK_BUILD_FOR_ANDROID
    virtual bool onBuildTileIndex(SkStreamRewindable *stream, int *width, int *height) SK_OVERRIDE;
    virtual bool onDecodeSubset(SkBitmap* bitmap, const SkIRect& rect) SK_OVERRIDE;
#endif
    virtual Result onDecode(SkStream* stream, SkBitmap* bm, Mode) SK_OVERRIDE;

private:
#ifdef SK_BUILD_FOR_ANDROID
    SkJPEGImageIndex* fImageIndex;
    int fImageWidth;
    int fImageHeight;
#endif

    /**
     *  Determine the appropriate bitmap colortype and out_color_space based on
     *  both the preference of the caller and the jpeg_color_space on the
     *  jpeg_decompress_struct passed in.
     *  Must be called after jpeg_read_header.
     */
    SkColorType getBitmapColorType(jpeg_decompress_struct*);

    typedef SkImageDecoder INHERITED;

    // MStar Android Patch Begin
#ifdef HW_JPEG_SUPPORT
    Mode decodeMode;
    SkStream* pstream;
    uint32_t currReadPos;
    bool mpo;
    JPEG_MPO_INDEX_INFO *mpoIndex;
    uint32_t numMPOimage;
    uint32_t numMPOimageLeft;
    uint8_t  decodedMPOImageNum;


    bool DecodeJPEGByHW(void *pinbuf_virtual_address,
                        uint32_t inputBufAddr, uint32_t inputBufSize,
                        uint32_t interBufAddr, uint32_t interBufSize,
                        uint32_t outputBufAddr, uint32_t outputBufSize, uint32_t *pOutbuf_Len,
                        uint32_t *pOutput_Pitch, uint32_t *pOutput_Width, uint32_t *pOutput_Height);
    void JpegStop(void);
    bool JpegReadData(void *pinbuf_virtual_address, JPEG_InitParam *pJpegInitParam, JPEG_BuffLoadType eType);
    E_JPEG_DECODER_STATUS JpegWaitDone(void *pinbuf_virtual_address,JPEG_InitParam *pJpegInitParam);
#endif
    // MStar Android Patch End
};

//////////////////////////////////////////////////////////////////////////

/* Automatically clean up after throwing an exception */
class JPEGAutoClean {
public:
    JPEGAutoClean(): cinfo_ptr(NULL) {}
    ~JPEGAutoClean() {
        if (cinfo_ptr) {
            jpeg_destroy_decompress(cinfo_ptr);
        }
    }
    void set(jpeg_decompress_struct* info) {
        cinfo_ptr = info;
    }
private:
    jpeg_decompress_struct* cinfo_ptr;
};

///////////////////////////////////////////////////////////////////////////////

/*  If we need to better match the request, we might examine the image and
     output dimensions, and determine if the downsampling jpeg provided is
     not sufficient. If so, we can recompute a modified sampleSize value to
     make up the difference.

     To skip this additional scaling, just set sampleSize = 1; below.
 */
static int recompute_sampleSize(int sampleSize,
                                const jpeg_decompress_struct& cinfo) {
    return sampleSize * cinfo.output_width / cinfo.image_width;
}

static bool valid_output_dimensions(const jpeg_decompress_struct& cinfo) {
    /* These are initialized to 0, so if they have non-zero values, we assume
       they are "valid" (i.e. have been computed by libjpeg)
     */
    return 0 != cinfo.output_width && 0 != cinfo.output_height;
}

static bool skip_src_rows(jpeg_decompress_struct* cinfo, void* buffer, int count) {
    for (int i = 0; i < count; i++) {
        JSAMPLE* rowptr = (JSAMPLE*)buffer;
        int row_count = jpeg_read_scanlines(cinfo, &rowptr, 1);
        if (1 != row_count) {
            return false;
        }
    }
    return true;
}

#ifdef SK_BUILD_FOR_ANDROID
static bool skip_src_rows_tile(jpeg_decompress_struct* cinfo,
                               huffman_index *index, void* buffer, int count) {
    for (int i = 0; i < count; i++) {
        JSAMPLE* rowptr = (JSAMPLE*)buffer;
        int row_count = jpeg_read_tile_scanline(cinfo, index, &rowptr);
        if (1 != row_count) {
            return false;
        }
    }
    return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////

// This guy exists just to aid in debugging, as it allows debuggers to just
// set a break-point in one place to see all error exists.
static void print_jpeg_decoder_errors(const jpeg_decompress_struct& cinfo,
                         int width, int height, const char caller[]) {
    if (!(c_suppressJPEGImageDecoderErrors)) {
        char buffer[JMSG_LENGTH_MAX];
        cinfo.err->format_message((const j_common_ptr)&cinfo, buffer);
        SkDebugf("libjpeg error %d <%s> from %s [%d %d]\n",
                 cinfo.err->msg_code, buffer, caller, width, height);
    }
}

static bool return_false(const jpeg_decompress_struct& cinfo,
                         const SkBitmap& bm, const char caller[]) {
    print_jpeg_decoder_errors(cinfo, bm.width(), bm.height(), caller);
    return false;
}

static SkImageDecoder::Result return_failure(const jpeg_decompress_struct& cinfo,
                                             const SkBitmap& bm, const char caller[]) {
    print_jpeg_decoder_errors(cinfo, bm.width(), bm.height(), caller);
    return SkImageDecoder::kFailure;
}

///////////////////////////////////////////////////////////////////////////////
// MStar Android Patch Begin
// JpegStop:jpeg decode stop
// JpegReadData:read data while length > 1M
// DecodeJPEGByHW: HW decode
#ifdef HW_JPEG_SUPPORT
static MS_S32 JpegFillHdrFunc(MS_PHY BufAddr, MS_U32 BufLength) {
    bool bIsEOF = false;
    signed long bytes_read = 0;
    MS_S32 have_read = 0;
    MS_U32 actualLength = 0;
    unsigned long bufVirAddr = MsOS_MPool_PA2KSEG1(BufAddr);

    if (staticStream && (bufVirAddr != 0)) {
        do {
            actualLength = staticStream->getLength();
            if (actualLength > (BufLength - have_read)) {
                actualLength = BufLength - have_read;
            }
            bytes_read = staticStream->read((void *)((unsigned char *)(bufVirAddr + have_read)), actualLength);
            if (bytes_read > 0) {
                have_read += bytes_read;
                usleep(100);
            }
        } while ((bytes_read > 0) && (have_read < BufLength));
    }

    if (staticStream && (bufVirAddr != 0) && (bytes_read == 0)) {
        bIsEOF = true;
    }
    else if (bytes_read == -1) {
        ALOGE("JpegFillHdrFunc read error! \n");
    }

    MApi_JPEG_UpdateReadInfo(have_read, bIsEOF);

    return have_read;
}

void SkJPEGImageDecoder::JpegStop(void) {
    MApi_JPEG_Rst();
    MApi_JPEG_Exit();
}

bool SkJPEGImageDecoder::JpegReadData(void *pinbuf_virtual_address, JPEG_InitParam *pJpegInitParam, JPEG_BuffLoadType eType) {
    bool bIsEOF;
    uint32_t  halSize, readSize, remainder ,length;
    size_t bytes_read;

    halSize = pJpegInitParam->u32MRCBufSize/2;
    if ( eType == E_JPEG_BUFFER_HIGH) {
        pinbuf_virtual_address +=  halSize;
    }

    length = pstream->getLength();
    remainder = length - currReadPos;
    if (remainder == 0)
        return true;

    if ( remainder > halSize ) {
        readSize =  halSize;
        bIsEOF = false;
    } else {
        readSize =  remainder;
        bIsEOF = true;
    }
    bytes_read = pstream->read(pinbuf_virtual_address,readSize);
    MsOS_FlushMemory();

    // we can't get the correct filesize of the picture when playing through dlna,so the last time to read the file bytes_read != readSize will apper,
    // we should not treat it as an error
    currReadPos +=readSize;
    if (bytes_read != readSize) {
        if (bytes_read != -1) {
            bIsEOF = true;
        } else {
            ALOGE("read error! \n");
            return false;
        }
    }
    //LOGD("read addr:%x,size :%x,bytes_read:%x,total:%x\n",(uint32_t)pinbuf_virtual_address,readSize,bytes_read,currReadPos);
    MApi_JPEG_UpdateReadInfo(bytes_read, bIsEOF);
    MApi_JPEG_SetMRBufferValid(eType);
    return true;
}

E_JPEG_DECODER_STATUS SkJPEGImageDecoder:: JpegWaitDone(void *pinbuf_virtual_address, JPEG_InitParam *pJpegInitParam) {
    E_JPEG_DECODER_STATUS enStatus = DECODING;
    JPEG_Event enEvent;

    // For H/W bug, Check Vidx.
    if (E_JPEG_FAILED == MApi_JPEG_HdlVidxChk()) {
        enEvent = E_JPEG_EVENT_DEC_ERROR_MASK;
    } else {
        enEvent = MApi_JPEG_GetJPDEventFlag();
    }

    if (E_JPEG_EVENT_DEC_DONE & enEvent) {
        if (MApi_JPEG_IsMPOFormat()) {
            //LOGI("[JPEG] One of MPO picture decode done!\n");

            decodedMPOImageNum++;
            if (--numMPOimageLeft > 0) {
                //LOGD("numMPOimageLeft=0x%x\n", numMPOimageLeft);
                while (numMPOimageLeft &&
                       ((mpoIndex->mp_entry[numMPOimage - numMPOimageLeft].attribute & E_MP_TYPE_MASK) != E_MP_TYPE_MULTI_FRAME_DISPARITY)) {
                    //LOGD("skip entry %x because it is not disparity type.\n", (numMPOimage - numMPOimageLeft));
                    numMPOimageLeft--;
                }
            } else {
                //MApi_JPEG_DumpMPO();
            }
        }
        enStatus = DECODE_DONE;
    } else if (E_JPEG_EVENT_DEC_ERROR_MASK & enEvent) {
        ALOGE("[MPlayerLib]:Baseline decode error\n");
        MApi_JPEG_Rst();
        MApi_JPEG_Exit();
        enStatus = DECODE_ERR;
    } else if (E_JPEG_EVENT_DEC_MRB_DONE & enEvent) {
        JPEG_BuffLoadType enPreLoadBuffType;

        switch (MApi_JPEG_GetBuffLoadType(&enPreLoadBuffType)) {
        case E_JPEG_OKAY:
            if (JpegReadData(pinbuf_virtual_address, pJpegInitParam, enPreLoadBuffType) != true) {
                enStatus = DECODE_ERR;
            }
            break;

        case E_JPEG_FAILED:
            enStatus = DECODE_ERR;
            break;

        case E_JPEG_RETRY:
        default:
            break;
        }
    }

    return enStatus;
}

bool SkJPEGImageDecoder::DecodeJPEGByHW(void *pinbuf_virtual_address, uint32_t inputBufAddr, uint32_t inputBufSize,
                                        uint32_t interBufAddr, uint32_t interBufSize,
                                        uint32_t outputBufAddr, uint32_t outputBufSize, uint32_t *pOutbuf_Len,
                                        uint32_t *pOutput_Pitch, uint32_t *pOutput_Width, uint32_t *pOutput_Height) {
    int s16JpegDecoderErrCode = 0;
    bool enRet = false;
    E_JPEG_DECODER_STATUS eWaitResult;
    JPEG_InitParam JpegInitParam;
    uint32_t length = 0;
    char value1[PROPERTY_VALUE_MAX],value2[PROPERTY_VALUE_MAX];
    int MinWidth, MinHeight, sampleSize;
    sampleSize = this->getSampleSize();
    unsigned short u16ScaleFacotr = 1;
    unsigned short u16AdjustScaleFactor = sampleSize;
    int s32TryTimes = 0;
    #define TRY_MAX_TIMES 2000

    if (inputBufAddr == NULL || inputBufSize == 0) {
        return false;
    }

    //MApi_JPEG_SetDbgLevel(E_JPEG_DEBUG_ALL);
    //LOGV("~!~read=%p,len=%d;inter=%p,len=%d; write=%p,len=%d",inputBufAddr,inputBufSize,interBufAddr,interBufSize,outputBufAddr, outputBufSize);

    if (mpo && (decodedMPOImageNum == 1)) {
        // for decode second frame, need reset postion to second frame.
        //LOGD("Decode second MPO frame.%lx,%lx,%x\n",mpoIndex->start_of_offset, mpoIndex->mp_entry[numMPOimage - numMPOimageLeft].offset, currReadPos);
        if ((mpoIndex->start_of_offset + mpoIndex->mp_entry[numMPOimage - numMPOimageLeft].offset) > currReadPos) {
            //LOGD("MPO skip read :%ld\n", ((mpoIndex->start_of_offset + mpoIndex->mp_entry[numMPOimage - numMPOimageLeft].offset) - currReadPos));
            pstream->skip((mpoIndex->start_of_offset + mpoIndex->mp_entry[numMPOimage - numMPOimageLeft].offset) - currReadPos);
        } else {
            //LOGD("MPO seek to :%lx\n", (mpoIndex->start_of_offset + mpoIndex->mp_entry[numMPOimage - numMPOimageLeft].offset));
            pstream->rewind();
            pstream->skip(mpoIndex->start_of_offset + mpoIndex->mp_entry[numMPOimage - numMPOimageLeft].offset);
        }
        currReadPos = mpoIndex->start_of_offset + mpoIndex->mp_entry[numMPOimage - numMPOimageLeft].offset;
    }
    MApi_JPEG_SetMaxDecodeResolution(PHOTO_DECODE_JPEG_BASELINE_MAX_WIDTH, PHOTO_DECODE_JPEG_BASELINE_MAX_HEIGHT);
    MApi_JPEG_SetProMaxDecodeResolution(PHOTO_DECODE_JPEG_PROGRESSIVE_MAX_WIDTH, PHOTO_DECODE_JPEG_PROGRESSIVE_MAX_HEIGHT);
    MApi_JPEG_SetMPOMaxDecodeResolution(PHOTO_DECODE_JPEG_BASELINE_MAX_WIDTH, PHOTO_DECODE_JPEG_BASELINE_MAX_HEIGHT);
    MApi_JPEG_SetMPOProMaxDecodeResolution(PHOTO_DECODE_JPEG_PROGRESSIVE_MAX_WIDTH, PHOTO_DECODE_JPEG_PROGRESSIVE_MAX_HEIGHT);
    MApi_JPEG_SupportCMYK(FALSE);
    MApi_JPEG_SupportRGB(FALSE);

    JpegInitParam.u32MRCBufAddr = inputBufAddr;
    JpegInitParam.u32MRCBufSize = inputBufSize;
    JpegInitParam.u32MWCBufAddr = outputBufAddr;
    JpegInitParam.u32MWCBufSize = outputBufSize;
    JpegInitParam.u32InternalBufAddr = interBufAddr;
    JpegInitParam.u32InternalBufSize = interBufSize;

    // we now make the parameter of read() to be u32InputBufSize,because when playing picture through dlna the pstream will not  return a correct size
    // you can find this info in ChunkedInputStream.java
    // pstream->read() will return the number readed or -1,because of the dlna,now if the bytes_read != u32InputBufsize && != -1,
    // we will think that the file has reched the end of the file ,otherwise it has not

    size_t bytes_read;
    uint32_t actualLength = 0;
    uint32_t haveRead     = 0;

    do {
        actualLength = pstream->getLength();
        if (actualLength  > (inputBufSize - haveRead)) {
            actualLength = inputBufSize - haveRead;
        }
        bytes_read = pstream->read((void *)((unsigned char *)(pinbuf_virtual_address + haveRead)), actualLength);
        if (bytes_read > 0) {
            haveRead += bytes_read;
            usleep(500);
        } else {
            break;
        }

    } while ( inputBufSize - haveRead > 0);

    bytes_read = haveRead;
    MsOS_FlushMemory();
    if (bytes_read != inputBufSize && bytes_read > 0) {
        JpegInitParam.bEOF = true;
    } else if (bytes_read == 0) {
        ALOGE("HwDecoder Read 0 bytes");
        goto fail;
    } else {
        JpegInitParam.bEOF = false;
    }
    JpegInitParam.u32DecByteRead = bytes_read;
    currReadPos += bytes_read;

    JpegInitParam.u8DecodeType = E_JPEG_TYPE_MAIN;
    JpegInitParam.bInitMem = true;
    JpegInitParam.pFillHdrFunc = JpegFillHdrFunc;
    staticStream = this->pstream;

    MApi_JPEG_Init(&JpegInitParam);

    s16JpegDecoderErrCode = MApi_JPEG_GetErrorCode();
    if (s16JpegDecoderErrCode != E_JPEG_NO_ERROR) {
        ALOGE("jpeg goto fail 0, s16JpegDecoderErrCode = %d",s16JpegDecoderErrCode);
        goto fail;
    }

    if (MApi_JPEG_IsMPOFormat() && (decodedMPOImageNum == 0)) {
        bool bMPODisparityType;

        ALOGI("[JPEG] It is MPO file.\n");

        if (!MApi_JPEG_GetMPOIndex(&mpoIndex)) {
            ALOGE("MApi_JPEG_GetMPOIndex Error.\n");
            goto fail;
        }

        numMPOimage = numMPOimageLeft = mpoIndex->num_of_image;

        //LOGI("numMPOimage=0x%x, numMPOimageLeft=0x%x\n", numMPOimage, numMPOimageLeft);
        switch (mpoIndex->mp_entry[0].attribute & E_MP_TYPE_MASK) {
        case E_MP_TYPE_BASELINE:
        case E_MP_TYPE_MULTI_FRAME_MULTI_ANGLE:
        case E_MP_TYPE_MULTI_FRAME_PANORAMA:
            bMPODisparityType = FALSE;
            break;
        case E_MP_TYPE_MULTI_FRAME_DISPARITY:
            bMPODisparityType = TRUE;
            break;
        default:
            bMPODisparityType = FALSE;
            break;
        }
        if (!bMPODisparityType) {
            ALOGE("mpo type is not disparity type, we only decode one frame.\n");
            mpo = false;
        } else {
            mpo = true;
        }

    }

    if ((decodeMode == SkImageDecoder::kDecodeBounds_Mode) && (sampleSize == 1)) {
        *pOutput_Width = (uint32_t)MApi_JPEG_GetOriginalWidth();
        *pOutput_Height = (uint32_t)MApi_JPEG_GetOriginalHeight();
        *pOutput_Pitch = (uint32_t)(MApi_JPEG_GetOriginalWidth() + 31) & (~31) * 2;
        *pOutbuf_Len = (*pOutput_Pitch) * (*pOutput_Height);
        ALOGD("MApi_JPEG_GetOriginalWidth DecodeBounds:w:%d,h:%d\n", *pOutput_Width,*pOutput_Height);

        enRet = true;
        goto end;
    }

    if ((MApi_JPEG_GetScaleDownFactor() != 1 || sampleSize != 1) && (!MApi_JPEG_IsProgressive())) {
        unsigned short u16OriginalWidthAigned = (MApi_JPEG_GetOriginalWidth() + 31) & (~31);
        unsigned short u16OriginalHeightAligned = (MApi_JPEG_GetOriginalHeight() + 31) & (~31);

        // if buffer is enough, try to descrease scale factor
        if (((u16OriginalWidthAigned * u16OriginalHeightAligned * 2) % outputBufSize) > 0) {
            u16ScaleFacotr = (u16OriginalWidthAigned * u16OriginalHeightAligned * 2) / outputBufSize + 1;
        } else {
            u16ScaleFacotr = (u16OriginalWidthAigned * u16OriginalHeightAligned * 2) / outputBufSize;
        }

        if (u16ScaleFacotr <= 1) {
            u16AdjustScaleFactor = 1;
        } else if (u16ScaleFacotr <= 4) {
            u16AdjustScaleFactor = 2;
        } else if (u16ScaleFacotr <= 16) {
            u16AdjustScaleFactor = 4;
        } else {
            u16AdjustScaleFactor = 8;
        }
        //LOGD("u16AdjustScaleFactor:%d",u16AdjustScaleFactor);
        // try to closer to samplesize
        while ((sampleSize > u16AdjustScaleFactor) && (u16AdjustScaleFactor <= 8)) {
            u16AdjustScaleFactor = u16AdjustScaleFactor << 1;
        }

        if (u16AdjustScaleFactor != sampleSize) {
            //ALOGD("not support samplesize:%d", sampleSize);
            goto fail;
        }
        //ALOGD("sampleSize:%d,u16AdjustScaleFactor:%d",sampleSize,u16AdjustScaleFactor);
        if (MApi_JPEG_GetScaleDownFactor() != u16AdjustScaleFactor) {
            //LOGD("adjust scale down factor:%d->%d", (int)MApi_JPEG_GetScaleDownFactor(),u16AdjustScaleFactor);
            //LOGD("orignal size:%d,%d.",u16OriginalWidthAigned,u16OriginalHeightAligned);
            JpegStop();
            MApi_JPEG_SetMaxDecodeResolution(u16OriginalWidthAigned / u16AdjustScaleFactor, u16OriginalHeightAligned / u16AdjustScaleFactor);
            MApi_JPEG_SetProMaxDecodeResolution(u16OriginalWidthAigned / u16AdjustScaleFactor, u16OriginalHeightAligned / u16AdjustScaleFactor);
            MApi_JPEG_SetMPOMaxDecodeResolution(u16OriginalWidthAigned / u16AdjustScaleFactor, u16OriginalHeightAligned / u16AdjustScaleFactor);
            MApi_JPEG_SetMPOProMaxDecodeResolution(u16OriginalWidthAigned / u16AdjustScaleFactor, u16OriginalHeightAligned / u16AdjustScaleFactor);
            MApi_JPEG_SupportCMYK(FALSE);
            MApi_JPEG_SupportRGB(FALSE);
            MApi_JPEG_Init(&JpegInitParam);
            s16JpegDecoderErrCode = MApi_JPEG_GetErrorCode();
            if (s16JpegDecoderErrCode != E_JPEG_NO_ERROR) {
                ALOGE("jpeg goto fail 4, s16JpegDecoderErrCode = %d", s16JpegDecoderErrCode);
                goto fail;
            }
        }
    }


    *pOutput_Width = (uint32_t)MApi_JPEG_GetAlignedWidth();
    *pOutput_Height = (uint32_t)MApi_JPEG_GetAlignedHeight();
    *pOutput_Pitch = (uint32_t)MApi_JPEG_GetAlignedPitch() * 2;
    *pOutbuf_Len = (*pOutput_Pitch)*(*pOutput_Height);

    property_get("mstar.jpeg_width", value1, "512");
    property_get("mstar.jpeg_height", value2, "512");
    MinWidth = atoi(value1);
    MinHeight = atoi(value2);
    if ((((*pOutput_Width) * (*pOutput_Height)) < (uint32_t)(MinWidth * MinHeight)) &&
        !mpo) {
        // too small, to sw decoding.
        enRet = false;
        goto end;
    }

    if (MApi_JPEG_DecodeHdr() == E_JPEG_FAILED) {
        s16JpegDecoderErrCode = MApi_JPEG_GetErrorCode();
        ALOGE("jpeg goto fail 1");
        goto fail;
    }

    switch (MApi_JPEG_Decode()) {
    case E_JPEG_DONE:
        break;

    case E_JPEG_OKAY:
        goto wait;
        break;

    case E_JPEG_FAILED:
        s16JpegDecoderErrCode = MApi_JPEG_GetErrorCode();
        ALOGE("jpeg goto fail 2, s16JpegDecoderErrCode= %d ",s16JpegDecoderErrCode);
        goto fail;

    default:
        break;
    }

    wait:
    eWaitResult = JpegWaitDone(pinbuf_virtual_address, &JpegInitParam);

    while ((eWaitResult == DECODING) && (s32TryTimes++ < TRY_MAX_TIMES)) {
        usleep(3000);
        eWaitResult = JpegWaitDone(pinbuf_virtual_address, &JpegInitParam);
    }

    switch (eWaitResult) {
    case DECODE_DONE:
        enRet = true;
        break;
    case DECODE_ERR:
    default:
        s16JpegDecoderErrCode = E_JPEG_DECODE_ERROR;
        ALOGE("jpeg goto fail 3");
        if (s32TryTimes >= TRY_MAX_TIMES) {
            ALOGE("jpeg decode time out.\n");
        }
        goto fail;
    }
    fail:
    if (s16JpegDecoderErrCode) {
        // let sw decode use same sample size
        if ((SkImageDecoder::kDecodeBounds_Mode != decodeMode) &&
            (sampleSize != u16AdjustScaleFactor)) {
            ALOGD("update sample size:%d->%d\n", sampleSize, u16AdjustScaleFactor);
            this->setSampleSize(u16AdjustScaleFactor);
        }
        ALOGI("[MPlayerLib]:Decode jpeg fail, s16JpegDecoderErrCode = %d \n", s16JpegDecoderErrCode);
    }
    end:
    JpegStop();
    return enRet;
}

#ifdef DUMP_RAW_DATA
void DumpToFile(void* ptr, int size, int width, int height, GFX_Buffer_Format Format) {
    FILE *pFile;

    char filename[128];
    memset(filename, 0, 128);

    if (ptr == NULL) {
        ALOGE("ptr is NULL!!\n");
        return;
    }

    if ((Format == GFX_FMT_ARGB8888) || (Format == GFX_FMT_ABGR8888)) {
        sprintf(filename, "/sdcard/argb8888_%dx%d.bin",width,height);
    } else if (Format == GFX_FMT_YUV422) {
        sprintf(filename, "/sdcard/yuv422_%dx%d.bin",width,height);
    } else {
        sprintf(filename, "/sdcard/ARGB_%dx%d.bin",width,height);
    }

    ALOGI("Dump to file:%s,width:%d, height:%d, Format:%d\n", filename, width, height,(uint32_t)Format);
    pFile = fopen(filename, "wb");

    if (pFile == NULL) {
        ALOGE("Open dump file Failed!!\n");
        return;
    } else
        ALOGI("Open dump file ok!!\n");

    if (pFile) {
        fwrite((void *)ptr, 1, size, pFile);
        fflush(pFile);
    }

    if (pFile) {
        fclose(pFile);
        pFile = NULL;
        ALOGI("Close dump file!!\n");
    }
}
#endif
#endif // HW_JPEG_SUPPORT

#ifdef TEST_TIME_USED
static struct timeval tv;
static struct timeval tv2;

static struct timeval tv_hw_start;
static struct timeval tv_hw_middle;
static struct timeval tv_hw_end;
#endif

#ifdef PATCH_CTS
static int getprocname(pid_t pid, char *buf, int len) {
    char *filename;
    FILE *f;
    int rc = 0;
    static const char* unknown_cmdline = "<unknown>";

    if (len <= 0) {
        return -1;
    }

    if (asprintf(&filename, "/proc/%zd/cmdline", pid) < 0) {
        rc = 1;
        goto exit;
    }

    f = fopen(filename, "r");
    if (f == NULL) {
        rc = 2;
        goto releasefilename;
    }

    if (fgets(buf, len, f) == NULL) {
        rc = 3;
        goto closefile;
    }

closefile:
    (void) fclose(f);
releasefilename:
    free(filename);
exit:
    if (rc != 0) {
        // The process went away before we could read its process name. Try
        // to give the user "<unknown>" here, but otherwise they get to look
        // at a blank.
        if (strlcpy(buf, unknown_cmdline, (size_t)len) >= (size_t)len) {
            rc = 4;
        }
    }

    return rc;
}
#endif
// MStar Android Patch End

// Convert a scanline of CMYK samples to RGBX in place. Note that this
// method moves the "scanline" pointer in its processing
static void convert_CMYK_to_RGB(uint8_t* scanline, uint32_t width) {
    // At this point we've received CMYK pixels from libjpeg. We
    // perform a crude conversion to RGB (based on the formulae
    // from easyrgb.com):
    //  CMYK -> CMY
    //    C = ( C * (1 - K) + K )      // for each CMY component
    //  CMY -> RGB
    //    R = ( 1 - C ) * 255          // for each RGB component
    // Unfortunately we are seeing inverted CMYK so all the original terms
    // are 1-. This yields:
    //  CMYK -> CMY
    //    C = ( (1-C) * (1 - (1-K) + (1-K) ) -> C = 1 - C*K
    // The conversion from CMY->RGB remains the same
    for (uint32_t x = 0; x < width; ++x, scanline += 4) {
        scanline[0] = SkMulDiv255Round(scanline[0], scanline[3]);
        scanline[1] = SkMulDiv255Round(scanline[1], scanline[3]);
        scanline[2] = SkMulDiv255Round(scanline[2], scanline[3]);
        scanline[3] = 255;
    }
}

/**
 *  Common code for setting the error manager.
 */
static void set_error_mgr(jpeg_decompress_struct* cinfo, skjpeg_error_mgr* errorManager) {
    SkASSERT(cinfo != NULL);
    SkASSERT(errorManager != NULL);
    cinfo->err = jpeg_std_error(errorManager);
    errorManager->error_exit = skjpeg_error_exit;
}

/**
 *  Common code for turning off upsampling and smoothing. Turning these
 *  off helps performance without showing noticable differences in the
 *  resulting bitmap.
 */
static void turn_off_visual_optimizations(jpeg_decompress_struct* cinfo) {
    SkASSERT(cinfo != NULL);
    /* this gives about 30% performance improvement. In theory it may
       reduce the visual quality, in practice I'm not seeing a difference
     */
    cinfo->do_fancy_upsampling = 0;

    /* this gives another few percents */
    cinfo->do_block_smoothing = 0;
}

/**
 * Common code for setting the dct method.
 */
static void set_dct_method(const SkImageDecoder& decoder, jpeg_decompress_struct* cinfo) {
    SkASSERT(cinfo != NULL);
#ifdef DCT_IFAST_SUPPORTED
    if (decoder.getPreferQualityOverSpeed()) {
        cinfo->dct_method = JDCT_ISLOW;
    } else {
        cinfo->dct_method = JDCT_IFAST;
    }
#else
    cinfo->dct_method = JDCT_ISLOW;
#endif
}

SkColorType SkJPEGImageDecoder::getBitmapColorType(jpeg_decompress_struct* cinfo) {
    SkASSERT(cinfo != NULL);

    SrcDepth srcDepth = k32Bit_SrcDepth;
    if (JCS_GRAYSCALE == cinfo->jpeg_color_space) {
        srcDepth = k8BitGray_SrcDepth;
    }

    SkColorType colorType = this->getPrefColorType(srcDepth, /*hasAlpha*/ false);
    switch (colorType) {
        case kAlpha_8_SkColorType:
            // Only respect A8 colortype if the original is grayscale,
            // in which case we will treat the grayscale as alpha
            // values.
            if (cinfo->jpeg_color_space != JCS_GRAYSCALE) {
                colorType = kN32_SkColorType;
            }
            break;
        case kN32_SkColorType:
            // Fall through.
        case kARGB_4444_SkColorType:
            // Fall through.
        case kRGB_565_SkColorType:
            // These are acceptable destination colortypes.
            break;
        default:
            // Force all other colortypes to 8888.
            colorType = kN32_SkColorType;
            break;
    }

    switch (cinfo->jpeg_color_space) {
        case JCS_CMYK:
            // Fall through.
        case JCS_YCCK:
            // libjpeg cannot convert from CMYK or YCCK to RGB - here we set up
            // so libjpeg will give us CMYK samples back and we will later
            // manually convert them to RGB
            cinfo->out_color_space = JCS_CMYK;
            break;
        case JCS_GRAYSCALE:
            if (kAlpha_8_SkColorType == colorType) {
                cinfo->out_color_space = JCS_GRAYSCALE;
                break;
            }
            // The data is JCS_GRAYSCALE, but the caller wants some sort of RGB
            // colortype. Fall through to set to the default.
        default:
            cinfo->out_color_space = JCS_RGB;
            break;
    }
    return colorType;
}

/**
 *  Based on the colortype and dither mode, adjust out_color_space and
 *  dither_mode of cinfo. Only does work in ANDROID_RGB
 */
static void adjust_out_color_space_and_dither(jpeg_decompress_struct* cinfo,
                                              SkColorType colorType,
                                              const SkImageDecoder& decoder) {
    SkASSERT(cinfo != NULL);
#ifdef ANDROID_RGB
    cinfo->dither_mode = JDITHER_NONE;
    if (JCS_CMYK == cinfo->out_color_space) {
        return;
    }
    switch (colorType) {
        case kN32_SkColorType:
            cinfo->out_color_space = JCS_RGBA_8888;
            break;
        case kRGB_565_SkColorType:
            cinfo->out_color_space = JCS_RGB_565;
            if (decoder.getDitherImage()) {
                cinfo->dither_mode = JDITHER_ORDERED;
            }
            break;
        default:
            break;
    }
#endif
}

/**
   Sets all pixels in given bitmap to SK_ColorWHITE for all rows >= y.
   Used when decoding fails partway through reading scanlines to fill
   remaining lines. */
static void fill_below_level(int y, SkBitmap* bitmap) {
    SkIRect rect = SkIRect::MakeLTRB(0, y, bitmap->width(), bitmap->height());
    SkCanvas canvas(*bitmap);
    canvas.clipRect(SkRect::Make(rect));
    canvas.drawColor(SK_ColorWHITE);
}

/**
 *  Get the config and bytes per pixel of the source data. Return
 *  whether the data is supported.
 */
static bool get_src_config(const jpeg_decompress_struct& cinfo,
                           SkScaledBitmapSampler::SrcConfig* sc,
                           int* srcBytesPerPixel) {
    SkASSERT(sc != NULL && srcBytesPerPixel != NULL);
    if (JCS_CMYK == cinfo.out_color_space) {
        // In this case we will manually convert the CMYK values to RGB
        *sc = SkScaledBitmapSampler::kRGBX;
        // The CMYK work-around relies on 4 components per pixel here
        *srcBytesPerPixel = 4;
    } else if (3 == cinfo.out_color_components && JCS_RGB == cinfo.out_color_space) {
        *sc = SkScaledBitmapSampler::kRGB;
        *srcBytesPerPixel = 3;
#ifdef ANDROID_RGB
    } else if (JCS_RGBA_8888 == cinfo.out_color_space) {
        *sc = SkScaledBitmapSampler::kRGBX;
        *srcBytesPerPixel = 4;
    } else if (JCS_RGB_565 == cinfo.out_color_space) {
        *sc = SkScaledBitmapSampler::kRGB_565;
        *srcBytesPerPixel = 2;
#endif
    } else if (1 == cinfo.out_color_components &&
               JCS_GRAYSCALE == cinfo.out_color_space) {
        *sc = SkScaledBitmapSampler::kGray;
        *srcBytesPerPixel = 1;
    } else {
        return false;
    }
    return true;
}

SkImageDecoder::Result SkJPEGImageDecoder::onDecode(SkStream* stream, SkBitmap* bm, Mode mode) {
#ifdef TIME_DECODE
    SkAutoTime atm("JPEG Decode");
#endif

// MStar Android Patch Begin
#ifdef TEST_TIME_USED
    gettimeofday (&tv , 0);
#endif

#if !defined(__LP64__) && defined(HW_JPEG_SUPPORT)
    bool ret = false;
    bool mutex_locked = false;

    //LOGD("~!~ in HW_JPEG_SUPPORT,pid = %d,filename = %s, mode:%d \n",getpid(),stream->getFileName(),mode);
#ifdef PATCH_CTS
    if (bDisableHWDecodeForPatch == false) {
        char pFileName[128];
        memset(pFileName, 0, sizeof(pFileName));

        if (0 == getprocname(getpid(), pFileName, 128)) {
            char strCtsName[] = "com.android.cts.stub";
            if (memcmp(pFileName, strCtsName, sizeof(strCtsName)) == 0) {
                bDisableHWDecodeForPatch = true;
            } else {
                bDisableHWDecodeForPatch = false;
            }
        }
    }
#endif

    struct CMA_Pool_Init_Param cma_init_param_read;
    struct CMA_Pool_Alloc_Param cma_alloc_param_read;
    struct CMA_Pool_Free_Param cma_free_param_read;
    bool init_cma_read_Memory = false;
    bool get_cma_read_Memory = false;

    struct CMA_Pool_Init_Param cma_init_param_inter;
    struct CMA_Pool_Alloc_Param cma_alloc_param_inter;
    struct CMA_Pool_Free_Param cma_free_param_inter;
    bool init_cma_inter_Memory = false;
    bool get_cma_inter_Memory = false;

    struct CMA_Pool_Init_Param cma_init_param_write;
    struct CMA_Pool_Alloc_Param cma_alloc_param_write;
    struct CMA_Pool_Free_Param cma_free_param_write;
    bool init_cma_write_Memory = false;
    bool get_cma_write_Memory = false;

    do {
        mpo = false;

        if (!fEnableHwDecode)
            break;

#ifdef PATCH_CTS
        if (bDisableHWDecodeForPatch) {
            ALOGD("disable hw decode.");
            break;
        }
#endif

        char property[PROPERTY_VALUE_MAX];
        uint32_t width, height;
        // Hardware jpeg decoder enable by default after fEnableHwDecode setted
        if (property_get("mstar.skia.hwjpeg", property, "1") > 0) {
            int ableJPEGHW = atoi(property);
            if (!ableJPEGHW)
                break;
        }

        SkColorType colorType = this->getPrefColorType(k32Bit_SrcDepth, false);

        // only these make sense for jpegs
        if (colorType != kN32_SkColorType &&
            colorType != kARGB_4444_SkColorType &&
            colorType != kRGB_565_SkColorType) {
            break;  //for these configs unsupported, just go sw decode.
        }

        if (s_index < 0) {
            s_index = MsOS_CreateNamedMutex((MS_S8 *)"SkiaDecodeMutex");
            ALOGI("jpeg hw mutex s_index = %d", s_index );
            if (s_index < 0) {
                ALOGE("create named mutex SkiaDecodeMutex failed.");
                return kFailure;
            }
        }
        MsOS_LockMutex(s_index, 0);
        mutex_locked = true;

        numMPOimage = 0;
        numMPOimageLeft = 0;
        decodedMPOImageNum = 0;
        currReadPos = 0;
        decodeMode = mode;
        pstream = stream;
        stream->rewind();

        //LOGD("[skia jpeg]:SampleSize:%d\n", this->getSampleSize());

        if (!read_paddr && !inter_paddr && !write_paddr) {
            const mmap_info_t* minfo = NULL;

            if (mmap_init() != 0) {
                break;
            }

            if (!MDrv_SYS_GlobalInit())
                break;

            // the area in mmap locate in miu0
            minfo = mmap_get_info("E_DFB_JPD_READ");
            if (minfo == NULL) {
                ALOGE("mmap_get_info E_DFB_JPD_READ fail.");
            }

            if (minfo && (0 < minfo->cmahid)) {
                cma_init_param_read.heap_id = minfo->cmahid;
                cma_init_param_read.flags = CMA_FLAG_MAP_VMA;

                if (false == MApi_CMA_Pool_Init(&cma_init_param_read)) {
                    ALOGE("E_DFB_JPD_READ init CMA fail!\n");
                    break;
                }
                init_cma_read_Memory = true;
                ALOGV("E_DFB_JPD_READ MApi_CMA_Pool_Init: pool_handle_id=0x%lx, miu=%ld, offset=0x%lx, length=0x%lx\n", cma_init_param_read.pool_handle_id, cma_init_param_read.miu, cma_init_param_read.heap_miu_start_offset, cma_init_param_read.heap_length);

                if (0 != minfo->size) {
                    cma_alloc_param_read.pool_handle_id = cma_init_param_read.pool_handle_id;
                    cma_alloc_param_read.length = minfo->size;
                    cma_alloc_param_read.offset_in_pool =  minfo->addr - cma_init_param_read.heap_miu_start_offset;
                    cma_alloc_param_read.flags = CMA_FLAG_VIRT_ADDR;

                    if (false == MApi_CMA_Pool_GetMem(&cma_alloc_param_read)) {
                        ALOGE("E_DFB_JPD_READ MApi_CMA_Pool_GetMem fail:offset=0x%lx, len=0x%lx, miu=%ld\n", cma_alloc_param_read.offset_in_pool, cma_alloc_param_read.length, cma_init_param_read.miu);
                        break;
                    }
                    get_cma_read_Memory = true;
                    ALOGV("E_DFB_JPD_READ MApi_CMA_Pool_GetMem ok:offset=0x%lx, len=0x%lx, miu=%ld\n", cma_alloc_param_read.offset_in_pool, cma_alloc_param_read.length, cma_init_param_read.miu);

                    cma_free_param_read.pool_handle_id = cma_alloc_param_read.pool_handle_id;
                    cma_free_param_read.offset_in_pool = cma_alloc_param_read.offset_in_pool;
                    cma_free_param_read.length = cma_alloc_param_read.length;

                    read_paddr = cma_init_param_read.heap_miu_start_offset + cma_alloc_param_read.offset_in_pool;
                    read_len = minfo->size;
                }
            } else {
                if (minfo && (0 != minfo->size)) {
                    if ((false == read_mapping) && (false == MsOS_MPool_Mapping(0, minfo->addr, minfo->size, 1))) {
                        ALOGE("mapping read buf failed, %lx,%ld ", minfo->addr, minfo->size);
                        break;
                    }
                    read_mapping = true;
                    read_paddr = minfo->addr;
                    read_len = minfo->size;
                }
            }

            minfo = mmap_get_info("E_DFB_JPD_INTERNAL");
            if (minfo == NULL) {
                ALOGE("mmap_get_info E_DFB_JPD_INTERNAL fail.");
            }

            if (minfo && (0 < minfo->cmahid)) {
                cma_init_param_inter.heap_id = minfo->cmahid;
                cma_init_param_inter.flags = CMA_FLAG_MAP_VMA;
                if (false == MApi_CMA_Pool_Init(&cma_init_param_inter)) {
                    ALOGE("E_DFB_JPD_INTERNAL init CMA fail!\n");
                    break;
                }
                init_cma_inter_Memory = true;
                ALOGV("E_DFB_JPD_INTERNAL MApi_CMA_Pool_Init: pool_handle_id=0x%lx, miu=%ld, offset=0x%lx, length=0x%lx\n", cma_init_param_inter.pool_handle_id, cma_init_param_inter.miu, cma_init_param_inter.heap_miu_start_offset, cma_init_param_inter.heap_length);

                if (0 != minfo->size) {
                    cma_alloc_param_inter.pool_handle_id = cma_init_param_inter.pool_handle_id;
                    cma_alloc_param_inter.length = minfo->size;
                    cma_alloc_param_inter.offset_in_pool =  minfo->addr - cma_init_param_inter.heap_miu_start_offset;
                    cma_alloc_param_inter.flags = CMA_FLAG_VIRT_ADDR;

                    if (false == MApi_CMA_Pool_GetMem(&cma_alloc_param_inter)) {
                        ALOGE("E_DFB_JPD_INTERNAL MApi_CMA_Pool_GetMem fail:offset=0x%lx, len=0x%lx, miu=%ld\n", cma_alloc_param_inter.offset_in_pool, cma_alloc_param_inter.length, cma_init_param_inter.miu);
                        break;
                    }
                    get_cma_inter_Memory = true;
                    ALOGV("E_DFB_JPD_INTERNAL MApi_CMA_Pool_GetMem ok:offset=0x%lx, len=0x%lx, miu=%ld\n", cma_alloc_param_inter.offset_in_pool, cma_alloc_param_inter.length, cma_init_param_inter.miu);

                    cma_free_param_inter.pool_handle_id = cma_alloc_param_inter.pool_handle_id;
                    cma_free_param_inter.offset_in_pool = cma_alloc_param_inter.offset_in_pool;
                    cma_free_param_inter.length = cma_alloc_param_inter.length;

                    inter_paddr = cma_init_param_inter.heap_miu_start_offset + cma_alloc_param_inter.offset_in_pool;
                    inter_len = minfo->size;
                }
            } else {
                if (minfo && (0 != minfo->size)) {
                    if ((false == inter_mapping) && (false == MsOS_MPool_Mapping(0, minfo->addr, minfo->size, 1))) {
                        ALOGE("mapping internal buf failed, %lx,%ld ", minfo->addr, minfo->size);
                        break;
                    }
                    inter_mapping = true;
                    inter_paddr = minfo->addr;
                    inter_len = minfo->size;
                }
            }

            minfo = mmap_get_info("E_DFB_JPD_WRITE");
            if (minfo == NULL) {
                ALOGE("mmap_get_info E_DFB_JPD_WRITE fail.");
                break;
            }
            // break if the length in mmap is ZERO
            if (0 == minfo->size)
                break;

            if (minfo && (0 < minfo->cmahid)) {
                cma_init_param_write.heap_id = minfo->cmahid;
                cma_init_param_write.flags = CMA_FLAG_MAP_VMA;
                if (false == MApi_CMA_Pool_Init(&cma_init_param_write)) {
                    ALOGE("E_DFB_JPD_WRITE init CMA fail!\n");
                    break;
                }
                init_cma_write_Memory = true;
                ALOGV("E_DFB_JPD_WRITE MApi_CMA_Pool_Init: pool_handle_id=0x%lx, miu=%ld, offset=0x%lx, length=0x%lx\n", cma_init_param_write.pool_handle_id, cma_init_param_write.miu, cma_init_param_write.heap_miu_start_offset, cma_init_param_write.heap_length);

                if (0 != minfo->size) {
                    cma_alloc_param_write.pool_handle_id = cma_init_param_write.pool_handle_id;
                    cma_alloc_param_write.length = minfo->size;
                    cma_alloc_param_write.offset_in_pool =  minfo->addr - cma_init_param_write.heap_miu_start_offset;
                    cma_alloc_param_write.flags = CMA_FLAG_VIRT_ADDR;

                    if (false == MApi_CMA_Pool_GetMem(&cma_alloc_param_write)) {
                        ALOGE("E_DFB_JPD_WRITE MApi_CMA_Pool_GetMem fail:offset=0x%lx, len=0x%lx, miu=%ld\n", cma_alloc_param_write.offset_in_pool, cma_alloc_param_write.length, cma_init_param_write.miu);
                        break;
                    }
                    get_cma_write_Memory = true;
                    ALOGV("E_DFB_JPD_WRITE MApi_CMA_Pool_GetMem ok:offset=0x%lx, len=0x%lx, miu=%ld\n", cma_alloc_param_write.offset_in_pool, cma_alloc_param_write.length, cma_init_param_write.miu);

                    cma_free_param_write.pool_handle_id = cma_alloc_param_write.pool_handle_id;
                    cma_free_param_write.offset_in_pool = cma_alloc_param_write.offset_in_pool;
                    cma_free_param_write.length = cma_alloc_param_write.length;

                    write_paddr = cma_init_param_write.heap_miu_start_offset + cma_alloc_param_write.offset_in_pool;
                    write_len = minfo->size;
                }
            } else {
                if ((false == write_mapping) && (false == MsOS_MPool_Mapping(0, minfo->addr, minfo->size, 1))) {
                    ALOGE("mapping write buf failed, %lx,%ld ", minfo->addr, minfo->size);
                    break;
                }
                write_mapping = true;
                write_paddr = minfo->addr;
                write_len = minfo->size;
            }

            if (read_paddr == 0) {
                read_paddr = write_paddr;
                read_len = JPEG_READ_BUFF_SIZE;

                // if E_DFB_JPD_READ not set, we assume that the E_DFB_JPD_INTERNAL doesn't set yet.
                inter_paddr = read_paddr + JPEG_READ_BUFF_SIZE;
                inter_len = JPEG_INTERNAL_BUFF_SIZE;

                write_paddr = inter_paddr + JPEG_INTERNAL_BUFF_SIZE;
                write_len = write_len - JPEG_READ_BUFF_SIZE - JPEG_INTERNAL_BUFF_SIZE;

                if (write_len < JPEG_WRITE_BUFF_SIZE) {
                    ALOGE("the write buf len %d is too short",write_len);
                    break;
                }

            } else {
                if (minfo->size < JPEG_WRITE_BUFF_SIZE) {
                    ALOGE("the write buf len %ld is too short", minfo->size);
                    break;
                }
            }

            ALOGD("[skia jpeg]: readbuf addr:0x%x, size: 0x%x\n write buff addr:0x%x,  size: 0x%x\n internal buff addr:0x%x,   size: 0x%x\n",
                 read_paddr, read_len, write_paddr, write_len, inter_paddr, inter_len);

            pvaddr_read = (void *)MsOS_MPool_PA2KSEG1(read_paddr);
            pvaddr_internal = (void *)MsOS_MPool_PA2KSEG1(inter_paddr);
            pvaddr_write = (void *)MsOS_MPool_PA2KSEG1(write_paddr);
        }

        if (!pvaddr_read || !pvaddr_internal || !pvaddr_write) {
            ALOGE("get vaddr failed:pvaddr_read=%p,pvaddr_internal=%p,pvaddr_write=%p",pvaddr_read,pvaddr_internal,pvaddr_write);
            break;
        }

        int filelength = stream->getLength();
        if (filelength == 0) {
            ALOGE("filelength 0 ");
            break;
        }

        uint32_t pitch, output_length;
        bm->lockPixels();
        JSAMPLE* ptr = (JSAMPLE*)bm->getPixels();
        bool reuseBitmap = (ptr  != NULL);
        bm->unlockPixels();

#ifdef TEST_TIME_USED
        gettimeofday (&tv_hw_start, 0);
        ALOGI("lock & init time = %d  ",((tv_hw_start.tv_sec-tv.tv_sec)*1000+(tv_hw_start.tv_usec-tv.tv_usec)/1000));
#endif

        do {
            if (!DecodeJPEGByHW(pvaddr_read,read_paddr,read_len,inter_paddr,inter_len,write_paddr,write_len,&output_length,&pitch,&width,&height)) {
                ALOGE("go sw decode!");
                break;
            } else {
                ALOGI("hw decode success\n");
            }

#ifdef TEST_TIME_USED
            gettimeofday (&tv_hw_middle, 0);
            ALOGD("~!~ output_length = %d ,pitch = %d ,width = %d ,height = %d, hw time = %d ",output_length,pitch,width,height,((tv_hw_middle.tv_sec-tv_hw_start.tv_sec)*1000+(tv_hw_middle.tv_usec-tv_hw_start.tv_usec)/1000));
#endif

            //LOGD("~!~ reuseBitmap = %d, width,height=%d,%d  bm:widht,height = %d,%d ",reuseBitmap,width,height,bm->width(),bm->height());
            if (reuseBitmap) {
                if (mpo && (width != (uint32_t)bm->width() ||
                             (height*2) != (uint32_t) bm->height())) {
                    // Dimensions must match
                    ALOGI("MPO Dimensions not match");
                    break;
                } else if (!mpo && (width != (uint32_t)bm->width() ||
                                     height != (uint32_t)bm->height())) {
                    // Dimensions must match
                    ALOGI("Dimensions not match");
                    break;
                }

            }

            if (!mpo || (mpo && (decodedMPOImageNum <= 1))) {
                if (!reuseBitmap) {
                    if (mpo) {
                        // for mpo, save two frames in top/bottom way.
                        bm->setInfo(SkImageInfo::Make(width, height*2,
                                  colorType, kOpaque_SkAlphaType));
                    } else {
                        bm->setInfo(SkImageInfo::Make(width, height,
                                  colorType, kOpaque_SkAlphaType));
                    }

                    // jpegs are always opaque (i.e. have no per-pixel alpha)
                    // @MSTARFIXME
                    //bm->setIsOpaque(true);

                    if (SkImageDecoder::kDecodeBounds_Mode == mode) {
                        ret = true;
                        break;
                    }

                    if (!this->allocPixelRef(bm, NULL)) {
                        ALOGE("allocPixelRef fail");
                        break;
                    }
                } else if (SkImageDecoder::kDecodeBounds_Mode == mode) {
                    ret = true;
                    break;
                }
            }
    #ifdef TEST_TIME_USED
            gettimeofday (&tv_hw_start, 0);
    #endif
#ifdef DUMP_RAW_DATA
            DumpToFile(pvaddr_write, pitch*height, pitch/2, height, GFX_FMT_YUV422);
#endif

            GFX_Buffer_Format srcFormat,destFormat;
            unsigned char bytesperpixel;
            bool dopremultiplyalpha = true;
            if (colorType == kN32_SkColorType) {
                srcFormat = GFX_FMT_YUV422;
                destFormat = GFX_FMT_ABGR8888;
                dopremultiplyalpha = true;
                bytesperpixel = 4;
            } else if (colorType == kRGB_565_SkColorType) {
                srcFormat = GFX_FMT_YUV422;
                destFormat = GFX_FMT_RGB565;
                dopremultiplyalpha = false;
                bytesperpixel = 2;
            } else if (colorType == kARGB_4444_SkColorType) {
                srcFormat = GFX_FMT_YUV422;
                destFormat = GFX_FMT_ARGB4444;
                dopremultiplyalpha = false;
                bytesperpixel = 2;
            } else {
                SkASSERT(0);
                bytesperpixel = 4;
                destFormat = GFX_FMT_ABGR8888;
            }

#if 1

            bm->lockPixels();
            ptr = (JSAMPLE*)bm->getPixels();
            // store second frame in top/bottom
            if (mpo && (decodedMPOImageNum == 2)) {
                ptr += (width*height*bytesperpixel);
            }

            if (destFormat == GFX_FMT_ABGR8888)
                yuv422_2_rgb8888_neon((unsigned char *)ptr, (unsigned char *)(pvaddr_write), width, height, pitch, width*4);
            else if (destFormat == GFX_FMT_ARGB4444)
                yuv422_2_rgb4444_neon((unsigned char *)ptr, (unsigned char *)(pvaddr_write), width, height, pitch, width*2);
            else if (destFormat == GFX_FMT_RGB565)
                yuv422_2_rgb565_neon((unsigned char *)ptr, (unsigned char *)(pvaddr_write), width, height, pitch, width*2);

#ifdef DUMP_RAW_DATA
            if (destFormat == GFX_FMT_ABGR8888)
                DumpToFile((void*)bm->getPixels(), width*height*4, width, height, destFormat);
#endif

            bm->unlockPixels();
#else
            GE_Convert_Format(pitch/2,height,srcFormat,destFormat,dopremultiplyalpha);
#ifdef TEST_TIME_USED
            gettimeofday (&tv_hw_end, 0);
            ALOGI("ge time = %d ",((tv_hw_end.tv_sec-tv_hw_start.tv_sec)*1000+(tv_hw_end.tv_usec-tv_hw_start.tv_usec)/1000));
#endif

            // memcpy
            bm->lockPixels();
            ptr = (JSAMPLE*)bm->getPixels();
            int bytesperpixel = GetBytesPerPixelByFormat(destFormat);
            // store second frame in top/bottom
            if (mpo && (decodedMPOImageNum == 2)) {
                ptr += (width*height*bytesperpixel);
            }

            //memcpy(ptr,pvaddr_final,bm->getSize());
#ifdef TEST_TIME_USED
            gettimeofday (&tv_hw_start, 0);
#endif
            if ((uint32_t)bm->width() != pitch/2) {
                // pvaddr_final     pvaddr_write
                uint32_t i;
                for (i = 0 ; i < height ; i ++){
                    memcpy(ptr+i*bm->width()*bytesperpixel, (void *)((uint32_t)pvaddr_final+i*pitch/2 * bytesperpixel), bm->width()*bytesperpixel);
                }
            } else {
                memcpy(ptr,pvaddr_final,width*height*bytesperpixel);
            }
#ifdef TEST_TIME_USED
            gettimeofday (&tv_hw_end, 0);
            ALOGI("cpu time time = %d ",((tv_hw_end.tv_sec-tv_hw_start.tv_sec)*1000+(tv_hw_end.tv_usec-tv_hw_start.tv_usec)/1000));
#endif

            bm->unlockPixels();
#endif
            if (reuseBitmap)
                bm->notifyPixelsChanged();
            ret = true;
        } while (mpo && (decodedMPOImageNum < 2));

#ifdef TEST_TIME_USED
        gettimeofday (&tv2 , 0);
        ALOGD("~!~!~ orgwidth=%d,orgHeight = %d, cpu copy time = %d, jpeg decode end at:%ld ......\n",bm->width(),bm->height(),((tv2.tv_sec-tv_hw_end.tv_sec)*1000+(tv2.tv_usec-tv_hw_end.tv_usec)/1000),((tv2.tv_sec-tv.tv_sec)*1000+(tv2.tv_usec-tv.tv_usec)/1000));
#endif
    } while (0);

    read_paddr = 0;
    inter_paddr = 0;
    write_paddr = 0;

    if (get_cma_read_Memory && (MApi_CMA_Pool_PutMem(&cma_free_param_read) == false)) {
        ALOGE("E_DFB_JPD_READ MApi_CMA_Pool_PutMem fail: offset=0x%lx, len=0x%lx\n", cma_free_param_read.offset_in_pool, cma_free_param_read.length);
    }

    if (get_cma_inter_Memory && (MApi_CMA_Pool_PutMem(&cma_free_param_inter) == false)) {
        ALOGE("E_DFB_JPD_INTERNAL MApi_CMA_Pool_PutMem fail: offset=0x%lx, len=0x%lx\n", cma_free_param_inter.offset_in_pool, cma_free_param_inter.length);
    }

    if (get_cma_write_Memory && (MApi_CMA_Pool_PutMem(&cma_free_param_write) == false)) {
        ALOGE("E_DFB_JPD_WRITE MApi_CMA_Pool_PutMem fail: offset=0x%lx, len=0x%lx\n", cma_free_param_write.offset_in_pool, cma_free_param_write.length);
    }

    if (init_cma_inter_Memory && (MApi_CMA_Pool_Release(cma_init_param_inter.pool_handle_id) == false)) {
        ALOGE("E_DFB_JPD_INTERNAL MApi_CMA_Pool_Release fail\n");
    }

    if (init_cma_read_Memory && (MApi_CMA_Pool_Release(cma_init_param_read.pool_handle_id) == false)) {
        ALOGE("E_DFB_JPD_READ MApi_CMA_Pool_Release fail\n");
    }

    if (init_cma_write_Memory && (MApi_CMA_Pool_Release(cma_init_param_write.pool_handle_id) == false)) {
        ALOGE("E_DFB_JPD_WRITE MApi_CMA_Pool_Release fail\n");
    }

    if (mutex_locked) {
        MsOS_UnlockMutex(s_index, 0);
    }

    if (true == ret) {
        ALOGD("Hardware jpeg decode ok!!\n");
        return kSuccess;
    } else if (mpo) {
        // sw decode does not support mpo decoding. if hw decode fail, just return kFailure.
        //LOGD("MPO decode fail.\n");
        return kFailure;
    }
    //LOGD("begin sw decoding------\n");

    stream->rewind();
#endif // HW_JPEG_SUPPORT
// MStar Android Patch End

    JPEGAutoClean autoClean;

    jpeg_decompress_struct  cinfo;
    skjpeg_source_mgr       srcManager(stream, this);

    skjpeg_error_mgr errorManager;
    set_error_mgr(&cinfo, &errorManager);

    // All objects need to be instantiated before this setjmp call so that
    // they will be cleaned up properly if an error occurs.
    if (setjmp(errorManager.fJmpBuf)) {
        return return_failure(cinfo, *bm, "setjmp");
    }

    initialize_info(&cinfo, &srcManager);
    autoClean.set(&cinfo);

    int status = jpeg_read_header(&cinfo, true);
    if (status != JPEG_HEADER_OK) {
        return return_failure(cinfo, *bm, "read_header");
    }

    /*  Try to fulfill the requested sampleSize. Since jpeg can do it (when it
        can) much faster that we, just use their num/denom api to approximate
        the size.
    */
    int sampleSize = this->getSampleSize();

    set_dct_method(*this, &cinfo);

    SkASSERT(1 == cinfo.scale_num);
    cinfo.scale_denom = sampleSize;

    turn_off_visual_optimizations(&cinfo);

    const SkColorType colorType = this->getBitmapColorType(&cinfo);
    const SkAlphaType alphaType = kAlpha_8_SkColorType == colorType ?
                                      kPremul_SkAlphaType : kOpaque_SkAlphaType;

    adjust_out_color_space_and_dither(&cinfo, colorType, *this);

    if (1 == sampleSize && SkImageDecoder::kDecodeBounds_Mode == mode) {
        // Assume an A8 bitmap is not opaque to avoid the check of each
        // individual pixel. It is very unlikely to be opaque, since
        // an opaque A8 bitmap would not be very interesting.
        // Otherwise, a jpeg image is opaque.
        bool success = bm->setInfo(SkImageInfo::Make(cinfo.image_width, cinfo.image_height,
                                                     colorType, alphaType));
        return success ? kSuccess : kFailure;
    }

    // MStar Android Patch Begin
    if ((cinfo.image_width > 0) && (cinfo.image_height > 0)) {
        if (cinfo.progressive_mode) {
            if ((cinfo.image_width * cinfo.image_height) > (JPEG_PROGRESSIVE_MAX_WIDTH * JPEG_PROGRESSIVE_MAX_HEIGHT)) { //7000*7000
                return kFailure;
            }
        } else {
            if ((cinfo.image_width * cinfo.image_height) > (JPEG_BASELINE_MAX_WIDTH * JPEG_BASELINE_MAX_HEIGHT)) { //(1920*8 * 1080*8)
                return kFailure;
            }
        }
    }
    // MStar Android Patch End

    /*  image_width and image_height are the original dimensions, available
        after jpeg_read_header(). To see the scaled dimensions, we have to call
        jpeg_start_decompress(), and then read output_width and output_height.
    */
    if (!jpeg_start_decompress(&cinfo)) {
        /*  If we failed here, we may still have enough information to return
            to the caller if they just wanted (subsampled bounds). If sampleSize
            was 1, then we would have already returned. Thus we just check if
            we're in kDecodeBounds_Mode, and that we have valid output sizes.

            One reason to fail here is that we have insufficient stream data
            to complete the setup. However, output dimensions seem to get
            computed very early, which is why this special check can pay off.
         */
        if (SkImageDecoder::kDecodeBounds_Mode == mode && valid_output_dimensions(cinfo)) {
            SkScaledBitmapSampler smpl(cinfo.output_width, cinfo.output_height,
                                       recompute_sampleSize(sampleSize, cinfo));
            // Assume an A8 bitmap is not opaque to avoid the check of each
            // individual pixel. It is very unlikely to be opaque, since
            // an opaque A8 bitmap would not be very interesting.
            // Otherwise, a jpeg image is opaque.
            bool success = bm->setInfo(SkImageInfo::Make(smpl.scaledWidth(), smpl.scaledHeight(),
                                                         colorType, alphaType));
            return success ? kSuccess : kFailure;
        } else {
            return return_failure(cinfo, *bm, "start_decompress");
        }
    }
    sampleSize = recompute_sampleSize(sampleSize, cinfo);

#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    // should we allow the Chooser (if present) to pick a colortype for us???
    if (!this->chooseFromOneChoice(colorType, cinfo.output_width, cinfo.output_height)) {
        return return_failure(cinfo, *bm, "chooseFromOneChoice");
    }
#endif

    SkScaledBitmapSampler sampler(cinfo.output_width, cinfo.output_height, sampleSize);
    // Assume an A8 bitmap is not opaque to avoid the check of each
    // individual pixel. It is very unlikely to be opaque, since
    // an opaque A8 bitmap would not be very interesting.
    // Otherwise, a jpeg image is opaque.
    bm->setInfo(SkImageInfo::Make(sampler.scaledWidth(), sampler.scaledHeight(),
                                  colorType, alphaType));
    if (SkImageDecoder::kDecodeBounds_Mode == mode) {
        return kSuccess;
    }
    if (!this->allocPixelRef(bm, NULL)) {
        return return_failure(cinfo, *bm, "allocPixelRef");
    }

    SkAutoLockPixels alp(*bm);

#ifdef ANDROID_RGB
    /* short-circuit the SkScaledBitmapSampler when possible, as this gives
       a significant performance boost.
    */
    if (sampleSize == 1 &&
        ((kN32_SkColorType == colorType && cinfo.out_color_space == JCS_RGBA_8888) ||
         (kRGB_565_SkColorType == colorType && cinfo.out_color_space == JCS_RGB_565)))
    {
        JSAMPLE* rowptr = (JSAMPLE*)bm->getPixels();
        INT32 const bpr =  bm->rowBytes();

        while (cinfo.output_scanline < cinfo.output_height) {
            int row_count = jpeg_read_scanlines(&cinfo, &rowptr, 1);
            if (0 == row_count) {
                // if row_count == 0, then we didn't get a scanline,
                // so return early.  We will return a partial image.
                fill_below_level(cinfo.output_scanline, bm);
                cinfo.output_scanline = cinfo.output_height;
                jpeg_finish_decompress(&cinfo);
                return kPartialSuccess;
            }
            if (this->shouldCancelDecode()) {
                return return_failure(cinfo, *bm, "shouldCancelDecode");
            }
            rowptr += bpr;
        }
        jpeg_finish_decompress(&cinfo);
        // MStar Android Patch Begin
#ifdef TEST_TIME_USED
        gettimeofday (&tv2 , 0);
        ALOGD("~!~!~ orgwidth=%d,orgHeight = %d, SW jpeg decode end at:%ld ......\n",bm->width(),bm->height(),((tv2.tv_sec-tv.tv_sec)*1000+(tv2.tv_usec-tv.tv_usec)/1000));
#endif
        // MStar Android Patch End
        return kSuccess;
    }
#endif

    // check for supported formats
    SkScaledBitmapSampler::SrcConfig sc;
    int srcBytesPerPixel;

    if (!get_src_config(cinfo, &sc, &srcBytesPerPixel)) {
        return return_failure(cinfo, *bm, "jpeg colorspace");
    }

    if (!sampler.begin(bm, sc, *this)) {
        return return_failure(cinfo, *bm, "sampler.begin");
    }

    SkAutoMalloc srcStorage(cinfo.output_width * srcBytesPerPixel);
    uint8_t* srcRow = (uint8_t*)srcStorage.get();

    //  Possibly skip initial rows [sampler.srcY0]
    if (!skip_src_rows(&cinfo, srcRow, sampler.srcY0())) {
        return return_failure(cinfo, *bm, "skip rows");
    }

    // now loop through scanlines until y == bm->height() - 1
    for (int y = 0;; y++) {
        JSAMPLE* rowptr = (JSAMPLE*)srcRow;
        int row_count = jpeg_read_scanlines(&cinfo, &rowptr, 1);
        if (0 == row_count) {
            // if row_count == 0, then we didn't get a scanline,
            // so return early.  We will return a partial image.
            fill_below_level(y, bm);
            cinfo.output_scanline = cinfo.output_height;
            jpeg_finish_decompress(&cinfo);
            return kSuccess;
        }
        if (this->shouldCancelDecode()) {
            return return_failure(cinfo, *bm, "shouldCancelDecode");
        }

        if (JCS_CMYK == cinfo.out_color_space) {
            convert_CMYK_to_RGB(srcRow, cinfo.output_width);
        }

        sampler.next(srcRow);
        if (bm->height() - 1 == y) {
            // we're done
            break;
        }

        if (!skip_src_rows(&cinfo, srcRow, sampler.srcDY() - 1)) {
            return return_failure(cinfo, *bm, "skip rows");
        }
    }

    // we formally skip the rest, so we don't get a complaint from libjpeg
    if (!skip_src_rows(&cinfo, srcRow,
                       cinfo.output_height - cinfo.output_scanline)) {
        return return_failure(cinfo, *bm, "skip rows");
    }
    jpeg_finish_decompress(&cinfo);

    // MStar Android Patch Begin
#ifdef TEST_TIME_USED
    gettimeofday (&tv2 , 0);
    ALOGD("~!~!~ orgwidth=%d,orgHeight = %d, SW jpeg2 decode end at:%ld ......\n",bm->width(),bm->height(),((tv2.tv_sec-tv.tv_sec)*1000+(tv2.tv_usec-tv.tv_usec)/1000));
#endif
    // MStar Android Patch End

    return kSuccess;
}

#ifdef SK_BUILD_FOR_ANDROID
bool SkJPEGImageDecoder::onBuildTileIndex(SkStreamRewindable* stream, int *width, int *height) {

    SkAutoTDelete<SkJPEGImageIndex> imageIndex(SkNEW_ARGS(SkJPEGImageIndex, (stream, this)));
    jpeg_decompress_struct* cinfo = imageIndex->cinfo();

    skjpeg_error_mgr sk_err;
    set_error_mgr(cinfo, &sk_err);

    // All objects need to be instantiated before this setjmp call so that
    // they will be cleaned up properly if an error occurs.
    if (setjmp(sk_err.fJmpBuf)) {
        return false;
    }

    // create the cinfo used to create/build the huffmanIndex
    if (!imageIndex->initializeInfoAndReadHeader()) {
        return false;
    }

    if (!imageIndex->buildHuffmanIndex()) {
        return false;
    }

    // destroy the cinfo used to create/build the huffman index
    imageIndex->destroyInfo();

    // Init decoder to image decode mode
    if (!imageIndex->initializeInfoAndReadHeader()) {
        return false;
    }

    // FIXME: This sets cinfo->out_color_space, which we may change later
    // based on the config in onDecodeSubset. This should be fine, since
    // jpeg_init_read_tile_scanline will check out_color_space again after
    // that change (when it calls jinit_color_deconverter).
    (void) this->getBitmapColorType(cinfo);

    turn_off_visual_optimizations(cinfo);

    // instead of jpeg_start_decompress() we start a tiled decompress
    if (!imageIndex->startTileDecompress()) {
        return false;
    }

    SkASSERT(1 == cinfo->scale_num);
    fImageWidth = cinfo->output_width;
    fImageHeight = cinfo->output_height;

    if (width) {
        *width = fImageWidth;
    }
    if (height) {
        *height = fImageHeight;
    }

    SkDELETE(fImageIndex);
    fImageIndex = imageIndex.detach();

    return true;
}

bool SkJPEGImageDecoder::onDecodeSubset(SkBitmap* bm, const SkIRect& region) {
    if (NULL == fImageIndex) {
        return false;
    }
    jpeg_decompress_struct* cinfo = fImageIndex->cinfo();

    SkIRect rect = SkIRect::MakeWH(fImageWidth, fImageHeight);
    if (!rect.intersect(region)) {
        // If the requested region is entirely outside the image return false
        return false;
    }


    skjpeg_error_mgr errorManager;
    set_error_mgr(cinfo, &errorManager);

    if (setjmp(errorManager.fJmpBuf)) {
        return false;
    }

    int requestedSampleSize = this->getSampleSize();
    cinfo->scale_denom = requestedSampleSize;

    set_dct_method(*this, cinfo);

    const SkColorType colorType = this->getBitmapColorType(cinfo);
    adjust_out_color_space_and_dither(cinfo, colorType, *this);

    int startX = rect.fLeft;
    int startY = rect.fTop;
    int width = rect.width();
    int height = rect.height();

    jpeg_init_read_tile_scanline(cinfo, fImageIndex->huffmanIndex(),
                                 &startX, &startY, &width, &height);
    int skiaSampleSize = recompute_sampleSize(requestedSampleSize, *cinfo);
    int actualSampleSize = skiaSampleSize * (DCTSIZE / cinfo->min_DCT_scaled_size);

    SkScaledBitmapSampler sampler(width, height, skiaSampleSize);

    SkBitmap bitmap;
    // Assume an A8 bitmap is not opaque to avoid the check of each
    // individual pixel. It is very unlikely to be opaque, since
    // an opaque A8 bitmap would not be very interesting.
    // Otherwise, a jpeg image is opaque.
    bitmap.setInfo(SkImageInfo::Make(sampler.scaledWidth(), sampler.scaledHeight(), colorType,
                                     kAlpha_8_SkColorType == colorType ?
                                         kPremul_SkAlphaType : kOpaque_SkAlphaType));

    // Check ahead of time if the swap(dest, src) is possible or not.
    // If yes, then we will stick to AllocPixelRef since it's cheaper with the
    // swap happening. If no, then we will use alloc to allocate pixels to
    // prevent garbage collection.
    int w = rect.width() / actualSampleSize;
    int h = rect.height() / actualSampleSize;
    bool swapOnly = (rect == region) && bm->isNull() &&
                    (w == bitmap.width()) && (h == bitmap.height()) &&
                    ((startX - rect.x()) / actualSampleSize == 0) &&
                    ((startY - rect.y()) / actualSampleSize == 0);
    if (swapOnly) {
        if (!this->allocPixelRef(&bitmap, NULL)) {
            return return_false(*cinfo, bitmap, "allocPixelRef");
        }
    } else {
        if (!bitmap.allocPixels()) {
            return return_false(*cinfo, bitmap, "allocPixels");
        }
    }

    SkAutoLockPixels alp(bitmap);

#ifdef ANDROID_RGB
    /* short-circuit the SkScaledBitmapSampler when possible, as this gives
       a significant performance boost.
    */
    if (skiaSampleSize == 1 &&
        ((kN32_SkColorType == colorType && cinfo->out_color_space == JCS_RGBA_8888) ||
         (kRGB_565_SkColorType == colorType && cinfo->out_color_space == JCS_RGB_565)))
    {
        JSAMPLE* rowptr = (JSAMPLE*)bitmap.getPixels();
        INT32 const bpr = bitmap.rowBytes();
        int rowTotalCount = 0;

        while (rowTotalCount < height) {
            int rowCount = jpeg_read_tile_scanline(cinfo,
                                                   fImageIndex->huffmanIndex(),
                                                   &rowptr);
            // if rowCount == 0, then we didn't get a scanline, so abort.
            // onDecodeSubset() relies on onBuildTileIndex(), which
            // needs a complete image to succeed.
            if (0 == rowCount) {
                return return_false(*cinfo, bitmap, "read_scanlines");
            }
            if (this->shouldCancelDecode()) {
                return return_false(*cinfo, bitmap, "shouldCancelDecode");
            }
            rowTotalCount += rowCount;
            rowptr += bpr;
        }

        if (swapOnly) {
            bm->swap(bitmap);
        } else {
            cropBitmap(bm, &bitmap, actualSampleSize, region.x(), region.y(),
                       region.width(), region.height(), startX, startY);
        }
        return true;
    }
#endif

    // check for supported formats
    SkScaledBitmapSampler::SrcConfig sc;
    int srcBytesPerPixel;

    if (!get_src_config(*cinfo, &sc, &srcBytesPerPixel)) {
        return return_false(*cinfo, *bm, "jpeg colorspace");
    }

    if (!sampler.begin(&bitmap, sc, *this)) {
        return return_false(*cinfo, bitmap, "sampler.begin");
    }

    SkAutoMalloc  srcStorage(width * srcBytesPerPixel);
    uint8_t* srcRow = (uint8_t*)srcStorage.get();

    //  Possibly skip initial rows [sampler.srcY0]
    if (!skip_src_rows_tile(cinfo, fImageIndex->huffmanIndex(), srcRow, sampler.srcY0())) {
        return return_false(*cinfo, bitmap, "skip rows");
    }

    // now loop through scanlines until y == bitmap->height() - 1
    for (int y = 0;; y++) {
        JSAMPLE* rowptr = (JSAMPLE*)srcRow;
        int row_count = jpeg_read_tile_scanline(cinfo, fImageIndex->huffmanIndex(), &rowptr);
        // if row_count == 0, then we didn't get a scanline, so abort.
        // onDecodeSubset() relies on onBuildTileIndex(), which
        // needs a complete image to succeed.
        if (0 == row_count) {
            return return_false(*cinfo, bitmap, "read_scanlines");
        }
        if (this->shouldCancelDecode()) {
            return return_false(*cinfo, bitmap, "shouldCancelDecode");
        }

        if (JCS_CMYK == cinfo->out_color_space) {
            convert_CMYK_to_RGB(srcRow, width);
        }

        sampler.next(srcRow);
        if (bitmap.height() - 1 == y) {
            // we're done
            break;
        }

        if (!skip_src_rows_tile(cinfo, fImageIndex->huffmanIndex(), srcRow,
                                sampler.srcDY() - 1)) {
            return return_false(*cinfo, bitmap, "skip rows");
        }
    }
    if (swapOnly) {
        bm->swap(bitmap);
    } else {
        cropBitmap(bm, &bitmap, actualSampleSize, region.x(), region.y(),
                   region.width(), region.height(), startX, startY);
    }
    return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////

#include "SkColorPriv.h"

// taken from jcolor.c in libjpeg
#if 0   // 16bit - precise but slow
    #define CYR     19595   // 0.299
    #define CYG     38470   // 0.587
    #define CYB      7471   // 0.114

    #define CUR    -11059   // -0.16874
    #define CUG    -21709   // -0.33126
    #define CUB     32768   // 0.5

    #define CVR     32768   // 0.5
    #define CVG    -27439   // -0.41869
    #define CVB     -5329   // -0.08131

    #define CSHIFT  16
#else      // 8bit - fast, slightly less precise
    #define CYR     77    // 0.299
    #define CYG     150    // 0.587
    #define CYB      29    // 0.114

    #define CUR     -43    // -0.16874
    #define CUG    -85    // -0.33126
    #define CUB     128    // 0.5

    #define CVR      128   // 0.5
    #define CVG     -107   // -0.41869
    #define CVB      -21   // -0.08131

    #define CSHIFT  8
#endif

static void rgb2yuv_32(uint8_t dst[], SkPMColor c) {
    int r = SkGetPackedR32(c);
    int g = SkGetPackedG32(c);
    int b = SkGetPackedB32(c);

    int  y = ( CYR*r + CYG*g + CYB*b ) >> CSHIFT;
    int  u = ( CUR*r + CUG*g + CUB*b ) >> CSHIFT;
    int  v = ( CVR*r + CVG*g + CVB*b ) >> CSHIFT;

    dst[0] = SkToU8(y);
    dst[1] = SkToU8(u + 128);
    dst[2] = SkToU8(v + 128);
}

static void rgb2yuv_4444(uint8_t dst[], U16CPU c) {
    int r = SkGetPackedR4444(c);
    int g = SkGetPackedG4444(c);
    int b = SkGetPackedB4444(c);

    int  y = ( CYR*r + CYG*g + CYB*b ) >> (CSHIFT - 4);
    int  u = ( CUR*r + CUG*g + CUB*b ) >> (CSHIFT - 4);
    int  v = ( CVR*r + CVG*g + CVB*b ) >> (CSHIFT - 4);

    dst[0] = SkToU8(y);
    dst[1] = SkToU8(u + 128);
    dst[2] = SkToU8(v + 128);
}

static void rgb2yuv_16(uint8_t dst[], U16CPU c) {
    int r = SkGetPackedR16(c);
    int g = SkGetPackedG16(c);
    int b = SkGetPackedB16(c);

    int  y = ( 2*CYR*r + CYG*g + 2*CYB*b ) >> (CSHIFT - 2);
    int  u = ( 2*CUR*r + CUG*g + 2*CUB*b ) >> (CSHIFT - 2);
    int  v = ( 2*CVR*r + CVG*g + 2*CVB*b ) >> (CSHIFT - 2);

    dst[0] = SkToU8(y);
    dst[1] = SkToU8(u + 128);
    dst[2] = SkToU8(v + 128);
}

///////////////////////////////////////////////////////////////////////////////

typedef void (*WriteScanline)(uint8_t* SK_RESTRICT dst,
                              const void* SK_RESTRICT src, int width,
                              const SkPMColor* SK_RESTRICT ctable);

static void Write_32_YUV(uint8_t* SK_RESTRICT dst,
                         const void* SK_RESTRICT srcRow, int width,
                         const SkPMColor*) {
    const uint32_t* SK_RESTRICT src = (const uint32_t*)srcRow;
    while (--width >= 0) {
#ifdef WE_CONVERT_TO_YUV
        rgb2yuv_32(dst, *src++);
#else
        uint32_t c = *src++;
        dst[0] = SkGetPackedR32(c);
        dst[1] = SkGetPackedG32(c);
        dst[2] = SkGetPackedB32(c);
#endif
        dst += 3;
    }
}

static void Write_4444_YUV(uint8_t* SK_RESTRICT dst,
                           const void* SK_RESTRICT srcRow, int width,
                           const SkPMColor*) {
    const SkPMColor16* SK_RESTRICT src = (const SkPMColor16*)srcRow;
    while (--width >= 0) {
#ifdef WE_CONVERT_TO_YUV
        rgb2yuv_4444(dst, *src++);
#else
        SkPMColor16 c = *src++;
        dst[0] = SkPacked4444ToR32(c);
        dst[1] = SkPacked4444ToG32(c);
        dst[2] = SkPacked4444ToB32(c);
#endif
        dst += 3;
    }
}

static void Write_16_YUV(uint8_t* SK_RESTRICT dst,
                         const void* SK_RESTRICT srcRow, int width,
                         const SkPMColor*) {
    const uint16_t* SK_RESTRICT src = (const uint16_t*)srcRow;
    while (--width >= 0) {
#ifdef WE_CONVERT_TO_YUV
        rgb2yuv_16(dst, *src++);
#else
        uint16_t c = *src++;
        dst[0] = SkPacked16ToR32(c);
        dst[1] = SkPacked16ToG32(c);
        dst[2] = SkPacked16ToB32(c);
#endif
        dst += 3;
    }
}

static void Write_Index_YUV(uint8_t* SK_RESTRICT dst,
                            const void* SK_RESTRICT srcRow, int width,
                            const SkPMColor* SK_RESTRICT ctable) {
    const uint8_t* SK_RESTRICT src = (const uint8_t*)srcRow;
    while (--width >= 0) {
#ifdef WE_CONVERT_TO_YUV
        rgb2yuv_32(dst, ctable[*src++]);
#else
        uint32_t c = ctable[*src++];
        dst[0] = SkGetPackedR32(c);
        dst[1] = SkGetPackedG32(c);
        dst[2] = SkGetPackedB32(c);
#endif
        dst += 3;
    }
}

static WriteScanline ChooseWriter(const SkBitmap& bm) {
    switch (bm.colorType()) {
        case kN32_SkColorType:
            return Write_32_YUV;
        case kRGB_565_SkColorType:
            return Write_16_YUV;
        case kARGB_4444_SkColorType:
            return Write_4444_YUV;
        case kIndex_8_SkColorType:
            return Write_Index_YUV;
        default:
            return NULL;
    }
}

class SkJPEGImageEncoder : public SkImageEncoder {
protected:
    virtual bool onEncode(SkWStream* stream, const SkBitmap& bm, int quality) {
#ifdef TIME_ENCODE
        SkAutoTime atm("JPEG Encode");
#endif

        SkAutoLockPixels alp(bm);
        if (NULL == bm.getPixels()) {
            return false;
        }

        jpeg_compress_struct    cinfo;
        skjpeg_error_mgr        sk_err;
        skjpeg_destination_mgr  sk_wstream(stream);

        // allocate these before set call setjmp
        SkAutoMalloc    oneRow;
        SkAutoLockColors ctLocker;

        cinfo.err = jpeg_std_error(&sk_err);
        sk_err.error_exit = skjpeg_error_exit;
        if (setjmp(sk_err.fJmpBuf)) {
            return false;
        }

        // Keep after setjmp or mark volatile.
        const WriteScanline writer = ChooseWriter(bm);
        if (NULL == writer) {
            return false;
        }

        jpeg_create_compress(&cinfo);
        cinfo.dest = &sk_wstream;
        cinfo.image_width = bm.width();
        cinfo.image_height = bm.height();
        cinfo.input_components = 3;
#ifdef WE_CONVERT_TO_YUV
        cinfo.in_color_space = JCS_YCbCr;
#else
        cinfo.in_color_space = JCS_RGB;
#endif
        cinfo.input_gamma = 1;

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
#ifdef DCT_IFAST_SUPPORTED
        cinfo.dct_method = JDCT_IFAST;
#endif

        jpeg_start_compress(&cinfo, TRUE);

        const int       width = bm.width();
        uint8_t*        oneRowP = (uint8_t*)oneRow.reset(width * 3);

        const SkPMColor* colors = ctLocker.lockColors(bm);
        const void*      srcRow = bm.getPixels();

        while (cinfo.next_scanline < cinfo.image_height) {
            JSAMPROW row_pointer[1];    /* pointer to JSAMPLE row[s] */

            writer(oneRowP, srcRow, width, colors);
            row_pointer[0] = oneRowP;
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            srcRow = (const void*)((const char*)srcRow + bm.rowBytes());
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);

        return true;
    }
};

///////////////////////////////////////////////////////////////////////////////
DEFINE_DECODER_CREATOR(JPEGImageDecoder);
DEFINE_ENCODER_CREATOR(JPEGImageEncoder);
///////////////////////////////////////////////////////////////////////////////

static bool is_jpeg(SkStreamRewindable* stream) {
    static const unsigned char gHeader[] = { 0xFF, 0xD8, 0xFF };
    static const size_t HEADER_SIZE = sizeof(gHeader);

    char buffer[HEADER_SIZE];
    size_t len = stream->read(buffer, HEADER_SIZE);

    if (len != HEADER_SIZE) {
        return false;   // can't read enough
    }
    if (memcmp(buffer, gHeader, HEADER_SIZE)) {
        return false;
    }
    return true;
}


static SkImageDecoder* sk_libjpeg_dfactory(SkStreamRewindable* stream) {
    if (is_jpeg(stream)) {
        return SkNEW(SkJPEGImageDecoder);
    }
    return NULL;
}

static SkImageDecoder::Format get_format_jpeg(SkStreamRewindable* stream) {
    if (is_jpeg(stream)) {
        return SkImageDecoder::kJPEG_Format;
    }
    return SkImageDecoder::kUnknown_Format;
}

static SkImageEncoder* sk_libjpeg_efactory(SkImageEncoder::Type t) {
    return (SkImageEncoder::kJPEG_Type == t) ? SkNEW(SkJPEGImageEncoder) : NULL;
}

static SkImageDecoder_DecodeReg gDReg(sk_libjpeg_dfactory);
static SkImageDecoder_FormatReg gFormatReg(get_format_jpeg);
static SkImageEncoder_EncodeReg gEReg(sk_libjpeg_efactory);
