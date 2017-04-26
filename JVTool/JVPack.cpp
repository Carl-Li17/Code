#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef   _WIN32 
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>	
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "third_party/include/png.h"

#define MAX_PATH          260

#define DATA_TYPE_EJV_CSS           0   //  CSS file used for junction view svg files
#define DATA_TYPE_EJV_SKY_SVG       1   //  enhanced sky svg files(compressed)
#define DATA_TYPE_EJV_SVG           2   //  enhanced junction view svg files(compressed)
#define DATA_TYPE_SAR_SVG           3   //  sign as real svg files(compressed)
#define DATA_TYPE_SAR_CSS           4   //  sign as real css files(compressed)
#define DATA_TYPE_EJV_SKY_PNG       5   //  enhanced junction view pattern PNG files(uncompressed)
#define DATA_TYPE_EJV_PATTERN_PNG   6   //  enhanced junction view pattern PNG files(uncompressed)
#define DATA_TYPE_EJV_ARROW_PNG     7   //  enhanced junction view arrow PNG files(uncompressed)
#define DATA_TYPE_SAR_PNG           8   //  sign as real PNG files(uncompressed)
#define DATA_TYPE_TTF_FONT          9   //  true type font data
#define DATA_TYPE_SAR_REFERENCE_DATA    10      //  store "SAR_INDEX_INFO" array

#define EJV_ITEM_STYLE_NIGHT_MODE       0x0001
#define EJV_ITEM_STYLE_MARROW           0x0002  //  multiple arrow layer

#define TAG_PACKAGED_FILE       "PK"
#define VERSION_BASE            0x0001
#define VERSION_CUR             (VERSION_BASE + 1)  //  2012/02/27 new file version

typedef struct tag_EJV_PackageItemInfo
{
	unsigned short nType;   //  DATA_TYPE_XXX
	unsigned short nStyle;  //  
	unsigned int nCRC;
	unsigned int nDataSize;
	unsigned int nDataOffset;
	char szItemName[64];
}EJV_PACKAGE_ITEM_INFO, *PEJV_PACKAGE_ITEM_INFO;

typedef struct tag_PackageItemMem
{
	EJV_PACKAGE_ITEM_INFO package_item_head;
	unsigned int key_value; //  used for sorting
	char szFileName[256];
}PACKAGE_ITEM_MEM, *PPACKAGE_ITEM_MEM;

static PPACKAGE_ITEM_MEM ppackage_item_mem = NULL;
static int nPackageItemCount = 0, nPackageItemCached = 0;
static unsigned int crc_table[1][256] =
{
	{
		0x00000000UL, 0x77073096UL, 0xee0e612cUL, 0x990951baUL, 0x076dc419UL,
			0x706af48fUL, 0xe963a535UL, 0x9e6495a3UL, 0x0edb8832UL, 0x79dcb8a4UL,
			0xe0d5e91eUL, 0x97d2d988UL, 0x09b64c2bUL, 0x7eb17cbdUL, 0xe7b82d07UL,
			0x90bf1d91UL, 0x1db71064UL, 0x6ab020f2UL, 0xf3b97148UL, 0x84be41deUL,
			0x1adad47dUL, 0x6ddde4ebUL, 0xf4d4b551UL, 0x83d385c7UL, 0x136c9856UL,
			0x646ba8c0UL, 0xfd62f97aUL, 0x8a65c9ecUL, 0x14015c4fUL, 0x63066cd9UL,
			0xfa0f3d63UL, 0x8d080df5UL, 0x3b6e20c8UL, 0x4c69105eUL, 0xd56041e4UL,
			0xa2677172UL, 0x3c03e4d1UL, 0x4b04d447UL, 0xd20d85fdUL, 0xa50ab56bUL,
			0x35b5a8faUL, 0x42b2986cUL, 0xdbbbc9d6UL, 0xacbcf940UL, 0x32d86ce3UL,
			0x45df5c75UL, 0xdcd60dcfUL, 0xabd13d59UL, 0x26d930acUL, 0x51de003aUL,
			0xc8d75180UL, 0xbfd06116UL, 0x21b4f4b5UL, 0x56b3c423UL, 0xcfba9599UL,
			0xb8bda50fUL, 0x2802b89eUL, 0x5f058808UL, 0xc60cd9b2UL, 0xb10be924UL,
			0x2f6f7c87UL, 0x58684c11UL, 0xc1611dabUL, 0xb6662d3dUL, 0x76dc4190UL,
			0x01db7106UL, 0x98d220bcUL, 0xefd5102aUL, 0x71b18589UL, 0x06b6b51fUL,
			0x9fbfe4a5UL, 0xe8b8d433UL, 0x7807c9a2UL, 0x0f00f934UL, 0x9609a88eUL,
			0xe10e9818UL, 0x7f6a0dbbUL, 0x086d3d2dUL, 0x91646c97UL, 0xe6635c01UL,
			0x6b6b51f4UL, 0x1c6c6162UL, 0x856530d8UL, 0xf262004eUL, 0x6c0695edUL,
			0x1b01a57bUL, 0x8208f4c1UL, 0xf50fc457UL, 0x65b0d9c6UL, 0x12b7e950UL,
			0x8bbeb8eaUL, 0xfcb9887cUL, 0x62dd1ddfUL, 0x15da2d49UL, 0x8cd37cf3UL,
			0xfbd44c65UL, 0x4db26158UL, 0x3ab551ceUL, 0xa3bc0074UL, 0xd4bb30e2UL,
			0x4adfa541UL, 0x3dd895d7UL, 0xa4d1c46dUL, 0xd3d6f4fbUL, 0x4369e96aUL,
			0x346ed9fcUL, 0xad678846UL, 0xda60b8d0UL, 0x44042d73UL, 0x33031de5UL,
			0xaa0a4c5fUL, 0xdd0d7cc9UL, 0x5005713cUL, 0x270241aaUL, 0xbe0b1010UL,
			0xc90c2086UL, 0x5768b525UL, 0x206f85b3UL, 0xb966d409UL, 0xce61e49fUL,
			0x5edef90eUL, 0x29d9c998UL, 0xb0d09822UL, 0xc7d7a8b4UL, 0x59b33d17UL,
			0x2eb40d81UL, 0xb7bd5c3bUL, 0xc0ba6cadUL, 0xedb88320UL, 0x9abfb3b6UL,
			0x03b6e20cUL, 0x74b1d29aUL, 0xead54739UL, 0x9dd277afUL, 0x04db2615UL,
			0x73dc1683UL, 0xe3630b12UL, 0x94643b84UL, 0x0d6d6a3eUL, 0x7a6a5aa8UL,
			0xe40ecf0bUL, 0x9309ff9dUL, 0x0a00ae27UL, 0x7d079eb1UL, 0xf00f9344UL,
			0x8708a3d2UL, 0x1e01f268UL, 0x6906c2feUL, 0xf762575dUL, 0x806567cbUL,
			0x196c3671UL, 0x6e6b06e7UL, 0xfed41b76UL, 0x89d32be0UL, 0x10da7a5aUL,
			0x67dd4accUL, 0xf9b9df6fUL, 0x8ebeeff9UL, 0x17b7be43UL, 0x60b08ed5UL,
			0xd6d6a3e8UL, 0xa1d1937eUL, 0x38d8c2c4UL, 0x4fdff252UL, 0xd1bb67f1UL,
			0xa6bc5767UL, 0x3fb506ddUL, 0x48b2364bUL, 0xd80d2bdaUL, 0xaf0a1b4cUL,
			0x36034af6UL, 0x41047a60UL, 0xdf60efc3UL, 0xa867df55UL, 0x316e8eefUL,
			0x4669be79UL, 0xcb61b38cUL, 0xbc66831aUL, 0x256fd2a0UL, 0x5268e236UL,
			0xcc0c7795UL, 0xbb0b4703UL, 0x220216b9UL, 0x5505262fUL, 0xc5ba3bbeUL,
			0xb2bd0b28UL, 0x2bb45a92UL, 0x5cb36a04UL, 0xc2d7ffa7UL, 0xb5d0cf31UL,
			0x2cd99e8bUL, 0x5bdeae1dUL, 0x9b64c2b0UL, 0xec63f226UL, 0x756aa39cUL,
			0x026d930aUL, 0x9c0906a9UL, 0xeb0e363fUL, 0x72076785UL, 0x05005713UL,
			0x95bf4a82UL, 0xe2b87a14UL, 0x7bb12baeUL, 0x0cb61b38UL, 0x92d28e9bUL,
			0xe5d5be0dUL, 0x7cdcefb7UL, 0x0bdbdf21UL, 0x86d3d2d4UL, 0xf1d4e242UL,
			0x68ddb3f8UL, 0x1fda836eUL, 0x81be16cdUL, 0xf6b9265bUL, 0x6fb077e1UL,
			0x18b74777UL, 0x88085ae6UL, 0xff0f6a70UL, 0x66063bcaUL, 0x11010b5cUL,
			0x8f659effUL, 0xf862ae69UL, 0x616bffd3UL, 0x166ccf45UL, 0xa00ae278UL,
			0xd70dd2eeUL, 0x4e048354UL, 0x3903b3c2UL, 0xa7672661UL, 0xd06016f7UL,
			0x4969474dUL, 0x3e6e77dbUL, 0xaed16a4aUL, 0xd9d65adcUL, 0x40df0b66UL,
			0x37d83bf0UL, 0xa9bcae53UL, 0xdebb9ec5UL, 0x47b2cf7fUL, 0x30b5ffe9UL,
			0xbdbdf21cUL, 0xcabac28aUL, 0x53b39330UL, 0x24b4a3a6UL, 0xbad03605UL,
			0xcdd70693UL, 0x54de5729UL, 0x23d967bfUL, 0xb3667a2eUL, 0xc4614ab8UL,
			0x5d681b02UL, 0x2a6f2b94UL, 0xb40bbe37UL, 0xc30c8ea1UL, 0x5a05df1bUL,
			0x2d02ef8dUL
	}
};
#define DO1 crc = crc_table[0][((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8)
#define DO8 DO1; DO1; DO1; DO1; DO1; DO1; DO1; DO1

typedef struct tag_PackedDataFileHead
{
	unsigned short nTag;
	unsigned short nVersionNo;
	char szAuthor[32];
	char szDataVendorInfo[64];
	char szFileDescription[64];
	unsigned int nDataCreatedTimeLow;
	unsigned int nDataCreatedTimeHigh;
	unsigned int nDataSize;     //  total data size(included this heading structure)
	unsigned int nIndexDataOffset;
	unsigned int nIndexDataSize;
	unsigned int nIndexDataCRC;
}PACKED_DATA_FILE_HEAD, *PPACKED_DATA_FILE_HEAD;

typedef struct tag_SAR_SubPanelInfo
{
	char  szSubPanelName[16];
	short nLeft;    //  pixel position in the SAR panel
	short nTop;
	short nRight;
	short nBottom;
}SAR_SUB_PANEL_INFO, *PSAR_SUB_PANEL_INFO;

#define MAX_SAR_SUB_PANEL   6

typedef struct tag_SAR_PanelInfo
{
	unsigned short nType;           //  DATA_TYPE_XXX
	unsigned short nSubPanelCount;  //  sub-panel count(max: MAX_SUB_PANEL)
	unsigned int nCRC;              //  CRC value of packed file
	unsigned int nPanelDataSize;
	unsigned int nPanelDataOffSet;

	char szPanelName[48];           //  48 enough for us
	SAR_SUB_PANEL_INFO subpanel_info[MAX_SAR_SUB_PANEL];
}SAR_PANEL_INFO, *PSAR_PANEL_INFO;

typedef struct tag_SAR_PanelInfoMem
{
	char szFullName[256];           //  full file
	SAR_PANEL_INFO sar_panel_info;
}SAR_PANEL_INFO_MEM, *PSAR_PANEL_INFO_MEM;

extern "C" unsigned int calculate_crc32(unsigned int crc, unsigned char* buf, unsigned int len);

typedef struct FileInformation
{
	unsigned long    size;
	char             name[260];
	FileInformation* next;
}FILEINFO;

char *g_strupr(char *s)
{
	char *t=s;
	while(*t)
	{
		if(*t>='a'&&*t<='z')
		{
			*t+='A'-'a';
		}
		t++;
	}
	return s;
}

int getDirFileInfo(char* basePath, FILEINFO* &FileInfoList)
{
	FileInfoList = NULL;
#ifdef   _WIN32
	_finddata_t fileDir;
	char*dir = basePath;
	long lfDir =_findfirst(dir,&fileDir);
	if (lfDir == -1l)   
	{
		_findclose(lfDir);
		// printf("Warning: Can not find file under %s \n(%s %d)\n", dir, __FILE__, __LINE__);
		return -1;
	}
	FILEINFO* pSentry = NULL;
	do 
	{
		FILEINFO *pFileInfo = new FILEINFO; 
		pFileInfo->next = NULL;
		strcpy(pFileInfo->name, fileDir.name);
		pFileInfo->size = fileDir.size;

		if (NULL == FileInfoList)
		{
			FileInfoList = pFileInfo;
			pSentry = FileInfoList;
		}
		else
		{
			pSentry->next = pFileInfo;
			pSentry = pSentry->next;
		}
	}while (_findnext(lfDir,&fileDir) == 0);
	_findclose(lfDir);

#else
	DIR *dir;
	struct dirent *pDirent;
	char *pStr = strrchr(basePath, '*');
	if (NULL != pStr && *(pStr+1) == '.')
	{
		while('\0' != *pStr) *(pStr++) = '\0'; 
	}

	if ((dir=opendir(basePath)) == NULL)
	{
		printf("Open dir error---%s.\n", basePath);
		return -1;
	}
	char fullpath[255];
    FILEINFO* pSentry = NULL;
	while ((pDirent=readdir(dir)) != NULL)
	{
		if(strcmp(pDirent->d_name,".")==0 
		|| strcmp(pDirent->d_name,"..")==0
		|| pDirent->d_type != 8) 
		{
			continue;
		};	
		struct stat file_stat; 
		memset(fullpath, '\0', sizeof(fullpath));  
		strcpy(fullpath,  basePath); 		 
		strcat(fullpath, pDirent->d_name); 
		if (lstat(fullpath, &file_stat) < 0 )  
		{  
			continue;  
		}
		FILEINFO *pFileInfo = new FILEINFO; 
		pFileInfo->next = NULL;
		strcpy(pFileInfo->name, pDirent->d_name);
		pFileInfo->size = file_stat.st_blksize;

		if (NULL == FileInfoList)
		{
			FileInfoList = pFileInfo;
			pSentry = FileInfoList;
		}
		else
		{
			pSentry->next = pFileInfo;
			pSentry = pSentry->next;
		}
	}
	closedir(dir);
#endif
	return 0;
}
void addPathSprit(char* szFolder)
{
	if (szFolder[strlen(szFolder) - 1] != '\\' && szFolder[strlen(szFolder) - 1] != '/')
	{
	// maybe strcat(szFolder, "/"); is ok                            
#ifdef   _WIN32 
		strcat(szFolder, "\\");
#else
		strcat(szFolder, "/");
#endif
	}
}

unsigned int calculate_crc32(unsigned int crc, unsigned char* buf, unsigned int len)
{
	if (buf == 0) return 0UL;

	crc = crc ^ 0xffffffffUL;
	while (len >= 8) 
	{
		DO8;
		len -= 8;
	}

	if (len) do {
		DO1;
	} while (--len);

	return (crc ^ 0xffffffffUL);
}

static int s_collect_ejv_images(const char *szParentFolder, const char *szSubFolder, int nItemType, int nItemStyle)
{
	char szFolder[320] = "", szFullName[320] = "", szMsg[512] = "";
	strcpy(szFolder, szParentFolder);
	if (szSubFolder[0] != '\\' && szSubFolder[0] != '/')
	{
		addPathSprit(szFolder);
	}
	strcat(szFolder, szSubFolder);
	addPathSprit(szFolder);

	sprintf(szFullName, "%s*.png", szFolder);

	char szPatternType[64] = "", szDayNightMode[60] = "";
	if (DATA_TYPE_EJV_SKY_PNG == nItemType)
	{
		strcpy(szPatternType, "sky pattern");
		if (nItemStyle & EJV_ITEM_STYLE_NIGHT_MODE)
		{
			strcpy(szDayNightMode, "night mode");
		}
		else
		{
			strcpy(szDayNightMode, "day mode");
		}
	}
	else if (DATA_TYPE_EJV_PATTERN_PNG == nItemType)
	{
		strcpy(szPatternType, "junction pattern");
		if (nItemStyle & EJV_ITEM_STYLE_NIGHT_MODE)
		{
			strcpy(szDayNightMode, "night mode");
		}
		else
		{
			strcpy(szDayNightMode, "day mode");
		}
	}
	else if (DATA_TYPE_EJV_ARROW_PNG == nItemType)
	{
		strcpy(szPatternType, "arrow pattern");
	}
	else
	{
		strcpy(szPatternType, "unknown pattern");
	}
	// hFind = FindFirstFileA(szFullName, &win32_find_data);
	
	FILEINFO *pFileInfoList = NULL;
	if (getDirFileInfo(szFullName, pFileInfoList) < 0
		&& pFileInfoList == NULL)   
	{
		printf("Warning: Can not find file under %s folder.\n ", szFullName);
		return -1;
	}
	FILEINFO *pFileInfo = pFileInfoList;
	while(pFileInfo != NULL) 
	{
		PACKAGE_ITEM_MEM package_item_mem = {0};
		char szFileName[128] = "", *ptr;

		if (pFileInfo->size < 16)// ((win32_find_data.nFileSizeLow < 16) || (win32_find_data.nFileSizeHigh != 0))
		{
			continue;
		}

		strncpy(szFileName, pFileInfo->name /*win32_find_data.cFileName*/, sizeof(szFileName) - 1);
		ptr = strrchr(szFileName, '.');
		if (ptr)    *ptr = 0;
		if ((strlen(szFileName) >= sizeof(package_item_mem.szFileName)) || 
			(szFileName[0] == 0))
		{
			continue;
		}

		package_item_mem.package_item_head.nType  = nItemType;
		package_item_mem.package_item_head.nStyle = nItemStyle;
		package_item_mem.package_item_head.nDataSize = pFileInfo->size/*win32_find_data.nFileSizeLow*/;
		strcpy(package_item_mem.package_item_head.szItemName, szFileName);
		g_strupr(package_item_mem.package_item_head.szItemName);
		package_item_mem.key_value = 
			calculate_crc32(0, (unsigned char*)(package_item_mem.package_item_head.szItemName), 
			(unsigned int)strlen(package_item_mem.package_item_head.szItemName));
		sprintf(package_item_mem.szFileName, "%s%s", szFolder, pFileInfo->name/*win32_find_data.cFileName*/);
		if (nPackageItemCount >= nPackageItemCached)
		{
			nPackageItemCached += 1024;

			PPACKAGE_ITEM_MEM ppackage_item_mem_new = new PACKAGE_ITEM_MEM[nPackageItemCached];
			if (nPackageItemCount > 0)
			{
				memcpy(ppackage_item_mem_new, ppackage_item_mem, nPackageItemCount * sizeof(PACKAGE_ITEM_MEM));
			}

			delete[] ppackage_item_mem;
			ppackage_item_mem = ppackage_item_mem_new;
		}
		ppackage_item_mem[nPackageItemCount] = package_item_mem;
		nPackageItemCount++;

		if (szDayNightMode[0])
		{
			sprintf(szMsg, "collect file(%d): %s. size: %d. type: %s. day-night mode: %s.\n", 
				nPackageItemCount, package_item_mem.szFileName, pFileInfo->size/*win32_find_data.nFileSizeLow*/, 
				szPatternType, szDayNightMode);
		}
		else
		{
			sprintf(szMsg, "collect file(%d): %s. size: %d. type: %s.\n", 
				nPackageItemCount, package_item_mem.szFileName, pFileInfo->size/*win32_find_data.nFileSizeLow*/, 
				szPatternType);
		}
		FILEINFO *pInfo = pFileInfo->next;
		delete pFileInfo;
		pFileInfo = pInfo;
		// OutputDebugStringA(szMsg);
		printf(szMsg);
		
	} 
	return 0;
}

static int s_item_compare(const PACKAGE_ITEM_MEM& left, const PACKAGE_ITEM_MEM& right)
{
	if (left.package_item_head.nType < right.package_item_head.nType)   return -1;
	else if (left.package_item_head.nType > right.package_item_head.nType)  return 1;

	if (left.package_item_head.nStyle < right.package_item_head.nStyle) return -1;
	else if (left.package_item_head.nStyle > right.package_item_head.nStyle)    return 1;

	if (left.key_value < right.key_value)   return -1;
	else if (left.key_value > right.key_value)   return 1;

	if (left.package_item_head.nDataSize < right.package_item_head.nDataSize)   return -1;
	else if (left.package_item_head.nDataSize > right.package_item_head.nDataSize) return 1;

	return 0;
}

static void s_sort_package_items()
{
	for (int i = 0; i < nPackageItemCount; i++)
	{
		for (int j = i + 1; j < nPackageItemCount; j++)
		{
			if (s_item_compare(ppackage_item_mem[i], ppackage_item_mem[j]) > 0)
			{
				PACKAGE_ITEM_MEM tmp = ppackage_item_mem[i];
				ppackage_item_mem[i] = ppackage_item_mem[j];
				ppackage_item_mem[j] = tmp;
			}
		}
	}
}

static int s_package_items(const char* szDstFile)
{
	char szMsg[512] = "";
	int  i = 0, nFileCount = 0;

	FILE *fp = fopen(szDstFile, "wb");
	if (!fp)
	{
		sprintf(szMsg, "ejv pattern packaging. create package file failure!\n");
		// OutputDebugStringA(szMsg);
		printf(szMsg);
		return -1;
	}

	sprintf(szMsg, "ejv pattern packaging started.\n");
	// OutputDebugStringA(szMsg);
	printf(szMsg);

	PEJV_PACKAGE_ITEM_INFO pejv_package_item_info = new EJV_PACKAGE_ITEM_INFO[nPackageItemCount];

	unsigned int nFileHeadSize = sizeof(PACKED_DATA_FILE_HEAD);
	unsigned int nIndexDataSize = sizeof(EJV_PACKAGE_ITEM_INFO) * nPackageItemCount;
	unsigned int nDataOffset = nFileHeadSize + nIndexDataSize;

	unsigned char *ptrPatternBuffer = NULL;
	unsigned int nPatternDataSizeCur = 0, nPatternDataSizeCached = 0;
	for (i = 0; i < nPackageItemCount; i++)
	{
		pejv_package_item_info[i] = ppackage_item_mem[i].package_item_head;
		FILE *fp_src = fopen(ppackage_item_mem[i].szFileName, "rb");
		if (!fp_src)
		{
			pejv_package_item_info[i].nCRC = 0;
			pejv_package_item_info[i].nDataSize = pejv_package_item_info[i].nDataOffset = 0;
			continue;
		}

		fseek(fp_src, 0, SEEK_END);
		nPatternDataSizeCur = ftell(fp_src);
		if (nPatternDataSizeCur > 16)
		{
			if (nPatternDataSizeCur > nPatternDataSizeCached)
			{
				nPatternDataSizeCached = ((nPatternDataSizeCur + 1023) >> 10) << 10;
				delete[] ptrPatternBuffer;

				ptrPatternBuffer = new unsigned char[nPatternDataSizeCached];
			}

			fseek(fp_src, 0, SEEK_SET);
			fread(ptrPatternBuffer, 1, nPatternDataSizeCur, fp_src);
			fclose(fp_src); fp_src = NULL;

			nDataOffset = ((nDataOffset + 3) >> 2) << 2;
			pejv_package_item_info[i].nDataSize   = nPatternDataSizeCur;
			pejv_package_item_info[i].nDataOffset = nDataOffset;
			pejv_package_item_info[i].nCRC        = calculate_crc32(0, ptrPatternBuffer, nPatternDataSizeCur);
			fseek(fp, nDataOffset, SEEK_SET);
			fwrite(ptrPatternBuffer, 1, nPatternDataSizeCur, fp);

			nDataOffset += pejv_package_item_info[i].nDataSize;

			sprintf(szMsg, "...packaging file(%d): %s. key: %u, size: %d\n", 
				nFileCount + 1, ppackage_item_mem[i].szFileName, ppackage_item_mem[i].key_value, nPatternDataSizeCur);
			// OutputDebugStringA(szMsg);
			printf(szMsg);

			nFileCount++;
		}

		if (fp_src) fclose(fp_src);
	}

	PACKED_DATA_FILE_HEAD packed_data_file_head = {0};
	packed_data_file_head.nDataSize        = nDataOffset;
	packed_data_file_head.nIndexDataSize   = nIndexDataSize;
	packed_data_file_head.nIndexDataOffset = sizeof(packed_data_file_head);
	packed_data_file_head.nVersionNo       = VERSION_CUR;
	strncpy((char*)&(packed_data_file_head.nTag), TAG_PACKAGED_FILE, 2);
	strcpy(packed_data_file_head.szAuthor, "shboli(@Telenav)");
	strcpy(packed_data_file_head.szDataVendorInfo, "Navtaq-ANZ(NT-2016Q1)");
	strcpy(packed_data_file_head.szFileDescription, "EJV pattern for ANZ(NT-2016Q1)");
	{
		//FILETIME file_time = {0};        
		//GetSystemTimeAsFileTime(&file_time);
		time_t now;  time(&now);   
		packed_data_file_head.nDataCreatedTimeLow  = (unsigned int)(now);
		packed_data_file_head.nDataCreatedTimeHigh = (unsigned int)(now>>32);
	}

	packed_data_file_head.nIndexDataCRC = calculate_crc32(0, (unsigned char*)pejv_package_item_info, nIndexDataSize);
	fseek(fp, 0, SEEK_SET);
	fwrite(&packed_data_file_head, 1, sizeof(packed_data_file_head), fp);

	fseek(fp, packed_data_file_head.nIndexDataOffset, SEEK_SET);
	fwrite(pejv_package_item_info, 1, packed_data_file_head.nIndexDataSize, fp);

	delete[] pejv_package_item_info; pejv_package_item_info = NULL;

	fclose(fp);

	sprintf(szMsg, "ejv pattern packaging done. %d files packaged. file size: %d. press press to exist.\n", 
		nFileCount, packed_data_file_head.nDataSize);
	// OutputDebugStringA(szMsg);
	printf(szMsg);

	return 0;
}


int packege_ejv_images_new(const char *szParentFolder, const char* szDstFile)
{
    delete[] ppackage_item_mem; ppackage_item_mem = NULL;
    nPackageItemCount = nPackageItemCached = 0;

    if ((!szParentFolder) || (!szParentFolder[0])) 
	{
		printf("packege_ejv_images_new()===(!szParentFolder) || (!szParentFolder[0])\n");
		return -1;
	}

    if ((!szDstFile) || (!szDstFile[0]))
    {
		printf("packege_ejv_images_new()===(!szDstFile) || (!szDstFile[0])\n");
		return -1;
    }

    typedef struct tag_FilesCollectInfo
    {
        char szFolders[120];
        int nItemType;
        unsigned int nStyle;
    }FILES_COLLECT_INFO;

    FILES_COLLECT_INFO files_collect_info[] = 
    {
/*        {"ca\\day\\",   DATA_TYPE_EJV_PATTERN_PNG, 0},    
        {"ca\\night\\", DATA_TYPE_EJV_PATTERN_PNG, EJV_ITEM_STYLE_NIGHT_MODE},    
        {"ca\\arrow\\", DATA_TYPE_EJV_ARROW_PNG, 0},    
        {"ca\\arrow_m\\", DATA_TYPE_EJV_ARROW_PNG, EJV_ITEM_STYLE_MARROW},    

        {"pt\\day\\",   DATA_TYPE_EJV_PATTERN_PNG, 0},    
        {"pt\\night\\", DATA_TYPE_EJV_PATTERN_PNG, EJV_ITEM_STYLE_NIGHT_MODE},    
        {"pt\\arrow\\", DATA_TYPE_EJV_ARROW_PNG, 0},    
        {"pt\\arrow_m\\", DATA_TYPE_EJV_ARROW_PNG, EJV_ITEM_STYLE_MARROW},    

        {"us\\day\\",   DATA_TYPE_EJV_PATTERN_PNG, 0},    
        {"us\\night\\", DATA_TYPE_EJV_PATTERN_PNG, EJV_ITEM_STYLE_NIGHT_MODE},    
        {"us\\arrow\\", DATA_TYPE_EJV_ARROW_PNG, 0},    
        {"us\\arrow_m\\", DATA_TYPE_EJV_ARROW_PNG, EJV_ITEM_STYLE_MARROW},   */ 

		{"day",   DATA_TYPE_EJV_PATTERN_PNG, 0},    
		{"night", DATA_TYPE_EJV_PATTERN_PNG, EJV_ITEM_STYLE_NIGHT_MODE},      
		{"arrow_m", DATA_TYPE_EJV_ARROW_PNG, EJV_ITEM_STYLE_MARROW}, 

		{"sky_d",      DATA_TYPE_EJV_SKY_PNG, 0},    
        {"sky_n",    DATA_TYPE_EJV_SKY_PNG, EJV_ITEM_STYLE_NIGHT_MODE},    
    };

    char szMsg[512] = "";

    sprintf(szMsg, "ejv pattern collecting started.\n");
    // OutputDebugStringA(szMsg);
    printf(szMsg);

    for (int i = 0; i < sizeof(files_collect_info) / sizeof(files_collect_info[0]); i++)
    {
        s_collect_ejv_images(szParentFolder, files_collect_info[i].szFolders, files_collect_info[i].nItemType, files_collect_info[i].nStyle);
    }

    sprintf(szMsg, "ejv pattern collecting done. %d files collected.\n", nPackageItemCount);
    if (nPackageItemCount <= 0)
    {
        sprintf(szMsg, "ejv pattern collecting completed. nothing to do. returned\n");
        // OutputDebugStringA(szMsg);
        printf(szMsg);
        return -1;
    }

    sprintf(szMsg, "sorting pattern files start....\n");
    // OutputDebugStringA(szMsg);
    printf(szMsg);

    s_sort_package_items();

    sprintf(szMsg, "sorting pattern files done!\n");
    // OutputDebugStringA(szMsg);
    printf(szMsg);

    s_package_items(szDstFile);

    delete[] ppackage_item_mem; ppackage_item_mem = NULL;
    nPackageItemCount = nPackageItemCached = 0;

    return 0;
}


int extract_ejv_images_new(const char *szPackageFile, const char *szParentFolder)
{
	if ((!szPackageFile) || (!szPackageFile[0]))    
	{
		printf("extract_ejv_images_new()===((!szPackageFile) || (!szPackageFile[0]))\n");
		return -1;
	}
	if ((!szParentFolder) || (!szParentFolder[0])) 
	{
		printf("extract_ejv_images_new()===(!szParentFolder) || (!szParentFolder[0])\n");
		return -1;
	}

	PACKED_DATA_FILE_HEAD packed_data_file_head = {};
	FILE *fp_src;
	unsigned int nTotalFileDataSize = 0;
	char szFullName[320] = "", szFolder[320] = "", szSubFolder[128] = "";
	char szMsg[512] = "";

	strncpy(szFolder, szParentFolder, sizeof(szFolder) - 1);
	addPathSprit(szFolder);
	fp_src = fopen(szPackageFile, "rb");
	if (!fp_src)    
	{
		printf("file open failure!\n");
		return -1;
	}

	fseek(fp_src, 0, SEEK_END);
	nTotalFileDataSize = ftell(fp_src);
	fseek(fp_src, 0, SEEK_SET);

	if (nTotalFileDataSize <= sizeof(packed_data_file_head))
	{
		fclose(fp_src);
		printf("invalid ejv package file!\n");
		return -1;
	}

	fread(&packed_data_file_head, 1, sizeof(packed_data_file_head), fp_src);
	if ((packed_data_file_head.nVersionNo < VERSION_BASE) || (packed_data_file_head.nVersionNo > VERSION_CUR) || 
		(strncmp((const char*)&(packed_data_file_head.nTag), TAG_PACKAGED_FILE, 2) != 0))
	{
		fclose(fp_src);
		sprintf(szMsg, "invalid ejv package file!\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);
		return -1;
	}

	if ((packed_data_file_head.nIndexDataSize < sizeof(EJV_PACKAGE_ITEM_INFO)) || 
		(packed_data_file_head.nIndexDataSize + packed_data_file_head.nIndexDataOffset> nTotalFileDataSize))
	{
		fclose(fp_src);
		sprintf(szMsg, "invalid ejv package file!\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);
		return -1;
	}

	if (packed_data_file_head.nDataSize > nTotalFileDataSize)
	{
		sprintf(szMsg, "warning, package file data size mis-match!\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);
	}

	packed_data_file_head.szAuthor[sizeof(packed_data_file_head.szAuthor) - 1] = 0;
	packed_data_file_head.szDataVendorInfo[sizeof(packed_data_file_head.szDataVendorInfo) - 1] = 0;
	packed_data_file_head.szFileDescription[sizeof(packed_data_file_head.szFileDescription) - 1] = 0;
	sprintf(szMsg, "ejv package file: author: %s; data vendor: %s; description: %s\n", 
		packed_data_file_head.szAuthor, 
		packed_data_file_head.szDataVendorInfo, 
		packed_data_file_head.szFileDescription);
	printf(szMsg);
	// OutputDebugStringA(szMsg);

	unsigned char* ptrEjvIndexBuffer = new unsigned char[packed_data_file_head.nIndexDataSize];
	fseek(fp_src, packed_data_file_head.nIndexDataOffset, SEEK_SET);
	fread(ptrEjvIndexBuffer, 1, packed_data_file_head.nIndexDataSize, fp_src);

	if (packed_data_file_head.nIndexDataCRC != calculate_crc32(0, ptrEjvIndexBuffer, packed_data_file_head.nIndexDataSize))
	{
		delete[] ptrEjvIndexBuffer;
		fclose(fp_src);

		sprintf(szMsg, "invalid ejv package file. CRC checking error!\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);
		return -1;
	}

	PEJV_PACKAGE_ITEM_INFO pejv_package_item_info = (PEJV_PACKAGE_ITEM_INFO)ptrEjvIndexBuffer;
	if (packed_data_file_head.nIndexDataSize % sizeof(EJV_PACKAGE_ITEM_INFO))
	{
		sprintf(szMsg, "warning, index data size mis-match!\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);
	}

	unsigned int nEJV_PatternIndexCount = packed_data_file_head.nIndexDataSize / sizeof(EJV_PACKAGE_ITEM_INFO);
	sprintf(szMsg, "%d patterns found in package!\n", nEJV_PatternIndexCount);
	printf(szMsg);
	// OutputDebugStringA(szMsg);

	unsigned char *ptrFileBuffer = NULL;
	unsigned int nCachedBufferSize = 0, i = 0;

	char szSubFolderArray[][128] = 
	{
		"arrow_m/",
		"arrow/",
		"sky/",
		"sky/day/",
		"sky/night/",
		"pattern/",
		"pattern/day/",
		"pattern/night/",
	};
	for (i = 0; i < sizeof(szSubFolderArray) / sizeof(szSubFolderArray[0]); i++)
	{
		char szMergedFolder[320];

		sprintf(szMergedFolder, "%s%s", szFolder, szSubFolderArray[i]);
		#ifdef   _WIN32
		mkdir(szMergedFolder);
		#else
		mkdir(szMergedFolder, (S_IRWXU|S_IRWXG|S_IRWXO));	
		#endif
	}

	int nPatternCount = 0;
	for (i = 0; i < nEJV_PatternIndexCount; i++)
	{
		if ((0 == pejv_package_item_info[i].nDataSize) || 
			(0 == pejv_package_item_info[i].nDataOffset) || 
			(0 == pejv_package_item_info[i].szItemName[0]))
		{
			continue;
		}

		if ((pejv_package_item_info[i].nDataOffset + pejv_package_item_info[i].nDataSize) > nTotalFileDataSize)
		{
			continue;
		}

		char szPatternType[128], szPatternStyle[128];
		if (DATA_TYPE_EJV_ARROW_PNG == pejv_package_item_info[i].nType)
		{
			sprintf(szPatternType, "arrow pattern");
			if (pejv_package_item_info[i].nStyle & EJV_ITEM_STYLE_MARROW)
			{
				sprintf(szPatternStyle, "multiple arrow");
				sprintf(szSubFolder, "arrow_m/");
			}
			else
			{
				sprintf(szPatternStyle, "single arrow");
				sprintf(szSubFolder, "arrow/");
			}
		}
		else if (DATA_TYPE_EJV_PATTERN_PNG == pejv_package_item_info[i].nType)
		{
			sprintf(szPatternType, "EJV bkgrnd pattern");
			if (pejv_package_item_info[i].nStyle & EJV_ITEM_STYLE_NIGHT_MODE)
			{
				sprintf(szPatternStyle, "night mode");
				sprintf(szSubFolder, "pattern/night/");
			}
			else
			{
				sprintf(szPatternStyle, "day mode");
				sprintf(szSubFolder, "pattern/day/");
			}
		}
		else if (DATA_TYPE_EJV_SKY_PNG == pejv_package_item_info[i].nType)
		{
			sprintf(szPatternType, "sky pattern");
			if (pejv_package_item_info[i].nStyle & EJV_ITEM_STYLE_NIGHT_MODE)
			{
				sprintf(szPatternStyle, "night mode");
				sprintf(szSubFolder, "sky/night/");
			}
			else
			{
				sprintf(szPatternStyle, "day mode");
				sprintf(szSubFolder, "sky/day/");
			}
		}
		else
		{
			sprintf(szMsg, "warning, got wrong pattern, unknown pattern type: %d\n", pejv_package_item_info[i].nType);
			continue;
		}

		sprintf(szFullName, "%s%s%s.png", szFolder, szSubFolder, pejv_package_item_info[i].szItemName);
		FILE *fpDst = fopen(szFullName, "wb");
		if (fpDst)
		{
			if (nCachedBufferSize < pejv_package_item_info[i].nDataSize)
			{
				nCachedBufferSize = ((pejv_package_item_info[i].nDataSize + 1023) >> 10) << 10;
				delete[] ptrFileBuffer;

				ptrFileBuffer = new unsigned char[nCachedBufferSize];
			}

			fseek(fp_src, pejv_package_item_info[i].nDataOffset, SEEK_SET);
			fread(ptrFileBuffer, 1, pejv_package_item_info[i].nDataSize, fp_src);

			unsigned int nCRC = calculate_crc32(0, ptrFileBuffer, pejv_package_item_info[i].nDataSize);
			if (nCRC != pejv_package_item_info[i].nCRC)
			{
				sprintf(szMsg, "warning, EJV pattern data CRC mis-match!\n");
				printf(szMsg);
				// OutputDebugStringA(szMsg);
			}

			fwrite(ptrFileBuffer, 1, pejv_package_item_info[i].nDataSize, fpDst);

			fclose(fpDst);

			sprintf(szMsg, "got one pattern(%d). name: %s. size: %d. type: %s. style: %s\n", 
				nPatternCount + 1,
				pejv_package_item_info[i].szItemName, 
				pejv_package_item_info[i].nDataSize, 
				szPatternType, 
				szPatternStyle);
			printf(szMsg);
			// OutputDebugStringA(szMsg);

			nPatternCount++;
		}
	}

	fclose(fp_src); fp_src = NULL;
	delete[] ptrFileBuffer;
	delete[] ptrEjvIndexBuffer;

	if (nEJV_PatternIndexCount == nPatternCount)
	{
		sprintf(szMsg, "ejv package file extracting done. %d patterns got. press enter to exit.\n", nPatternCount);
	}
	else
	{
		sprintf(szMsg, "ejv package file extracting done. %d patterns got, warning, not all %d patterns extracted. press enter to exit.\n", 
			nPatternCount, nEJV_PatternIndexCount);
	}

	printf(szMsg);
	// OutputDebugStringA(szMsg);

	return 0;
}



int sar_package_extract(const char* szPackageFile, const char* dst_folder)
{
	if ((!szPackageFile) || (!szPackageFile[0]))
	{
		printf("sar_package_extract()===((!szPackageFile) || (!szPackageFile[0]))\n");
		return -1;
	}

	if ((!dst_folder) || (!dst_folder[0]))
	{
		printf("sar_package_extract()===((!dst_folder) || (!dst_folder[0]))\n");
		return -1;
	}

	char szDstFolder[320] = "", szFullName[360] = "";
	strncpy(szDstFolder, dst_folder, sizeof(szDstFolder) - 1);
	addPathSprit(szDstFolder);
	FILE *fp = fopen(szPackageFile, "rb");
	if (!fp)    
	{
		printf("file open failure!\n");
		return -1;
	}

	char szMsg[512] = "";
	unsigned int nFileDataSize = 0;
	fseek(fp, 0, SEEK_END);
	nFileDataSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (nFileDataSize <= sizeof(PACKED_DATA_FILE_HEAD))
	{
		fclose(fp);
		printf("invalid sar package file!\n");
		return -1;
	}

	PACKED_DATA_FILE_HEAD packed_data_file_head = {0};

	fread(&packed_data_file_head, 1, sizeof(packed_data_file_head), fp);
	if ((packed_data_file_head.nVersionNo < VERSION_BASE) || (packed_data_file_head.nVersionNo > VERSION_CUR) || 
		(strncmp((const char*)&(packed_data_file_head.nTag), TAG_PACKAGED_FILE, 2) != 0))
	{
		fclose(fp);
		sprintf(szMsg, "invalid sar package file!\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);
		return -1;
	}

	if ((packed_data_file_head.nIndexDataSize < sizeof(SAR_PANEL_INFO)) || 
		(packed_data_file_head.nIndexDataSize + packed_data_file_head.nIndexDataOffset> nFileDataSize))
	{
		fclose(fp);
		sprintf(szMsg, "invalid sar package file!\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);
		return -1;
	}

	if (packed_data_file_head.nDataSize > nFileDataSize)
	{
		sprintf(szMsg, "warning, package file data size mis-match!\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);
	}

	packed_data_file_head.szAuthor[sizeof(packed_data_file_head.szAuthor) - 1] = 0;
	packed_data_file_head.szDataVendorInfo[sizeof(packed_data_file_head.szDataVendorInfo) - 1] = 0;
	packed_data_file_head.szFileDescription[sizeof(packed_data_file_head.szFileDescription) - 1] = 0;
	sprintf(szMsg, "sar package file: author: %s; data vendor: %s; description: %s\n", 
		packed_data_file_head.szAuthor, 
		packed_data_file_head.szDataVendorInfo, 
		packed_data_file_head.szFileDescription);
	printf(szMsg);
	// OutputDebugStringA(szMsg);

	unsigned char* ptrSarIndexBuffer = new unsigned char[packed_data_file_head.nIndexDataSize];
	fseek(fp, packed_data_file_head.nIndexDataOffset, SEEK_SET);
	fread(ptrSarIndexBuffer, 1, packed_data_file_head.nIndexDataSize, fp);

	if (packed_data_file_head.nIndexDataCRC != calculate_crc32(0, ptrSarIndexBuffer, packed_data_file_head.nIndexDataSize))
	{
		delete[] ptrSarIndexBuffer;
		fclose(fp);

		sprintf(szMsg, "invalid sar package file. CRC checking error!\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);
		return -1;
	}

	PSAR_PANEL_INFO psar_panel_info = (PSAR_PANEL_INFO)ptrSarIndexBuffer;
	if (packed_data_file_head.nIndexDataSize % sizeof(SAR_PANEL_INFO))
	{
		sprintf(szMsg, "warning, index data size mis-match!\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);
	}

	int nSarIndexCount = packed_data_file_head.nIndexDataSize / sizeof(SAR_PANEL_INFO);
	sprintf(szMsg, "%d panels found in package!\n", nSarIndexCount);
	printf(szMsg);
	// OutputDebugStringA(szMsg);

	unsigned char *ptrFileBuffer = NULL;
	unsigned int nCachedBufferSize = 0;

	int nPanelCount = 0;
	for (int i = 0; i < nSarIndexCount; i++)
	{
		if ((DATA_TYPE_SAR_PNG != psar_panel_info[i].nType) ||
			(0 == psar_panel_info[i].nSubPanelCount) || 
			(0 == psar_panel_info[i].nPanelDataSize) || 
			(0 == psar_panel_info[i].nPanelDataOffSet) || 
			(0 == psar_panel_info[i].szPanelName[0]))
		{
			continue;
		}

		if (psar_panel_info[i].nSubPanelCount > MAX_SAR_SUB_PANEL)
		{
			psar_panel_info[i].nSubPanelCount = MAX_SAR_SUB_PANEL;
		}

		if ((psar_panel_info[i].nPanelDataOffSet + psar_panel_info[i].nPanelDataSize) > nFileDataSize)
		{
			continue;
		}

		sprintf(szFullName, "%s%s.png", szDstFolder, psar_panel_info[i].szPanelName);
		FILE *fpDst = fopen(szFullName, "wb");
		if (fpDst)
		{
			if (nCachedBufferSize < psar_panel_info[i].nPanelDataSize)
			{
				nCachedBufferSize = ((psar_panel_info[i].nPanelDataSize + 1023) >> 10) << 10;
				delete[] ptrFileBuffer;

				ptrFileBuffer = new unsigned char[nCachedBufferSize];
			}

			fseek(fp, psar_panel_info[i].nPanelDataOffSet, SEEK_SET);
			fread(ptrFileBuffer, 1, psar_panel_info[i].nPanelDataSize, fp);

			unsigned int nCRC = calculate_crc32(0, ptrFileBuffer, psar_panel_info[i].nPanelDataSize);
			if (nCRC != psar_panel_info[i].nCRC)
			{
				sprintf(szMsg, "warning, panel data CRC mis-match!\n");
				printf(szMsg);
				// OutputDebugStringA(szMsg);
			}

			fwrite(ptrFileBuffer, 1, psar_panel_info[i].nPanelDataSize, fpDst);

			fclose(fpDst);
		}

		sprintf(szMsg, "got one panel(%d). name: %s. size: %d. sub-panel count: %d. sub-panel detail info:\n", 
			nPanelCount + 1,
			psar_panel_info[i].szPanelName, 
			psar_panel_info[i].nPanelDataSize, 
			psar_panel_info[i].nSubPanelCount);
		printf(szMsg);
		// OutputDebugStringA(szMsg);

		for (unsigned int j = 0; j < psar_panel_info[i].nSubPanelCount; j++)
		{
			sprintf(szMsg, "    name: %s. [l, r, t, b] = [%d, %d, %d, %d]\n", 
				psar_panel_info[i].subpanel_info[j].szSubPanelName, 
				psar_panel_info[i].subpanel_info[j].nLeft, 
				psar_panel_info[i].subpanel_info[j].nTop, 
				psar_panel_info[i].subpanel_info[j].nRight, 
				psar_panel_info[i].subpanel_info[j].nBottom);

			printf(szMsg);
			// OutputDebugStringA(szMsg);
		}

		sprintf(szMsg, "\n");
		printf(szMsg);
		// OutputDebugStringA(szMsg);

		nPanelCount++;
	}

	fclose(fp);
	delete[] ptrFileBuffer;
	delete[] ptrSarIndexBuffer;

	sprintf(szMsg, "sar package file extracting done. %d panels got. press enter to exit.\n", nPanelCount);
	printf(szMsg);
	// OutputDebugStringA(szMsg);
	return 0;
}
static int s_find_index(const unsigned int *crc_array, int array_count, unsigned int crc_match)
{
	int nStartIndex = 0;
	int nEndIndex   = array_count - 1;
	while (nStartIndex <= nEndIndex)
	{
		int nMiddleIndex = (nStartIndex + nEndIndex) / 2;
		if (crc_match == crc_array[nMiddleIndex])   return nMiddleIndex;

		if (crc_match > crc_array[nMiddleIndex])
		{
			nStartIndex = nMiddleIndex + 1;
		}
		else
		{
			nEndIndex = nMiddleIndex - 1;
		}
	}

	return -1;
}
static void s_local_file_reading_proc(png_structp png_ptr, png_bytep data, png_size_t length)
{
	if (data && length)
	{
		FILE *file = (FILE*)png_get_io_ptr(png_ptr);
		fread(data, 1, length, file);
	}
}

static void s_local_png_decoding_error_proc(png_structp png_ptr, png_const_charp error_msg)
{
	//  png_error(png_ptr, "Read Error");
}

static int s_get_png_file_bound_box(const char *szPNG_File, int* left, int* top, int* right, int* bottom)
{
    *left = *top = *right = *bottom = -1;

    if ((!szPNG_File) || (!szPNG_File[0]))  return -1;
    FILE    *file = fopen(szPNG_File, "rb");
    if (!file)  return -1;

    png_struct *png_ptr  = NULL;
    png_info   *info_ptr = NULL;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (void *)NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        fclose(file);
        return -1;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) 
    {
        fclose(file);
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        return -1;
    }

    png_set_read_fn(png_ptr, file, (png_rw_ptr)s_local_file_reading_proc);
    png_set_error_fn(png_ptr, NULL, (png_error_ptr)s_local_png_decoding_error_proc, NULL);

    png_read_info(png_ptr, info_ptr);

    png_uint_32 nImgWidth, nImgHeight, nImgRowBytes, nImgRowBytesAlphaChannel, nImgHalfHeight;
    int nImgBitDepth, nImgColorType, nImgInterlaceType, nImgCompressType, nImgFilterType;
    int number_passes;

    png_get_IHDR(png_ptr, info_ptr, 
                 &nImgWidth, &nImgHeight, 
                 &nImgBitDepth, &nImgColorType, 
                 &nImgInterlaceType, 
                 &nImgCompressType, 
                 &nImgFilterType);
    number_passes = png_set_interlace_handling(png_ptr);

    if ((nImgColorType != PNG_COLOR_TYPE_RGB_ALPHA) || 
        (nImgInterlaceType != PNG_INTERLACE_NONE)   || 
        (nImgBitDepth != 8) || 
        (nImgWidth < 4) || (nImgHeight < 4) || (number_passes != 1))
    {
        fclose(file);
        png_read_end(png_ptr, info_ptr);
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

        return -1;
    }

    nImgHalfHeight = (nImgHeight + 1) >> 1;
    nImgRowBytes  = (png_uint_32)png_get_rowbytes(png_ptr, info_ptr);
    nImgRowBytesAlphaChannel = (((nImgWidth + 3) >> 2) << 2);

    unsigned char *img_row_buffer    = new unsigned char[nImgRowBytes + 1024];
    unsigned char *img_alpha_channel = new unsigned char[nImgRowBytesAlphaChannel * nImgHeight];
    unsigned char *img_row_buffer_dst = img_alpha_channel;

    int nMinX = -1, nMaxX = -1;
    int nMinY = -1, nMaxY = -1;

    png_start_read_image(png_ptr);
    for (png_uint_32 row = 0; row < nImgHeight; row++)
    {
        memset(img_row_buffer, 0x00, nImgRowBytes);
        png_read_row(png_ptr, img_row_buffer, NULL);

        unsigned char *tmp_row_buffer_src = img_row_buffer;
        unsigned char *tmp_row_buffer_dst = img_row_buffer_dst;

        int nMinX_Row, nMaxX_Row;

        nMinX_Row = nMaxX_Row = -1;
        for (png_uint_32 col = 0; col < nImgWidth; col++)
        {
            tmp_row_buffer_dst[col] = tmp_row_buffer_src[3];
            if (tmp_row_buffer_dst[col])
            {
                if (nMinX_Row < 0)
                {
                    nMinX_Row = nMaxX_Row = col;
                }
                else
                {
                    nMaxX_Row = col;
                }
            }

            tmp_row_buffer_src += 4;
        }

        img_row_buffer_dst += nImgRowBytesAlphaChannel;

        if (nMinX_Row >= 0)
        {   //  with ahpla in this line:
            if (nMinX < 0)
            {
                nMinX = nMinX_Row;
                nMaxX = nMaxX_Row;
            }
            else
            {
                if (nMinX_Row < nMinX)
                {
                    nMinX = nMinX_Row;
                }

                if (nMaxX_Row > nMaxX)
                {
                    nMaxX = nMaxX_Row;
                }
            }

            if (nMinY < 0)
            {
                nMinY = nMaxY = row;
            }
            else
            {
                nMaxY = row;
            }
        }
#if 1
        else
        {
            if ((nMaxY > 0) && (row > nImgHalfHeight))
            {
                /*because we already know the lefted part are all transparented*/
                break;  //  <------------thus you CAN NOT call function "png_read_end"
            }
        }
#endif
    }

    delete[] img_row_buffer;
    delete[] img_alpha_channel;

    /* 
    read the rest of the file, getting any additional chunks in info_ptr
    (please DO NOT call this function because our file pointer maybe not correct due to breacked out earlier) 
    */
    //  png_read_end(png_ptr, info_ptr);

    /* clean up after the read, and free any memory allocated - REQUIRED */
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

    fclose(file);

    *left   = nMinX;
    *top    = nMinY;
    *right  = nMaxX;
    *bottom = nMaxY;

    return (nMaxY > 0) ? 0 : (-1);
}


int pack_sar_file_new(const char *sar_pkg_file,
					  const char *sub_panel_parent_folder, 
					  const char **sub_panel_sub_folders, 
					  int nSubPanelSubFolders, 

					  const char *full_panel_parent_folder, 
					  const char **full_panel_sub_folders, 
					  int nFullPanelSubFolders
					  )
{
	if ((!sar_pkg_file) || (!sar_pkg_file[0]))  return -1;

	if ((!sub_panel_parent_folder) || (!sub_panel_parent_folder[0]))
	{
		return -1;
	}

	if ((!sub_panel_sub_folders) || (nSubPanelSubFolders <= 0))   return -1;

	if ((!full_panel_parent_folder) || (!full_panel_parent_folder[0]))
	{
		return -1;
	}

	if ((!full_panel_sub_folders) || (nFullPanelSubFolders <= 0))   return -1;

	const char szSubPanelPrefix[] = "_SIGN_";
	const int nSubPanelPrefixLen = (int)strlen(szSubPanelPrefix);

	// WIN32_FIND_DATAA win32_find_dat = {0};
	// HANDLE  hFileFind = NULL;
	char szFullFolder[320] = "", szFullName[320] = "", szMsg[640] = "";
	int  nFileCount = 0, i;

	PSAR_PANEL_INFO psar_panel_info = NULL;
	int nPanelCount = 0, nCacheSize = 0;

	unsigned int nTime1, nTime2;

	// nTime1 = GetTickCount();
	printf("start sub panel png file processing. waiting...\n");
	// OutputDebugStringA("start sub panel png file processing. waiting...\n");

	for (i = 0; i < nSubPanelSubFolders; i++)
	{
		if ((sub_panel_sub_folders[i] == NULL) || (sub_panel_sub_folders[i][0] == 0x00))
		{
			continue;
		}

		strncpy(szFullFolder, sub_panel_parent_folder, sizeof(szFullFolder) - 1);
		addPathSprit(szFullFolder);
		strcat(szFullFolder, sub_panel_sub_folders[i]);
		addPathSprit(szFullFolder);

		sprintf(szFullName, "%s%s", szFullFolder, "*.png");

		// hFileFind = FindFirstFileA(szFullName, &win32_find_dat);
		FILEINFO *pFileInfoList = NULL;
		if (getDirFileInfo(szFullName, pFileInfoList) < 0
			&& pFileInfoList == NULL)   
		{
			printf("Warning: Can not find file under %s folder.\n ", szFullName);
			return -1;
		}
		FILEINFO *pFileInfo = pFileInfoList;
		while(pFileInfo != NULL) 
		// while (hFileFind != (HANDLE)(-1))
		{
			char *ptr, *ptr2, szPanelName[64], szSubPanelName[64];
			int left, top, right, bottom;

			if (pFileInfo->size <= 16)// (win32_find_dat.nFileSizeLow <= 16)
			{
				continue; // goto _next_file;
			}

			ptr = strstr(pFileInfo->name/*win32_find_dat.cFileName*/, szSubPanelPrefix);
			if (NULL == ptr)    
			{
				continue; // goto _next_file;
			}

			ptr2 = strchr(ptr, '.');
			if (!ptr2) continue; // goto _next_file;

			*ptr = 0;
			strcpy(szPanelName, pFileInfo->name/*win32_find_dat.cFileName*/);
			*ptr = szSubPanelPrefix[0];

			*ptr2 = 0;
			strcpy(szSubPanelName, ptr + nSubPanelPrefixLen);
			*ptr2 = '.';

			sprintf(szFullName, "%s%s", szFullFolder, pFileInfo->name/*win32_find_dat.cFileName*/);
			if (0 == s_get_png_file_bound_box(szFullName, &left, &top, &right, &bottom))
			{
				SAR_SUB_PANEL_INFO subpanel_info = {0};

				subpanel_info.nLeft   = left;
				subpanel_info.nTop    = top;
				subpanel_info.nRight  = right;
				subpanel_info.nBottom = bottom;
				strncpy(subpanel_info.szSubPanelName, szSubPanelName, sizeof(subpanel_info.szSubPanelName) - 1);

				int nNewPanel = 1;
				if (nPanelCount > 0)
				{
					if (strcmp(psar_panel_info[nPanelCount - 1].szPanelName, szPanelName) == 0)
					{
						if (psar_panel_info[nPanelCount - 1].nSubPanelCount < MAX_SAR_SUB_PANEL)
						{
							psar_panel_info[nPanelCount - 1].subpanel_info[psar_panel_info[nPanelCount - 1].nSubPanelCount] = subpanel_info;
							psar_panel_info[nPanelCount - 1].nSubPanelCount += 1;
						}

						nNewPanel = 0;
					}
				}

				if (nNewPanel)
				{
					if (nPanelCount >= nCacheSize)
					{
						nCacheSize += 256;
						PSAR_PANEL_INFO psar_panel_info_new = new SAR_PANEL_INFO[nCacheSize];
						memset(psar_panel_info_new, 0x00, sizeof(SAR_PANEL_INFO) * nCacheSize);

						if (psar_panel_info)
						{
							memcpy(psar_panel_info_new, psar_panel_info, nPanelCount * sizeof(SAR_PANEL_INFO));
							delete[] psar_panel_info;
						}

						psar_panel_info = psar_panel_info_new;
					}

					psar_panel_info[nPanelCount].nSubPanelCount = 0;
					psar_panel_info[nPanelCount].nType = DATA_TYPE_SAR_PNG;
					psar_panel_info[nPanelCount].subpanel_info[psar_panel_info[nPanelCount].nSubPanelCount] = subpanel_info;
					psar_panel_info[nPanelCount].nSubPanelCount += 1;
					strncpy(psar_panel_info[nPanelCount].szPanelName, szPanelName, sizeof(psar_panel_info[nPanelCount].szPanelName) - 1);

					nPanelCount++;
				}

				nFileCount++;
				sprintf(szMsg, "file: %s(%d). panel: %s; sub-panel: %s; bound: [%d, %d, %d, %d]\n", 
					pFileInfo->name/*win32_find_dat.cFileName*/,
					nFileCount, 
					szPanelName, 
					szSubPanelName,
					left, top, right, bottom);
				// OutputDebugStringA(szMsg);
				printf(szMsg);
			}

//_next_file:
//			memset(&win32_find_dat, 0x00, sizeof(win32_find_dat));
//			if (!FindNextFileA(hFileFind, &win32_find_dat))
//			{
//				FindClose(hFileFind);
//				break;
//			}
			FILEINFO *pInfo = pFileInfo->next;
			delete pFileInfo;
			pFileInfo = pInfo;
		}
	}

	unsigned int *panel_name_crc_array = new unsigned int[nPanelCount];
	for (i = 0; i < nPanelCount; i++)
	{
		panel_name_crc_array[i] = calculate_crc32(0, (unsigned char*)(psar_panel_info[i].szPanelName), (unsigned int)strlen(psar_panel_info[i].szPanelName));
	}

	/*sort by name*/
	for (i = 0; i < nPanelCount; i++)
	{
		for (int j = i + 1; j < nPanelCount; j++)
		{
			if (panel_name_crc_array[i] > panel_name_crc_array[j])
			{
				unsigned int tmp = panel_name_crc_array[i];
				panel_name_crc_array[i] = panel_name_crc_array[j];
				panel_name_crc_array[j] = tmp;

				SAR_PANEL_INFO sar_panel_info = psar_panel_info[i];
				psar_panel_info[i] = psar_panel_info[j];
				psar_panel_info[j] = sar_panel_info;
			}
		}
	}

	// nTime2 = GetTickCount();

	// sprintf(szMsg, "sub panel files process done. %d panels collect. %d file collected. time(s): %d\n", nPanelCount, nFileCount, (nTime2 - nTime1 + 999) / 1000);
	// OutputDebugStringA(szMsg);
	printf(szMsg);

	if (nPanelCount <= 0)
	{
		delete[] psar_panel_info;
		delete[] panel_name_crc_array;

		return -1;
	}

	PSAR_PANEL_INFO_MEM psar_panel_info_mem = new SAR_PANEL_INFO_MEM[nPanelCount];
	for (i = 0; i < nPanelCount; i++)
	{
		psar_panel_info_mem[i].szFullName[0] = 0;
		psar_panel_info_mem[i].sar_panel_info = psar_panel_info[i];
		psar_panel_info_mem[i].sar_panel_info.nPanelDataSize   = 0;
		psar_panel_info_mem[i].sar_panel_info.nPanelDataOffSet = 0;
	}

	delete[] psar_panel_info;   psar_panel_info = NULL;

	// nTime1 = GetTickCount();
	printf("start full panel png file processing. waiting...\n");
	// OutputDebugStringA("start full panel png file processing. waiting...\n");

	nFileCount = 0;
	for (i = 0; i < nFullPanelSubFolders; i++)
	{
		if ((full_panel_sub_folders[i] == NULL) || (full_panel_sub_folders[i][0] == 0x00))
		{
			continue;
		}

		strncpy(szFullFolder, full_panel_parent_folder, sizeof(szFullFolder) - 1);
		addPathSprit(szFullFolder);
		strcat(szFullFolder, full_panel_sub_folders[i]);
		addPathSprit(szFullFolder);

		sprintf(szFullName, "%s%s", szFullFolder, "*.png");

		// hFileFind = FindFirstFileA(szFullName, &win32_find_dat);
		FILEINFO *pFileInfoList = NULL;
		if (getDirFileInfo(szFullName, pFileInfoList) < 0
			&& pFileInfoList == NULL)   
		{
			printf("Warning: Can not find file under %s folder.\n ", szFullName);
			return -1;
		}
		FILEINFO *pFileInfo = pFileInfoList;
		while(pFileInfo != NULL) 
		// while (hFileFind != (HANDLE)(-1))
		{
			if (pFileInfo->size > 16) // ((win32_find_dat.nFileSizeLow > 16) && (win32_find_dat.nFileSizeHigh == 0))
			{
				char *ptr = strrchr(pFileInfo->name/*win32_find_dat.cFileName*/, '.');
				if (NULL == ptr)    
				{
					continue; // goto _next_file1;
				}

				*ptr = 0;
				unsigned int nCrc = calculate_crc32(0, (unsigned char*)(pFileInfo->name/*win32_find_dat.cFileName*/), (unsigned int)strlen(pFileInfo->name/*win32_find_dat.cFileName*/));
				*ptr = '.';

				int nIndex = s_find_index(panel_name_crc_array, nPanelCount, nCrc);
				if (nIndex >= 0)
				{
					sprintf(psar_panel_info_mem[nIndex].szFullName, "%s%s", szFullFolder, pFileInfo->name/*win32_find_dat.cFileName*/);
					psar_panel_info_mem[nIndex].sar_panel_info.nPanelDataOffSet = 0;
					psar_panel_info_mem[nIndex].sar_panel_info.nPanelDataSize   = pFileInfo->size/*win32_find_dat.nFileSizeLow*/;

					nFileCount++;
					sprintf(szMsg, "got file(%d): %s, index: %d; size: %d\n", 
						nFileCount, psar_panel_info_mem[nIndex].szFullName, 
						nIndex, 
						psar_panel_info_mem[nIndex].sar_panel_info.nPanelDataSize);
				}
				else
				{
					sprintf(szFullName, "%s%s", szFullFolder, pFileInfo->name/*win32_find_dat.cFileName*/);
					printf(szMsg, "something wrong? file not matched. %s\n", szFullName);
				}

				// OutputDebugStringA(szMsg);
				printf(szMsg);
			}

//_next_file1:
//			memset(&win32_find_dat, 0x00, sizeof(win32_find_dat));
//			if (!FindNextFileA(hFileFind, &win32_find_dat))
//			{
//				FindClose(hFileFind);
//				break;
//			}
			FILEINFO *pInfo = pFileInfo->next;
			delete pFileInfo;
			pFileInfo = pInfo;
		}
	}  //  for-loop

	delete[] panel_name_crc_array;  panel_name_crc_array = NULL;

	// nTime2 = GetTickCount();

	// sprintf(szMsg, "full panel files process done. %d panels collect. time(s): %d\n", nFileCount, (nTime2 - nTime1 + 999) / 1000);
	// OutputDebugStringA(szMsg);
	printf(szMsg);

	if (nFileCount != nPanelCount)
	{
		sprintf(szMsg, "warning, maybe some full panels not matched. please check the log\n");
		// OutputDebugStringA(szMsg);
		printf(szMsg);
	}

	if (nFileCount <= 0)    
	{
		delete[] psar_panel_info_mem;
		return -1;
	}

	FILE    *file = fopen(sar_pkg_file, "wb");
	if (!file)
	{
		delete[] psar_panel_info_mem;

		sprintf(szMsg, "fatal error, destination package file open error!\n");
		// OutputDebugStringA(szMsg);
		printf(szMsg);
		return -1;
	}

	PACKED_DATA_FILE_HEAD packed_data_file_head = {0};
	strcpy((char*)&(packed_data_file_head.nTag), TAG_PACKAGED_FILE);
	packed_data_file_head.nVersionNo       = VERSION_CUR;

	packed_data_file_head.nDataSize        = sizeof(packed_data_file_head);
	strcpy(packed_data_file_head.szAuthor, "shboli");
	strcpy(packed_data_file_head.szDataVendorInfo, "Navteq 2016Q1 SAR ANZ");
	strcpy(packed_data_file_head.szFileDescription, "SAR data for ANZ(ver: NT2016Q1)");

	packed_data_file_head.nIndexDataOffset = sizeof(packed_data_file_head);
	int nRealPanelCount = 0;

	for (i = 0; i < nPanelCount; i++)
	{
		if (psar_panel_info_mem[i].szFullName[0] == 0)  continue;

		nRealPanelCount++;
	}

	psar_panel_info     = new SAR_PANEL_INFO[nRealPanelCount];

	// nTime1 = GetTickCount();
	printf("start panel data packaging. waiting...\n");

	unsigned char *pBufferSrc = NULL;
	unsigned int nBufferSizeCache = 0, nDataSizeCur;
	int nRealPanelCountOri = nRealPanelCount;

	unsigned int nDataOffSet = packed_data_file_head.nIndexDataOffset + nRealPanelCountOri * sizeof(SAR_PANEL_INFO);
	nRealPanelCount = 0;
	for (i = 0; i < nPanelCount; i++)
	{
		if (psar_panel_info_mem[i].szFullName[0] == 0)  continue;

		nDataOffSet = ((nDataOffSet + 3) >> 2) << 2;
		nDataSizeCur = 0;

		psar_panel_info[nRealPanelCount] = psar_panel_info_mem[i].sar_panel_info;
		FILE* file_src = fopen(psar_panel_info_mem[i].szFullName, "rb");
		if (file_src)
		{
			fseek(file_src, 0, SEEK_END);
			nDataSizeCur = ftell(file_src);
			fseek(file_src, 0, SEEK_SET);
			if (nDataSizeCur > nBufferSizeCache)
			{
				delete[] pBufferSrc;
				nBufferSizeCache = ((nDataSizeCur + 1023) >> 10) << 10;
				pBufferSrc = new unsigned char[nBufferSizeCache];
			}

			if (nDataSizeCur > 0)
			{
				fread(pBufferSrc, 1, nDataSizeCur, file_src);
			}

			fclose(file_src);


		}

		if (nDataSizeCur > 0)
		{
			psar_panel_info[nRealPanelCount].nPanelDataOffSet = nDataOffSet;
			psar_panel_info[nRealPanelCount].nPanelDataSize   = nDataSizeCur;
			psar_panel_info[nRealPanelCount].nCRC             = calculate_crc32(0, pBufferSrc, nDataSizeCur);

			fseek(file, nDataOffSet, SEEK_SET);
			fwrite(pBufferSrc, 1, nDataSizeCur, file);

			nDataOffSet += nDataSizeCur;

			sprintf(szMsg, "packaging file %s. done! size: %d\n", psar_panel_info_mem[i].szFullName, nDataSizeCur);
		}
		else
		{
			psar_panel_info[nRealPanelCount].nPanelDataOffSet = 0;
			psar_panel_info[nRealPanelCount].nPanelDataSize   = 0;
			psar_panel_info[nRealPanelCount].nCRC             = 0;

			sprintf(szMsg, "packaging file %s. failure!!!\n", psar_panel_info_mem[i].szFullName);
		}

		// OutputDebugStringA(szMsg);
		printf(szMsg);

		nRealPanelCount++;
	}

	delete[] pBufferSrc;    pBufferSrc = NULL;
	delete[] psar_panel_info_mem;   psar_panel_info_mem = NULL;

	if (!nRealPanelCount)
	{
		delete[] psar_panel_info;   psar_panel_info = NULL;
		fclose(file);

		remove(sar_pkg_file);
		return -1;
	}

	packed_data_file_head.nDataSize      = nDataOffSet;
	packed_data_file_head.nIndexDataSize = nRealPanelCountOri * sizeof(SAR_PANEL_INFO);
	packed_data_file_head.nIndexDataCRC  = calculate_crc32(0, (unsigned char*)psar_panel_info, packed_data_file_head.nIndexDataSize);

	{
		//FILETIME file_time = {0};        
		//GetSystemTimeAsFileTime(&file_time);
		time_t now;  time(&now);
		packed_data_file_head.nDataCreatedTimeLow  = (unsigned int)(now); // file_time.dwLowDateTime;
		packed_data_file_head.nDataCreatedTimeHigh = (unsigned int)(now>>32); // file_time.dwHighDateTime;
	}

	/*head*/
	fseek(file, 0, SEEK_SET);
	fwrite(&packed_data_file_head, 1, sizeof(packed_data_file_head), file);

	/*index info*/
	fseek(file, packed_data_file_head.nIndexDataOffset, SEEK_SET);
	fwrite(psar_panel_info, 1, packed_data_file_head.nIndexDataSize, file);

	fclose(file);

	delete[] psar_panel_info;   psar_panel_info = NULL;
	delete[] psar_panel_info_mem;

	// nTime2 = GetTickCount();

	//sprintf(szMsg, "congratulation! sar data packaging done! %d panels got. file size: %d. time: %d. press enter to exit.\n", 
	//	nRealPanelCount, nDataOffSet, (nTime2 - nTime1 + 999) / 1000);
	// OutputDebugStringA(szMsg);
	printf(szMsg);

	return 0;
}

static int s_check_replace_file(const char* szFullFileSrc, const char* szFullFileDst, 
								const char *szOriCharacters, const char *szDstCharacters)
{
	FILE *fpSrc = fopen(szFullFileSrc, "rt");
	if (NULL == fpSrc)  return -1;

	const int nBufferLen = 8192;

	FILE *fpDst = fopen(szFullFileDst, "wt");
	if (!fpDst)
	{
		fclose(fpSrc);
		return -1;
	}

	char    *szLineBufferSrc = new char[nBufferLen];
	char    *szLineBufferDst = new char[nBufferLen];

	bool bReplaced = false;

	while (1)
	{
		if (feof(fpSrc))
		{
			break;
		}

		szLineBufferSrc[0] = 0x00;
		fgets(szLineBufferSrc, nBufferLen, fpSrc);
		if (!szLineBufferSrc[0])    continue;

		bool bReplacedThisLine = false;

		int nReplaceSrcLen = strlen(szOriCharacters);
		int nReplaceDstLen = strlen(szDstCharacters);

		char *ptr = strstr(szLineBufferSrc, szOriCharacters);
		if (ptr)
		{
			memset(szLineBufferDst, 0x00, nBufferLen);
			char *ptrBaseSrc = szLineBufferSrc;
			char *ptrBaseDst = szLineBufferDst;

			while (1)
			{
				if (ptr > ptrBaseSrc)
				{
					strncpy(ptrBaseDst, ptrBaseSrc, ptr - ptrBaseSrc);
					ptrBaseDst += ptr - ptrBaseSrc;
				}

				strncpy(ptrBaseDst, szDstCharacters, nReplaceDstLen);
				ptrBaseDst += nReplaceDstLen;

				ptrBaseSrc = ptr + nReplaceSrcLen;
				ptr = strstr(ptrBaseSrc, szOriCharacters);
				if (!ptr)
				{
					strcpy(ptrBaseDst, ptrBaseSrc);
					break;
				}
			}

			strcpy(szLineBufferSrc, szLineBufferDst);

			bReplacedThisLine = true;
			bReplaced = true;
		}

		fputs(szLineBufferSrc, fpDst);
	}

	delete[] szLineBufferSrc;
	delete[] szLineBufferDst;

	fclose(fpSrc);
	fclose(fpDst);

	if (!bReplaced)
	{
		remove(szFullFileDst);

		return 1;
	}

	return 0;
}



int scan_specific_characters(const char* szSrcPath, const char* szDstPath, 
							 const char *szOriCharacters, const char* szDstCharacters)
{
	if ((NULL == szSrcPath) || (0 == szSrcPath[0])) return -1;
	if ((NULL == szDstPath) || (0 == szDstPath[0])) return -1;
	//  if ((NULL == szOriCharacters) || (0 == szOriCharacters[0])) return -1;
	//  if ((NULL == szDstCharacters) || (0 == szDstCharacters[0])) return -1;
	// WIN32_FIND_DATAA win32_find_dat = {0};
	// HANDLE  hFileFind = NULL;
	char    szFullFileSrc[320] = "", szFullFileDst[320] = "";
	int     nScanCount = 0, nPickCount = 0;

	printf("start SAR svg file content scaning. waiting...\n");
	sprintf(szFullFileSrc, "%s\\*.svg", szSrcPath);
	
	// hFileFind = FindFirstFileA(szFullFileSrc, &win32_find_dat);
	char szCommandLine[256] = "";
	FILEINFO *pFileInfoList = NULL;
	if (getDirFileInfo(szFullFileSrc, pFileInfoList) < 0
		&& pFileInfoList == NULL)   
	{
		printf("Warning: Can not find file under %s folder.\n ", szFullFileSrc);
		return -1;
	}
	FILEINFO *pFileInfo = pFileInfoList;
	while(pFileInfo != NULL) 
	// while (hFileFind != (HANDLE)(-1)) 
	{
		if (pFileInfo->size < 16)// ((win32_find_dat.nFileSizeLow < 16) || (win32_find_dat.nFileSizeHigh != 0))
		{
			continue; // goto _next_file;
		}

		sprintf(szFullFileSrc, "%s\\%s", szSrcPath, pFileInfo->size/*win32_find_dat.cFileName*/);
		sprintf(szFullFileDst, "%s\\%s", szDstPath, pFileInfo->size/*win32_find_dat.cFileName*/);

		int ret = s_check_replace_file(szFullFileSrc, szFullFileDst, szOriCharacters, szDstCharacters);
		printf("scan file %d. name: %s done\n", nScanCount + 1, pFileInfo->name/*win32_find_dat.cFileName*/);
		nScanCount++;

		if (0 == ret)
		{
			sprintf(szCommandLine, "@@@got one file. name(%d): %s\n", nPickCount + 1, pFileInfo->name/*win32_find_dat.cFileName*/);
			nPickCount++;
			printf(szCommandLine);
			// OutputDebugStringA(szCommandLine);
		}

//_next_file:
//		memset(&win32_find_dat, 0x00, sizeof(win32_find_dat));
//		if (!FindNextFileA(hFileFind, &win32_find_dat))
//		{
//			break;
//		}
		FILEINFO *pInfo = pFileInfo->next;
		delete pFileInfo;
		pFileInfo = pInfo;
	}
	// FindClose(hFind);

	return 0;
}

int main(int argc, char* argv[])
{
	int nError = 0;
	if (argc > 3) 
	{ 
		for (int i = 0; i<argc; i++) {printf("Parameter(%d) == %s\n", i, argv[i]);}
		const char* sType = argv[1];
		char* sInput = argv[2];
		char* sOutput = argv[3];
		const char * subfolder = "sub";
		const char * fullfolder = "full";
		char szOriCharacters[16] = "", szDstCharacters[16] = "";
		szOriCharacters[0] = 0xE2;
		szOriCharacters[1] = 0x80;
		szOriCharacters[2] = 0x99;
		szDstCharacters[0] = 0x27;
		if (0 == strcmp(sType, "scan_specific")){nError = scan_specific_characters(sInput, sOutput, szOriCharacters, szDstCharacters);}
		else if (0 == strcmp(sType,"pack_sar")){nError = pack_sar_file_new(sInput, sOutput, &subfolder, 1, sOutput, &fullfolder, 1);} 
		else if (0 == strcmp(sType, "extract_sar")){nError = sar_package_extract(sInput, sOutput);}
		else if (0 == strcmp(sType, "pack_ejv")){nError = packege_ejv_images_new(sInput, sOutput);}
		else if (0 == strcmp(sType, "extract_ejv")){nError = extract_ejv_images_new(sInput, sOutput);}
		else {
			printf("Parameter Operator Type = %s, error!!\n(Operator Type: scan_specific pack_sar extract_sar pack_ejv extract_ejv)\n", argv[1]);
			nError = -1;
		}
	}
	else
	{ 
		printf("Parameter error!!!\n Need three parameters: type input output\n");
		nError = -1;
	}

	if (nError < 0)
	{
		getchar();
	}
	return 0;
}

