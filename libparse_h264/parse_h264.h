/*
 * =====================================================================================
 *
 *       Filename:  parse_h264.h
 *
 *    Description:  
 *
 *        Version:  1.1.0
 *        Created:  11/12/2014
 *       Revision:  none
 *       Compiler:  
 *
 *         Author:  Erick Le
 *   Organization:  
 *
 * =====================================================================================
 */
 

#ifndef _PARSE_H264_H__
#define _PARSE_H264_H__
/***************************************************************************/
/*    Include Header Files                                                 */
/***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

int setDataSource(char* fileName, int video_num);
int stopDataSource(int video_num);
int get_frame_data(unsigned char* buffer, unsigned int *frame_size, int first_time, int *remain_size, int *start_code_prefix, int video_num);
#ifdef __cplusplus
};
#endif
#endif _PARSE_H264_H__ // _PARSE_H264_H__ 