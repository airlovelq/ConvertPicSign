#include "StdAfx.h"
#include "PubFunction.h"
namespace arxPub
{

AcDbObjectId PostToModelSpace(AcDbEntity *pEnt)
{
	assert(pEnt);//检查输入参数有效性

	/*获得当前图形数据库块表*/
	AcDbBlockTable *pBlkTbl = NULL;
	acdbHostApplicationServices()->workingDatabase()->getBlockTable(pBlkTbl, AcDb::kForRead);

	/*获得模型空间对应的块表记录*/
	AcDbBlockTableRecord *pBlkTblRcd = NULL;
	pBlkTbl->getAt(ACDB_MODEL_SPACE, pBlkTblRcd, AcDb::kForWrite);
	pBlkTbl->close();

	/*将实体添加进块表记录*/
	AcDbObjectId entId;
	Acad::ErrorStatus es = pBlkTblRcd->appendAcDbEntity(entId, pEnt);
	if(es != Acad::eOk)
	{
		pBlkTblRcd->close();
		delete pEnt;
		pEnt = NULL;
		return AcDbObjectId::kNull;
	}

	/*关闭实体和块表记录*/
	pBlkTblRcd->close();
	pEnt->close();
	return entId;
}

char* ChangeCstringToCh(CString strIn)
{
	char* pBuffer = NULL;
#ifdef _UNICODE
	long lBufferSize;
	lBufferSize = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)(strIn), -1, NULL, 0, NULL, NULL);
	pBuffer = new char[lBufferSize + 1];
	memset(pBuffer,0,lBufferSize + 1);
	WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)(strIn), -1,pBuffer, lBufferSize + 1, NULL, NULL);

#else
	pBuffer = new char[strIn.GetLength()+1];
	strcpy(pBuffer, strIn);
#endif
	return pBuffer;
}

}