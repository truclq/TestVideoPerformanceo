/*
 * =====================================================================================
 *
 *       Filename:  parse_h264.cpp
 *
 *    Description:  
 *
 *        Version:  1.0.0
 *        Created:  11/12/2014
 *       Revision:  none
 *       Compiler:  
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

#include <media/stagefright/DataSource.h>
#include <media/stagefright/FileSource.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/Utils.h>  // for U16_AT()
#include <avc_utils.h> // for AVCProfileToString()
#include "parse_h264.h"
/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
//#define DEBUG_OMX	1
#ifdef DEBUG_OMX
 #define DBG_PRINTF(format, args...) printf(format, ##args)
#else
 #define DBG_PRINTF(format, args...)
#endif
#define VIDEO_COMPONENT_NUM				2

/***************************************************************************/
/*    Local Variables                                                      */
/***************************************************************************/
using namespace android;
sp<MediaSource> mpeg4Source[VIDEO_COMPONENT_NUM];
/****************************************************************************/
/*    Function prototypes (private)                                         */
/****************************************************************************/
static int get_specific_data(unsigned char* buffer, unsigned int *buffer_size, int video_num);
static int check_start_code_prefix(unsigned char *ptr);
/***************************************************************************
 * SUMMARY:
 * PARAMETERS:
 * RETURNS: 0 : successful; -1 : error
 * DESCRIPTION: Create media source with filename
 * NOTES:
 ***************************************************************************/

int setDataSource(char* fileName, int video_num)
{
	// Create new FileSource
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : Create new FileSource with fileName = %s.\n", __FUNCTION__, __LINE__, fileName);
	sp<DataSource> dataSource = new FileSource(fileName);
	status_t err = dataSource->initCheck();

    if (OK != err) {
        return -1;
    }
	// register default sniff
	 DataSource::RegisterDefaultSniffers();
	// Create extractor
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : Create new MPEG4Extractor.\n", __FUNCTION__, __LINE__);
	sp<MediaExtractor> extractor = MediaExtractor::Create(dataSource, NULL);

    if (extractor == NULL) {
		printf("[Parse 264 lib] Error create extractor \n");
        return -1;
    }
	
	//create MPEG4Source
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : Create new MPEG4Source.\n", __FUNCTION__, __LINE__);
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : track num = %d .\n", __FUNCTION__, __LINE__, extractor->countTracks());
    for (size_t i = 0; i < extractor->countTracks(); ++i) {
        sp<MetaData> meta = extractor->getTrackMetaData(i);

        const char *_mime = NULL;
		meta->findCString(kKeyMIMEType, &_mime);
        String8 mime = String8(_mime);

        if (!strncasecmp(mime.string(), "video/", 6)) {
			DBG_PRINTF("[Parse 264 lib DBG] %s - %d : extractor->getTrack, i = %d .\n", __FUNCTION__, __LINE__, i);
            mpeg4Source[video_num] = extractor->getTrack(i);
        }
		
	}
	// start MPEG4Source
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : Start MPEG4Source.\n", __FUNCTION__, __LINE__);
	err = mpeg4Source[video_num]->start();
	if (OK != err) {
		printf("[Parse 264 lib] Error Start mpeg4Source \n");
		return -1;
	}
	
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : End setDataSource.\n", __FUNCTION__, __LINE__);
	return 0;
}
/***************************************************************************
 * SUMMARY:
 * PARAMETERS:
 * RETURNS:0 : successful; -1 : error
 * DESCRIPTION: Stop media source.
 * NOTES:
 ***************************************************************************/
int stopDataSource(int video_num)
{
	// stop MPEG4Source
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : Stop MPEG4Source.\n", __FUNCTION__, __LINE__);
	status_t err = mpeg4Source[video_num]->stop();
	if (OK != err) {
		printf("[Parse 264 lib] Error stop mpeg4Source \n");
		return -1;
	}
	
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : End stop DataSource.\n", __FUNCTION__, __LINE__);
	return 0;

}
/***************************************************************************
 * SUMMARY:
 * PARAMETERS:
 * RETURNS: 0 : successful; -1 : error
 * DESCRIPTION: Get NAL unit from Media source
 * NOTES:
 ***************************************************************************/
 int get_frame_data(unsigned char* buffer, unsigned int *frame_size, int first_time, int *remain_size, int *start_code_prefix, int video_num)
 {
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : Enter get frame data.\n", __FUNCTION__, __LINE__);
	MediaBuffer *srcBuffer = NULL;
	*frame_size = 0;
	*remain_size = 1;
	if (first_time) {
		// get SPS & PPS NAL for the first time
		get_specific_data(buffer, frame_size, video_num);
		*start_code_prefix = check_start_code_prefix(buffer);
		return 0;
	} else {
	// read MediaBuffer from MPEG4Source
	status_t err = mpeg4Source[video_num]->read(&srcBuffer);
	if (OK != err) {
		*remain_size = 0;
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : Reach OES remain size = %d \n", __FUNCTION__, __LINE__, *remain_size);
		return 0;
	}
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : Copy buffer data srcBuffer = %p\n", __FUNCTION__, __LINE__, srcBuffer);
	// copy buffer data 
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d :  copy srcBuffer->data() = %p to buffer = %p\n", __FUNCTION__, __LINE__, srcBuffer->data(), buffer);
	memcpy(buffer, srcBuffer->data() + srcBuffer->range_offset(), srcBuffer->range_length());
	*start_code_prefix = check_start_code_prefix(buffer);
	*frame_size = srcBuffer->range_length();
	// dump Media buffer to debug 
	size_t range_length = srcBuffer->range_length();
	size_t size = srcBuffer->size(); 
	size_t range_offset = srcBuffer->range_offset();
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : Media buffer info \n", __FUNCTION__, __LINE__);
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : range_length = %d \n", __FUNCTION__, __LINE__, range_length);
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : size = %d \n", __FUNCTION__, __LINE__, size);
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : range_offset = %d \n", __FUNCTION__, __LINE__, range_offset);
	// release media buffer
	if(NULL != srcBuffer){
			DBG_PRINTF("[Parse 264 lib DBG] %s - %d : Release srcBuffer \n", __FUNCTION__, __LINE__);
			srcBuffer->release();
			srcBuffer = NULL;
		}
	}
	
	return 0;
 }
 /***************************************************************************
 * SUMMARY:
 * PARAMETERS:
 * RETURNS: 0 : successful; -1 : error
 * DESCRIPTION: get 2 SPS and PPS NAL unit
 * NOTES:
 ***************************************************************************/
 static int get_specific_data(unsigned char* buffer, unsigned int *buffer_size, int video_num)
 {
	static const uint8_t kNALStartCode[4] = { 0x00, 0x00, 0x00, 0x01 };
	uint32_t type;
    const void *data;
    size_t size;
	unsigned profile, level;
	// Get  MetaData 
	sp<MetaData> meta = mpeg4Source[video_num]->getFormat();
	if(meta->findData(kKeyAVCC, &type, &data, &size)){
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : type = %d \n", __FUNCTION__, __LINE__, type);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : data = %p \n", __FUNCTION__, __LINE__, data);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : size = %d \n", __FUNCTION__, __LINE__, size);
	}
	// parse profile , level
	const uint8_t *ptr = (const uint8_t *)data;
	profile = ptr[1];
    level = ptr[3];
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : profile = %u (%s) \n", __FUNCTION__, __LINE__, profile, AVCProfileToString(profile));
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : level = %u \n", __FUNCTION__, __LINE__, level);
	// get SPS
	size_t numSeqParameterSets = ptr[5] & 31;
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : numSeqParameterSets = %d \n", __FUNCTION__, __LINE__, numSeqParameterSets);
    ptr += 6;
    size -= 6;
	size_t length = 0; 
	
    for (size_t i = 0; i < numSeqParameterSets; ++i) {
		length = U16_AT(ptr);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : length = %d \n", __FUNCTION__, __LINE__, length);
        ptr += 2;
        size -= 2;
		memcpy(buffer, kNALStartCode, 4);
		memcpy(buffer + 4, ptr, length);
		*buffer_size += length + 4;
        ptr += length;
        size -= length;
    }
	
	// get PPS
	size_t numPictureParameterSets = *ptr;
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : numPictureParameterSets = %d \n", __FUNCTION__, __LINE__, numPictureParameterSets);
	++ptr;
    --size;
	for (size_t i = 0; i < numPictureParameterSets; ++i) {
        length = U16_AT(ptr);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : length = %d \n", __FUNCTION__, __LINE__, length);
        ptr += 2;
        size -= 2;
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : begin copy PPS \n", __FUNCTION__, __LINE__);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : ptr = %p \n", __FUNCTION__, __LINE__, ptr);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : size = %d \n", __FUNCTION__, __LINE__, size);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : buffer = %p \n", __FUNCTION__, __LINE__, buffer + *buffer_size);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : buffer = %p \n", __FUNCTION__, __LINE__, buffer + *buffer_size + 4);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : copy start code to  buffer \n", __FUNCTION__, __LINE__);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : buffer_size = %p \n", __FUNCTION__, __LINE__, *buffer_size);
		//don't use memcpy because it was crashed in KitKat
		buffer[*buffer_size] = {0x00};
		buffer[*buffer_size + 1] = {0x00};
		buffer[*buffer_size + 2] = {0x00};
		buffer[*buffer_size + 3] = {0x01};
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : copy data PPS\n", __FUNCTION__, __LINE__);
		memcpy(buffer + (*buffer_size) + 4, ptr, length);
		(*buffer_size) += length + 4;
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : End copy PPS \n", __FUNCTION__, __LINE__);
		DBG_PRINTF("[Parse 264 lib DBG] %s - %d : buffer_size = %d \n", __FUNCTION__, __LINE__, buffer_size);
    }
	DBG_PRINTF("[Parse 264 lib DBG] %s - %d : End get Specific data \n", __FUNCTION__, __LINE__);
	return 0;
 }
  /***************************************************************************
 * SUMMARY:
 * PARAMETERS:
 * RETURNS: start codec prefix number 
 * DESCRIPTION: check start code prefix of NAL unit.
 * NOTES:
 ***************************************************************************/
 static int check_start_code_prefix(unsigned char *ptr){
	int start_code_prefix = 0;
	if ((ptr[0] == 0x00) && 
		(ptr[1] == 0x00) && 
		(ptr[2] == 0x00) && 
		(ptr[3] == 0x01))
		{
			start_code_prefix = 4;
		}
		else if ((ptr[0] == 0x00) && 
				(ptr[1] == 0x00) && 
				(ptr[2] == 0x01))
		{
			start_code_prefix = 3;
		}
		else
		{
			start_code_prefix = 0;
		}
	return start_code_prefix;
 }

