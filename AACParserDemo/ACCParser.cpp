//
//  ACCParser.cpp
//  AACParserDemo
//
//  Created by BZF on 2021/11/5.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * ACC分为：
 * 1、文件格式（ADIF）：用于文件存储和文件播放
 * 2、流格式（ADTS）：用于流媒体在线播放
 *
 *   ADTS的组成：头部信息+ACC裸数据
 *   ADTS Frame的组成：每一帧数据=固定头 + 可变头 + 帧数据
 *
 */
int getADTSframe(unsigned char *buffer, int bufSize, unsigned char *data, int *dataSzie){
    
    int size = 0;
    if(!buffer || !data || !dataSzie){
        return -1;
    }
    
    while (1) {
        if(bufSize < 7){
            return -1;
        }
        
        //判断同步头（synacword） 总是0xFFF 。代表着一个ADTS帧的开始
        if(buffer[0] == 0xff && ((buffer[1] & 0xf0) == 0xf0)){
            size |= ((buffer[3] & 0x03) << 11); //high 2 bit
            size |= buffer[4] << 3;  // middle 8 bit
            size |= (buffer[5] & 0xe0) >> 5; // low 3bit
            break;
        }
        --bufSize;
        ++buffer;
    }
    
    if(bufSize < size){
        return 1;
    }
    
    memcpy(data, buffer, size);
    *dataSzie = size;
    
    return 0;
}


/**
 * 步骤：
 * 1、从码流中搜索0xFFF，分离出ADTS frame
 * 2、然后再分析ADTS frame的首部各个字段
 *
 *
 */
int aacParser(char *url){
    int dataSize = 0;
    int size = 0;
    int count = 0;
    int offset = 0;
    
    //等价与 FILE *myOut = fopen("output_log.txt","wb+");
    FILE *myOut = stdout;
    
    //给一帧ADTS分配5kb内存
    unsigned char *aacFrame = (unsigned char *)malloc(1024 * 5);
    //申请1M的缓冲区用于读取aac数据
    unsigned char *aacBuffer = (unsigned char *)malloc(1024 * 1024);
    
    FILE *file = fopen(url, "rb");
    if(file == NULL){
        printf("Fail to open file!\n");
        return -1;
    }
    
    printf("-----+- ADTS Frame Table -+------+\n");
    printf(" NUM | Profile | Frequency| Size |\n");
    printf("-----+---------+----------+------+\n");
    
    while (!feof(file)) {
        dataSize = fread(aacBuffer + offset, 1, 1024*1024 - offset, file);
        unsigned char *inputData = aacBuffer;
        
        while (1) {
            int ret = getADTSframe(inputData, dataSize, aacFrame, &size);
            if(ret == -1){
                break;
            }else if(ret == 1){
                memcpy(aacBuffer, inputData, dataSize);
                offset = dataSize;
                break;
            }
            
            char profileStr[10] = {0};
            char frequenceStr[10] = {0};
            
            //0XC0二进制为：1100 0000 和aacFrame[2]进行&运算后如果还是1100 0000，说明第三个字节的前两位是11
            unsigned char profile = aacFrame[2] & 0xC0;
            profile = profile >> 6;
            
            
            switch (profile) {
                case 0:
                    sprintf(profileStr, "Main"); break;
                    break;
                case 1:
                    sprintf(profileStr, "LC"); break;
                    break;
                case 2:
                    sprintf(profileStr, "SSR"); break;
                    break;
                case 3:
                    sprintf(profileStr, "unknown"); break;
                    break;
            }
            
            unsigned char samlingFrequencyIndex = aacFrame[2] & 0x3C;
            samlingFrequencyIndex = samlingFrequencyIndex >> 2;
            switch (samlingFrequencyIndex) {
                case 0:
                    sprintf(frequenceStr, "96000Hz");
                    break;
                case 1:
                    sprintf(frequenceStr, "88200Hz");
                    break;
                case 2:
                    sprintf(frequenceStr, "64000Hz");
                    break;
                case 3: sprintf(frequenceStr,"48000Hz");break;
                case 4: sprintf(frequenceStr,"44100Hz");break;
                case 5: sprintf(frequenceStr,"32000Hz");break;
                case 6: sprintf(frequenceStr,"24000Hz");break;
                case 7: sprintf(frequenceStr,"22050Hz");break;
                case 8: sprintf(frequenceStr,"16000Hz");break;
                case 9: sprintf(frequenceStr,"12000Hz");break;
                case 10: sprintf(frequenceStr,"11025Hz");break;
                case 11: sprintf(frequenceStr,"8000Hz");break;
                default:sprintf(frequenceStr,"unknown");break;
                    
            }
            
            fprintf(myOut, "%5d|  %8s|  %8s|  %5d|\n", count, profileStr, frequenceStr, size);
            dataSize -= size;
            inputData += size;
            count++;
            
        }
        
    }
    fclose(file);
    free(aacBuffer);
    free(aacFrame);
    
    return 0;
}
