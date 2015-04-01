/*
 * =====================================================================================
 *
 *       Filename:  overlay_omx.h
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
 
#include <system/window.h>

#ifndef __DISPLAY_OMX_H__
#define __DISPLAY_OMX_H__
/***************************************************************************/
/*    Include Header Files                                                 */
/***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
ANativeWindow* getANativeWindow(int frameWidth, int frameHeight, int auto_scaling, int video_num);
void* getBufferHandle(struct ANativeWindowBuffer* buf);
int display_deinit(int video_num);

#ifdef __cplusplus
};
#endif
#endif __DISPLAY_OMX_H__ /* !__DISPLAY_OMX_H__ */
