
#include "wilc_oswrapper.h"


#define tHANDLE	void *

typedef struct {
	u8		*pu8Buffer;
	WILC_Uint32 u32BufferLength;
	WILC_Uint32 u32WriteOffset;
	WILC_Uint32 u32ReadOffset;
	WILC_Uint32 u32TotalBytes;
	struct semaphore SemBuffer;
} tstrFifoHandler;


extern WILC_Uint32 FIFO_InitBuffer(tHANDLE *hBuffer,
								   WILC_Uint32 u32BufferLength);
extern WILC_Uint32 FIFO_DeInit(tHANDLE hFifo);
extern WILC_Uint32 FIFO_ReadBytes(tHANDLE hFifo, u8 *pu8Buffer,
				WILC_Uint32 u32BytesToRead, WILC_Uint32 *pu32BytesRead);
extern WILC_Uint32 FIFO_WriteBytes(tHANDLE hFifo, u8 *pu8Buffer,
				WILC_Uint32 u32BytesToWrite, WILC_Bool bForceOverWrite);
