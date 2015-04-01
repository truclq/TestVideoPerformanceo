/*
 * =====================================================================================
 *
 *       Filename:  display_omx.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  10/03/2015
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Erick Le
 *   Organization:
 *
 * =====================================================================================
 */

/* CHANGE HISTORY:
 *   Date        Rev.    Who           Details & Keyword
 *   ==========  ======  ============  ====================
 *   
 */

/***************************************************************************/
/*    Include Header Files                                                 */
/***************************************************************************/
#include <sys/types.h>
#include <asm/types.h>
#include <linux/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <hardware/gralloc.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <cutils/memory.h>
#include <utils/String8.h>
#include <sys/time.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <binder/IMemory.h>
#include <gui/ISurfaceComposer.h>
#include <private/gui/ComposerService.h>
#include "display_omx.h"

#ifndef BUILD_JB
// Android Kitkat 
#include <android/native_window.h>
#endif
//Parcel 
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#define ALIGN(x,a)	(((x) + (a) - 1L) & ~((a) - 1L))
#define HW_ALIGN	(32)
#define RVC_VERSION "1.0.3"
#define LCD_WIDTH 800
#define LCD_HEIGHT 480
#define VIDEO_COMPONENT_NUM				2
/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Local Variables                                                      */
/***************************************************************************/
using namespace android;
sp<SurfaceComposerClient> client[VIDEO_COMPONENT_NUM];
sp<SurfaceControl> surfaceControl[VIDEO_COMPONENT_NUM];
sp<Surface> surface[VIDEO_COMPONENT_NUM];
ANativeWindow* anw[VIDEO_COMPONENT_NUM];

int width[VIDEO_COMPONENT_NUM];
int height[VIDEO_COMPONENT_NUM];
// int is_resize[VIDEO_COMPONENT_NUM] = {-1, -1};
int is_resize[VIDEO_COMPONENT_NUM] = {1, 1};
/****************************************************************************/
/*    Function prototypes (private)                                         */
/****************************************************************************/

/* Deinit the display module */
//int display_deinit();

ANativeWindow* getANativeWindow(int frameWidth, int frameHeight, int auto_scaling, int video_num) {
  printf("IN getANativeWindow\n");
  
  int align_w, align_h;
  float resize_w = 1, resize_h = 1;
  int ret;
  
  if( anw[video_num] == NULL) {
    width[video_num] = frameWidth;
    height[video_num] = frameHeight;
    is_resize[video_num] = auto_scaling;
    int align_w, align_h;
	align_w = ALIGN(frameWidth, HW_ALIGN); //align for hardware renderer (HW_ALIGN = 32)
	align_h = frameHeight; //no need to align height
    printf("\t Create new Surface\n");
    client[video_num] = new SurfaceComposerClient();
    // Need to set position and stuffs
    surfaceControl[video_num] = client[video_num]->createSurface( String8("My Surface"), frameWidth, frameHeight, HAL_PIXEL_FORMAT_YV12, 0);
    SurfaceComposerClient::openGlobalTransaction();
    surfaceControl[video_num]->setLayer(INT_MAX);
    surfaceControl[video_num]->show();
    /*int ret;
	// If width > 854 then resize to 864
		if (align_w > LCD_WIDTH) {
			resize_w = LCD_WIDTH*1.0f/align_w;
			is_resize[video_num] = 1;
		} 
		
		// If height > 480 then resize to 480
		if (align_h > LCD_HEIGHT) {
			resize_h = LCD_HEIGHT*1.0f/align_h;
			is_resize[video_num] = 1;
		} 
		
		if (is_resize[video_num] == 1) {
			surfaceControl[video_num]->setMatrix(resize_w, 0.0, 0.0, resize_h);
			if (ret < 0) {
				printf("Set auto_scalling failed.");
			}
		}*/
	
    align_w = ALIGN(width[video_num], HW_ALIGN); //align for hardware renderer (HW_ALIGN = 32)
    align_h = height[video_num];//ALIGN(height,32); no need to align height
    // If width > 854 then resize to 864
    if (align_w > LCD_WIDTH) {
      resize_w = LCD_WIDTH*1.0f/align_w;
    } 
    
    // If height > 480 then resize to 480
    if (align_h > LCD_HEIGHT) {
      resize_h = LCD_HEIGHT*1.0f/align_h;
    } 
    
    if (is_resize[video_num] == 1) {
      ret = surfaceControl[video_num]->setMatrix(resize_w, 0.0, 0.0, resize_h);
      if (ret < 0) {
	      printf("Set auto_scalling failed.");
      }
    } 
	
    // Set position for surface
    if(video_num == 0) {
      surfaceControl[video_num]->setPosition(0, 0);
      printf("getANativeWindow: Set video %d position at: (%d, %d) \n", video_num, 0, 0);
    }
    else {
      if(is_resize[video_num -1] == -1) {
	surfaceControl[video_num]->setPosition(0, height[video_num-1]);
	printf("getANativeWindow: is_resize=-1. Set video %d position at: (%d, %d) \n", video_num, 0, height[video_num-1]);
      } else if (is_resize[video_num -1] == 1) {
	surfaceControl[video_num]->setPosition(0, LCD_HEIGHT);
	printf("getANativeWindow: is_resize=1. Set video %d position at: (%d, %d) \n", video_num, 0, LCD_HEIGHT);
      }
    }
    SurfaceComposerClient::closeGlobalTransaction();
    surface[video_num] = surfaceControl[video_num]->getSurface();
    
    anw[video_num] = new Surface(surface[video_num]->getIGraphicBufferProducer());
  }
  return anw[video_num];
}

void* getBufferHandle(struct ANativeWindowBuffer* buf) {
	printf("IN getBufferHandle\n");
	printf(" buf = 0x%lx\n", buf);
	sp<GraphicBuffer> graphicBuffer(new GraphicBuffer(buf, false));
	printf("After create GraphicBuffer handle = 0x%lx\n", graphicBuffer->handle);
	// The complicate line below is to get away from this error:
	//     reinterpret_cast from type 'buffer_handle_t {aka const native_handle*}' to type 'void*' casts away qualifiers
	return const_cast<void*>(reinterpret_cast<const void*>(graphicBuffer->handle));
}

int display_deinit(int video_num){

	int ret = 0;
	int i;
	//Destroy surfaceFlinger for next repeat
	client[video_num]->destroySurface(0);
	client[video_num]->dispose();
	anw[video_num] = NULL;
	printf("De-init display module finished.\n");
	return ret;
}
