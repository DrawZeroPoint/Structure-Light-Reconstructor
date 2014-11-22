#include "plyloader.h"
#include <string.h>

PlyLoader::PlyLoader(QObject *parent) :
    QObject(parent)
{
}

int PlyLoader::LoadModel(char* filename)
{
    char* pch = strstr(filename,".PLY");//Returns a pointer to the first occurrence of str2 in str1, or a null pointer if str2 is not part of str1.

    if (pch != NULL)
    {
        FILE *file = NULL;
        file = fopen(filename,"r");
        if (!file)
        {
            return -1;
        }
        fseek(file, 0, SEEK_END);//获取文件全部数据
        mp_vertexXYZ = (float*)malloc(2000000);//long int ftell (FILE *stream); Returns the current value of the position indicator of the stream.

        fseek(file, 0, SEEK_SET);//操作符指向文件流开头
        if (file)
        {
            int i = 0;
            char buffer[3000];
            fgets(buffer, 300, file);//char *fgets (char *str, int num, FILE *stream); Get string from stream
            // READ HEADER
            // Find number of vertexes
            while (strncmp("element vertex", buffer,strlen("element vertex")) != 0  )//int strncmp (const char *str1, const char *str2, size_t num); Compare characters of two strings
            {
              fgets(buffer, 300, file);	//如果一直没有找到element vertex字符串，就一直从文件流中取出300个元素，直到找到为止
            }
            strcpy(buffer, buffer + strlen("element vertex"));//char *strcpy ( char *destination, const char *source ); Copy string
            sscanf(buffer, "%i", &this->m_totalConnectedPoints);//int sscanf ( const char * s, const char * format, ...); Read formatted data from string
            // go to end_header
            while (strncmp( "end_header", buffer,strlen("end_header")) != 0  )
            {
              fgets(buffer, 600, file);
            }

            // read vertices
            i =0;
            for (int iterator = 0; iterator < this->m_totalConnectedPoints; iterator++)
            {
                fgets(buffer, 600, file);
                sscanf(buffer,"%f %f %f", &mp_vertexXYZ[i], &mp_vertexXYZ[i+1], &mp_vertexXYZ[i+2]);
                i += 3;
            }
            fclose(file);
        }
    }
    return 0;
}
