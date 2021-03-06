/*
 * Copyright 2017 NXP
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

#include "Ov5640Csi8MQ.h"

Ov5640Csi8MQ::Ov5640Csi8MQ(int32_t id, int32_t facing, int32_t orientation, char* path)
    : Camera(id, facing, orientation, path)
{
    mVideoStream = new OvStream(this);
}

Ov5640Csi8MQ::~Ov5640Csi8MQ()
{
}

int Ov5640Csi8MQ::getCaptureMode(int width, int height)
{
    int capturemode = 0;

    if ((width == 640) && (height == 480)) {
        capturemode = 0;
    }
    else if ((width == 720) && (height == 480)) {
        capturemode = 1;
    }
    else if ((width == 1280) && (height == 720)) {
        capturemode = 2;
    }
    else if ((width == 1920) && (height == 1080)) {
        capturemode = 3;
    }
    else if ((width == 2592) && (height == 1944)) {
        capturemode = 4;
    }
    else {
        ALOGE("width:%d height:%d is not supported.", width, height);
    }
    return capturemode;
}

PixelFormat Ov5640Csi8MQ::getPreviewPixelFormat()
{
    ALOGI("%s", __func__);
    return HAL_PIXEL_FORMAT_YCbCr_422_I;
}

status_t Ov5640Csi8MQ::initSensorStaticData()
{
    int32_t fd = open(mDevPath, O_RDWR);
    if (fd < 0) {
        ALOGE("OvDevice: initParameters sensor has not been opened");
        return BAD_VALUE;
    }

    // first read sensor format.
    int ret = 0, index = 0;
    int sensorFormats[MAX_SENSOR_FORMAT];
    int availFormats[MAX_SENSOR_FORMAT];
    memset(sensorFormats, 0, sizeof(sensorFormats));
    memset(availFormats, 0, sizeof(availFormats));

    // only support yuv422
    sensorFormats[index] = v4l2_fourcc('Y', 'U', 'Y', 'V');
    availFormats[index++] = v4l2_fourcc('Y', 'U', 'Y', 'V');

    mSensorFormatCount = changeSensorFormats(sensorFormats, mSensorFormats, index);
    if (mSensorFormatCount == 0) {
        ALOGE("%s no sensor format enum", __func__);
        close(fd);
        return BAD_VALUE;
    }

    mAvailableFormatCount = changeSensorFormats(availFormats, mAvailableFormats, index);

    ret = 0;
    index = 0;
    char TmpStr[20];
    int  previewCnt = 0, pictureCnt = 0;
    struct v4l2_frmsizeenum vid_frmsize;
    struct v4l2_frmivalenum vid_frmval;
    while (ret == 0) {
        memset(TmpStr, 0, 20);
        memset(&vid_frmsize, 0, sizeof(struct v4l2_frmsizeenum));
        vid_frmsize.index        = index++;
        vid_frmsize.pixel_format = convertPixelFormatToV4L2Format(mSensorFormats[0]);
        ret = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &vid_frmsize);
        if (ret != 0) {
            continue;
        }
        ALOGI("enum frame size w:%d, h:%d",
                vid_frmsize.discrete.width, vid_frmsize.discrete.height);

        // On mscale, resolution less than 720p is not supported, will block on dqbuf
        if((vid_frmsize.discrete.width < 1280) || (vid_frmsize.discrete.height < 720)) {
          ALOGI("omit resolution less 720p");
          continue;
        }

        memset(&vid_frmval, 0, sizeof(struct v4l2_frmivalenum));
        vid_frmval.index        = 0;
        vid_frmval.pixel_format = vid_frmsize.pixel_format;
        vid_frmval.width        = vid_frmsize.discrete.width;
        vid_frmval.height       = vid_frmsize.discrete.height;
        ret = ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &vid_frmval);
        if (ret != 0) {
            continue;
        }
        ALOGI("vid_frmval denominator:%d, numeraton:%d",
                vid_frmval.discrete.denominator,
                vid_frmval.discrete.numerator);

        //If w/h ratio is not same with senserW/sensorH, framework assume that
        //first crop little width or little height, then scale.
        //But 1920x1080, 176x144 not work in this mode.
        // 1920x1080 is required by CTS.
       if(!(vid_frmsize.discrete.width == 1920 && vid_frmsize.discrete.height == 1080)) {
           mPictureResolutions[pictureCnt++] = vid_frmsize.discrete.width;
           mPictureResolutions[pictureCnt++] = vid_frmsize.discrete.height;
       }

       if (vid_frmval.discrete.denominator / vid_frmval.discrete.numerator > 15) {
           mPreviewResolutions[previewCnt++] = vid_frmsize.discrete.width;
           mPreviewResolutions[previewCnt++] = vid_frmsize.discrete.height;
       }
    } // end while

    mPreviewResolutionCount = previewCnt;
    mPictureResolutionCount = pictureCnt;

    mMinFrameDuration = 33331760L;
    mMaxFrameDuration = 30000000000L;
    int i;
    for (i=0; i<MAX_RESOLUTION_SIZE && i<pictureCnt; i+=2) {
        ALOGI("SupportedPictureSizes: %d x %d", mPictureResolutions[i], mPictureResolutions[i+1]);
    }

    adjustPreviewResolutions();
    for (i=0; i<MAX_RESOLUTION_SIZE && i<previewCnt; i+=2) {
        ALOGI("SupportedPreviewSizes: %d x %d", mPreviewResolutions[i], mPreviewResolutions[i+1]);
    }
    ALOGI("FrameDuration is %lld, %lld", mMinFrameDuration, mMaxFrameDuration);

    i = 0;
    mTargetFpsRange[i++] = 10;
    mTargetFpsRange[i++] = 30;
    mTargetFpsRange[i++] = 30;
    mTargetFpsRange[i++] = 30;

    setMaxPictureResolutions();
    ALOGI("mMaxWidth:%d, mMaxHeight:%d", mMaxWidth, mMaxHeight);

    mFocalLength = 3.37f;
    mPhysicalWidth = 3.6288f;   //2592 x 1.4u
    mPhysicalHeight = 2.7216f;  //1944 x 1.4u
    mActiveArrayWidth = 2592;
    mActiveArrayHeight = 1944;
    mPixelArrayWidth = 2592;
    mPixelArrayHeight = 1944;

    ALOGI("ov5640Csi, mFocalLength:%f, mPhysicalWidth:%f, mPhysicalHeight %f",
        mFocalLength, mPhysicalWidth, mPhysicalHeight);

    close(fd);
    return NO_ERROR;
}

int32_t Ov5640Csi8MQ::OvStream::onDeviceConfigureLocked()
{
    ALOGI("%s", __func__);
    int32_t ret = 0;
    if (mDev <= 0) {
        ALOGE("%s invalid fd handle", __func__);
        return BAD_VALUE;
    }

    int32_t fps = mFps;
    int32_t vformat;
    vformat = convertPixelFormatToV4L2Format(mFormat);

    if ((mWidth == 2592) && (mHeight == 1944)) {
        fps = 15;
    } else {
        fps = 30;
    }

    ALOGI("Width * Height %d x %d format %c%c%c%c, fps: %d",
          mWidth, mHeight, vformat&0xFF, (vformat>>8)&0xFF,
          (vformat>>16)&0xFF, (vformat>>24)&0xFF, fps);

    struct v4l2_streamparm param;
    memset(&param, 0, sizeof(param));
    param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    param.parm.capture.timeperframe.numerator   = 1;
    param.parm.capture.timeperframe.denominator = fps;
    param.parm.capture.capturemode = mCamera->getCaptureMode(mWidth, mHeight);
    ret = ioctl(mDev, VIDIOC_S_PARM, &param);
    if (ret < 0) {
        ALOGE("%s: VIDIOC_S_PARM Failed: %s", __func__, strerror(errno));
        return ret;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type                 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width        = mWidth & 0xFFFFFFF8;
    fmt.fmt.pix.height       = mHeight & 0xFFFFFFF8;
    fmt.fmt.pix.pixelformat  = vformat;
    fmt.fmt.pix.priv         = 0;
    fmt.fmt.pix.sizeimage    = 0;
    fmt.fmt.pix.bytesperline = 0;

    ret = ioctl(mDev, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        ALOGE("%s: VIDIOC_S_FMT Failed: %s", __func__, strerror(errno));
        return ret;
    }

    return 0;
}
