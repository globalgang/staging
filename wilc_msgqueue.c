
#include "wilc_msgqueue.h"
#include <linux/spinlock.h>
#include <linux/errno.h>

/*!
 *  @author		syounan
 *  @date		1 Sep 2010
 *  @note		copied from FLO glue implementatuion
 *  @version		1.0
 */
int wilc_mq_create(WILC_MsgQueueHandle *pHandle)
{
	spin_lock_init(&pHandle->strCriticalSection);
	sema_init(&pHandle->hSem, 0);
	pHandle->pstrMessageList = NULL;
	pHandle->u32ReceiversCount = 0;
	pHandle->bExiting = false;
	return 0;
}

/*!
 *  @author		syounan
 *  @date		1 Sep 2010
 *  @note		copied from FLO glue implementatuion
 *  @version		1.0
 */
int wilc_mq_destroy(WILC_MsgQueueHandle *pHandle)
{
	pHandle->bExiting = true;

	/* Release any waiting receiver thread. */
	while (pHandle->u32ReceiversCount > 0) {
		up(&pHandle->hSem);
		pHandle->u32ReceiversCount--;
	}

	while (pHandle->pstrMessageList) {
		Message *pstrMessge = pHandle->pstrMessageList->pstrNext;

		kfree(pHandle->pstrMessageList);
		pHandle->pstrMessageList = pstrMessge;
	}

	return 0;
}

/*!
 *  @author		syounan
 *  @date		1 Sep 2010
 *  @note		copied from FLO glue implementatuion
 *  @version		1.0
 */
int wilc_mq_send(WILC_MsgQueueHandle *pHandle,
			     const void *pvSendBuffer, u32 u32SendBufferSize)
{
	int s32RetStatus = 0;
	unsigned long flags;
	Message *pstrMessage = NULL;

	if ((!pHandle) || (u32SendBufferSize == 0) || (!pvSendBuffer)) {
		PRINT_ER("pHandle or pvSendBuffer is null\n");
		s32RetStatus = -EFAULT;
		goto ERRORHANDLER;
	}

	if (pHandle->bExiting) {
		PRINT_ER("pHandle fail\n");
		s32RetStatus = -EFAULT;
		goto ERRORHANDLER;
	}

	spin_lock_irqsave(&pHandle->strCriticalSection, flags);

	/* construct a new message */
	pstrMessage = kmalloc(sizeof(Message), GFP_ATOMIC);
	if (!pstrMessage)
		return -ENOMEM;
	pstrMessage->u32Length = u32SendBufferSize;
	pstrMessage->pstrNext = NULL;
	pstrMessage->pvBuffer = kmalloc(u32SendBufferSize, GFP_ATOMIC);
	if (!pstrMessage->pvBuffer) {
		s32RetStatus = -ENOMEM;
		goto ERRORHANDLER;
	}
	memcpy(pstrMessage->pvBuffer, pvSendBuffer, u32SendBufferSize);

	/* add it to the message queue */
	if (!pHandle->pstrMessageList) {
		pHandle->pstrMessageList  = pstrMessage;
	} else {
		Message *pstrTailMsg = pHandle->pstrMessageList;

		while (pstrTailMsg->pstrNext)
			pstrTailMsg = pstrTailMsg->pstrNext;

		pstrTailMsg->pstrNext = pstrMessage;
	}

	spin_unlock_irqrestore(&pHandle->strCriticalSection, flags);

	up(&pHandle->hSem);

ERRORHANDLER:
	/* error occured, free any allocations */
	if (pstrMessage) {
		kfree(pstrMessage->pvBuffer);
		kfree(pstrMessage);
	}

	return s32RetStatus;
}

/*!
 *  @author		syounan
 *  @date		1 Sep 2010
 *  @note		copied from FLO glue implementatuion
 *  @version		1.0
 */
int wilc_mq_recv(WILC_MsgQueueHandle *pHandle,
			     void *pvRecvBuffer, u32 u32RecvBufferSize,
			     u32 *pu32ReceivedLength)
{
	Message *pstrMessage;
	int s32RetStatus = 0;
	unsigned long flags;

	if ((!pHandle) || (u32RecvBufferSize == 0)
	    || (!pvRecvBuffer) || (!pu32ReceivedLength)) {
		PRINT_ER("pHandle or pvRecvBuffer is null\n");
		return -EINVAL;
	}

	if (pHandle->bExiting) {
		PRINT_ER("pHandle fail\n");
		return -EFAULT;
	}

	spin_lock_irqsave(&pHandle->strCriticalSection, flags);
	pHandle->u32ReceiversCount++;
	spin_unlock_irqrestore(&pHandle->strCriticalSection, flags);

	down(&pHandle->hSem);

	/* other non-timeout scenarios */
	if (s32RetStatus) {
		PRINT_ER("Non-timeout\n");
		return s32RetStatus;
	}

	if (pHandle->bExiting) {
		PRINT_ER("pHandle fail\n");
		return -EFAULT;
	}

	spin_lock_irqsave(&pHandle->strCriticalSection, flags);

	pstrMessage = pHandle->pstrMessageList;
	if (!pstrMessage) {
		spin_unlock_irqrestore(&pHandle->strCriticalSection, flags);
		PRINT_ER("pstrMessage is null\n");
		return -EFAULT;
	}
	/* check buffer size */
	if (u32RecvBufferSize < pstrMessage->u32Length)	{
		spin_unlock_irqrestore(&pHandle->strCriticalSection, flags);
		up(&pHandle->hSem);
		PRINT_ER("u32RecvBufferSize overflow\n");
		return -EOVERFLOW;
	}

	/* consume the message */
	pHandle->u32ReceiversCount--;
	memcpy(pvRecvBuffer, pstrMessage->pvBuffer, pstrMessage->u32Length);
	*pu32ReceivedLength = pstrMessage->u32Length;

	pHandle->pstrMessageList = pstrMessage->pstrNext;

	kfree(pstrMessage->pvBuffer);
	kfree(pstrMessage);

	spin_unlock_irqrestore(&pHandle->strCriticalSection, flags);

	return s32RetStatus;
}
